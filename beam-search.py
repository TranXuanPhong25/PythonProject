import networkx as nx
from collections import deque
import heapq
import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.pyplot as plt

# Khởi tạo đồ thị có trọng số
G = nx.Graph()

# Danh sách các cạnh với trọng số
edges = [
    ("S", "A", 3), ("S", "D", 4), ("A", "B", 4), ("A", "D", 5),
    ("B", "C", 4), ("B", "E", 5), ("D", "E", 2), ("E", "F", 4),
    ("F", "G", 3)
]

# Thêm các cạnh vào đồ thị
G.add_weighted_edges_from(edges)

# Vị trí cố định để vẽ đồ thị
pos = {
    "S": (0, 2), "A": (1, 3), "B": (2, 3), "C": (3, 3),
    "D": (1, 1), "E": (2, 1), "F": (3, 1), "G": (4, 1)
}

K = 2

# SADEBFCGA
def beam_search(graph, start, goal):
    pq = [(0, start)]  # Priority queue
    order = []
    visited = set()

    while pq:
        priority, current = heapq.heappop(pq)

        if current == goal:
            order.append(current)  # Include goal in the order
            return order

        if current in visited:
            continue

        visited.add(current)
        order.append(current)

        best_nodes = []
        for node in graph.neighbors(current):
            if node not in visited:
                best_nodes.append((graph[current][node].get("weight", 1)+priority, node))

        # Convert best_nodes into a heap before popping elements
        heapq.heapify(best_nodes)

        # Select top-K best nodes
        for _ in range(min(K, len(best_nodes))):
            heapq.heappush(pq, heapq.heappop(best_nodes))

    return order  # If no path is found, return visited order

order_path = beam_search(G, "S", "G")
# order_path = nx.shortest_path(G, "S", "G")

# Animation setup
fig, ax = plt.subplots()


def update(frame):
    ax.clear()

    # Get nodes visited up to current frame
    visited_nodes = set(order_path[:frame + 1])
    node_colors = ["red" if node in visited_nodes else "lightblue" for node in G.nodes]

    # Draw the graph with updated colors
    nx.draw(G, pos, with_labels=True, node_color=node_colors, node_size=2000, font_size=15, ax=ax)


# Create animation
ani = animation.FuncAnimation(fig, update, frames=len(order_path), interval=1000, repeat=False)

plt.show()
