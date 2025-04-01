import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import heapq

# Create a directed graph
G = nx.Graph()

# Add edges based on the given image
edges = [
    ('S', 'A'),
    ('S', 'D'),
    ('A', 'B'),
    ('A', 'D'),
    ('B', 'C'),
    ('B', 'E'),
    ('D', 'E'),
    ('E', 'F'),
    ('F', 'G')
]

G.add_edges_from(edges)
# Define node positions for better visualization
pos = nx.spring_layout(G, seed=42)

heuristic = {
    "S": 11,
    "A": 10.4,
    "B": 6.7,
    "C": 4,
    "D": 8.9,
    "E": 6.9,
    "F": 3,
    "G": 0
}


# BFS traversal function
def hill_climbing(graph, start, goal):
    queue = [start]
    order =[]
    while queue:
        top = queue.pop(0)
        order.append(top)
        if top == goal:
            return order
        L = []
        for node in graph.neighbors(top):
            heapq.heappush(L,(heuristic[node], node))
        L1=[]
        for priority, node in L:
            L1.append(node)
        L1.extend(queue)
        queue = L1
    return order


# Get BFS order from node 'A'
bfs_order = hill_climbing(G, "S", "G")

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
