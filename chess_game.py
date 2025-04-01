import pygame
import chess
import chess.engine
import time

# Cấu hình Pygame
pygame.init()
WIDTH, HEIGHT = 600, 600
SQUARE_SIZE = WIDTH // 8
WHITE = (240, 217, 181)
BROWN = (181, 136, 99)
HIGHLIGHT = (186, 202, 68)  # Màu highlight ô được chọn
FONT = pygame.font.Font(None, 36)  # Font hiển thị thời gian

# Load ảnh quân cờ
piece_images = {}
pieces = ['p', 'r', 'n', 'b', 'q', 'k', 'P', 'R', 'N', 'B', 'Q', 'K']
for piece in pieces:
    piece_images[piece] = pygame.transform.scale(
        pygame.image.load(f'assets/png/{piece}.png'), (SQUARE_SIZE, SQUARE_SIZE)
    )

# Hàm vẽ bàn cờ
def draw_board(screen, board, selected_square, player_time, ai_time):
    for row in range(8):
        for col in range(8):
            color = WHITE if (row + col) % 2 == 0 else BROWN
            if selected_square is not None and selected_square == chess.square(col, 7 - row):
                color = HIGHLIGHT  # Highlight ô đang chọn
            pygame.draw.rect(screen, color, (col * SQUARE_SIZE, row * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE))

            piece = board.piece_at(chess.square(col, 7 - row))
            if piece:
                screen.blit(piece_images[piece.symbol()], (col * SQUARE_SIZE, row * SQUARE_SIZE))
    pygame.draw.rect(screen, color, (8 * SQUARE_SIZE, 0 * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE))

    # Hiển thị thời gian
    player_text = FONT.render(f"Player: {player_time:.2f}s", True, (255, 255, 255))
    ai_text = FONT.render(f"Stockfish: {ai_time:.2f}s", True, (255, 255, 255))
    screen.blit(player_text, (10, 10))  # Góc trái trên
    screen.blit(ai_text, (WIDTH - ai_text.get_width() - 10, 10))  # Góc phải trên

# Khởi tạo Stockfish
engine = chess.engine.SimpleEngine.popen_uci("/usr/bin/stockfish")

board = chess.Board()

# Khởi tạo cửa sổ Pygame
screen = pygame.display.set_mode((WIDTH+SQUARE_SIZE, HEIGHT))
pygame.display.set_caption("Người vs Stockfish")

running = True
selected_square = None  # Vị trí quân cờ được chọn
player_turn = True  # True nếu đến lượt người chơi
player_time, ai_time = 0, 0  # Thời gian suy nghĩ của mỗi bên

while running:
    screen.fill((0, 0, 0))
    draw_board(screen, board, selected_square, player_time, ai_time)
    pygame.display.flip()

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

        if player_turn and event.type == pygame.MOUSEBUTTONDOWN:
            start_time = time.time()  # Bắt đầu đo thời gian
            x, y = pygame.mouse.get_pos()
            col, row = x // SQUARE_SIZE, 7 - (y // SQUARE_SIZE)
            square = chess.square(col, row)

            if selected_square is None:
                # Chọn quân cờ nếu nó là của người chơi (trắng)
                piece = board.piece_at(square)
                if piece and piece.color == chess.WHITE:
                    selected_square = square
            else:
                # Thử di chuyển quân cờ
                move = chess.Move(selected_square, square)
                if move in board.legal_moves:
                    board.push(move)
                    player_time = time.time() - start_time  # Đo thời gian người chơi
                    player_turn = False  # Đến lượt Stockfish
                selected_square = None  # Bỏ chọn quân cờ
    # Stockfish chơi khi đến lượt
    if not player_turn and not board.is_game_over():
        start_time = time.time()  # Bắt đầu đo thời gian của AI
        result = engine.play(board, chess.engine.Limit(time=0.0000001))
        board.push(result.move)
        ai_time = time.time() - start_time  # Đo thời gian của Stockfish
        player_turn = True  # Đến lượt người chơi

    # Kiểm tra kết thúc game
    if board.is_game_over():
        print("Game Over:", board.result())
        # pygame.time.wait(3000)
        running = False

engine.quit()
pygame.quit()
