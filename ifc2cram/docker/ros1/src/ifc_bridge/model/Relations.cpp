#include "Relations.hpp"
#include <iostream>

static const ParamNode* at(
    const std::vector<std::unique_ptr<ParamNode>>& v, std::size_t i) {
    return (i < v.size() && v[i]) ? v[i].get() : nullptr;
}

static const char* val(const ParamNode* p) {
    if (!p) return nullptr;
    return p->value.c_str();
}

static std::vector<std::string> listChildren(const ParamNode* list) {
    std::vector<std::string> out;
    if (!list) return out;
    for (const auto& ch : list->children) {
        if (!ch) continue;
        out.emplace_back(ch->value);
    }
    return out;
}

bool isRelAggregates(const Entity& e) { return e.type == "IFCRELAGGREGATES"; }
bool isRelContainedInSpatialStructure(const Entity& e) { return e.type == "IFCRELCONTAINEDINSPATIALSTRUCTURE"; }
bool isRelSpaceBoundary(const Entity& e) { return e.type == "IFCRELSPACEBOUNDARY"; }
bool isRelDefinesByProperties(const Entity& e) { return e.type == "IFCRELDEFINESBYPROPERTIES"; }


std::optional<std::string> getRelatingId(const Entity& e) {
    if (isRelAggregates(e)) {
        if (const char* s = val(at(e.params, 4))) return std::string(s);
    } else if (isRelContainedInSpatialStructure(e)) {
        if (const char* s = val(at(e.params, 5))) return std::string(s);
    } else if (isRelSpaceBoundary(e)) {
        if (const char* s = val(at(e.params, 4))) return std::string(s);
    }
    return std::nullopt;
}

std::vector<std::string> getRelatedIds(const Entity& e) {
    if (isRelAggregates(e)) {
        return listChildren(at(e.params, 5));
    } else if (isRelContainedInSpatialStructure(e)) {
        return listChildren(at(e.params, 4));
    } else if (isRelSpaceBoundary(e)) {
        const char* s = val(at(e.params, 5));
        if (s && s[0] != '\0' && s[0] != '$') return { std::string(s) };
    }
    return {};
}
