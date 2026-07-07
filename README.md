# Lox Compiler and LLVM implemented in C
This is my project in implementing a compiler and LLVM in C for Lox (from Crafting Interpreter). Even though I have implemented the interpreter in Go [(visit repo)](github.com/WanakitsamphanTh/golox), I chose C for compiler and LLVM since I can implement my own memory model without relying on Go's garbage collector.

## Key differences
- The compiler in Crafting Interpreter uses Pratt Parsing without building AST. My compiler traverses AST and emits bytecodes for LLVM. I implemented almost everything besides LLVM from scratch, so a large chunk of compiler code needs optimization.
- Therefore, I have a distinct scope resolver from compiler. Also expressions and statements that include identifiers contain corresponding Symbol objects inside.
- The way boolean values are evaluated, boolean true is evaluated to true, everything else is evaluated to false. (hence non-boolean values are false)
- ObjString is implemented as variable-sized struct.
- Constant pool has no duplicate values since `addConstant` never adds identical values.
- My CLox features Array.
    - the formal grammar for array is \
     $ array := \{ [expression [, expression]*]? \} $
    - array/string operation
        - [ ] get/set element
        - [ ] slice
        - [ ] binary operation $ in: x, array \rightarrow \{true, false\} $
        - [ ] binary operation $ +: a, b \mapsto \{a_1,...a_n,b_1,...b_n\}  $
        - [ ] unary operation $ \text{len}: arr \rightarrow number$
        - [ ] element-wise comparition `==` and `!=`

## Note
- Runtime stack and constant pool is too small
- I should optimize the loop in `addConstant()` and `lookUpSymbol()` to reduce time complexity to O(N)
- I'm thinking about the better way to implement array at runtime

    **currently**
    - push all array elements on stack
    - use array instruction to collect


    **optimization approach**
    - allocate array and push on stack
    - push i-th element on stack and set i-th elemnt (i = 0...n) (JVM and Python apprroach)
    - or partition array, each has number of elements not exceeding the predefined limit, collect elements into subarrays, and concatenate all subarrays into a complete array (Problably Lua approach)

- Maybe I'll add another data type that represents a single byte to my CLox, constants of which are a part of bytecode instead of living in the constant pool.
- Maybe adding tuple and tuple unpacking
- I should add constant folding in my compiler to reduce number of constants.


*Should you have any suggestions, please feel free to reach me*

## Reference
*Crafting Interpreters* by Robert Nystrom 
