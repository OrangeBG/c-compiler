# c-compiler
 
## Introduction

A C compiler that targets the C17 standard for the x64 instruction set. The compiler currently supports the following:

### Operating Systems

- Linux
- MacOS

### Data Types

- int
- long
- unsigned int
- unsigned long

### Unary Operators

- Complement
- Negate
- Not
- Increment
- Decrement

### Binary Operators

- Add
- And
- Or
- Equal
- Not Equal
- Less Than
- Less Than Or Equal
- Greater Than
- Greater Than Or Equal
- Subtract 
- Multiply
- Divide
- Remainder
- Bitwise And
- Bitwise Or
- Bitwise Xor
- Bitwise Left Shift
- Bitwise Right Shift

### Variable/Function Declarations

- static specifier
- extern specifier

### Loops

- While
- Do
- For

### Conditionals

- if condition
- ternary operator

## Running The Compiler
For now, the compiler supports a single source file. After the project is built, the following example command should compile a given source file: `./c-compiler main.c`. If the source code successfully compiles, an `assembly.asm` file will be written with the x64 assembly instructions to the file path where the binary was executed. Additionally, the terminal outputs each phase of the compilation steps.
