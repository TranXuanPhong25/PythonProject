import numpy as np
import heapq
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import random


def generate_maze(height, width):
    """
    Tạo mê cung bằng thuật toán recursive backtracking.
    height, width: số hàng, cột (phải là số lẻ)
    Mỗi ô: 1 là tường, 0 là đường đi.
    """
    maze = np.ones((height, width), dtype=int)
    start_cell = (1, 1)
    maze[start_cell] = 0
    stack = [start_cell]

    while stack:
        x, y = stack[-1]
        neighbors = []
        for dx, dy in [(2, 0), (-2, 0), (0, 2), (0, -2)]:
            nx, ny = x + dx, y + dy
            # Kiểm tra xem ô láng giềng có trong giới hạn và chưa mở cửa (vẫn là tường)
            if 0 < nx < height - 1 and 0 < ny < width - 1 and maze[nx, ny] == 1:
                neighbors.append((nx, ny))
        if neighbors:
            next_cell = random.choice(neighbors)
            # Xóa tường giữa ô hiện tại và ô kế tiếp
            maze[(x + next_cell[0]) // 2, (y + next_cell[1]) // 2] = 0
            maze[next_cell] = 0
            stack.append(next_cell)
        else:
            stack.pop()
    return maze


def heuristic(a, b):
    """Hàm heuristic: Khoảng cách Manhattan"""
    return abs(a[0] - b[0]) + abs(a[1] - b[1])


def astar(grid, start, goal):
    """Thuật toán A* tìm đường đi ngắn nhất từ start đến goal"""
    rows, cols = grid.shape
    open_list = []  # Hàng đợi ưu tiên
    heapq.heappush(open_list, (0, start))
    came_from = {start: None}
    g_score = {start: 0}
    f_score = {start: heuristic(start, goal)}
    explored = []

    while open_list:
        _, current = heapq.heappop(open_list)
        explored.append(current)
        if current == goal:
            path = []
            while current:
                path.append(current)
                current = came_from[current]
            return path[::-1], explored  # Trả về đường đi từ start đến goal
        for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:  # 4 hướng di chuyển
            neighbor = (current[0] + dx, current[1] + dy)
            if 0 <= neighbor[0] < rows and 0 <= neighbor[1] < cols and grid[neighbor] == 0:
                tentative_g_score = g_score[current] + 1
                if neighbor not in g_score or tentative_g_score < g_score[neighbor]:
                    came_from[neighbor] = current
                    g_score[neighbor] = tentative_g_score
                    f_score[neighbor] = tentative_g_score + heuristic(neighbor, goal)
                    heapq.heappush(open_list, (f_score[neighbor], neighbor))
    return None, explored  # Không tìm thấy đường đi


def animate_path(grid, start, goal, path, explored):
    """Tạo animation cho quá trình tìm đường"""
    fig, ax = plt.subplots(figsize=(20, 20))
    ax.set_xlim(-0.5, grid.shape[1] - 0.5)
    ax.set_ylim(-0.5, grid.shape[0] - 0.5)
    # ax.set_xticks(range(grid.shape[1]))

    # ax.set_yticks(range(grid.shape[0]))

    # Vẽ các tường của mê cung
    for i in range(grid.shape[0]):
        for j in range(grid.shape[1]):
            if grid[i, j] == 1:
                ax.fill_between([j - 0.5, j + 0.5], i - 0.5, i + 0.5, color='gray')

    ax.scatter(start[1], start[0], color='red', s=200, marker='*', label='Start')
    ax.scatter(goal[1], goal[0], color='green', s=200, marker='*', label='Goal')

    explored_points = []
    path_points = []

    def update(frame):
        if frame < len(explored):
            (x, y) = explored[frame]
            explored_points.append(ax.scatter(y, x, color='orange', s=50))
        elif frame - len(explored) < len(path):
            (x, y) = path[frame - len(explored)]
            path_points.append(ax.scatter(y, x, color='blue', s=50))
        return explored_points + path_points

    ani = animation.FuncAnimation(fig, update, frames=len(explored) + len(path), interval=10, blit=True)
    plt.show()


# --- Main Program ---
maze = [
    "1111111111111111111111111111",
    "1000000000110000000010000001",
    "1011111110111110111010111101",
    "1000000000000000111010111101",
    "1011111110111110011010111101",
    "1000001000000110000000000001",
    "1010111011110111011011011101",
    "1000000011110111000010010001",
    "1011111011110111011011011101",
    "1000000000000000000010000001",
    "1111111111111111111111111111"
]

# Chuyển đổi map thành ma trận numpy (0: đường, 1: tường)
grid = np.array([[1 if cell == '1' else 0 for cell in row] for row in maze], dtype=int)

# Xác định điểm bắt đầu và đích
start = (1, 1)      # Chọn vị trí trong ô mở
goal = (9, 25)      # Chọn một vị trí mở ở phía đối diện
# Tìm đường đi với A*
path, explored = astar(grid, start, goal)
if path is None:
    print("Không tìm thấy đường đi!")
else:
    animate_path(grid, start, goal, path, explored)
