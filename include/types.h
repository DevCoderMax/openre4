#ifndef TYPES_H
#define TYPES_H

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_WIN64) || defined(__LP64__) || defined(TARGET_PC)
#include <stdint.h>
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef signed long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
#endif
typedef float f32;
typedef double f64;
typedef int BOOL;

#ifndef NULL
#define NULL 0
#endif

#if defined(__cplusplus)
#include <cstddef>

template <typename T>
struct Ptr32 {
    u32 addr;

    Ptr32() : addr(0) {}
    Ptr32(T* p) : addr((u32)(uintptr_t)p) {}
    Ptr32(u32 a) : addr(a) {}
    Ptr32(int a) : addr((u32)a) {}

    T* get() const { return (T*)(uintptr_t)addr; }
    template <typename U>
    operator U*() const { return (U*)(uintptr_t)addr; }
    explicit operator uintptr_t() const { return (uintptr_t)addr; }
    explicit operator u32() const { return addr; }
    explicit operator int() const { return (int)addr; }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    T& operator[](size_t idx) const { return get()[idx]; }
    T* operator+(size_t idx) const { return get() + idx; }
    T* operator-(size_t idx) const { return get() - idx; }

    template <typename U>
    Ptr32& operator=(U* p) {
        addr = (u32)(uintptr_t)p;
        return *this;
    }
    Ptr32& operator=(u32 a) {
        addr = a;
        return *this;
    }
    Ptr32& operator=(int a) {
        addr = (u32)a;
        return *this;
    }
    Ptr32& operator=(decltype(nullptr)) {
        addr = 0;
        return *this;
    }

    bool operator!() const { return addr == 0; }
    explicit operator bool() const { return addr != 0; }
    bool operator==(const Ptr32& o) const { return addr == o.addr; }
    bool operator!=(const Ptr32& o) const { return addr != o.addr; }
    template <typename U>
    bool operator==(U* p) const { return addr == (u32)(uintptr_t)p; }
    template <typename U>
    bool operator!=(U* p) const { return addr != (u32)(uintptr_t)p; }
    bool operator==(decltype(nullptr)) const { return addr == 0; }
    bool operator!=(decltype(nullptr)) const { return addr != 0; }
};

template <>
struct Ptr32<void> {
    u32 addr;

    Ptr32() : addr(0) {}
    Ptr32(void* p) : addr((u32)(uintptr_t)p) {}
    Ptr32(u32 a) : addr(a) {}
    Ptr32(int a) : addr((u32)a) {}

    void* get() const { return (void*)(uintptr_t)addr; }
    template <typename U>
    operator U*() const { return (U*)(uintptr_t)addr; }
    explicit operator uintptr_t() const { return (uintptr_t)addr; }
    explicit operator u32() const { return addr; }
    explicit operator int() const { return (int)addr; }

    template <typename U>
    Ptr32& operator=(U* p) {
        addr = (u32)(uintptr_t)p;
        return *this;
    }
    Ptr32& operator=(u32 a) {
        addr = a;
        return *this;
    }
    Ptr32& operator=(int a) {
        addr = (u32)a;
        return *this;
    }
    Ptr32& operator=(decltype(nullptr)) {
        addr = 0;
        return *this;
    }

    bool operator!() const { return addr == 0; }
    explicit operator bool() const { return addr != 0; }
    bool operator==(const Ptr32& o) const { return addr == o.addr; }
    bool operator!=(const Ptr32& o) const { return addr != o.addr; }
    template <typename U>
    bool operator==(U* p) const { return addr == (u32)(uintptr_t)p; }
    template <typename U>
    bool operator!=(U* p) const { return addr != (u32)(uintptr_t)p; }
    bool operator==(decltype(nullptr)) const { return addr == 0; }
    bool operator!=(decltype(nullptr)) const { return addr != 0; }
};
#endif

#endif
