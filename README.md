# Lox Compiler and LLVM implemented in C
This is my project in implementing a compiler and LLVM in C for Lox (from Crafting Interpreter). Even though I have implemented the interpreter in Go [(visit repo)](github.com/WanakitsamphanTh/golox), I chose C for compiler and LLVM since I can implement my own memory model without relying on Go's garbage collector.

## Key differences
- The compiler in Crafting Interpreter uses Pratt Parsing without building AST. My compiler traverses AST and emits bytecodes for LLVM. I implemented almost everything besides LLVM from scratch, so a large chunk of compiler code needs optimization.
- Therefore, I have a distinct scope resolver from compiler. Also expressions and statements that include identifiers contain corresponding Symbol objects inside.
- For that reason, I implemented Scope and Symbol objects from scratch. There's definitely problems to fix and maybe I need to optimize how those objects are freed. (but the pointer ownership model here is sound, I guess)
- My CLox (as well as GoLox) has a quirky boolean evalutation. true is evaluated to true, everything else is evaluated to false. (hence non-boolean values are false)
- ObjString is implemented as variable-sized struct.
- Constant pool has no duplicate values since `addConstant` never adds identical values.
- A little different design on function object. Mine are ObjCallable, ObjNativeFn, and ObjFn. The first two are different aliases of the same type, and the base of ObjFn. I utilize dynamic dispatch to call a function, that is, either calling a native function directly or call the wrapper function for a compiled chunk. 
- Every callable has handler object to handle errors inside a function. This is used to catch any error from native function calls. Ideally, I'd love to implement error handling mechanism but I'm busy building basic stuff in the tutorial.
- My CLox features Array.
    - the formal grammar for array is \
     $ array := \{ [expression [, expression]*]? \} $
    - array/string operation (to be implemented)
        - [ ] get/set element
        - [ ] slice
        - [ ] binary operation $ in: x, array \rightarrow \{true, false\} $
        - [ ] binary operation $ +: a, b \mapsto \{a_1,...a_n,b_1,...b_n\}  $
        - [ ] unary operation $ \text{len}: arr \rightarrow number$
        - [ ] element-wise comparition `==` and `!=`
- Some different function names and OpCodes due to personal preference.

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
- Also implementing serialization to make the bytecode portable
- I should add constant folding in my compiler to reduce number of constants.
- I got a problem with circle reference, but the way I solve it feels a bit inconsistent.
- **Realized that I should have written this in C++**


*Should you have any suggestions, please feel free to reach me*

## Reference
*Crafting Interpreters* by Robert Nystrom 
