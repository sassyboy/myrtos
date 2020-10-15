#ifndef _PLATFORM_DEFS_H
#define _PLATFORM_DEFS_H

#include <stdint.h>
typedef uint64_t timestamp_t;

static inline void MemFenceRd(){
  asm volatile("dmb ishld" ::: "memory");
}
static inline void MemFenceWr(){
  asm volatile("dmb ishst" ::: "memory");
}
static inline void MemFenceAll(){
  asm volatile("dmb ish" ::: "memory");
}

struct spinlock{
};

#endif