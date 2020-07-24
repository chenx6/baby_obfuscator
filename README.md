# Baby obfuscation

Using LLVM Pass to obfuscate programs.

Write up (in Chinese): <https://zhuanlan.zhihu.com/p/104735336>

## Features

- Obfuscate constant string

## Requirement

```bash
llvm-9 and llvm-9-dev
cmake >= 3.10
gcc / clang
```

## Build

```bash
mkdir build && cd build
cmake .. -DLLVM_DIR=/usr/lib/llvm-9/lib/cmake/llvm/
cmake --build . -- -j$(nproc)
```

## How to use

```bash
# You should assign ${fullname} and ${basename}
# Example: fullname=test.c; basename=test
# Compile origin program
clang-9 -emit-llvm -S ${fullname} -o ${basename}.ll
# Load obfuscator to obfuscate program
opt-9 -p \
    -load ../build/src/libobfuscate.so \
    -obfstr ${basename}.ll \
    -o ${basename}_obfuscated.bc
# If you are using ObfuscateString Pass, you should link encrypt.c with your program
# Compile to target platform
clang-9 ${basename}_obfuscated.bc -o ${basename}
```
