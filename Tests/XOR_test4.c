// RUN: clang %s -S -emit-llvm -o %t.ll
// RUN: opt -load %bindir/lib/LLVMXOR${MOD_EXT} -xor %t.ll -S -o %t0.ll
// RUN: FileCheck %s < %t0.ll
// RUN: clang %t0.ll -o %t0
// RUN: %t0
int foo(int i, int j, int k, int l) {
    return i;
}

int main() {

    int a = -3;
    // CHECK: load
    // CHECK: xor
    // CHECK: store
    // CHECK: ret
    return foo(0, 3, 0, a);
}
