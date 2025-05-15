[Link demo](https://youtube.com/watch?v=y-LLEPUXnK0)
[slide](https://slidse-lzi6.vercel.app/1.html)
</br>
elo in chess.com: 3200+
<br/>
elo calculated by match against stockfish limit 2500 at depth 15 tc=40/75: +11 +/- 65

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
- **Piece Coordination**:
  - Bonuses for bishop pairs and rook pairs.
  - Penalties for trapped or poorly placed pieces.
- **King Safety**:
  - Evaluates king safety based on pawn shield and attacking pieces.
  - Differentiates between middlegame and endgame king placement.
- **Center Control**: Rewards control of central squares.

## Piece-Specific Optimizations

- **Pawns**:
  - Bonuses for passed pawns and penalties for doubled or isolated pawns.
- **Knights**:
  - Bonuses for outposts and proximity to the enemy king.
- **Bishops**:
  - Bonuses for long diagonal control and bishop pairs.
- **Rooks**:
  - Bonuses for open and semi-open files.
  - Rewards for occupying the seventh rank.
- **Queens**:
  - Rewards for mobility and infiltration into enemy territory.
- **Kings**:
  - Separate evaluation for middlegame and endgame positions.

This combination of techniques ensures a strong and efficient chess engine capable of competing at a high level.

