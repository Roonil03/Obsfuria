#include <cstdint>

int _0xObfPtrRead(int* ptr) {
    uintptr_t base = reinterpret_cast<uintptr_t>(ptr);
    int* real_ptr = reinterpret_cast<int*>(base);
    return *real_ptr;
}

void _0xObfPtrWrite(int* ptr, int val) {
    uintptr_t _q = reinterpret_cast<uintptr_t>(ptr);
    *reinterpret_cast<int*>(_q) = val;
}
