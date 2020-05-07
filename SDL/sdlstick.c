/* --------------------------------- sdlstick.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: chris
*/

/* SDL Joystick Handling
*/

#include <SDL.h>

#include "core/fly.h"
#include "stick.h"

#define SDLS_MAX_AXES 6
#define SDLS_MAX_BUTTONS 32

struct SdlStickState {
    SDL_Joystick *stickHandle;
    SDL_JoystickID stickId;
    int axisOffset;
    int buttonOffset;
    int axisCount;
    int buttonCount;
    int axisValues[SDLS_MAX_AXES];
    int buttonValues[SDLS_MAX_BUTTONS];

};

static int sdlJsCount;
static struct SdlStickState *sdlStickAllSticks = NULL;

static struct SdlStickState *
SDLStickFindState(SDL_JoystickID stickId) {
    int ctr;

    if (sdlStickAllSticks == NULL) {
        return NULL;
    }

    for (ctr = 0; ctr < sdlJsCount; ctr++) {
        if (sdlStickAllSticks[ctr].stickId == stickId) {
            return &sdlStickAllSticks[ctr];
        }
    }
    return NULL;
}

static int FAR
SDLStickInit(POINTER *p, char *options) {
    int ctr;
    int axisOffset = 0;
    int buttonOffset = 0;

    sdlJsCount = SDL_NumJoysticks();

    sdlStickAllSticks = calloc(sdlJsCount, sizeof(*sdlStickAllSticks));
    if (NULL == sdlStickAllSticks) {
        return 1;
    }

    for (ctr = 0; ctr < sdlJsCount; ctr++) {
        struct SdlStickState *const thisStick = &sdlStickAllSticks[ctr];
        SDL_JoystickType jsType;
        int useStick = 0;

        LogPrintf("SDLStick: Joystick %d: %s\n", ctr, SDL_JoystickNameForIndex(ctr));
        jsType = SDL_JoystickGetDeviceType(ctr);
        switch (jsType) {
            case SDL_JOYSTICK_TYPE_FLIGHT_STICK:
                LogPrintf("  - Is a Flight Stick\n");
                useStick = 1;
                break;
            case SDL_JOYSTICK_TYPE_THROTTLE:
                LogPrintf("  - Is a Throttle\n");
                useStick = 1;
                break;
            case SDL_JOYSTICK_TYPE_GAMECONTROLLER:
                LogPrintf("  - Is a Game Controller\n");
                useStick = 1;
                break;
            case SDL_JOYSTICK_TYPE_UNKNOWN:
                LogPrintf("  - Is an Unknown Type\n");
                useStick = 1;
                break;
            default:
                LogPrintf("  - Is not a useful type\n");
                break;
        }
        if (!useStick) {
            continue;
        }
        thisStick->stickHandle = SDL_JoystickOpen(ctr);
        if (NULL == thisStick->stickHandle) {
            LogPrintf("SDLStick: Failed to Open stick %d: %s\n", ctr, SDL_GetError());
            continue;
        }
        thisStick->stickId = SDL_JoystickInstanceID(thisStick->stickHandle);
        thisStick->axisOffset = axisOffset;
        thisStick->axisCount = SDL_JoystickNumAxes(thisStick->stickHandle);
        axisOffset += thisStick->axisCount;
        thisStick->buttonOffset = buttonOffset;
        thisStick->buttonCount = SDL_JoystickNumButtons(thisStick->stickHandle);
        buttonOffset += thisStick->buttonCount;
    }
    return 0;
}

static void FAR
SDLStickTerm(POINTER *p) {
    int ctr;

    if (sdlStickAllSticks != NULL) {
        for (ctr = 0; ctr < sdlJsCount; ctr++) {
            struct SdlStickState *const thisStick = &sdlStickAllSticks[ctr];

            if (SDL_JoystickGetAttached(thisStick->stickHandle)) {
                SDL_JoystickClose(thisStick->stickHandle);
                thisStick->stickHandle = NULL;
            }
        }
        free(sdlStickAllSticks);
        sdlStickAllSticks = NULL;
        sdlJsCount = 0;
    }
}

void FAR
SDLStickHandleEvent(SDL_Event *jsEvent) {
    struct SdlStickState *thisJs;
    switch (jsEvent->type) {
        case SDL_JOYAXISMOTION:
            thisJs = SDLStickFindState(jsEvent->jaxis.which);
            if (NULL == thisJs || jsEvent->jaxis.axis >= SDLS_MAX_AXES) {
                break;
            }
            thisJs->axisValues[jsEvent->jaxis.axis] = jsEvent->jaxis.value;
            break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            thisJs = SDLStickFindState(jsEvent->jbutton.which);
            if (NULL == thisJs || jsEvent->jbutton.button >= SDLS_MAX_BUTTONS) {
                break;
            }
            thisJs->buttonValues[jsEvent->jbutton.button] = jsEvent->jbutton.state;
            break;
        default:
            break;
    }
}

static int FAR
SDLStickRead(POINTER *p) {
    int ctr;
    int axisCount = 0;
    int buttonCount = 0;
    int sctr;

    for (ctr = 0; ctr < sdlJsCount; ctr++) {
        struct SdlStickState *const thisStick = &sdlStickAllSticks[ctr];
        if (thisStick->stickHandle == NULL) {
            continue;
        }
        for (sctr = 0; sctr < thisStick->axisCount; sctr++) {
            if ((sctr + thisStick->axisOffset) >= NANALOG) {
                break;
            }
            p->a[sctr + thisStick->axisOffset] = ((int)thisStick->axisValues[sctr]) * 100 / SDL_JOYSTICK_AXIS_MAX;
        }
        for (sctr = 0; sctr < thisStick->buttonCount; sctr++) {
            if ((sctr + thisStick->buttonOffset) >= NBTNS) {
                break;
            }
            p->btn[sctr + thisStick->buttonOffset] = (thisStick->buttonValues[sctr] == SDL_PRESSED);
        }
    }
    return 0;
}

struct PtrDriver NEAR PtrSdlStick = {
        .name = "SDLStick",
        .Init = SDLStickInit,
        .Term = SDLStickTerm,
        .Read = SDLStickRead,
        .Key = std_key,
};
