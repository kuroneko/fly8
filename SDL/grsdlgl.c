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
#include <SDL_opengl.h>

#include "fly.h"

SDL_Window *sdlMainWindow = NULL;
SDL_GLContext sdlGlContext = NULL;
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

static void GrSDLGLTerm(DEVICE *dev);

static int
GrSDLGLInit(DEVICE *dev, char *options) {
    SDL_RendererInfo renderInfo;
#if 1
    if (SDL_CreateWindowAndRenderer(dev->sizex, dev->sizey,
                                    0, &sdlMainWindow, &sdlRenderer)) {
        LogPrintf("SDL Failed to create window: %s\n", SDL_GetError());
        goto badret;
    }
    SDL_SetWindowTitle(sdlMainWindow, "fly8");
#else
    sdlMainWindow = SDL_CreateWindow("fly8",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     dev->sizex,
                                     dev->sizey,
                                     SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    if (sdlMainWindow == NULL) {
        LogPrintf("SDL Failed to Create Window: %s\n", SDL_GetError());
        goto badret;
    }
#if 0
    /* configure the GL context for GL4.6 Core */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, GL_TRUE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

    /* create the GL context */
    sdlGlContext = SDL_GL_CreateContext(sdlMainWindow);
    if (NULL == sdlGlContext) {
        LogPrintf("SDL Failed to Create OpenGL Context: %s\n", SDL_GetError());
        goto badret;
    }
#endif
    /* finally, create our accelerated renderer */
    sdlRenderer = SDL_CreateRenderer(sdlMainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (sdlRenderer == NULL) {
        LogPrintf("SDL Failed to Create Accelerated Renderer: %s\n", SDL_GetError());
        goto badret;
    }
#endif

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
    GrSDLGLTerm(dev);
    return 1;
}

static void
GrSDLGLTerm(DEVICE *dev) {
    if (sdlRenderer != NULL) {
        SDL_DestroyRenderer(sdlRenderer);
        sdlRenderer = NULL;
    }
    if (sdlGlContext != NULL) {
        SDL_GL_DeleteContext(sdlGlContext);
        sdlGlContext = NULL;
    }
    if (sdlMainWindow != NULL) {
        SDL_DestroyWindow(sdlMainWindow);
        sdlMainWindow = NULL;
    }
}

static void
GrSDLGLMoveTo(Uint x1, Uint y1) {
    currx = x1;
    curry = y1;
}

static int sdlActiveColor = 0;
static void
GrSDLGLSetActiveColor(Uint c) {
    SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(sdlRenderer,
                           sdlLogicalPalette[c].r,
                           sdlLogicalPalette[c].g,
                           sdlLogicalPalette[c].b,
                           SDL_ALPHA_OPAQUE);
    sdlActiveColor = c;
}

static void
GrSDLGLDrawTo(Uint x2, Uint y2, Uint c) {
    GrSDLGLSetActiveColor(c);
    SDL_RenderDrawLine(sdlRenderer, currx, curry, x2, y2);
    currx = x2;
    curry = y2;
}

static int
GrSDLGLSetVisual(int page) {
    SDL_RenderPresent(sdlRenderer);
    return 0;
}

static int
GrSDLGLSetActive(int page) {
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdlRenderer);
    return 0;
}

void
GrSDLGLClear(Uint x, Uint y, Uint sx, Uint sy, Uint color) {
    SDL_Rect clearRect = {
            .x = x,
            .y = y,
            .w = sx,
            .h = sy,
    };

    GrSDLGLSetActiveColor(color);
    SDL_RenderFillRect(sdlRenderer, &clearRect);
}

int
GrSDLGLSetPalette(int index, long color) {
    sdlLogicalPalette[index].r = LOBYTE(C_RGB_R(color));
    sdlLogicalPalette[index].g = LOBYTE(C_RGB_G(color));
    sdlLogicalPalette[index].b = LOBYTE(C_RGB_B(color));
    LogPrintf("Updated Pal[%d]: %d, %d, %d\n",
            index,
            sdlLogicalPalette[index].r,
            sdlLogicalPalette[index].g,
            sdlLogicalPalette[index].b);
    if (index == sdlActiveColor) {
        // if the color was already active, reset the pen.
        GrSDLGLSetActiveColor(index);
    }
    return index;
}

struct GrDriver NEAR GrSDLOpenGL = {
        .name = "GrSDLOpenGL",
        .Init = GrSDLGLInit,
        .Term = GrSDLGLTerm,
        .MoveTo = GrSDLGLMoveTo,
        .DrawTo = GrSDLGLDrawTo,
        .SetVisual = GrSDLGLSetVisual,
        .SetActive = GrSDLGLSetActive,
        .Clear = GrSDLGLClear,
        .SetPalette = GrSDLGLSetPalette,
        //.Flush = GrSDLGLFlush,
};

