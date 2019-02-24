#define ENABLE_NXLINK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#ifndef ENABLE_NXLINK
#define TRACE(fmt, ...) ((void)0)
#else

#include <unistd.h>

#define TRACE(fmt, ...) printf("%s: " fmt "\n", __PRETTY_FUNCTION__, ## __VA_ARGS__)

static int s_nxlinkSock = -1;

static void initNxLink() {
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        TRACE("printf output now goes to nxlink server");
    else
        socketExit();
}

static void deinitNxLink() {
    if (s_nxlinkSock >= 0) {
        close(s_nxlinkSock);
        socketExit();
        s_nxlinkSock = -1;
    }
}

extern "C" void userAppInit() {
    initNxLink();
}

extern "C" void userAppExit() {
    deinitNxLink();
}

#endif

typedef struct {
    SDL_Texture *texture;
    SDL_Surface *surface;
}
        images;
images Logo;

Mix_Chunk *mlg;

std::vector<int> buttons;
std::vector<int> code = {13, 13, 15, 15, 12, 14, 12, 14, 1, 0};

void renderTexture(SDL_Texture *tex, SDL_Renderer *ren, int Srcx, int Srcy, int Destx, int Desty, int w, int h) {
    SDL_Rect srce;
    srce.x = Srcx;
    srce.y = Srcy;
    srce.w = w;
    srce.h = h;

    SDL_Rect dest;
    dest.x = Destx;
    dest.y = Desty;
    dest.w = w;
    dest.h = h;

    SDL_RenderCopy(ren, tex, &srce, &dest);
}

void draw_logo(SDL_Renderer *renderer, int x, int y, int r, int g, int b, int img_size) {
    SDL_SetTextureColorMod(Logo.texture, r, g, b);
    renderTexture(Logo.texture, renderer, 0, 0, x, y, img_size, img_size);
}

int main(int argc, char *argv[]) {
    SDL_Event event;
    SDL_Window *window;
    SDL_Renderer *renderer;
    int done = 0, w = 1280, h = 720, img_size = 225;

    int speed = 3;

    int r = rand() % 255;
    int g = rand() % 255;
    int b = rand() % 255;
    int x = rand() % w - img_size;
    int y = rand() % h - img_size;
    int v_x = rand() % 1;
    int v_y = rand() % 1;
    if (v_x == 0) v_x = -1;
    if (v_y == 0) v_y = -1;
    v_x *= speed;
    v_y *= speed;

    // mandatory at least on switch, else gfx is not properly closed
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        SDL_Log("SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    IMG_Init(IMG_INIT_PNG);
    romfsInit();

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024);
    Mix_AllocateChannels(2);

    mlg = Mix_LoadWAV("romfs:/resources/audio/mlg-horns-sound-effect.wav");
    if (mlg) {
        printf("Loaded audio sucessfully.\n");
    } else {
        printf("AHHHHHHHHHHH!\n");
    }

    // create an SDL window (OpenGL ES2 always enabled)
    // when SDL_FULLSCREEN flag is not set, viewport is automatically handled by SDL (use SDL_SetWindowSize to "change resolution")
    // available switch SDL2 video modes :
    // 1920 x 1080 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888)
    // 1280 x 720 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888)
    window = SDL_CreateWindow("Screensaver", 0, 0, 1280, 720, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // create a renderer (OpenGL ES2)
    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // open CONTROLLER_PLAYER_1 and CONTROLLER_PLAYER_2
    // when railed, both joycons are mapped to joystick #0,
    // else joycons are individually mapped to joystick #0, joystick #1, ...
    // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L45
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            SDL_Log("SDL_JoystickOpen: %s\n", SDL_GetError());
            SDL_Quit();
            return -1;
        }
    }

    Logo.surface = IMG_Load("romfs:/resources/images/logo_big.png");
    Logo.texture = SDL_CreateTextureFromSurface(renderer, Logo.surface);
    SDL_FreeSurface(Logo.surface);

    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_JOYBUTTONDOWN:
                    SDL_Log("Joystick %d button %d down\n",
                            event.jbutton.which, event.jbutton.button);
                    // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L52
                    // seek for joystick #0
                    if (event.jbutton.which == 0) {
                        int button = event.jbutton.button;
                        if (button == 10) {
                            // (+) button down
                            done = 1;
                        } else {
                            buttons.push_back(button);
                            if (buttons.size() > code.size()) {
                                buttons.erase(buttons.begin());
                            }
                            if (buttons == code) {
                                v_x /= speed;
                                v_y /= speed;
                                if (speed == 3) speed = 20;
                                else speed = 3;
                                v_x *= speed;
                                v_y *= speed;
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        draw_logo(renderer, x, y, r, g, b, img_size);

        SDL_RenderPresent(renderer);

        int hit = 0;

        x += v_x;
        y += v_y;
        if (x >= w - img_size) {
            x = (w - img_size) - ((w - img_size) - x);
            v_x = -v_x;
            hit++;
        } else if (x <= 0) {
            x = -x;
            v_x = -v_x;
            hit++;
        }
        if (y >= h - img_size) {
            y = (h - img_size) - ((h - img_size) - y);
            v_y = -v_y;
            hit++;
        } else if (y <= 0) {
            y = -y;
            v_y = -v_y;
            hit++;
        }

        if (hit) {
            r = rand() % 255;
            g = rand() % 255;
            b = rand() % 255;
            if (hit == 2) {
                printf("I saw it. I saw it, and it was amazing!\n");
                printf("Who said I didn't see it? Did Jim say that I didn't see it?\n");
                printf("I saw it!\n");
                Mix_PlayChannel(1, mlg, 0);
            } else {
                printf("Boing.\n");
            }
        }
    }

    Mix_FreeChunk(mlg);
    Mix_CloseAudio();

    SDL_DestroyTexture(Logo.texture);
    romfsExit();
    IMG_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}