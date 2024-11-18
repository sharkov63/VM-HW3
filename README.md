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