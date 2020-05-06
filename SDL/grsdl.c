/* --------------------------------- grsdlgl.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Chris Collins
*/

/* SDL+OpenGL Video Driver
 *
 * This is a compatibility shim driver to tide us over until the renderer has
 * been replaced.
*/

#include <SDL.h>

#include "fly.h"

SDL_Window *sdlMainWindow = NULL;
SDL_Renderer *sdlRenderer = NULL;

struct color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
};
static struct color sdlLogicalPalette[256] = {
        {0,   0,   0},
        {128, 0,   0},
        {0,   128, 0},
        {128, 128, 0},
        {0,   0,   128},
        {128, 0,   128},
        {0,   128, 128},
        {192, 192, 192},
        {128, 128, 128},
        {255, 0,   0},
        {0,   255, 0},
        {255, 255, 0},
        {0,   0,   255},
        {255, 0,   255},
        {0,   255, 255},
        {255, 255, 255},
};

static int currx = 0, curry = 0;

static void GrSDLTerm(DEVICE *dev);

static int
GrSDLInit(DEVICE *dev, char *options) {
    SDL_RendererInfo renderInfo;
    sdlMainWindow = SDL_CreateWindow("fly8",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     dev->sizex,
                                     dev->sizey,
                                     0);
    if (sdlMainWindow == NULL) {
        LogPrintf("SDL Failed to Create Window: %s\n", SDL_GetError());
        goto badret;
    }
    /* finally, create our accelerated renderer */
    sdlRenderer = SDL_CreateRenderer(sdlMainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (sdlRenderer == NULL) {
        LogPrintf("SDL Failed to Create Accelerated Renderer: %s\n", SDL_GetError());
        goto badret;
    }

    if (SDL_GetRendererInfo(sdlRenderer, &renderInfo)) {
        LogPrintf("SDL GetRenderInfo failed: %s\n", SDL_GetError());
        goto badret;
    }
    LogPrintf("SDL: Renderer: %s\n", renderInfo.name);
    LogPrintf("SDL:   Flags: ");
    if (renderInfo.flags & SDL_RENDERER_SOFTWARE) {
        LogPrintf("SOFT ");
    }
    if (renderInfo.flags & SDL_RENDERER_ACCELERATED) {
        LogPrintf("HWACCEL ");
    }
    if (renderInfo.flags & SDL_RENDERER_PRESENTVSYNC) {
        LogPrintf("VSYNC ");
    }
    if (renderInfo.flags & SDL_RENDERER_TARGETTEXTURE) {
        LogPrintf("TGTTEXTURE ");
    }
    LogPrintf("\n");

    return 0;
badret:
    GrSDLTerm(dev);
    return 1;
}

static void
GrSDLTerm(DEVICE *dev) {
    if (sdlRenderer != NULL) {
        SDL_DestroyRenderer(sdlRenderer);
        sdlRenderer = NULL;
    }
    if (sdlMainWindow != NULL) {
        SDL_DestroyWindow(sdlMainWindow);
        sdlMainWindow = NULL;
    }
}

static void
GrSDLMoveTo(Uint x1, Uint y1) {
    currx = x1;
    curry = y1;
}

static int sdlActiveColor = 0;
static void
GrSDLSetActiveColor(Uint c) {
    SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(sdlRenderer,
                           sdlLogicalPalette[c].r,
                           sdlLogicalPalette[c].g,
                           sdlLogicalPalette[c].b,
                           SDL_ALPHA_OPAQUE);
    sdlActiveColor = c;
}

static void
GrSDLDrawTo(Uint x2, Uint y2, Uint c) {
    GrSDLSetActiveColor(c);
    SDL_RenderDrawLine(sdlRenderer, currx, curry, x2, y2);
    currx = x2;
    curry = y2;
}

static int
GrSDLSetVisual(int page) {
    SDL_RenderPresent(sdlRenderer);
    return 0;
}

static int
GrSDLSetActive(int page) {
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdlRenderer);
    return 0;
}

void
GrSDLClear(Uint x, Uint y, Uint sx, Uint sy, Uint color) {
    SDL_Rect clearRect = {
            .x = x,
            .y = y,
            .w = sx,
            .h = sy,
    };

    GrSDLSetActiveColor(color);
    SDL_RenderFillRect(sdlRenderer, &clearRect);
}

int
GrSDLSetPalette(int index, long color) {
    sdlLogicalPalette[index].r = C_RGB_R(color);
    sdlLogicalPalette[index].g = C_RGB_G(color);
    sdlLogicalPalette[index].b = C_RGB_B(color);
    LogPrintf("Updated Pal[%d]: %d, %d, %d\n",
            index,
            sdlLogicalPalette[index].r,
            sdlLogicalPalette[index].g,
            sdlLogicalPalette[index].b);
    if (index == sdlActiveColor) {
        // if the color was already active, reset the pen.
        GrSDLSetActiveColor(index);
    }
    return index;
}

struct GrDriver NEAR GrSDL = {
        .name = "GrSDL",
        .Init = GrSDLInit,
        .Term = GrSDLTerm,
        .MoveTo = GrSDLMoveTo,
        .DrawTo = GrSDLDrawTo,
        .SetVisual = GrSDLSetVisual,
        .SetActive = GrSDLSetActive,
        .Clear = GrSDLClear,
        .SetPalette = GrSDLSetPalette,
        //.Flush = GrSDLGLFlush,
};

