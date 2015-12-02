// RUN: clang %s -S -emit-llvm -o %t.ll
// RUN: opt -load %bindir/lib/LLVMXOR${MOD_EXT} -xor %t.ll -S -o %t0.ll
// RUN: FileCheck %s < %t0.ll
// RUN: clang %t0.ll -o %t0
// RUN: %t0
#include <stdio.h>

int main()
{

    // CHECK: load
    // CHECK: xor
    // CHECK: store
    int var = 0;

    // CHECK: load
    // CHECK: xor
    // CHECK: store
    int foo = 0;

    // CHECK: load
    // CHECK: xor
    // CHECK: store
    int ifoo = 0;

    // CHECK: load
    // CHECK: xor
    // CHECK: store
    int bar = 0;

    // CHECK: load
    // CHECK: xor
    // CHECK: store
    int quack = 0;

    // CHECK: load
    // CHECK: xor
    // CHECK: store
    quack = 0;

    printf("quack equals %d\n", quack);
    return 0;
}
