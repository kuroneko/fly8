/* --------------------------------- console.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: chris
*/

/* Console and Event Loop Handler for SDL
*/

#include <SDL.h>

#include "fly.h"
#include "mouse.h"

static int shift = 0, ctrl = 0, alt = 0;

#define PUSH_SIZE    256
static int    FAR push_buf[PUSH_SIZE] = {0};
static int push_head = 0, push_tail = 0, push_size = 0;

LOCAL_FUNC int NEAR
SdlPush(int c) {
    if (push_size == PUSH_SIZE)
        return (1);

    push_buf[push_tail++] = c;
    if (push_tail == PUSH_SIZE)
        push_tail = 0;
    ++push_size;

    return (0);
}

LOCAL_FUNC int NEAR
SdlPop(void) {
    int c;

    if (!push_size)
        return (-1);

    c = push_buf[push_head++];
    if (push_head == PUSH_SIZE)
        push_head = 0;
    --push_size;

    return (c);
}

LOCAL_FUNC int FAR
kread(void) {
    return SdlPop();
}


void FAR
SdlConsoleHandleEvent(SDL_Event *event) {
    int key = -1;
    switch (event->type) {
        case SDL_KEYUP:
            switch (event->key.keysym.sym) {
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                    ctrl = 0;
                    break;
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    shift = 0;
                    break;
                case SDLK_LALT:
                case SDLK_RALT:
                    alt = 0;
                    break;
                default:
                    break;
            }
            break;
        case SDL_KEYDOWN:
            /* do a coarse filter for scancoded keys first - these must be
             * explicitly converted or they are rejected
            */
            if (event->key.keysym.sym & SDLK_SCANCODE_MASK) {
                switch (event->key.keysym.sym) {
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        ctrl = K_CTRL;
                        break;
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT:
                        shift = 32;
                        break;
                    case SDLK_LALT:
                    case SDLK_RALT:
                        alt = K_ALT;
                        break;
                    case SDLK_ESCAPE:
                        key = K_ESC;
                        break;
                    case SDLK_SPACE:
                        key = ' ';
                        break;
                    case SDLK_KP_0:
                        key = '0';
                        break;
                    case SDLK_KP_1:
                    case SDLK_KP_2:
                    case SDLK_KP_3:
                    case SDLK_KP_4:
                    case SDLK_KP_6:
                    case SDLK_KP_7:
                    case SDLK_KP_8:
                    case SDLK_KP_9:
                        key = '1' + event->key.keysym.sym - SDLK_KP_1;
                        break;
                    case SDLK_KP_5:
                        key = K_CENTER;
                        break;
                    case SDLK_KP_ENTER:
                    case SDLK_RETURN2:
                        key = K_ENTER;
                        break;
                    case SDLK_BACKSPACE:
                        key = K_RUBOUT;
                        break;
                    case SDLK_KP_PERIOD:
                        key = '.';
                        break;
                    case SDLK_KP_PLUS:
                        key = '+';
                        break;
                    case SDLK_KP_MINUS:
                        key = '-';
                        break;
                    case SDLK_KP_DIVIDE:
                        key = '/';
                        break;
                    case SDLK_KP_MULTIPLY:
                        key = '*';
                        break;
                    case SDLK_SEPARATOR:
                        key = '\\';
                        break;
                    case SDLK_F1:
                        key = K_F1;
                        break;
                    case SDLK_F2:
                        key = K_F2;
                        break;
                    case SDLK_F3:
                        key = K_F3;
                        break;
                    case SDLK_F4:
                        key = K_F4;
                        break;
                    case SDLK_F5:
                        key = K_F5;
                        break;
                    case SDLK_F6:
                        key = K_F6;
                        break;
                    case SDLK_F7:
                        key = K_F7;
                        break;
                    case SDLK_F8:
                        key = K_F8;
                        break;
                    case SDLK_F9:
                        key = K_F9;
                        break;
                    case SDLK_F10:
                        key = K_F10;
                        break;
                    case SDLK_F11:
                        key = K_F11;
                        break;
                    case SDLK_F12:
                        key = K_F12;
                        break;
                    case SDLK_UP:
                        key = K_UP;
                        break;
                    case SDLK_DOWN:
                        key = K_DOWN;
                        break;
                    case SDLK_LEFT:
                        key = K_LEFT;
                        break;
                    case SDLK_RIGHT:
                        key = K_RIGHT;
                        break;
                    case SDLK_END:
                        key = K_END;
                        break;
                    case SDLK_HOME:
                        key = K_HOME;
                        break;
                    case SDLK_INSERT:
                        key = K_INS;
                        break;
                    case SDLK_PAGEUP:
                        key = K_PGUP;
                        break;
                    case SDLK_PAGEDOWN:
                        key = K_PGDN;
                        break;
                }
            } else {
                switch (event->key.keysym.sym) {
                    case SDLK_RETURN:
                        key = K_ENTER;
                        break;
                    case SDLK_ESCAPE:
                        key = K_ESC;
                        break;
                    case SDLK_TAB:
                        key = K_TAB;
                        break;
                    case SDLK_DELETE:
                        key = K_DEL;
                        break;
                    case SDLK_BACKSPACE:
                        key = K_RUBOUT;
                        break;
                    default:
                        key = event->key.keysym.sym;
                        if (key >= 'a' && key <= 'z') {
                            key -= shift;
                        }
                        break;
                }
            }
            if (key >= 0) {
                if (alt)
                    key |= K_ALT;
                if (ctrl)
                    key |= K_CTRL;
                SdlPush(key);
            }
            break;
        default:
            break;
    }
}


LOCAL_FUNC int FAR
kwait(void) {
    int esc, c;

    for (esc = 0; -1 != (c = kread());)
        if (K_ESC == c)
            esc = 1;
    return (esc);
}

LOCAL_FUNC int FAR
kgetch(void) {
    int c;

    while ((c = kread()) == -1)
        sys_poll(20);
    return (c);
}

LOCAL_FUNC int FAR
kinit(char *options) {
    options = options;
    return (0);
}

LOCAL_FUNC void FAR
kterm(void) {}

struct KbdDriver NEAR KbdConsole = {
        .name = "SdlConsole",
        .Init = kinit,
        .Term = kterm,
        .Read = kread,
        .Getch = kgetch,
        .Wait = kwait,
};

/*FIXME:  this should just be copying state driven by the event loop rather than
 * polling SDL's mouse functions. */
#ifndef MAX_MOUSE_BUTTONS
#define MAX_MOUSE_BUTTONS 5
#endif

extern int FAR
GetMouse(int *x, int *y, char *btn, int *nbtn) {
    int c;
    Uint32 buttonMask = SDL_GetMouseState(x, y);

    *nbtn = MAX_MOUSE_BUTTONS;
    for (c = 0; c < MAX_MOUSE_BUTTONS; c++) {
        btn[c] = ((buttonMask & SDL_BUTTON(c + 1)) != 0);
    }
    return (0);
}
