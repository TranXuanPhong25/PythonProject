import os
from dotenv import load_dotenv

import pygame
import chess
import chess.engine
import time

load_dotenv()
# Cấu hình Pygame
LAST_MOVE = (255, 255, 0, 10, 10)  # Light yellow color for last move highlighting
pygame.init()
pygame.mixer.init()  # Initialize Pygame mixer for sound effects
MOVE_SOUND = pygame.mixer.Sound("assets/sounds/move.mp3")  # Path to move sound effect
CAPTURE_SOUND = pygame.mixer.Sound("assets/sounds/capture.mp3")  # Path to capture sound effect
WIDTH, HEIGHT = 1040, 860  # Increased width to accommodate the sidebar
BOARD_WIDTH = 840
SQUARE_SIZE = 100
# scale down

WHITE = (240, 217, 181)
BROWN = (181, 136, 99)
HIGHLIGHT = (186, 202, 68)  # Màu highlight ô được chọn
FONT = pygame.font.Font(None, 36)  # Font hiển thị thời gian
MENU_FONT = pygame.font.Font(None, 48)  # Font cho menu
MOVE_FONT = pygame.font.Font(None, 24)  # Font for move history
SIDEBAR_BG = (40, 40, 40)  # Dark background for sidebar
SIDEBAR_TEXT = (220, 220, 220)  # Light color for sidebar text
MOVE_HIGHLIGHT = (100, 100, 220)  # Highlight color for moves
AI_TIME_LIMIT = float(os.getenv("AI_TIME_LIMIT", 10))
ENGINE_PATH = os.getenv("ENGINE_PATH", "chess-engine/bin/uci.exe").split()
CUSTOM_FEN = os.getenv("CUSTOM_FEN",
                       "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")  # Empty string if not set
SCALE = int(os.getenv("SCALE", 100))  # Scale percentage for the board
# Add this constant near the top with your other color definitions
LEGAL_MOVE_INDICATOR = (255, 255, 255, 170)  # Semi-transparent white for move indicators
UNDO_HIGHLIGHT = (255, 100, 100, 200)  # Red highlight for undo moves
REDO_HIGHLIGHT = (100, 255, 100, 200)  # Green highlight for redo moves

SQUARE_SIZE = int(SQUARE_SIZE * SCALE/100)
BOARD_WIDTH = int(BOARD_WIDTH * SCALE/100)
WIDTH = WIDTH * SCALE/100
HEIGHT = HEIGHT * SCALE/100
# Game states
MODE_SELECTION = -1  # New state for mode selection
HUMAN_VS_HUMAN = 0
HUMAN_VS_BOT = 1
BOT_VS_HUMAN = 2
BOT_VS_BOT = 3

MENU = 4  # Change from 0 to 4
PLAYING = 5  # Change from 1 to 5
GAME_OVER = 6  # Change from 2 to 6

# Animation variables
animating = False
animation_start_time = 0
animation_duration = 0.3  # seconds
animation_piece = None
animation_from_pos = None
animation_to_pos = None
animation_from_square = None
animation_to_square = None

# Undo/redo variables
undo_redo_flash_time = 0
undo_redo_flash_duration = 0.5  # seconds
undo_redo_squares = []
is_undo_action = True  # True for undo, False for redo

# Move history variables
move_history = []
redo_stack = []

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


def move_to_algebraic(board, move):
    """
    Convert a move into standard algebraic notation (SAN), similar to board.san(move),
    but custom-built for better understanding.
    """
    if move not in board.legal_moves:
        return "illegal"

    piece = board.piece_at(move.from_square)
    if piece is None:
        return "error"

    # Handle castling
    if piece.piece_type == chess.KING:
        if chess.square_file(move.from_square) == 4:
            if chess.square_file(move.to_square) == 6:
                return "O-O"
            elif chess.square_file(move.to_square) == 2:
                return "O-O-O"

    is_capture = board.is_capture(move)

    # Disambiguation
    disambiguation = ""
    if piece.piece_type != chess.PAWN:
        candidates = []
        for other_move in board.legal_moves:
            if other_move.to_square == move.to_square and other_move != move:
                other_piece = board.piece_at(other_move.from_square)
                if other_piece and other_piece.piece_type == piece.piece_type and other_piece.color == piece.color:
                    candidates.append(other_move.from_square)

        if candidates:
            from_file = chess.square_file(move.from_square)
            from_rank = chess.square_rank(move.from_square)

            same_file = any(chess.square_file(sq) == from_file for sq in candidates)
            same_rank = any(chess.square_rank(sq) == from_rank for sq in candidates)

            if same_file and same_rank:
                disambiguation = chess.square_name(move.from_square)
            elif same_file:
                disambiguation = str(from_rank + 1)
            else:
                disambiguation = chess.FILE_NAMES[from_file]

    # Piece letter
    piece_symbol = "" if piece.piece_type == chess.PAWN else piece.symbol().upper()

    # Captures
    capture_symbol = ""
    if is_capture:
        if piece.piece_type == chess.PAWN:
            capture_symbol = chess.square_name(move.from_square)[0] + "x"
        else:
            capture_symbol = "x"

    # Destination square
    to_square = chess.square_name(move.to_square)

    # Promotion
    promotion = ""
    if move.promotion:
        promotion = f"={chess.piece_symbol(move.promotion).upper()}"

    # Apply the move temporarily to check for check/mate
    board_copy = board.copy()
    board_copy.push(move)

    check_symbol = ""
    if board_copy.is_checkmate():
        check_symbol = "#"
    elif board_copy.is_check():
        check_symbol = "+"

    # Final notation
    return f"{piece_symbol}{disambiguation}{capture_symbol}{to_square}{promotion}{check_symbol}"


def update_move_history(board, move):
    """Update move history"""
    global move_history, redo_stack
    
    if move and not animating:
        redo_stack = []
    
    if move:
        algebraic = move_to_algebraic(board, move)

        
        # Update move history based on whose turn it is
        if not board.turn:  # After a move is made, the turn changes, so we need to check the opposite
            # White's move (board.turn would be False after white moves)
            move_pair = f"{board.fullmove_number}. {algebraic}"
            move_history.append(move_pair)
        else:
            # Black's move (board.turn would be True after black moves)
            if move_history and board.fullmove_number > 0:
                last_move = move_history[-1]
                if "..." not in last_move and last_move.startswith(f"{board.fullmove_number}."):
                    # Append to existing white move
                    move_history[-1] += f" {algebraic}"
                else:
                    # Create a new black move entry
                    move_history.append(f"{board.fullmove_number}... {algebraic}")
            else:
                # First move in history is by black
                move_history.append(f"{board.fullmove_number}... {algebraic}")


def get_promotion_choice(screen):
    """Display a UI for selecting promotion piece"""
    pieces = ["q", "r", "n", "b"]  # Queen, rook, knight, bishop

    overlay = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
    overlay.fill((0, 0, 0, 128))  # Semi-transparent black
    screen.blit(overlay, (0, 0))

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

    while True:
        for event in pygame.event.get():
            if event.type == pygame.MOUSEBUTTONDOWN:
                for i, rect in enumerate(option_rect):
                    if rect.collidepoint(event.pos):
                        return pieces[i]


def draw_move_history(screen, current_move_index):
    """Draw the move history sidebar"""
    sidebar_rect = pygame.Rect(BOARD_WIDTH, 0, WIDTH - BOARD_WIDTH, HEIGHT)
    pygame.draw.rect(screen, SIDEBAR_BG, sidebar_rect)

    pygame.draw.line(screen, (100, 100, 100), (BOARD_WIDTH, 0), (BOARD_WIDTH, HEIGHT), 2)

    title = FONT.render("Move History", True, SIDEBAR_TEXT)
    screen.blit(title, (BOARD_WIDTH + 20, 20))

    pygame.draw.line(screen, (100, 100, 100),
                     (BOARD_WIDTH + 10, 60),
                     (WIDTH - 10, 60), 1)

    y_pos = 80
    visible_moves = 28

    display_start = max(0, len(move_history) - visible_moves)

    for i, move in enumerate(move_history[display_start:]):
        text_color = SIDEBAR_TEXT
        bg_color = None

        if i + display_start == current_move_index:
            bg_color = MOVE_HIGHLIGHT
            text_rect = pygame.Rect(BOARD_WIDTH + 10, y_pos, WIDTH - BOARD_WIDTH - 20, 25)
            pygame.draw.rect(screen, bg_color, text_rect)

        move_text = MOVE_FONT.render(move, True, text_color)
        screen.blit(move_text, (BOARD_WIDTH + 20, y_pos))
        y_pos += 25

    if display_start > 0:
        pygame.draw.polygon(screen, SIDEBAR_TEXT,
                            [(WIDTH - 40, 65), (WIDTH - 30, 45), (WIDTH - 20, 65)])


def draw_board(screen, board, selected_square, player_time, ai_time, depth, last_move_from=None, last_move_to=None):
    """Hàm vẽ bàn cờ with animation support"""
    screen.fill((0, 0, 0))

    for row in range(8):
        for col in range(8):
            color = WHITE if (row + col) % 2 == 0 else BROWN
            square = chess.square(col, 7 - row)
            square_rect = (20 + col * SQUARE_SIZE, 40 + row * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE)


            if undo_redo_flash_time > 0 and square in undo_redo_squares:
                flash_progress = undo_redo_flash_time / undo_redo_flash_duration
                flash_alpha = int(200 * flash_progress)

                flash_surface = pygame.Surface((SQUARE_SIZE, SQUARE_SIZE), pygame.SRCALPHA)
                if is_undo_action:
                    flash_surface.fill((*UNDO_HIGHLIGHT[:3], flash_alpha))
                else:
                    flash_surface.fill((*REDO_HIGHLIGHT[:3], flash_alpha))

                pygame.draw.rect(screen, color, square_rect)
                screen.blit(flash_surface, (20 + col * SQUARE_SIZE, 40 + row * SQUARE_SIZE))
            else:
                pygame.draw.rect(screen, color, square_rect)

            fx_surface = pygame.Surface((SQUARE_SIZE, SQUARE_SIZE), pygame.SRCALPHA)
            if selected_square is not None and selected_square == square:
                fx_surface.fill((*HIGHLIGHT[:3], 200))
                screen.blit(fx_surface, (20 + col * SQUARE_SIZE, 40 + row * SQUARE_SIZE))
            elif last_move_from is not None and square == last_move_from:
                fx_surface.fill((*LAST_MOVE[:3], 100))
                screen.blit(fx_surface, (20 + col * SQUARE_SIZE, 40 + row * SQUARE_SIZE))
            elif last_move_to is not None and square == last_move_to:
                fx_surface.fill((*LAST_MOVE[:3], 100))
                screen.blit(fx_surface, (20 + col * SQUARE_SIZE, 40 + row * SQUARE_SIZE))

            piece = board.piece_at(square)
            if piece and not (animating and square == animation_to_square):
                screen.blit(piece_images[piece.symbol()], (20 + col * SQUARE_SIZE, 40 + row * SQUARE_SIZE))

    if animating and animation_piece:
        progress = min(1.0, (time.time() - animation_start_time) / animation_duration)
        progress = 1 - (1 - progress) ** 3

        current_x = animation_from_pos[0] + (animation_to_pos[0] - animation_from_pos[0]) * progress
        current_y = animation_from_pos[1] + (animation_to_pos[1] - animation_from_pos[1]) * progress

        screen.blit(piece_images[animation_piece], (current_x, current_y))

    if selected_square is not None:
        for move in board.legal_moves:
            if move.from_square == selected_square:
                to_col = chess.square_file(move.to_square)
                to_row = 7 - chess.square_rank(move.to_square)

                center_x = 20 + to_col * SQUARE_SIZE + SQUARE_SIZE // 2
                center_y = 40 + to_row * SQUARE_SIZE + SQUARE_SIZE // 2

                circle_surface = pygame.Surface((SQUARE_SIZE, SQUARE_SIZE), pygame.SRCALPHA)

                circle_radius = SQUARE_SIZE // 6
                pygame.draw.circle(circle_surface, LEGAL_MOVE_INDICATOR,
                                   (SQUARE_SIZE // 2, SQUARE_SIZE // 2), circle_radius)

                screen.blit(circle_surface, (20 + to_col * SQUARE_SIZE, 40 + to_row * SQUARE_SIZE))

    label_font = pygame.font.Font(None, 24)

    for col in range(8):
        file_label = chr(ord('a') + col)
        text_color = BROWN if col % 2 == 0 else WHITE
        label = label_font.render(file_label, True, text_color)
        screen.blit(label, (col * SQUARE_SIZE + 65, HEIGHT - 20))

    for row in range(8):
        rank_label = str(8 - row)
        text_color = BROWN if row % 2 == 1 else WHITE
        label = label_font.render(rank_label, True, text_color)
        screen.blit(label, (5, row * SQUARE_SIZE + 85))

    if game_mode != HUMAN_VS_HUMAN:
        ai_text = FONT.render(f"AI: {ai_time:.2f}s", True, (255, 255, 255))
        screen.blit(ai_text, (BOARD_WIDTH - ai_text.get_width() - 10, 10))
        depth_text = FONT.render(f"Depth: {depth}", True, (255, 255, 255))
        screen.blit(depth_text, (20, 10))

    current_move_index = len(move_history) - 1 if move_history else -1
    draw_move_history(screen, current_move_index)
    # Draw undo button in bottom-right corner
    undo_color = (180, 180, 180)
    pygame.draw.rect(screen, undo_color, (WIDTH - SQUARE_SIZE*2, HEIGHT-SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE))
    undo_text = FONT.render("<-", True, (0, 0, 0))
    screen.blit(undo_text, (WIDTH - SQUARE_SIZE*2+40, HEIGHT-SQUARE_SIZE+40))

    # Add redo button below undo button
    if redo_stack:  # Only show if there are moves to redo
        redo_color = (180, 222, 180)
        pygame.draw.rect(screen, redo_color, (WIDTH - SQUARE_SIZE , HEIGHT-SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE))
        redo_text = FONT.render("->", True, (0, 0, 0))
        screen.blit(redo_text, (WIDTH - SQUARE_SIZE+40, HEIGHT-SQUARE_SIZE+40))



def play_move_sound(board, move):
    """Play appropriate sound for a move."""
    if board.is_capture(move):
        CAPTURE_SOUND.play()
    else:
        MOVE_SOUND.play()


def start_animation(board, move):
    """Start piece animation and play move sound."""
    global animating, animation_start_time, animation_piece, animation_from_pos, animation_to_pos
    global animation_from_square, animation_to_square, animation_duration

    piece = board.piece_at(move.from_square)
    if piece:
        animating = True
        animation_start_time = time.time()
        animation_piece = piece.symbol()
        animation_from_square = move.from_square
        animation_to_square = move.to_square

        from_col = chess.square_file(move.from_square)
        from_row = 7 - chess.square_rank(move.from_square)
        to_col = chess.square_file(move.to_square)
        to_row = 7 - chess.square_rank(move.to_square)

        animation_from_pos = (20 + from_col * SQUARE_SIZE, 40 + from_row * SQUARE_SIZE)
        animation_to_pos = (20 + to_col * SQUARE_SIZE, 40 + to_row * SQUARE_SIZE)

        move_distance = max(abs(from_col - to_col), abs(from_row - to_row))
        animation_duration = 0.2 + (move_distance * 0.05)

        # Play move sound
        play_move_sound(board, move)


def trigger_undo_redo_effect(squares, is_undo=True):
    """Trigger the undo/redo visual effect"""
    global undo_redo_flash_time, undo_redo_squares, is_undo_action
    undo_redo_flash_time = undo_redo_flash_duration
    undo_redo_squares = squares
    is_undo_action = is_undo


def update_animation():
    """Handle move animation updates"""
    global animating

    if animating:
        if time.time() - animation_start_time >= animation_duration:
            animating = False


def update_undo_redo_flash(dt):
    """Handle undo/redo flash effect updates"""
    global undo_redo_flash_time

    if undo_redo_flash_time > 0:
        undo_redo_flash_time -= dt
        if undo_redo_flash_time < 0:
            undo_redo_flash_time = 0


def draw_menu(screen, input_text):
    """Hàm hiển thị menu chọn độ sâu"""
    screen.fill((50, 50, 50))
    title = MENU_FONT.render("Select AI Search Depth", True, (255, 255, 255))
    screen.blit(title, (WIDTH // 2 - title.get_width() // 2, 100))

    input_box = pygame.Rect(WIDTH // 2 - 50, 200, 100, 50)
    pygame.draw.rect(screen, (100, 100, 200), input_box)
    text_surface = FONT.render(input_text, True, (255, 255, 255))
    screen.blit(text_surface, (input_box.x + 5, input_box.y + 10))

    return input_box


def draw_game_over(screen):
    """Hàm hiển thị màn hình kết thúc game"""
    screen.fill((50, 50, 50))
    title = MENU_FONT.render("Game Over", True, (255, 255, 255))
    screen.blit(title, (WIDTH // 2 - title.get_width() // 2, 100))

    replay_button = pygame.Rect(WIDTH // 2 - 100, 300, 200, 50)
    pygame.draw.rect(screen, (100, 100, 200), replay_button)
    replay_text = FONT.render("Replay", True, (255, 255, 255))
    screen.blit(replay_text, (replay_button.centerx - replay_text.get_width() // 2,
                              replay_button.centery - replay_text.get_height() // 2))

    return replay_button


def draw_mode_selection(screen):
    """Add a new function to draw the mode selection screen"""
    screen.fill((50, 50, 50))
    title = MENU_FONT.render("Select Game Mode", True, (255, 255, 255))
    screen.blit(title, (WIDTH // 2 - title.get_width() // 2, 100))

    button_width = 370
    button_height = 60
    button_spacing = 20

    buttons = []

    hvh_button = pygame.Rect(WIDTH // 2 - button_width // 2, 200, button_width, button_height)
    buttons.append((hvh_button, HUMAN_VS_HUMAN))
    pygame.draw.rect(screen, (100, 100, 200), hvh_button)
    hvh_text = FONT.render("Human vs Human", True, (255, 255, 255))
    screen.blit(hvh_text, (hvh_button.centerx - hvh_text.get_width() // 2,
                           hvh_button.centery - hvh_text.get_height() // 2))

    hvb_button = pygame.Rect(WIDTH // 2 - button_width // 2,
                             200 + button_height + button_spacing,
                             button_width, button_height)
    buttons.append((hvb_button, HUMAN_VS_BOT))
    pygame.draw.rect(screen, (100, 100, 200), hvb_button)
    hvb_text = FONT.render("Human (White) vs Bot (Black)", True, (255, 255, 255))
    screen.blit(hvb_text, (hvb_button.centerx - hvb_text.get_width() // 2,
                           hvb_button.centery - hvb_text.get_height() // 2))

    bvh_button = pygame.Rect(WIDTH // 2 - button_width // 2,
                             200 + 2 * (button_height + button_spacing),
                             button_width, button_height)
    buttons.append((bvh_button, BOT_VS_HUMAN))
    pygame.draw.rect(screen, (100, 100, 200), bvh_button)
    bvh_text = FONT.render("Bot (White) vs Human (Black)", True, (255, 255, 255))
    screen.blit(bvh_text, (bvh_button.centerx - bvh_text.get_width() // 2,
                           bvh_button.centery - bvh_text.get_height() // 2))

    bvb_button = pygame.Rect(WIDTH // 2 - button_width // 2,
                             200 + 3 * (button_height + button_spacing),
                             button_width, button_height)
    buttons.append((bvb_button, BOT_VS_BOT))
    pygame.draw.rect(screen, (100, 100, 200), bvb_button)
    bvb_text = FONT.render("Bot vs Bot", True, (255, 255, 255))
    screen.blit(bvb_text, (bvb_button.centerx - bvb_text.get_width() // 2,
                           bvb_button.centery - bvb_text.get_height() // 2))

    note_text = FONT.render("(Human vs Human will skip AI depth selection)", True, (200, 200, 200))
    screen.blit(note_text, (WIDTH // 2 - note_text.get_width() // 2,
                            200 + 4 * (button_height + button_spacing) + 20))

    return buttons


board = chess.Board(CUSTOM_FEN)

game_state = MODE_SELECTION  # Start in mode selection state
game_mode = None
ai_depth = 3  # Default depth
input_text = ''  # Text input for depth

screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Chess Game")

running = True
selected_square = None  # Vị trí quân cờ được chọn
player_turn = True  # True nếu đến lượt người chơi
player_time, ai_time = 0, 0  # Thời gian suy nghĩ của mỗi bên
last_time = time.time()  # For calculating delta time

engine = None

while running:
    current_time = time.time()
    dt = current_time - last_time
    last_time = current_time

    update_animation()
    update_undo_redo_flash(dt)

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

                        if game_mode == HUMAN_VS_HUMAN:
                            game_state = PLAYING
                            board = chess.Board()
                            player_turn = True
                            player_time, ai_time = 0, 0
                            move_history = []
                            redo_stack = []
                        else:
                            game_state = MENU

    elif game_state == MENU:
        input_box = draw_menu(screen, input_text)
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            input_active = True
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
                        if game_mode != HUMAN_VS_HUMAN:
                            engine = chess.engine.SimpleEngine.popen_uci(ENGINE_PATH)
                        board = chess.Board(CUSTOM_FEN)
                        if game_mode == HUMAN_VS_BOT or game_mode == HUMAN_VS_HUMAN:
                            player_turn = True
                        else:
                            player_turn = False
                        player_time, ai_time = 0, 0
                        input_text = ''
                        move_history = []
                        redo_stack = []
                    except ValueError:
                        input_text = ''
                elif event.key == pygame.K_BACKSPACE:
                    input_text = input_text[:-1]
                else:
                    input_text += event.unicode

    elif game_state == PLAYING:
        draw_board(screen, board, selected_square, player_time, ai_time, ai_depth, last_move_from, last_move_to)
        pygame.display.flip()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            if animating:
                continue

            if event.type == pygame.MOUSEBUTTONDOWN and (
                    (game_mode == HUMAN_VS_BOT and player_turn) or
                    (game_mode == BOT_VS_HUMAN and player_turn) or
                    (game_mode == HUMAN_VS_HUMAN)
            ):
                x, y = pygame.mouse.get_pos()
                start_time = time.time()
                col = (x - 20) // SQUARE_SIZE
                row = (y - 40) // SQUARE_SIZE

                if (WIDTH - SQUARE_SIZE * 2 < x and x < WIDTH - SQUARE_SIZE) and (HEIGHT - SQUARE_SIZE < y and y < HEIGHT):
                    print("Undo button clicked")
                    selected_square = None

                    if game_mode == HUMAN_VS_HUMAN and board.move_stack:
                        last_move = board.pop()
                        last_move_from = last_move.from_square
                        last_move_to = last_move.to_square
                        redo_stack.append(last_move)

                        if move_history:
                            if "..." in move_history[-1]:
                                parts = move_history[-1].split(" ")
                                if len(parts) > 2:
                                    move_history[-1] = " ".join(parts[:-1])
                                else:
                                    move_history.pop()
                            else:
                                move_history.pop()

                        squares_affected = [last_move.from_square, last_move.to_square]
                        trigger_undo_redo_effect(squares_affected, is_undo=True)

                    elif game_mode != HUMAN_VS_HUMAN and board.move_stack and len(board.move_stack) >= 2:
                        ai_move = board.pop()
                        human_move = board.pop()
                        redo_stack.append(human_move)
                        redo_stack.append(ai_move)

                        if len(move_history) >= 2:
                            move_history = move_history[:-2]
                        elif move_history:
                            move_history.pop()

                        squares_affected = [
                            human_move.from_square, human_move.to_square,
                            ai_move.from_square, ai_move.to_square
                        ]
                        trigger_undo_redo_effect(squares_affected, is_undo=True)

                    continue
                #redo
                if (WIDTH - SQUARE_SIZE < x and x < WIDTH ) and (
                        HEIGHT - SQUARE_SIZE  < y and y < HEIGHT):
                    print("re button clicked")

                    if game_mode == HUMAN_VS_HUMAN and redo_stack:
                        move_to_redo = redo_stack.pop()
                        last_move_from = move_to_redo.from_square
                        last_move_to = move_to_redo.to_square
                        start_animation(board, move_to_redo)


                        update_move_history(board, move_to_redo)
                        board.push(move_to_redo)

                        squares_affected = [move_to_redo.from_square, move_to_redo.to_square]
                        trigger_undo_redo_effect(squares_affected, is_undo=False)

                    elif game_mode != HUMAN_VS_HUMAN and len(redo_stack) >= 2:
                        ai_move = redo_stack.pop()
                        human_move = redo_stack.pop()
                        last_move_from = human_move.from_square
                        last_move_to = human_move.to_square
                        start_animation(board, human_move)

                        update_move_history(board, human_move)
                        board.push(human_move)
                        update_move_history(board, ai_move)
                        board.push(ai_move)

                        squares_affected = [
                            human_move.from_square, human_move.to_square,
                            ai_move.from_square, ai_move.to_square
                        ]
                        trigger_undo_redo_effect(squares_affected, is_undo=False)

                    continue

                if 0 <= col < 8 and 0 <= row < 8:
                    square = chess.square(col, 7 - row)

                    correct_color = chess.WHITE if game_mode != BOT_VS_HUMAN else chess.BLACK

                    if selected_square is None:
                        piece = board.piece_at(square)
                        if piece and piece.color == (board.turn if game_mode == HUMAN_VS_HUMAN else correct_color):
                            selected_square = square
                    else:
                        piece = board.piece_at(selected_square)
                        is_promotion = False

                        if piece and piece.piece_type == chess.PAWN:
                            if (piece.color == chess.WHITE and chess.square_rank(square) == 7) or \
                                    (piece.color == chess.BLACK and chess.square_rank(square) == 0):
                                is_promotion = True

                        if is_promotion:
                            promotion_choice = get_promotion_choice(screen)

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
                            move = chess.Move(selected_square, square)

                        if move in board.legal_moves:
                            last_move_from = selected_square
                            last_move_to = square

                            redo_stack = []

                            start_animation(board, move)


                            update_move_history(board, move)
                            board.push(move)

                            player_time = time.time() - start_time

                            if game_mode != HUMAN_VS_HUMAN:
                                player_turn = False

                        selected_square = None

        if not player_turn and not board.is_game_over() and game_mode != HUMAN_VS_HUMAN and not animating:
            start_time = time.time()
            try:
                result = engine.play(board, chess.engine.Limit(depth=ai_depth, time=AI_TIME_LIMIT))
                if result.move is not None:
                    last_move_from = result.move.from_square
                    last_move_to = result.move.to_square

                    start_animation(board, result.move)


                    update_move_history(board, result.move)
                    board.push(result.move)
                else:
                    legal_moves = list(board.legal_moves)
                    if legal_moves:
                        random_move = legal_moves[0]
                        start_animation(board, random_move)
                        update_move_history(board, random_move)
                        board.push(random_move)
                        print("Engine returned no move, using fallback move")
            except Exception as e:
                print(f"Error during AI move calculation: {e}")
                legal_moves = list(board.legal_moves)
                if legal_moves:
                    random_move = legal_moves[0]
                    start_animation(board, random_move)
                    update_move_history(board, random_move)
                    board.push(random_move)

            ai_time = time.time() - start_time

            if game_mode != BOT_VS_BOT:
                player_turn = True
            elif game_mode == BOT_VS_BOT:
                pygame.time.delay(300)

        if board.is_game_over() and not animating:
            game_state = GAME_OVER

    elif game_state == GAME_OVER:
        replay_button = draw_game_over(screen)
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            if event.type == pygame.MOUSEBUTTONDOWN:
                pos = pygame.mouse.get_pos()
                if replay_button.collidepoint(pos):
                    game_state = MODE_SELECTION

if engine:
    engine.quit()
pygame.quit()
