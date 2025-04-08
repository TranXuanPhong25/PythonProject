import os
from dotenv import load_dotenv

import pygame
import chess
import chess.engine
import time

load_dotenv()

# Cấu hình Pygame
pygame.init()
WIDTH, HEIGHT = 600, 600
SQUARE_SIZE = WIDTH // 8
WHITE = (240, 217, 181)
BROWN = (181, 136, 99)
HIGHLIGHT = (186, 202, 68)  # Màu highlight ô được chọn
FONT = pygame.font.Font(None, 36)  # Font hiển thị thời gian
MENU_FONT = pygame.font.Font(None, 48)  # Font cho menu
AI_TIME_LIMIT = float(os.getenv("AI_TIME_LIMIT", 10))
ENGINE_PATH = os.getenv("ENGINE_PATH", "python uci/claude.py").split()

# Game states
MENU = 0
PLAYING = 1
GAME_OVER = 2

# Load ảnh quân cờ
piece_images = {}
pieces = ['p', 'r', 'n', 'b', 'q', 'k', 'P', 'R', 'N', 'B', 'Q', 'K']
for piece in pieces:
    if piece.islower():
        piece_images[piece] = pygame.transform.scale(
            pygame.image.load(f'assets/black/{piece}.png'), (SQUARE_SIZE, SQUARE_SIZE)
        )
    else:
        piece_images[piece] = pygame.transform.scale(
            pygame.image.load(f'assets/white/{piece}.png'), (SQUARE_SIZE, SQUARE_SIZE)
        )
def get_promotion_choice(screen):
    """Display a UI for selecting promotion piece"""
    pieces = ["q", "r", "n", "b"]  # Queen, rook, knight, bishop
    
    # Draw background
    overlay = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
    overlay.fill((0, 0, 0, 128))  # Semi-transparent black
    screen.blit(overlay, (0, 0))
    
    # Draw promotion options
    option_width = SQUARE_SIZE * 2
    option_height = SQUARE_SIZE
    option_rect = []
    
    title = FONT.render("Choose promotion piece:", True, (255, 255, 255))
    screen.blit(title, (WIDTH//2 - title.get_width()//2, HEIGHT//3 - 40))
    
    for i, piece in enumerate(pieces):
        rect = pygame.Rect(WIDTH//2 - option_width*2 + i*option_width, HEIGHT//3, 
                          option_width, option_height)
        option_rect.append(rect)
        
        pygame.draw.rect(screen, (200, 200, 200), rect)
        screen.blit(piece_images[piece.upper()], 
                   (rect.x + option_width//2 - SQUARE_SIZE//2, rect.y))
    
    pygame.display.flip()
    
    # Wait for selection
    while True:
        for event in pygame.event.get():
            if event.type == pygame.MOUSEBUTTONDOWN:
                for i, rect in enumerate(option_rect):
                    if rect.collidepoint(event.pos):
                        return pieces[i]

# Hàm vẽ bàn cờ
def draw_board(screen, board, selected_square, player_time, ai_time, depth):
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

    # Hiển thị thời gian và độ sâu
    player_text = FONT.render(f"Player: {player_time:.2f}s", True, (255, 255, 255))
    ai_text = FONT.render(f"AI: {ai_time:.2f}s", True, (255, 255, 255))
    depth_text = FONT.render(f"Depth: {depth}", True, (255, 255, 255))
    screen.blit(player_text, (10, 10))  # Góc trái trên
    screen.blit(ai_text, (WIDTH - ai_text.get_width() - 10, 10))  # Góc phải trên
    screen.blit(depth_text, (WIDTH // 2 - depth_text.get_width() // 2, 10))  # Giữa trên


# Hàm hiển thị menu chọn độ sâu
def draw_menu(screen, input_text):
    screen.fill((50, 50, 50))
    title = MENU_FONT.render("Select AI Search Depth", True, (255, 255, 255))
    screen.blit(title, (WIDTH // 2 - title.get_width() // 2, 100))

    input_box = pygame.Rect(WIDTH // 2 - 50, 200, 100, 50)
    pygame.draw.rect(screen, (100, 100, 200), input_box)
    text_surface = FONT.render(input_text, True, (255, 255, 255))
    screen.blit(text_surface, (input_box.x + 5, input_box.y + 10))

    return input_box


# Hàm hiển thị màn hình kết thúc game
def draw_game_over(screen, result):
    screen.fill((50, 50, 50))
    title = MENU_FONT.render("Game Over", True, (255, 255, 255))
    screen.blit(title, (WIDTH // 2 - title.get_width() // 2, 100))

    result_text = FONT.render(f"Result: {result}", True, (255, 255, 255))
    screen.blit(result_text, (WIDTH // 2 - result_text.get_width() // 2, 200))

    replay_button = pygame.Rect(WIDTH // 2 - 100, 300, 200, 50)
    pygame.draw.rect(screen, (100, 100, 200), replay_button)
    replay_text = FONT.render("Replay", True, (255, 255, 255))
    screen.blit(replay_text, (replay_button.centerx - replay_text.get_width() // 2,
                              replay_button.centery - replay_text.get_height() // 2))

    return replay_button


# Khởi tạo biến
board = chess.Board()
game_state = MENU  # Start in menu state
ai_depth = 3  # Default depth
input_text = ''  # Text input for depth

# Khởi tạo cửa sổ Pygame
screen = pygame.display.set_mode((WIDTH + SQUARE_SIZE, HEIGHT))
pygame.display.set_caption("Người vs AI")

running = True
selected_square = None  # Vị trí quân cờ được chọn
player_turn = True  # True nếu đến lượt người chơi
player_time, ai_time = 0, 0  # Thời gian suy nghĩ của mỗi bên

# Khởi tạo engine
engine = None

while running:
    if game_state == MENU:
        input_box = draw_menu(screen, input_text)
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            if event.type == pygame.MOUSEBUTTONDOWN:
                if input_box.collidepoint(event.pos):
                    input_active = True
                else:
                    input_active = False

            if event.type == pygame.KEYDOWN and input_active:
                if event.key == pygame.K_RETURN:
                    try:
                        ai_depth = int(input_text)
                        game_state = PLAYING
                        # Khởi tạo engine sau khi đã chọn độ sâu
                        engine = chess.engine.SimpleEngine.popen_uci(ENGINE_PATH)
                        board = chess.Board()  # Reset board
                        player_turn = True  # Reset turn
                        player_time, ai_time = 0, 0  # Reset times
                        input_text = ''  # Reset input text
                    except ValueError:
                        input_text = ''  # Reset input text on invalid input
                elif event.key == pygame.K_BACKSPACE:
                    input_text = input_text[:-1]
                else:
                    input_text += event.unicode

    elif game_state == PLAYING:
        screen.fill((0, 0, 0))
        draw_board(screen, board, selected_square, player_time, ai_time, ai_depth)
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
                    # Check if this is potentially a pawn promotion move
                    piece = board.piece_at(selected_square)
                    is_promotion = False

                    if piece and piece.piece_type == chess.PAWN:
                        # Check if white pawn is moving to 8th rank (promotion)
                        if chess.square_rank(square) == 7:
                            is_promotion = True

                    if is_promotion:
                        # Get promotion piece choice from player
                        promotion_choice = get_promotion_choice(screen)
                        
                        # Map the promotion choice to the correct chess piece type
                        promotion_piece = None
                        if promotion_choice == 'q':
                            promotion_piece = chess.QUEEN
                        elif promotion_choice == 'r':
                            promotion_piece = chess.ROOK
                        elif promotion_choice == 'b':
                            promotion_piece = chess.BISHOP
                        elif promotion_choice == 'n':
                            promotion_piece = chess.KNIGHT
                            
                        move = chess.Move(selected_square, square, promotion=promotion_piece)
                    else:
                        # Normal move
                        move = chess.Move(selected_square, square)

                    if move in board.legal_moves:
                        board.push(move)
                        player_time = time.time() - start_time  # Đo thời gian người chơi
                        player_turn = False  # Đến lượt AI
                    selected_square = None  # Bỏ chọn quân cờ
                # select square at top right corner to restore last move
                if x > WIDTH - SQUARE_SIZE and y < SQUARE_SIZE:
                    board.pop()

        # AI chơi khi đến lượt
        if not player_turn and not board.is_game_over():
            start_time = time.time()  # Bắt đầu đo thời gian của AI
            try:
                # Sử dụng độ sâu và thời gian được chọn
                result = engine.play(board, chess.engine.Limit(depth=ai_depth, time=AI_TIME_LIMIT))
                if result.move is not None:
                    board.push(result.move)
                else:
                    # If engine returned None, select a legal move
                    legal_moves = list(board.legal_moves)
                    if legal_moves:
                        random_move = legal_moves[0]
                        board.push(random_move)
                        print("Engine returned no move, using fallback move")
            except Exception as e:
                print(f"Error during AI move calculation: {e}")
                # Fallback to selecting any legal move
                legal_moves = list(board.legal_moves)
                if legal_moves:
                    random_move = legal_moves[0]
                    board.push(random_move)

            ai_time = time.time() - start_time  # Đo thời gian của AI
            player_turn = True  # Đến lượt người chơi

        # Kiểm tra kết thúc game
        if board.is_game_over():
            game_state = GAME_OVER
            game_result = board.result()

    elif game_state == GAME_OVER:
        replay_button = draw_game_over(screen, game_result)
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            if event.type == pygame.MOUSEBUTTONDOWN:
                pos = pygame.mouse.get_pos()
                if replay_button.collidepoint(pos):
                    game_state = MENU

# Đóng engine và pygame
if engine:
    engine.quit()
pygame.quit()
