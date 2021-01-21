# Baby obfuscator

Using LLVM Pass to obfuscate programs.

Write up (in Chinese):

- [使用 LLVM Pass 实现字符串加密](https://zhuanlan.zhihu.com/p/104735336)
- [重写 OLLVM 之虚假控制流](https://www.anquanke.com/post/id/212768)
- [重写 OLLVM 之控制流平坦化](https://zhuanlan.zhihu.com/p/345843635)

## Features

| Feature | Enable Flag |
| - | - |
| Obfuscate constant string | `-obfstr` |
| Add bogus control flow | `-boguscf` |
| Instruction Substitution | `-subobf` |
| Call graph flattening | `-flattening` |

## Requirement

```bash
llvm-9 and llvm-9-dev
cmake >= 3.10
gcc / clang
```

## Build

```bash
mkdir build && cd build
# You should modify the DLLVM_DIR option to fit your environment
cmake .. -DLLVM_DIR=/usr/lib/llvm-9/lib/cmake/llvm/
cmake --build . -- -j$(nproc)
```

## How to use

```bash
# You should assign ${fullname} and ${basename} as your program's name
# Example: fullname=test.c; basename=test
# Compile origin program
clang-9 -emit-llvm -S ${fullname} -o ${basename}.ll
# Load obfuscator to obfuscate program
# You can change -obfstr option to other options
opt-9 -load /part/to/libObfuscator.so \
      -obfstr ${basename}.ll \
      -o ${basename}_obfuscated.bc
# If you are using ObfuscateString Pass, you should link encrypt.c with your program
# Compile to target platform
clang-9 ${basename}_obfuscated.bc -o ${basename}
```

## Acknowledgement

The project has "borrow" some code from these projects:

[obfuscator-llvm/obfuscator](https://github.com/obfuscator-llvm/obfuscator)

## License

MIT License
