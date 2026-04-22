# 2-3-4 Tree (B-Tree of Order 4) — Assignment 2

## Project Description

This project implements a **2-3-4 Tree**, which is a B-Tree of order 4 (minimum degree `t = 2`).
Each node holds between 1 and 3 keys, and between 2 and 4 children.
The implementation supports insertion, search, level-order visualization, file-based input parsing,
operation logging, and a full save/restore persistence system — all without using STL containers.

---

## Project Structure

```
BTree-Assignment/
├── src/
│   └── main.cpp       ← Full implementation (BTreeNode, BTree, parser, logger)
├── input.txt          ← Operations file (I, S, D, SAVE, RESTORE)
├── output.txt         ← Tree state after every operation (generated at runtime)
├── log.txt            ← Debug log with split counts (generated at runtime)
├── snapshot.dat       ← Serialized tree for persistence (generated at runtime)
└── README.md          ← This file
```

---

## How to Compile and Run

```bash
g++ main.cpp -o btree
./btree
```

Make sure `input.txt` is in the same directory as the executable.

---

## Input File Format (`input.txt`)

Each line contains one operation:

| Format     | Meaning                        |
|------------|--------------------------------|
| `I x`      | Insert integer x into the tree |
| `S x`      | Search for integer x           |
| `D x`      | Delete x (optional feature)    |
| `SAVE`     | Serialize tree to snapshot.dat |
| `RESTORE`  | Rebuild tree from snapshot.dat |
| `# comment`| Ignored (comment line)         |
| blank line | Ignored                        |

**Example `input.txt`:**
```
I 10
I 20
I 5
I 6
I 12
S 20
SAVE
I 50
RESTORE
S 10
```

---

## Output Files

### `output.txt`
Clean record of every operation and the resulting tree state. Mirrors what is printed to the console. Example:
```
=== After Each Operation ===
Insert 10:
Level 0: [10]
In-Order : 10
Splits so far: 0

=== After Each Operation ===
Insert 20:
Level 0: [10 20]
In-Order : 10 20
Splits so far: 0
```

### `log.txt`
Developer-facing debug log. Includes operation name, running split count, and full tree state after every step. Used for tracing and debugging tree transformations.

### `snapshot.dat`
Binary-style text file produced by `SAVE`. Stores the tree in preorder traversal format:
```
2
3 0 5 10 20
1 1 5
...
```
Each line: `n isLeaf k0 k1 ... k(n-1)` where `n` = number of keys, `isLeaf` = 1 or 0.

---

## Implementation Details

### `BTreeNode`
- Stores keys in `int keys[2t-1]` and children in `BTreeNode* children[2t]`
- **`insertNonFull(k)`** — inserts into a node guaranteed not to be full; shifts keys right to maintain sorted order
- **`splitChild(i, y)`** — splits full child `y` at index `i`: copies right half into a new node `z`, promotes the median key up into the current node, increments the global `splitCount`
- **`search(k)`** — walks down the tree comparing keys, returns `true` if found
- **`saveNode(out)`** — writes node to file in preorder (node first, then children left to right)

### `BTree`
- **`insert(k)`** — if root is full (`n == 2t-1`), creates a new root and splits the old root before inserting; otherwise calls `insertNonFull` directly
- **`levelOrder()`** — BFS using a fixed-size pointer queue (no STL); prints `Level N: [k1 k2] [k3] ...`
- **`save(filename)`** — writes `t` and then calls `saveNode` recursively
- **`restore(filename)`** — reads `t`, then rebuilds the tree node-by-node using `restoreNode` (recursive preorder reconstruction)

### Output helpers (free functions)
- **`printLevelOrder(ostream&, BTree&)`** — writes level-order to any stream (cout or file)
- **`printInOrder(ostream&, BTree&)`** — writes sorted keys to any stream
- **`printAfterOp(ostream&, BTree&, desc)`** — called after every operation; writes to both console and `output.txt` in one call
- **`logState(ofstream&, BTree&, desc)`** — writes detailed snapshot to `log.txt`

---

## Concept Questions

### 1. Why is a 2-3-4 Tree a special case of a B-Tree?
A B-Tree of minimum degree `t` allows between `t-1` and `2t-1` keys per node.
A 2-3-4 Tree uses `t = 2`, so each node holds 1–3 keys and 2–4 children.
This makes it exactly a B-Tree of order 4 — a specific parameterization of the general B-Tree structure.

### 2. Maximum number of children per node?
With `t = 2`, the maximum number of children is `2t = 4`. This is where the name "2-3-4" comes from: a node can have 2, 3, or 4 children.

### 3. Explain the split operation
When a node is full (has `2t-1 = 3` keys), it must be split before insertion can proceed:
1. The **median key** (middle key) is promoted up to the parent node
2. The **left half** of the keys stays in the original node
3. The **right half** of the keys moves into a newly created sibling node
4. If the root splits, tree height increases by 1 (a new root is created)

Example — splitting `[10 20 30]` with parent `[40]`:
```
Before:        After:
   [40]          [20 40]
   /             /    \
[10 20 30]    [10]   [30]
```

### 4. Time complexity of insertion?
- **O(log n)** — the tree height is always `O(log n)` because splits propagate upward and the tree grows only at the root
- Each level requires at most one split, and there are `O(log n)` levels
- Key comparisons per level: O(t) = O(1) since t=2 is constant for a 2-3-4 tree

---

## Part 8: Reverse Reconstruction Analysis

Inserting `[10, 20, 30, 40, 50]` forward vs. reverse `[50, 40, 30, 20, 10]`:

| Property     | Forward order | Reverse order |
|--------------|---------------|---------------|
| Height       | Same          | Same          |
| Structure    | May differ    | May differ    |
| Split count  | Same          | Same          |

Because B-Trees split proactively and rebalance on every insertion, the **height is always the same** regardless of insertion order for the same set of values. The internal structure (which keys end up as separators) may differ.

---

## Part 11: Save / Restore System

### Why saving only keys is not enough
Keys alone do not encode the **shape** of the tree. Two different tree structures can contain the exact same set of keys. Without knowing which keys belong to which node, and whether a node is a leaf or internal node, it is impossible to reconstruct the original tree.

### How node types are distinguished in serialization
Each serialized line stores an `isLeaf` flag (1 for leaf, 0 for internal node):
```
n isLeaf k0 k1 ... k(n-1)
```
During restore, this flag tells the reconstruction function whether to recursively read `n+1` children after reading the current node's keys.

---

## Extra Challenge: Descending Input Analysis

Input: `100 90 80 70 60 50 40 30 20 10`

Since the 2-3-4 tree splits proactively (when a full node is encountered on the way down), descending order causes the same number of splits as ascending order for the same set of values. The tree stays balanced throughout. Expected splits ≈ 3–4 for 10 elements, with height = 2.

---

## GitHub Repository

https://github.com/janayasserfawzi/CSCE2211_assign2project.git 

---

## Submitted by

- **Name:** Jana Fawzi
- **ID:** 900241258