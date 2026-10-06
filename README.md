# CS 238P Project 1

JIT Compiled Expression Evaluator

## Build and run

```
make
./cs238 'expression'
```

The program prints one line, the sigmoid of the expression, with six digits after the decimal point. Example:

```
./cs238 '(1+2)*3-4/5'
0.999725
```

That is sigmoid(8.2). An invalid expression prints nothing and exits with status -1.

## Test

`make test` builds `cs238`, then runs the checker. To run the checker on its own, build first:

```
make
python3 test_cs238.py
```

`test_cs238.py` looks for `./cs238` in this directory. It compares the program's output with sigmoid of each expression and prints `PASSED` or `FAILED`.

## What this submission adds

`jitc.c` implements `jitc_compile`, `jitc_open`, `jitc_close`, and `jitc_lookup`. `jitc_compile` forks a child and execs `/usr/bin/gcc` to build a shared object, then `waitpid` waits for that child. `jitc_open` loads it with `dlopen`, `jitc_lookup` finds `evaluate` with `dlsym`, and `jitc_close` unloads it.

`main.c` walks the parser DAG and writes `out.c`. Each node becomes a temporary (`tN`). The generated function is:

```c
double evaluate(double (*sigmoid_fn)(double))
```

`main` passes the address of its own `sigmoid`, and the generated code calls that pointer.

## Notable behavior

`sigmoid` is the stable form: e^x / (1 + e^x) when x is negative, and 1 / (1 + e^(-x)) otherwise.

Division by zero becomes 0.0, including 0/0.

`out.c` and `out.so` are deleted before the program exits.

If the library name has no slash, `dlopen` is tried again as `./name`. Setting `LD_LIBRARY_PATH` is not required.

`gcc` must be installed at `/usr/bin/gcc`.
