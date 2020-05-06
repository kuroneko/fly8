/* --------------------------------- timer.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: chris
*/

/* Time services for SDL.
*/

#include <SDL.h>
#include <time.h>

#include "fly.h"

static Uint64 tmPerfFrequency;

static int FAR
SDLTmInit(char *options) {
    tmPerfFrequency = SDL_GetPerformanceFrequency();
    return 0;
}

static Ulong FAR
SDLTmMilli(void) {
    return (SDL_GetTicks());
}

static int FAR
SDLTmHires(void) {
    return ((int)SDL_GetPerformanceCounter());
}

static char *FAR
SDLTmCtime(void) {
    time_t tm;
    char *t;

    tm = time(0);
    t = ctime(&tm);
    t[strlen(t) - 1] = '\0';    /* kill NewLine */
    return (t);
}

#define NINTS        10

/* res is the desired frequency we want the interval reported in */
static Ulong FAR
SDLTmInterval(int mode, Ulong res) {
    static Uint64 last_time[NINTS];
    static int n = -1;
    Uint64 t, tt = 0;

    if (mode & TMR_PUSH) {
        ++n;
        if (n >= NINTS) {
            LogPrintf("timer: too many PUSHes... aborting\n");
            die();
        }
    } else if (n < 0) {
        LogPrintf("timer: too many POPs... aborting\n");
        die();
    }

    if (mode & (TMR_READ | TMR_SET))
        tt = SDL_GetPerformanceCounter();

    if (mode & TMR_READ) {
        t = tt - last_time[n];
        if (res)
            t = t * res / tmPerfFrequency;
    } else
        t = 0;

    if (mode & TMR_SET)
        last_time[n] = tt;

    if (mode & TMR_POP)
        --n;
    return ((Ulong) t);
}

struct TmDriver NEAR TmDriver = {
        .name = "SDLSystem",
        .Init = SDLTmInit,
        .Milli = SDLTmMilli,
        .Hires = SDLTmHires,
        .Ctime = SDLTmCtime,
        .Interval= SDLTmInterval,
};
