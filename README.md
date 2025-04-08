Have to create .env first
```terminal
  AI_TIME_LIMIT=10
  ENGINE_PATH=/chess-engine/bin/uci
```
then run following command
```terminal
  cd chess-engine
  make
```
Benchmark:
  make bench
  ./bench

Implemented
Chess representation and movegen
 - bitboard (copy)
 - fancy magic board (copy)
Search
 - negamax with alpha-beta cutoff
 - order moves
 - transposition table (copy)
Evaluation
 - count material with piece-square
Scoring move
  - mvv_lva
  - promotion
  - tt_move