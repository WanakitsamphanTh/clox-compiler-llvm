# Lox Compiler and LLVM implemented in C
This is my project in implementing a compiler and LLVM in C for Lox (from Crafting Interpreter). Even though I have implemented the interpreter in Go [(visit repo)](github.com/WanakitsamphanTh/golox), I chose C for compiler and LLVM since I can implement my own memory model without relying on Go's garbage collector.

## Key differences
- The compiler in Crafting Interpreter uses Pratt Parsing without building AST. My compiler traverses AST and emits bytecodes for LLVM. I implemented almost everything besides LLVM from scratch, so a large chunk of compiler code needs optimization.
- ObjString is implemented as variable-sized struct.
- Constant pool has no duplicate values since `addConstant` never adds identical values.
- My CLox features Array.
    - the formal grammar for array is \
     $ array := \{ [expression [, expression]*]? \} $
- My CLox will have compile-time constant evaluation in which all constant expressions are evaluated and stored constant pool instead of doing it at runtime.

## Note
- Runtime stack and constant pool is too small
- I should optimize the loop in `addConstant()`
- I may also need semantic resolver to calculate right jump offset for `break` and `skip` statements
- I'm thinking about the better way to implement array at runtime


    **currently**
    - push all array elements on stack
    - use array instruction to collect


    **optimization approach**
    - array of compile-time constants &rarr; compile-time evaluation
    - array of compile-time constants and identifiers &rarr; partition array by identifiers, evaluate constant part at compile-time, append subarrays at runtime(?)


*Should you have any suggestions, please feel free to reach me*

## Reference
*Crafting Interpreters* by Robert Nystrom 
