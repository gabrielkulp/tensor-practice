# Tensor Practice

## What is this?
I needed to check my understanding of tensor trace and contraction operations, so I wrote some code in Python, then C, and finally Rust. They all do approximately the same thing and are structured in similar ways.

## How do I run it?
- **Python:** just run the scripts. They're independent and don't have any file I/O.
- **C:** there's a Makefile, but there's nothing complicated to it; just run your favorite compiler on `*.c` and it will probably work fine. The top-level operations are described in `main.c`, so notice that it needs to open `../T.coo`, which is a file containing a sparse tensor in the COO (coordinate) format.
- **Rust:** build and run with `cargo run`. Note that it will also try to read `../T.coo`, so make sure you run it from the `Rust` directory, and not `src` inside it.

