#include "SpatialTree.hpp" 
#include "Relations.hpp" 
#include <iostream>
#include <iomanip>
#include <array>
#include <cmath>    
#include <stdexcept>
#include <functional>

std::string getIfcName(const Entity& e) {
    const auto& params = e.params;
    auto at = [&](std::size_t i) -> const ParamNode* {
        return (i < params.size() && params[i]) ? params[i].get() : nullptr;
    };
    return at(2) ? at(2)->value : "Unnamed Element";
}

static std::optional<std::array<double, 3>> parseDirectionOrPoint(
    const ParamNode* ref, 
    const std::unordered_map<std::string, const Entity*>& entById) 
{
    if (!ref) return std::nullopt;

    auto it = entById.find(ref->value);
    if (it == entById.end()) return std::nullopt;
    const Entity* pointOrDir = it->second;

    if (pointOrDir->type != "IFCCARTESIANPOINT" && pointOrDir->type != "IFCDIRECTION") {
        return std::nullopt;
    }

    const std::vector<std::unique_ptr<ParamNode>>* sourceParams = nullptr;

    if (pointOrDir->params.size() >= 3 && pointOrDir->params[0] && pointOrDir->params[1] && pointOrDir->params[2]) {
        sourceParams = &pointOrDir->params;
    } 
    else if (pointOrDir->params.size() == 1 && pointOrDir->params[0]->children.size() >= 3) {
        sourceParams = &pointOrDir->params[0]->children;
    }
    
    if (!sourceParams) {
        return std::nullopt;
    }
    
    std::array<double, 3> coords = {0.0, 0.0, 0.0};

    try {
        coords[0] = std::stod(sourceParams->at(0)->value);
        coords[1] = std::stod(sourceParams->at(1)->value);
        coords[2] = std::stod(sourceParams->at(2)->value);
        
        return coords;

    } catch (const std::exception& e) {
        std::cerr << "ERROR during coordinate parsing (std::stod) for Entity " << pointOrDir->id << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

static std::array<double, 3> crossProduct(const std::array<double, 3>& a, const std::array<double, 3>& b) {
    return {
        a[1] * b[2] - a[2] * b[1], 
        a[2] * b[0] - a[0] * b[2], 
        a[0] * b[1] - a[1] * b[0]  
    };
}



static Matrix4x4 buildMatrixFromAxisPlacement(
    const Entity& axisPlacement,
    const std::unordered_map<std::string, const Entity*>& entById) 
{
    Matrix4x4 mat; 
    
    // --- 1. Translation (Location) --- (Attribut 1, Index 0)
    if (axisPlacement.params.size() > 0 && axisPlacement.params[0]) {
        if (auto Location_Opt = parseDirectionOrPoint(axisPlacement.params[0].get(), entById)) {
            mat.m[0][3] = Location_Opt.value()[0]; // Tx
            mat.m[1][3] = Location_Opt.value()[1]; // Ty
            mat.m[2][3] = Location_Opt.value()[2]; // Tz 
        }
    }
    mat.m[3][3] = 1.0; 

    // --- 2. Z-Axis --- (Attribut 2, Index 1)
    std::array<double, 3> ZDir = {0.0, 0.0, 1.0};
    if (axisPlacement.params.size() > 1 && axisPlacement.params[1]) {
        if (auto ZDir_Opt = parseDirectionOrPoint(axisPlacement.params[1].get(), entById)) {
            ZDir = ZDir_Opt.value();
        }
    }
    mat.m[0][2] = ZDir[0]; mat.m[1][2] = ZDir[1]; mat.m[2][2] = ZDir[2];
    

    // --- 3. X-Axis (RefDirection) --- (Attribut 3, Index 2)
    std::array<double, 3> XDir = {1.0, 0.0, 0.0};
    if (axisPlacement.params.size() > 2 && axisPlacement.params[2]) {
        if (auto XDir_Opt = parseDirectionOrPoint(axisPlacement.params[2].get(), entById)) {
            XDir = XDir_Opt.value();
        }
    }
    mat.m[0][0] = XDir[0]; mat.m[1][0] = XDir[1]; mat.m[2][0] = XDir[2];


    // --- 4. Y-Axis (Cross Product) ---
    std::array<double, 3> YVector = crossProduct(ZDir, XDir);
    mat.m[0][1] = YVector[0]; mat.m[1][1] = YVector[1]; mat.m[2][1] = YVector[2];
    
    return mat;
}


static std::optional<Matrix4x4> parseBoundaryPlanePlacement(
    const Entity& boundaryRel,
    const std::unordered_map<std::string, const Entity*>& entById) 
{
    // 1. IFCCONNECTIONSURFACEGEOMETRY find (Attribute 7 / Index 6) 
    if (boundaryRel.params.size() <= 6 || !boundaryRel.params[6]) return std::nullopt;
    const ParamNode* geometryRef = boundaryRel.params[6].get();

    auto itGeom = entById.find(geometryRef->value);
    if (itGeom == entById.end()) return std::nullopt;
    const Entity* geometryEnt = itGeom->second;

    // 2. IFCCURVEBOUNDEDPLANE find (IFCCONNECTIONSURFACEGEOMETRY Attribute 1 / Index 0)
    if (geometryEnt->params.empty() || !geometryEnt->params[0]) return std::nullopt;
    
    auto itSurface = entById.find(geometryEnt->params[0]->value);
    if (itSurface == entById.end()) return std::nullopt;
    const Entity* surfaceEnt = itSurface->second;
    
    // 3. IFCPLANE find (IFCCURVEBOUNDEDPLANE Attribute 1 / Index 0)
    if (surfaceEnt->type != "IFCCURVEBOUNDEDPLANE" || surfaceEnt->params.empty() || !surfaceEnt->params[0]) return std::nullopt;

    auto itPlane = entById.find(surfaceEnt->params[0]->value);
    if (itPlane == entById.end() || itPlane->second->type != "IFCPLANE") return std::nullopt;
    const Entity* planeEnt = itPlane->second;

    // 4. IFCAXIS2PLACEMENT3D find (IFCPLANE Attribute 1 / Index 0)
    if (planeEnt->params.empty() || !planeEnt->params[0]) return std::nullopt;
    const ParamNode* axisPlacementRef = planeEnt->params[0].get();
    
    auto itAxis = entById.find(axisPlacementRef->value);
    if (itAxis == entById.end() || itAxis->second->type != "IFCAXIS2PLACEMENT3D") return std::nullopt;
    const Entity* axisPlacement = itAxis->second;

    // 5. Matrix from IFCAXIS2PLACEMENT3D
    return buildMatrixFromAxisPlacement(*axisPlacement, entById);
}



static std::optional<Matrix4x4> parseLocalPlacement(const Entity& e, 
                                             const std::unordered_map<std::string, const Entity*>& entById) {
    
    // Attribute 6 (Index 5): ObjectPlacement
    if (e.params.size() <= 5 || !e.params[5]) return std::nullopt;
    const ParamNode* placementRef = e.params[5].get(); 

    // 1. Find IfcLocalPlacement
    auto itLocal = entById.find(placementRef->value);
    if (itLocal == entById.end() || itLocal->second->type != "IFCLOCALPLACEMENT") return std::nullopt;
    const Entity* localPlacement = itLocal->second;

    // 2. Find IfcAxis2Placement3D (Attribute 2, Index 1: RelativePlacement)
    if (localPlacement->params.size() <= 1 || !localPlacement->params[1]) return std::nullopt;
    const ParamNode* axisPlacementRef = localPlacement->params[1].get();

    auto itAxis = entById.find(axisPlacementRef->value);
    if (itAxis == entById.end() || itAxis->second->type != "IFCAXIS2PLACEMENT3D") return std::nullopt;
    const Entity* axisPlacement = itAxis->second;

    // 3. Matrix from IFCAXIS2PLACEMENT3D
    return buildMatrixFromAxisPlacement(*axisPlacement, entById);
}


static std::vector<std::array<double, 3>> parsePolylinePoints(
    const Entity& boundaryRel,
    const std::unordered_map<std::string, const Entity*>& entById) 
{
    std::vector<std::array<double, 3>> points;
    
    // 1. ConnectionGeometry find (Attribute 7 / Index 6) 
    if (boundaryRel.params.size() <= 6 || !boundaryRel.params[6]) return points;
    const ParamNode* geometryRef = boundaryRel.params[6].get();

    auto itGeom = entById.find(geometryRef->value);
    if (itGeom == entById.end()) return points;
    const Entity* geometryEnt = itGeom->second;

    // 2. Determine curve reference (Start point of curve traversal)
    const ParamNode* curveRef = nullptr;
    const Entity* surfaceEnt = nullptr;

    // Only IFCCONNECTIONSURFACEGEOMETRY is used in the existing IFC files
    if (geometryEnt->type == "IFCCONNECTIONSURFACEGEOMETRY" && geometryEnt->params.size() > 0 && geometryEnt->params[0]) {
        auto itSurface = entById.find(geometryEnt->params[0]->value);
        if (itSurface == entById.end()) return points;
        surfaceEnt = itSurface->second;
    } else {
        return points;
    }
    
    // 3. If surface found, get outer boundary curve from the surface 
    if (surfaceEnt) {
        // IFCCURVEBOUNDEDPLANE: Attribute 2 (Index 1) is OuterBoundary. 
        if (surfaceEnt->type == "IFCCURVEBOUNDEDPLANE") {
            if (surfaceEnt->params.size() > 1 && surfaceEnt->params[1]) {
                curveRef = surfaceEnt->params[1].get(); 
            }
        }
    }

    if (!curveRef) return points;
    
    // 4. Find curve entity
    auto itCurve = entById.find(curveRef->value);
    if (itCurve == entById.end()) return points;
    const Entity* curveEnt = itCurve->second;

    // 5. Navigate to IFCPOLYLINE (directly or via wrapper)
    const Entity* polylineEnt = nullptr;
    const Entity* currentEnt = curveEnt;
    std::string lastId = ""; 

    for (int i = 0; i < 5 && currentEnt; ++i) {
        if (currentEnt->type == "IFCPOLYLINE") {
            polylineEnt = currentEnt;
            break;
        }
        
        if (currentEnt->id == lastId) break; 
        lastId = currentEnt->id;

        const ParamNode* nextRef = nullptr;
        
        // --- Wrapper logic for types in existing IFCs ---
        if (currentEnt->type == "IFCCOMPOSITECURVE") 
        {
            if (currentEnt->params.size() > 0 && currentEnt->params[0] && !currentEnt->params[0]->children.empty()) {
                nextRef = currentEnt->params[0]->children[0].get();
            }
        }
        else if (currentEnt->type == "IFCCOMPOSITECURVESEGMENT") {
            if (currentEnt->params.size() > 2 && currentEnt->params[2]) {
                 nextRef = currentEnt->params[2].get();
            }
        }
        else if (currentEnt->params.size() > 0 && currentEnt->params[0]) {
             nextRef = currentEnt->params[0].get();
        }
        
        if (nextRef) {
            const ParamNode* targetNode = nextRef->value.empty() && !nextRef->children.empty() 
                                         ? nextRef->children[0].get() 
                                         : nextRef;

            auto itNext = entById.find(targetNode->value);
            if (itNext != entById.end()) {
                 currentEnt = itNext->second;
                 continue; 
            }
        }

        break; 
    }

    if (!polylineEnt) {
        std::cerr << "WARNING: Could not resolve curve entity " << curveEnt->id 
                  << " (Type: " << curveEnt->type << ") to IFCPOLYLINE.\n";
        return points;
    }
    
    // 6. Find points list in IFCPOLYLINE
    if (polylineEnt->params.empty() || !polylineEnt->params[0] || polylineEnt->params[0]->children.empty()) {
        std::cerr << "WARNING: IFCPOLYLINE " << polylineEnt->id << " has no or invalid points list.\n";
        return points;
    }
    
    const std::vector<std::unique_ptr<ParamNode>>* pointRefs = &polylineEnt->params[0]->children;
    
    // 7. Resolve all Cartesian Points
    for (const auto& pointRef : *pointRefs) {
        if (auto p = parseDirectionOrPoint(pointRef.get(), entById)) {
            // Points are in the local 2D coordinate system of the Boundary Plane (Z=0)
            points.push_back(*p);
        } else {
            std::cerr << "WARNING: Failed to parse Cartesian Point for Polyline " << polylineEnt->id << "\n";
        }
    }

    return points;
}