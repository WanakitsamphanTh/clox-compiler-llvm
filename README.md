# Lox Compiler and LLVM implemented in C
This is my project in implementing a compiler and LLVM in C for Lox (from Crafting Interpreter). Even though I have implemented the interpreter in Go [(visit repo)](github.com/WanakitsamphanTh/golox), I chose C for compiler and LLVM since I can implement my own memory model without relying on Go's garbage collector.

## Key differences
- The compiler in Crafting Interpreter uses Pratt Parsing without building AST. My compiler traverses AST and emits bytecodes for LLVM. I implemented almost everything besides LLVM from scratch, so a large chunk of compiler code needs optimization.

*Should you have any suggestions, please feel free to reach me*

## Reference
*Crafting Interpreters* by Robert Nystrom 
