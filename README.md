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
$ cd test
$ ./run.sh ${source_file}
# will build ${source_file} to final.out binary file
```
