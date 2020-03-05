fullname=${1}
basename=${fullname/.c/}
clang-9 -emit-llvm -S ${fullname} -o ${basename}.ll
clang-9 -emit-llvm -S encrypt.c -o encrypt.ll
opt-9 -p \
    -load ../build/obfuscate/libobfuscate.so \
    -obfstr ${basename}.ll \
    -o ${basename}_out.bc
llvm-link-9 encrypt.ll ${basename}_out.bc -o final.bc
clang-9 final.bc -o final.out
