
## Core Engine Components

### Board Representation

- **Bitboards**: Fast 64-bit representations of the board.
- **Fancy Magic Bitboards**: Optimized move generation for sliding pieces.
- **Zobrist Hashing**: Fast position hashing for use in transposition tables.
- **Pre-calculated Attack Tables**: Accelerated move generation and evaluation.

### Search Techniques

- **Negamax Framework**: Built on alpha-beta pruning.
- **Transposition Table**: Cache previously evaluated positions.
- **Iterative Deepening**: Incremental depth increase to ensure time control accuracy.
- **Aspiration Windows**: Narrow search windows for performance.
- **Principal Variation Search (PVS)**: Efficient null-window searches for non-PV nodes.

---

## Pruning Methods

- **Null Move Pruning**
- **Futility Pruning**
- **Late Move Pruning (LMP)**
- **History-based Pruning**
- **Reverse Futility Pruning**
- **Static Null Move Pruning (SNMP)**
- **Probcut**
- **Multi-Cut**
- **Delta Pruning** *(used in quiescence search)* ?

---

## Move Ordering

- **MVV-LVA** (Most Valuable Victim - Least Valuable Attacker)
- **Killer Moves**
- **History Heuristic**
- **Counter Moves Table**
- **Continuation History**
- **SEE (Static Exchange Evaluation)**

---

## Evaluation

- **Material Counting**
- **Piece-Square Tables**
- **Mobility Evaluation**
- **King Safety** (attack zones, pawn shields)
- **Pawn Structure** (passed, isolated, doubled, backward)
- **Bishop Pair Bonus**
---

## Extensions

---

## Time Management

---

## Advanced Features

### Search Improvements

- **Internal Iterative Deepening (IID)**
- **Late Move Reductions (LMR)** with enhancements
- **Verification Searches**
- **Zugzwang Detection**

### History and Learning

- **Countermove History**
- **Continuation History**
- **Butterfly History**
- **Depth-Based History Updates**

---


## Technical Details

- **Bitboards**: 64-bit unsigned integers
- **Magic Bitboard Constants**: Pre-computed values
- **Zobrist Keys**: 64-bit hashing with additional keys for castling and piece-square

---

## Acknowledgments

Inspired by open-source engines like:

- [Stockfish](https://stockfishchess.org/)
- [Ethereal](https://github.com/AndyGrant/Ethereal)
- [Rice](https://github.com/rafid-dev/rice)


---

