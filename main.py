import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import heapq

# Create a directed graph
G = nx.DiGraph()

# Add edges based on the given image
edges = [
    ("S", "A"),
    ("A", "B"), ("A", "D"),
    ("B", "C"), ("D", "T"),
    ("E", "D"), ("A", "E"),
    ("D", "T")
]
G.add_edges_from(edges)
# Define node positions for better visualization
pos = nx.spring_layout(G, seed=42)

heuristic = {
    "A": 5,
    "B": 6,
    "C": 7,
    "D": 2,
    "E": 1,
    "S": 6,
    "T": 0,
}
# BFS traversal function
def greedy_best_first_search(graph, start, goal):
    L = []
    heapq.heappush(L, (heuristic[start], start))  # Push as (priority, node)
    order = []

    while L:
        priority,task = heapq.heappop(L)
        order.append(task)
        if task == goal: return order
        for node in graph.neighbors(task):
            heapq.heappush(L, (heuristic[node], node))

    return order


# Get BFS order from node 'A'
bfs_order = greedy_best_first_search(G, "S", "T")

# Animation setup
fig, ax = plt.subplots()


def update(frame):
    ax.clear()

    # Get nodes visited up to current frame
    visited_nodes = set(bfs_order[:frame + 1])
    node_colors = ["red" if node in visited_nodes else "lightblue" for node in G.nodes]

    # Draw the graph with updated colors
    nx.draw(G, pos, with_labels=True, node_color=node_colors, node_size=2000, font_size=15, ax=ax)


# Create animation
ani = animation.FuncAnimation(fig, update, frames=len(bfs_order), interval=1000, repeat=False)

plt.show()
