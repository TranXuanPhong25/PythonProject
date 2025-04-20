import os
from dotenv import load_dotenv

import pygame
import chess
import chess.engine
import time

load_dotenv()

# Cấu hình Pygame
LAST_MOVE = (255, 255, 0)  # Light yellow color for last move highlighting
pygame.init()
WIDTH, HEIGHT = 840, 860
SQUARE_SIZE = 100
WHITE = (240, 217, 181)
BROWN = (181, 136, 99)
HIGHLIGHT = (186, 202, 68)  # Màu highlight ô được chọn
FONT = pygame.font.Font(None, 36)  # Font hiển thị thời gian
MENU_FONT = pygame.font.Font(None, 48)  # Font cho menu
AI_TIME_LIMIT = float(os.getenv("AI_TIME_LIMIT", 10))
ENGINE_PATH = os.getenv("ENGINE_PATH", "chess-engine/bin/uci.exe").split()
CUSTOM_FEN = os.getenv("CUSTOM_FEN",
                       "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")  # Empty string if not set
# Add this constant near the top with your other color definitions
LEGAL_MOVE_BORDER = (255, 0, 0)  # Red border for legal moves
LEGAL_MOVE_INDICATOR = (255, 255, 255, 170)  # Semi-transparent red for move indicators

# Game states
MODE_SELECTION = -1  # New state for mode selection
HUMAN_VS_HUMAN = 0
HUMAN_VS_BOT = 1
BOT_VS_HUMAN = 2
BOT_VS_BOT = 3

MENU = 4  # Change from 0 to 4
PLAYING = 5  # Change from 1 to 5
GAME_OVER = 6  # Change from 2 to 6

# Add these variables near the beginning of your game initialization
last_move_from = None
last_move_to = None

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
    screen.blit(title, (WIDTH // 2 - title.get_width() // 2, HEIGHT // 3 - 40))

    for i, piece in enumerate(pieces):
        rect = pygame.Rect(WIDTH // 2 - option_width * 2 + i * option_width, HEIGHT // 3,
                           option_width, option_height)
        option_rect.append(rect)

        pygame.draw.rect(screen, (200, 200, 200), rect)
        screen.blit(piece_images[piece.upper()],
                    (rect.x + option_width // 2 - SQUARE_SIZE // 2, rect.y))

    pygame.display.flip()

    # Wait for selection
    while True:
        for event in pygame.event.get():
            if event.type == pygame.MOUSEBUTTONDOWN:
                for i, rect in enumerate(option_rect):
                    if rect.collidepoint(event.pos):
                        return pieces[i]


# Hàm vẽ bàn cờ
def draw_board(screen, board, selected_square, player_time, ai_time, depth, last_move_from=None, last_move_to=None):
    for row in range(8):
        for col in range(8):
            color = WHITE if (row + col) % 2 == 0 else BROWN
            # Square position
            square = chess.square(col, 7 - row)

            # Highlight selected square
            if selected_square is not None and selected_square == square:
                color = HIGHLIGHT
            # Highlight last move squares
            elif last_move_from is not None and square == last_move_from:
                color = LAST_MOVE
            elif last_move_to is not None and square == last_move_to:
                color = LAST_MOVE

            pygame.draw.rect(screen, color, (20 + col * SQUARE_SIZE, 40 + row * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE))

            piece = board.piece_at(square)
            if piece:
                screen.blit(piece_images[piece.symbol()], (20 + col * SQUARE_SIZE, 40 + row * SQUARE_SIZE))

    # Add this code to show legal moves with center circles
    if selected_square is not None:
        for move in board.legal_moves:
            if move.from_square == selected_square:
                # Draw a circle in the center of possible destination squares
                to_col = chess.square_file(move.to_square)
                to_row = 7 - chess.square_rank(move.to_square)

                # Calculate center position of the square
                center_x = 20 + to_col * SQUARE_SIZE + SQUARE_SIZE // 2
                center_y = 40 + to_row * SQUARE_SIZE + SQUARE_SIZE // 2

                # Create a surface with per-pixel alpha for semi-transparency
                circle_surface = pygame.Surface((SQUARE_SIZE, SQUARE_SIZE), pygame.SRCALPHA)

                # Draw circle on the surface - using 1/4 of square size for radius
                circle_radius = SQUARE_SIZE // 6
                pygame.draw.circle(circle_surface, LEGAL_MOVE_INDICATOR,
                                   (SQUARE_SIZE // 2, SQUARE_SIZE // 2), circle_radius)

                # Blit the circle surface onto the screen
                screen.blit(circle_surface, (20 + to_col * SQUARE_SIZE, 40 + to_row * SQUARE_SIZE))

    label_font = pygame.font.Font(None, 24)

    for col in range(8):
        file_label = chr(ord('a') + col)
        text_color = BROWN if col % 2 == 0 else WHITE
        label = label_font.render(file_label, True, text_color)
        screen.blit(label, (col * SQUARE_SIZE + 65, HEIGHT - 20))

        # Draw rank labels (1 through 8) on the left
    for row in range(8):
        rank_label = str(8 - row)
        text_color = BROWN if row % 2 == 1 else WHITE
        label = label_font.render(rank_label, True, text_color)
        screen.blit(label, (5, row * SQUARE_SIZE + 85))

    # Hiển thị thời gian và độ sâu
    if game_mode != HUMAN_VS_HUMAN:
        ai_text = FONT.render(f"AI: {ai_time:.2f}s", True, (255, 255, 255))
        screen.blit(ai_text, (WIDTH - ai_text.get_width() - 10, 10))
        depth_text = FONT.render(f"Depth: {depth}", True, (255, 255, 255))
        screen.blit(depth_text, (20, 10))


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

    # result_text = FONT.render(f"Result: {result}", True, (255, 255, 255))
    # screen.blit(result_text, (WIDTH // 2 - result_text.get_width() // 2, 200))

    replay_button = pygame.Rect(WIDTH // 2 - 100, 300, 200, 50)
    pygame.draw.rect(screen, (100, 100, 200), replay_button)
    replay_text = FONT.render("Replay", True, (255, 255, 255))
    screen.blit(replay_text, (replay_button.centerx - replay_text.get_width() // 2,
                              replay_button.centery - replay_text.get_height() // 2))

    return replay_button


# Add a new function to draw the mode selection screen
def draw_mode_selection(screen):
    screen.fill((50, 50, 50))
    title = MENU_FONT.render("Select Game Mode", True, (255, 255, 255))
    screen.blit(title, (WIDTH // 2 - title.get_width() // 2, 100))

    button_width = 370
    button_height = 60
    button_spacing = 20

    buttons = []

    # Human vs Human button
    hvh_button = pygame.Rect(WIDTH // 2 - button_width // 2, 200, button_width, button_height)
    buttons.append((hvh_button, HUMAN_VS_HUMAN))
    pygame.draw.rect(screen, (100, 100, 200), hvh_button)
    hvh_text = FONT.render("Human vs Human", True, (255, 255, 255))
    screen.blit(hvh_text, (hvh_button.centerx - hvh_text.get_width() // 2,
                           hvh_button.centery - hvh_text.get_height() // 2))

    # Human vs Bot button
    hvb_button = pygame.Rect(WIDTH // 2 - button_width // 2,
                             200 + button_height + button_spacing,
                             button_width, button_height)
    buttons.append((hvb_button, HUMAN_VS_BOT))
    pygame.draw.rect(screen, (100, 100, 200), hvb_button)
    hvb_text = FONT.render("Human (White) vs Bot (Black)", True, (255, 255, 255))
    screen.blit(hvb_text, (hvb_button.centerx - hvb_text.get_width() // 2,
                           hvb_button.centery - hvb_text.get_height() // 2))

    # Bot vs Human button
    bvh_button = pygame.Rect(WIDTH // 2 - button_width // 2,
                             200 + 2 * (button_height + button_spacing),
                             button_width, button_height)
    buttons.append((bvh_button, BOT_VS_HUMAN))
    pygame.draw.rect(screen, (100, 100, 200), bvh_button)
    bvh_text = FONT.render("Bot (White) vs Human (Black)", True, (255, 255, 255))
    screen.blit(bvh_text, (bvh_button.centerx - bvh_text.get_width() // 2,
                           bvh_button.centery - bvh_text.get_height() // 2))

    # Bot vs Bot button
    bvb_button = pygame.Rect(WIDTH // 2 - button_width // 2,
                             200 + 3 * (button_height + button_spacing),
                             button_width, button_height)
    buttons.append((bvb_button, BOT_VS_BOT))
    pygame.draw.rect(screen, (100, 100, 200), bvb_button)
    bvb_text = FONT.render("Bot vs Bot", True, (255, 255, 255))
    screen.blit(bvb_text, (bvb_button.centerx - bvb_text.get_width() // 2,
                           bvb_button.centery - bvb_text.get_height() // 2))

    # Add a note about depth selection
    note_text = FONT.render("(Human vs Human will skip AI depth selection)", True, (200, 200, 200))
    screen.blit(note_text, (WIDTH // 2 - note_text.get_width() // 2,
                            200 + 4 * (button_height + button_spacing) + 20))

    return buttons


# Khởi tạo biến
board = chess.Board(CUSTOM_FEN)

game_state = MODE_SELECTION  # Start in mode selection state
game_mode = None
ai_depth = 3  # Default depth
input_text = ''  # Text input for depth

# Khởi tạo cửa sổ Pygame
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Người vs AI")

running = True
selected_square = None  # Vị trí quân cờ được chọn
player_turn = True  # True nếu đến lượt người chơi
player_time, ai_time = 0, 0  # Thời gian suy nghĩ của mỗi bên

# Khởi tạo engine
engine = None

while running:
    if game_state == MODE_SELECTION:
        buttons = draw_mode_selection(screen)
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            if event.type == pygame.MOUSEBUTTONDOWN:
                pos = pygame.mouse.get_pos()
                for button, mode in buttons:
                    if button.collidepoint(pos):
                        game_mode = mode

                        # Skip depth selection for Human vs Human mode
                        if game_mode == HUMAN_VS_HUMAN:
                            # Initialize the game directly
                            game_state = PLAYING
                            board = chess.Board()
                            player_turn = True
                            player_time, ai_time = 0, 0
                        else:
                            # For AI modes, proceed to depth selection
                            game_state = MENU

    elif game_state == MENU:
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
                        if game_mode != HUMAN_VS_HUMAN:  # Only init engine for modes with AI
                            engine = chess.engine.SimpleEngine.popen_uci(ENGINE_PATH)
                        board = chess.Board(CUSTOM_FEN)  # Reset board
                        # Set the initial turn based on the game mode
                        if game_mode == HUMAN_VS_BOT or game_mode == HUMAN_VS_HUMAN:
                            player_turn = True
                        else:  # BOT_VS_HUMAN or BOT_VS_BOT
                            player_turn = False
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
        draw_board(screen, board, selected_square, player_time, ai_time, ai_depth, last_move_from, last_move_to)
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            # Handle human moves (White in HUMAN_VS_BOT, Black in BOT_VS_HUMAN, both in HUMAN_VS_HUMAN)
            if event.type == pygame.MOUSEBUTTONDOWN and (
                    (game_mode == HUMAN_VS_BOT and player_turn) or
                    (game_mode == BOT_VS_HUMAN and player_turn) or
                    (game_mode == HUMAN_VS_HUMAN)
            ):
                start_time = time.time()  # Start timing
                x, y = pygame.mouse.get_pos()
                # Convert screen position to board coordinates, adjust for the board offset
                col = (x - 20) // SQUARE_SIZE
                row = (y - 40) // SQUARE_SIZE
                if 0 <= col < 8 and 0 <= row < 8:  # Make sure we're within bounds
                    square = chess.square(col, 7 - row)

                    # In BOT_VS_HUMAN mode, human plays as black
                    correct_color = chess.WHITE if game_mode != BOT_VS_HUMAN else chess.BLACK

                    if selected_square is None:
                        # Select piece if it belongs to the current player
                        piece = board.piece_at(square)
                        if piece and piece.color == (board.turn if game_mode == HUMAN_VS_HUMAN else correct_color):
                            selected_square = square
                    else:
                        # Check if this is potentially a pawn promotion move
                        piece = board.piece_at(selected_square)
                        is_promotion = False

                        if piece and piece.piece_type == chess.PAWN:
                            # Check if pawn is moving to promotion rank (8th for white, 1st for black)
                            if (piece.color == chess.WHITE and chess.square_rank(square) == 7) or \
                                    (piece.color == chess.BLACK and chess.square_rank(square) == 0):
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
                            last_move_from = selected_square
                            last_move_to = square
                            board.push(move)
                            player_time = time.time() - start_time  # Record player's time

                            # In human vs human, we don't switch to AI turn
                            if game_mode != HUMAN_VS_HUMAN:
                                player_turn = False  # Switch to AI's turn

                        selected_square = None  # Unselect the piece

                # Undo last move functionality (top-right corner click)
                if x > WIDTH - SQUARE_SIZE and y < SQUARE_SIZE:
                    # In human vs human, just pop once
                    # In human vs bot/bot vs human, pop twice to maintain correct turn
                    if board.move_stack and game_mode == HUMAN_VS_HUMAN:
                        board.pop()
                    elif board.move_stack and len(board.move_stack) >= 2 and game_mode != HUMAN_VS_HUMAN:
                        board.pop()
                        board.pop()

        # AI logic for modes with bots
        if not player_turn and not board.is_game_over() and game_mode != HUMAN_VS_HUMAN:
            start_time = time.time()  # Start timing the AI
            try:
                # Use the selected depth and time limit
                result = engine.play(board, chess.engine.Limit(depth=ai_depth, time=AI_TIME_LIMIT))
                if result.move is not None:
                    last_move_from = result.move.from_square
                    last_move_to = result.move.to_square
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

            ai_time = time.time() - start_time  # Record AI's time

            # In BOT_VS_BOT mode, don't switch to player turn
            if game_mode != BOT_VS_BOT:
                player_turn = True
            # Short delay between bot moves in BOT_VS_BOT mode
            elif game_mode == BOT_VS_BOT:
                pygame.time.delay(500)  # 500ms delay to see the moves

        # Check for game over
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