
#ifndef COMMON_H
#define COMMON_H

#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef  uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef  int32_t i32;
typedef    float f32;
typedef   double f64;

typedef  _Atomic(u8)  a8;
typedef _Atomic(u16) a16;
typedef _Atomic(u32) a32;
typedef _Atomic(u64) a64;
typedef _Atomic(f64) af64;

#endif
