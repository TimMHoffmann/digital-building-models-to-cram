# ...existing code...
#!/usr/bin/env python3
import networkx as nx
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from collections import defaultdict
import pickle
from typing import Any

GP = "/tmp/ifc_spatial_graph.gpickle"

def color_by_type(G):
    cmap = {"space":"tab:blue","door":"tab:green","column":"tab:orange","virtual":"tab:purple","element":"tab:gray","boundary":"tab:olive"}
    colors = []
    for n,d in G.nodes(data=True):
        t = d.get("type","element")
        colors.append(cmap.get(t, "black"))
    return colors

def label_for_node(n, d):
    t = d.get("type","")
    if t == "space":
        return d.get("data",{}).get("space_name", n)
    data = d.get("data", {})
    return data.get("element_id") or data.get("boundary_id") or n

def load_graph(path: str) -> nx.Graph:
    # robust loader: try networkx helpers, then pickle fallback
    try:
        if hasattr(nx, "read_gpickle"):
            return nx.read_gpickle(path)
    except Exception:
        pass
    try:
        # some networkx installs expose read_gpickle under readwrite.gpickle
        from networkx.readwrite import gpickle as nx_gpickle
        return nx_gpickle.read_gpickle(path)
    except Exception:
        pass
    # final fallback: pickle.load
    with open(path, "rb") as f:
        return pickle.load(f)

def main():
    G = load_graph(GP)
    pos = nx.spring_layout(G, k=0.8, iterations=150)
    colors = color_by_type(G)
    plt.figure(figsize=(14,10))
    nx.draw_networkx_nodes(G, pos, node_color=colors, node_size=[100 if d.get("type")!="space" else 600 for _,d in G.nodes(data=True)])
    nx.draw_networkx_edges(G, pos, alpha=0.6)
    labels = {n: label_for_node(n,d) for n,d in G.nodes(data=True) if d.get("type")=="space" or d.get("type")=="door"}
    nx.draw_networkx_labels(G, pos, labels, font_size=9)
    plt.axis("off")
    plt.title("IFC spatial graph (spaces + elements)")
    plt.tight_layout()
    out = "/root/cram_ws/src/ifc_tree_to_graph/ifc_spatial_graph.png"
    plt.savefig(out, dpi=150)
    print("Saved", out)
    # no plt.show() in headless/container mode

if __name__ == "__main__":
    main()