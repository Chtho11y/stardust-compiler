# SUSTech CS323 Project: Stardust

## Environment

| Name  | Value                                       |
| ----- | ------------------------------------------- |
| Flex  | flex 2.6.4                                  |
| Bison | bison (GNU Bison) 3.8.2                     |
| gcc   | gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0   |
| Make  | GNU Make 4.3. Built for x86_64-pc-linux-gnu |
| CMake | cmake version 3.22.1                        |
| LLVM  | llvm 10.0.1                                 |

## **Project Structure**
```
Stardust
├-- sdlib
|	├--lib.c
|	├--lib.o
|	└─-libsd.a
├-- inc
|	├--ast.h
|	├--context.h
|	├--error.h
|	├--literal_process.h
|	├--parse.h
|	├--util.h
|	├--sdtype.h
|	├--llvm_header.h
|	├--llvm_ir.h
|	├--arg_parse.h 
|	└─-clipp.h 	(https://github.com/muellan/clipp)
└─- src
	├-- lex
	|	├-- lex.l
	|	└─- syntax.y
	├-- ast.cpp 
	├-- context.cpp 
	├-- error.cpp 
	├-- literal.cpp
	├-- llvm_ir.cpp
	├-- main.cpp 
	├-- operator.cpp 
	├-- sdtype.cpp 
	└─- util.cpp
	
```

## How to Use

- Create `build` directory &  use CMake / Make to build the Stardust compiler:

  ```
  mkdir -p build
  cd build
  cmake ..
  make
  ```

- The compiler can be invoke in terminal:

  ```
  SYNOPSIS
          stardust [<input>] [-v] [-o <output path>] [-q] [--print-ast|--no-print-ast]
                   [--print-sym|--no-print-sym] [-h] [-s] [-c] [-nc]
  
  OPTIONS
          -v, --version
                      version info
  
          -q, --quiet suppress all warnings
          -h, --help  usage lines
          -s          output as asm file
          -c          output as LLVM IR
  ```

  - To generate executable file:

    ```
    ./stardust input_path -o output_path
    ```

  - To generate asm file:

    ```
    ./stardust input_path -s -o output_path
    ```

  - To generate LLVM-IR:

    ```
    ./stardust input_path -c -o output_path
    ```

  - To print ast / sym-table in terminal:

    ```
    ./stardust input_path --print-ast --print-sym
    ```

    

## Todo:

- **Type Alias**(Done)

- **Constant evaluation**

- **Member Function**(Done)

- **Lambda Expression**

- **Function Overload Support**  
- **Unify Function Type and Function Pointer**(Done)  
- **Array Instance**(Done)
- **Generic**(Ongoing)
- **Reference Type**(Done)

- **File Inclusion**

- **Macro**

- **Trait**(Ongoing)
