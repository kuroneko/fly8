/* ------------------------------- sdlplatform.c ---------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: chris
*/

/* Generic Platform Start-up for SDL
*/

#include <SDL.h>

#include "fly.h"


static int FAR
SdlSysInit (char * options)
{
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        LogPrintf("SDL System Initialisation Failed: %s\n", SDL_GetError());
        return 1;
    }
    return (0);
}

static void FAR
SdlSysTerm (void)
{
    SDL_Quit();
}

extern void SdlConsoleHandleEvent(SDL_Event *event);

static void FAR
SdlSysPoll (void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_CLOSE:
                    die();
                    break;
                default:
                    break;
            }
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            SdlConsoleHandleEvent(&event);
            break;
        default:
            break;
        }
    }
}

static Ulong FAR
SdlSysDisable (void)
{return (0);}

static void FAR
SdlSysEnable (Ulong flags)
{flags=flags;}

/* Build file name from parts.
 * path is NULL for "current directory".
 * path is ""   for "root directory".
*/
static void FAR
SdlSysBuildFileName (char *FullName, char *path, char *name, char *ext)
{
    FullName[0] = '\0';

    if (path) {
        strcat (FullName, path);
        strcat (FullName, "/");
    }
    strcat (FullName, name);
    if (ext && ext[0]) {
        strcat (FullName, ".");
        strcat (FullName, ext);
    }
}

struct SysDriver NEAR SysDriver = {
        .name = "SDLGeneric",
        .Init = SdlSysInit,
        .Term = SdlSysTerm,
        .Poll = SdlSysPoll,
        .Disable = SdlSysDisable,
        .Enable = SdlSysEnable,
        .BuildFileName = SdlSysBuildFileName,
};
