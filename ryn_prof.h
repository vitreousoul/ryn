/*
ryn_prof v0.01 - A simple, recursive profiler.

Written while following the "Performance-Aware Programming Series" at https://www.computerenhance.com/

License:
    Permission to use, copy, modify, and/or distribute this software for
    any purpose with or without fee is hereby granted.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
    FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
    DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
    AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
    OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
Example:

#include "ryn_prof.h"

typedef enum
{
    TB_foo,
    TB_case_1,
    TB_case_2,
} timed_block_kind;

void Foo(int I)
{
    int J, K, X = 0;
    if (I < 50000)
    {
        BEGIN_TIMED_BLOCK(TB_case_1);
        for (J = 0; J < 10000; J++)
        {
            X += 1;
        }
        END_TIMED_BLOCK(TB_case_1);
    }
    else
    {
        BEGIN_TIMED_BLOCK(TB_case_2);
        for (K = 0; K < 10000; K++)
        {
            X += 1;
        }
        END_TIMED_BLOCK(TB_case_2);
    }
}

int main(void)
{
    int I;
    BeginProfile();
    BEGIN_TIMED_BLOCK(TB_foo);
    for (I = 0; I < 80000; I++)
    {
        Foo(I);
    }
    END_TIMED_BLOCK(TB_foo);
    EndAndPrintProfile();
    return 0;
}

*/

/* TODO: include windows code-paths */
/* TODO: prefix external names with "ryn" */
#include <stdio.h>
#include <x86intrin.h>
#include <sys/time.h>

typedef uint32_t u32;
typedef uint64_t u64;
typedef double f64;

#ifndef PROFILER
#define PROFILER 1
#endif

#define ArrayCount(a) ((sizeof(a))/(sizeof((a)[0])))

u64 ReadCPUTimer(void);
u64 ReadOSTimer(void);
void BeginProfile(void);
void EndAndPrintProfile(void);

inline u64 ReadCPUTimer(void)
{
    return __rdtsc();
}

static u64 GetOSTimerFreq(void)
{
    return 1000000;
}

u64 ReadOSTimer(void)
{
    struct timeval Value;
    gettimeofday(&Value, 0);
    u64 Result = GetOSTimerFreq()*(u64)Value.tv_sec + (u64)Value.tv_usec;
    return Result;
}

static int EstimateCpuFrequency()
{
    u64 MillisecondsToWait = 100;
    u64 OSFreq = GetOSTimerFreq();
    u64 CPUStart = ReadCPUTimer();
    u64 OSStart = ReadOSTimer();
    u64 OSEnd = 0;
    u64 OSElapsed = 0;
    u64 OSWaitTime = OSFreq * MillisecondsToWait / 1000;
    while(OSElapsed < OSWaitTime)
    {
        OSEnd = ReadOSTimer();
        OSElapsed = OSEnd - OSStart;
    }
    u64 CPUEnd = ReadCPUTimer();
    u64 CPUElapsed = CPUEnd - CPUStart;
    u64 CPUFreq = 0;
    if(OSElapsed)
    {
        CPUFreq = OSFreq * CPUElapsed / OSElapsed;
    }
    return CPUFreq;
}

#if PROFILER

/* TODO: check that registered timer indices are between 0-MAX_TIMERS */
#define MAX_TIMERS 1024

typedef struct
{
    u64 ElapsedExclusive;
    u64 ElapsedInclusive;
    u64 HitCount;
    char *Label;
} timer_data;

typedef struct
{
    timer_data Timers[MAX_TIMERS];
    u64 StartTime;
    u64 EndTime;
} profiler;
static profiler GlobalProfiler;
u32 GlobalActiveTimer;

/* NOTE: We add 1 to TimerKey in order to reserve the 0 index for the root timer. */
#define GET_TIMER_BY_KEY(TimerKey) (GlobalProfiler.Timers[(TimerKey) + 1])

#define _BEGIN_TIMED_BLOCK(TimerKey, TargetTimer)                       \
    u32 ParentTimer##TimerKey = GlobalActiveTimer;                      \
    GET_TIMER_BY_KEY(TimerKey).Label = #TimerKey;                       \
    u64 OldTSCElapsedInclusive##TimerKey = GET_TIMER_BY_KEY(TimerKey).ElapsedInclusive; \
    TargetTimer = TimerKey + 1;                                         \
    u64 StartTime##TimerKey = ReadCPUTimer();

#define _END_TIMED_BLOCK(TimerKey, TargetTimer)                         \
    u64 Elapsed##TimerKey = ReadCPUTimer() - StartTime##TimerKey;       \
    TargetTimer = ParentTimer##TimerKey;                                \
    GlobalProfiler.Timers[ParentTimer##TimerKey].ElapsedExclusive -= Elapsed##TimerKey; \
    GET_TIMER_BY_KEY(TimerKey).ElapsedExclusive += Elapsed##TimerKey;   \
    GET_TIMER_BY_KEY(TimerKey).ElapsedInclusive = OldTSCElapsedInclusive##TimerKey + Elapsed##TimerKey; \
    GET_TIMER_BY_KEY(TimerKey).HitCount += 1

#define BEGIN_TIMED_BLOCK(TimerKey) _BEGIN_TIMED_BLOCK(TimerKey, GlobalActiveTimer)
#define END_TIMED_BLOCK(TimerKey) _END_TIMED_BLOCK(TimerKey, GlobalActiveTimer)

static void PrintTimeElapsed(u64 TotalElapsedTime, timer_data *Timer)
{
    f64 Percent = 100.0 * ((f64)Timer->ElapsedExclusive / (f64)TotalElapsedTime);
    printf("  %s[%llu]: %llu (%.2f%%", Timer->Label, Timer->HitCount, Timer->ElapsedExclusive, Percent);
    if(Timer->ElapsedInclusive != Timer->ElapsedExclusive)
    {
        f64 PercentWithChildren = 100.0 * ((f64)Timer->ElapsedInclusive / (f64)TotalElapsedTime);
        printf(", %.2f%% w/children", PercentWithChildren);
    }
    printf(")\n");
}

void BeginProfile(void)
{
    GlobalProfiler.StartTime = ReadCPUTimer();
}

void EndAndPrintProfile(void)
{
    GlobalProfiler.EndTime = ReadCPUTimer();
    u64 CPUFreq = EstimateCpuFrequency();

    u64 TotalElapsedTime = GlobalProfiler.EndTime - GlobalProfiler.StartTime;

    if(CPUFreq)
    {
        float TotalElapsedTimeInMs = 1000.0 * (f64)TotalElapsedTime / (f64)CPUFreq;
        printf("\nTotal time: %0.4fms (CPU freq %llu)\n", TotalElapsedTimeInMs, CPUFreq);
    }

    for(u32 TimerIndex = 0; TimerIndex < ArrayCount(GlobalProfiler.Timers); ++TimerIndex)
    {
        timer_data *Timer = GlobalProfiler.Timers + TimerIndex;
        if(Timer->ElapsedInclusive)
        {
            PrintTimeElapsed(TotalElapsedTime, Timer);
        }
    }
}

#else

typedef struct
{
    u64 StartTime;
    u64 EndTime;
} profiler;
static profiler GlobalProfiler;

#define BEGIN_TIMED_BLOCK(...)
#define END_TIMED_BLOCK(...)
#define BEGIN_TIMED_TIMER(...)
#define END_TIMED_TIMER(...)

void BeginProfile(void)
{
    GlobalProfiler.StartTime = ReadCPUTimer();
}

void EndAndPrintProfile()
{
    GlobalProfiler.EndTime = ReadCPUTimer();
    u64 CPUFreq = EstimateCpuFrequency();
    u64 TotalElapsedTime = GlobalProfiler.EndTime - GlobalProfiler.StartTime;
    if(CPUFreq)
    {
        float TotalElapsedTimeInMs = 1000.0 * (f64)TotalElapsedTime / (f64)CPUFreq;
        printf("\nTotal time: %0.4fms (CPU freq %llu)\n", TotalElapsedTimeInMs, CPUFreq);
    }
}

#endif
