#ifndef HY3D_BASE_H
#define HY3D_BASE_H 1

// NOTE(heyyod): This produces a compilation error all of a sudden. wtf???? 
#include <cstdint>

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef float     f32;
typedef double    f64;

#define U32_MAX 0xFFFFFFFF
#define F32_MAX_HEX 0xFFFF7F7F
#define F32_MAX_EXP 3.4028235e+38

#define global_var static
#define local_var static
#define func  static

#define KILOBYTES(val) (val * 1024ULL)
#define MEGABYTES(val) (KILOBYTES(val) * 1024ULL)
#define GIGABYTES(val) (MEGABYTES(val) * 1024ULL)
#define TERABYTES(val) (GIGABYTES(val) * 1024ULL)

#define Min(a, b) ((a) > (b) ? (b) : (a))
#define Max(a, b) ((a) < (b) ? (b) : (a))
#define Abs(a) ((a) > 0 ? (a) : -(a))
#define Mod(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))
#define Square(x) ((x) * (x))

// TODO: Make this an actual assetion
#define AssertBreak() *(int *)0 = 0

#if DEBUG_MODE
#define Assert(val) \
if (!(val))     \
AssertBreak()
#else
#define Assert(val)
#endif

#include <iostream>
#define Print(val) std::cout << val

#if DEBUG_MODE
// TODO(heyyod): Use fast_io instead?
#define DebugPrint(val) Print(val)
#else
#define DebugPrint(val)
#endif

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#if DEBUG_MODE
#define AssertArraySize(array, count) \
if (ArrayCount(array) < count)    \
AssertBreak()
#else
#define AssertArraySize(array, count)
#endif

#define AdvancePointer(ptr, bytes) ptr = (u8*)ptr + bytes

#if DEBUG_MODE
#include <chrono>
#include <thread>
global_var std::chrono::steady_clock::time_point timeStart;
global_var std::chrono::steady_clock::time_point timeEnd;
global_var std::chrono::duration<f32> duration;
global_var f32 elapsedTime;
#define Now() std::chrono::steady_clock::now()
#define TimeStart() timeStart = Now()
#define TimeEnd() \
timeEnd = Now(); \
duration = timeEnd - timeStart; \
elapsedTime = duration.count()
#define PrintTimeElapsed() \
Print("Time Elapsed: " << elapsedTime << "s" << std::endl)
#define SleepFor(x) std::this_thread::sleep_for(std::chrono::milliseconds(x));
#else
#define Now()
#define TimeStart()
#define TimeEnd()
#define PrintTimeElapsed()
#endif

#endif
