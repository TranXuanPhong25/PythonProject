[Link demo](https://youtube.com/watch?v=y-LLEPUXnK0)
<br/>
[slide](https://slidse-lzi6.vercel.app/1.html)
<br/>
Elo in chess.com: 3000+
<br/>
Elo calculated by match against Stockfish limit 2500 at depth 15 tc=40/75: +11 +/- 65
<br/>
Match against Pigeon (1800 elo on CCRL) at depth 15 tc=40/75 : 23-17-13 (win-loss-draw)

## Acknowledgments

Inspired by open-source engines like:

- [Stockfish](https://stockfishchess.org/)
- [Ethereal](https://github.com/AndyGrant/Ethereal)
- [Rice](https://github.com/rafid-dev/rice)

# Chess Engine Overview

This chess engine implements a variety of advanced techniques in search heuristics, evaluation, and piece-specific optimizations to achieve high performance and strong playing strength.

## Search Heuristics

The engine employs modern search techniques to efficiently explore the game tree:

- **Alpha-Beta Pruning**: Reduces the number of nodes evaluated by eliminating branches that cannot influence the final decision.
- **Principal Variation Search (PVS)**: Optimizes alpha-beta pruning by searching the most promising moves first.
- **Iterative Deepening**: Progressively deepens the search while maintaining time constraints.
- **Aspiration Windows**: Narrows the search window to improve efficiency.
- **Transposition Table**: Caches previously evaluated positions to avoid redundant calculations.
- **Quiescence Search**: Extends the search to evaluate only captures and checks, avoiding the horizon effect.
- **Pruning Techniques**:
  - **Null Move Pruning**: Skips moves in non-critical positions to save time.
  - **Late Move Reductions (LMR)**: Reduces the depth of less promising moves.
  - **Futility Pruning**: Avoids searching moves unlikely to improve the position.
  - **Static Exchange Evaluation (SEE)**: Evaluates the outcome of captures and exchanges.
  - **Razoring**: Skips searches near leaf nodes unlikely to improve alpha.
  - **History Pruning**: Prunes moves with a poor history of effectiveness.
  - **ProbCut**: A selective pruning technique that uses shallow searches to estimate whether a move is unlikely to exceed the beta threshold. 
- **Move Ordering**:
  - **Killer Moves**: Tracks non-capturing moves that cause cutoffs.
  - **History Heuristic**: Prioritizes moves that have been effective in the past.
  - **Continuation History**: Tracks sequences of moves that work well together.

## Evaluation Function

The evaluation function combines multiple factors to assess the quality of a position:

- **Material Balance**: Counts the value of pieces for both sides.
- **Piece-Square Tables**: Evaluates piece placement based on precomputed tables.
- **Game Phase Interpolation**: Adjusts evaluation between middlegame and endgame based on remaining material.
- **Pawn Structure**:
  - Evaluates isolated, doubled, and passed pawns.
  - Considers connected pawns and pawn chains.
- **King Safety**:
  - Evaluates king safety based on pawn shield and attacking pieces.
  - Differentiates between middlegame and endgame king placement.
- **Center Control**: Rewards control of central squares.

This combination of techniques ensures a strong and efficient chess engine capable of competing at a high level.

# Contribution
- Nguyễn Quang Minh: evaluation, evaluate_features
- Trần Xuân Phong: search heuristic
- Triệu Cao Tấn: evaluate_pieces
