// RUN: clang %s -S -emit-llvm -o %t.ll
// RUN: opt -load %bindir/lib/LLVMXOR${MOD_EXT} -xor %t.ll -S -o %t0.ll
// RUN: FileCheck %s < %t0.ll
// RUN: clang %t0.ll -o %t0
// RUN: %t0
int foo(int i) {
    return 0;
}

int main() {

    // CHECK: load
    // CHECK: xor
    // CHECK: store
    return foo(0);
}
