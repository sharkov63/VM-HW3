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
- One `IdiomAnalyzer` has `8 * N` in the worst case assuming each instruction is 1-byte long. On practice this should be much less. Here `8` is size of `IdiomOccurrence`: one word for instruction pointer and one word for count.
- Only one `IdiomAnalyzer` exists at a given moment.

So max heap usage is `9 * N`.
