# VM HW3

## Prerequisites

`lamac` available in PATH

## Usage

After `git clone`, please initialize all submodules:
```
git submodule update --init --recursive
```

```
cd performance
lamac -b Sort.lama
cd ..
```

```
make
./idioman performance/Sort.bc
```

## Heap Memory Usage analysis

32-bit implementation.

Let `N` be code region size.
- `N` is allocated for `aux`
- `Parser` has a stack with `4 * N`, but it is destroyed after parsing.
- One `IdiomAnalyzer` has `8 * N` in the worst case assuming each instruction is 1-byte long. On practice this should be much less. Here `8` is size of `IdiomOccurrence`: one word for instruction pointer and one word for count.
- After the idioms are uniqed, the idiom vector is shrinked to minimum size. There are only `O(1)` idioms which have only 1-byte instructions, and there can be at most one of them. All other idioms contain a 4-byte word. Thus for 1-idioms storage size is bounded by `N + O(1)` and for 2-idioms `2N + O(1)`. Combined it is `3N + O(1)`.
