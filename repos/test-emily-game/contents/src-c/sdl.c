#include "SDL.h"
#include <stdlib.h> /* for atexit() */
#include <stdbool.h>
#include "util.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.h"

// Calls in this file follow this convention:
// Return 0 for no error, nonzero for error
// On error, a string will be written into globalError
#define ERRLEN 256
static char globalError[ERRLEN];
#define SILENTFAIL(cond) if (cond) { return -1; }
#define FAIL(cond, ...) if (cond) { snprintf( globalError, ERRLEN, __VA_ARGS__ ); return -1; }
#define SDLFAIL(cond, where) FAIL((cond), "SDL failure in %s: %s\n", where, SDL_GetError());
#define INITFAIL() FAIL(!renderer || !pics || !pics[0].texture, "Not initialized")
#define EXISTFAILBASE(i, condition) INITFAIL(); FAIL((i) < 0 || (i) >= picCount || (condition), "Img not found: %d", img)
#define EXISTFAIL(i) EXISTFAILBASE(i, 0)
#define EXISTFAILTEXTURE(i) EXISTFAILBASE(i, !pics[i].texture)
#define EXISTFAILPIXELS(i) EXISTFAILBASE(i, !pics[i].pixels)
#define CHAIN(x) { int TEMP = (x); if (TEMP < 0) return TEMP; }

char *GAMEglobalError() { return globalError; }

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const unsigned int rmask = 0xff000000;
    const unsigned int gmask = 0x00ff0000;
    const unsigned int bmask = 0x0000ff00;
    const unsigned int amask = 0x000000ff;
#else
    const unsigned int rmask = 0x000000ff;
    const unsigned int gmask = 0x0000ff00;
    const unsigned int bmask = 0x00ff0000;
    const unsigned int amask = 0xff000000;
#endif

enum {
    TextureCanDisplay = 1,
    TextureCanEdit = 2,
    TextureCanComposit = 4,
};

enum {
    ImgInfoWidth = 0,
    ImgInfoHeight = 1,
};

enum {
    EventConfused = 0,
    EventDraw = 2,
    EventEscPress     = 4,
    EventEscRelease   = 5,
    EventLeftPress    = 6,
    EventLeftRelease  = 7,
    EventRightPress   = 8,
    EventRightRelease = 9,
    EventUpPress      = 10,
    EventUpRelease    = 11,
    EventDownPress    = 12,
    EventDownRelease  = 13,
};

// Setup

typedef struct {
    int w,h;
    unsigned char *pixels;
    SDL_Texture *texture;
} pic;

SDL_Window *window = NULL;
int windoww = 0, windowh = 0, windowscale = 1;
SDL_Renderer *renderer = NULL;
pic *pics = NULL;
int picCount = 0, picStorage = 0;

// Vector helpers
static int expandStorage()
{
    int maxGrow = 128;
    int newStorage = picStorage >= maxGrow ? picStorage + maxGrow : picStorage * 2;
    pic *newPics = realloc(pics, sizeof(pic)*newStorage);
    SILENTFAIL(!newPics);

    // Zero new members
    memset(&newPics[picStorage], 0, sizeof(pic)*(newStorage-picStorage));

    // New is valid
    pics = newPics;
    picStorage = newStorage;

    return 0;
}

static int bumpStorage()
{
    if (picCount >= picStorage) {
        SILENTFAIL( expandStorage() );
    }

    return 0;
}

static SDL_Texture *makeTexture(int w, int h, int flags)
{
    return SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
        flags&TextureCanComposit ? SDL_TEXTUREACCESS_TARGET : SDL_TEXTUREACCESS_STATIC,
        w, h);
}

static int min(int x, int y)
{
    return x < y ? x : y;
}

#define resourcePost ""

static char *__basePath = NULL;
char *GAMEresourcePath() {
    if (!__basePath) {
        char *sdlPath = SDL_GetBasePath();
        if (!sdlPath) {
            sdlPath = SDL_strdup("./");
        }

        // This next line might not be appropriate once there is an OS X package.
        __basePath = concat(sdlPath, "resource/");

        SDL_free(sdlPath);
    }
    return __basePath;
}

int GAMEimgClear(int img, double r, double g, double b, double a);

int GAMEfinish()
{
    // Destroy texture stack
    for(int c = 0; c < picCount; c++) {
        stbi_image_free(pics[c].pixels);
        SDL_DestroyTexture(pics[c].texture);
    }
    free(pics);
    pics = NULL; picCount = 0; picStorage = 0;

    // Destroy window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    renderer = NULL;
    window = NULL;

    return 0;
}

int GAMEinit(int fullscreen, int w, int h)
{
    // Init main SDL system
    int temp = SDL_Init(SDL_INIT_VIDEO);
    int panewidth=640, paneheight=360;
    SDLFAIL(temp < 0, "init");

    atexit(SDL_Quit);

    // Init window
    window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        panewidth,                         // width, in pixels
        paneheight,                        // height, in pixels
        (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) // flags
    );
    SDLFAIL(!window, "create window");

    SDL_GL_GetDrawableSize(window, &windoww, &windowh);

    // Ascertain "screen" size within window
    for(int c = 2; c < 100000; c++) { // Cap somewhere in case they gave us 0s
        int w2 = w*c, h2 = h*c;
        if (w2 <= windoww && h2 <= windowh)
            windowscale = c;
        else
            break;
    }

    // Make renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Make "main screen" texture
    if (renderer) {
        picCount = 1; picStorage = 1;
        pics = calloc(sizeof(pic), 1);
        pics[0].w = w; pics[0].h = h;
        pics[0].texture = makeTexture(pics[0].w, pics[0].h, TextureCanComposit);

        GAMEimgClear(0, 0, 0, 0, 1); // Clear to black
    }

    // Handle failures somewhere
    if (!pics || !pics[0].texture) {
        GAMEfinish();

        SDLFAIL(1, "create renderer");
    }

    return 0;
}

int GAMEdelay(int delay)
{
    SDL_Delay(delay);

    return 0;
}

// Image loading/creation

int GAMEimgCount()
{
    return picCount;
}

int GAMEimgValid(int img)
{
    return img >= 0 && img < picCount && pics && pics[img].texture;
}

int GAMEimgFree(int img)
{
    EXISTFAIL(img);
    FAIL(img == 0, "Cannot free window image");

    stbi_image_free(pics[img].pixels);
    pics[img].pixels = NULL;
    SDL_DestroyTexture(pics[img].texture);
    pics[img].texture = NULL;
    return 0;
}

int GAMEimgPushBlank(int flags)
{
    INITFAIL();
    FAIL(bumpStorage(), "Couldn't expand image storage");
    picCount++;
    return 0;
}

int GAMEimgPush(int w, int h, int flags)
{
    INITFAIL();
    FAIL(bumpStorage(), "Couldn't expand image storage");

    if (w < 0 || h < 0) {
        w = pics[0].w; h = pics[0].h;
    }

    // TODO: honor flags here
    pics[picCount].texture = makeTexture(w,h,flags);
    FAIL(!pics[picCount].texture, "Couldn't allocate image");

    // Make pic real
    pics[picCount].w = w; pics[picCount].h = h;
    picCount++;

    return 0;
}

int GAMEimgLoad(char *name, int flags)
{
    INITFAIL();
    FAIL(bumpStorage(), "Couldn't expand image storage");
    FAIL(flags & TextureCanComposit, "Can't composit into an img loaded from a file")

    char *fullname = concat( GAMEresourcePath(), name );
    FAIL(!fullname, "Bad file name on image load");

    FILE *file = fopen(fullname, "rb");
    free(fullname);
    FAIL(!file, "Could not open image %s", name);

    int w, h, comp;
    unsigned char *pixels = stbi_load_from_file(file, &w, &h, &comp, 4);
    fclose(file);
    FAIL(!pixels, "Could not process image %s", name);

    // Must not deallocate pixels until both operations finished
    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;
    bool needTexture = flags & TextureCanDisplay;

    if (needTexture)
        surface = SDL_CreateRGBSurfaceFrom(pixels, w, h, 32, 4*w, rmask, gmask, bmask, amask);
    if (surface)
        texture = SDL_CreateTextureFromSurface(renderer, surface);

    // Free if pixels are unneeded or if we have failed
    if ((needTexture && !texture) || !(flags & TextureCanEdit)) {
        stbi_image_free(pixels);
        pixels = NULL;
    }

    FAIL(needTexture && !surface, "Could not convert image %s", name);
    SDL_FreeSurface(surface);
    FAIL(needTexture && !texture, "Could not texture-convert image %s", name);

    // Make pic real
    pics[picCount].w = w; pics[picCount].h = h;
    pics[picCount].pixels = pixels;
    pics[picCount].texture = texture;
    picCount++;

    return 0;
}

int GAMEimgRead(int img, int x, int y, int channel)
{
    EXISTFAILPIXELS(img);
    int w = pics[img].w; int h = pics[img].h;
    FAIL(x < 0 || y < 0 || x >= w || y >= h, "Read point %d, %d outside size %d, %d", x, y, w, h);

    return pics[img].pixels[ (y*w + x)*4 + channel ];
}

int GAMEimgInfo(int img, int key)
{
    EXISTFAIL(img);

    switch (key)
    {
        case ImgInfoWidth:
            return pics[img].w;
        case ImgInfoHeight:
            return pics[img].h;
        default:
            FAIL(1, "Unknown info key");
    }
}

// Rendering

int GAMEimgClear(int img, double r, double g, double b, double a)
{
    EXISTFAILTEXTURE(img);

    SDL_SetRenderTarget(renderer, pics[img].texture);
    SDL_SetRenderDrawColor(renderer, r*255, g*255, b*255, a*255);
    SDL_RenderClear(renderer);

    return 0;
}

int GAMEimgCopy(int dst, int img, int x, int y, int srcX, int srcY, int blend, double alpha)
{
    EXISTFAILTEXTURE(dst); EXISTFAILTEXTURE(img);

    SDL_Rect dstRect = { x, y,       pics[img].w, pics[img].h };
    SDL_Rect imgRect = { srcX, srcY, pics[img].w, pics[img].h };

    SDL_SetTextureAlphaMod(pics[img].texture, alpha*255);
    SDL_SetTextureBlendMode(pics[img].texture, blend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_SetRenderTarget(renderer, pics[dst].texture);
    SDL_RenderCopy(renderer, pics[img].texture, &imgRect, &dstRect); // "Entire source"
    return 0;
}

static bool firstFrame = true;
static unsigned int lastDraw = 0;

int GAMEpresent()
{
    INITFAIL()

    int w = pics[0].w*windowscale, h = pics[0].h*windowscale;
    SDL_Rect dstRect = { (windoww-w)/2, (windowh-h)/2, w, h };

    SDL_SetRenderTarget(renderer, NULL); // Reset to draw on screen
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    SDL_RenderClear(renderer);

    SDL_SetTextureAlphaMod(pics[0].texture, 255); // Just in case
    SDL_SetTextureBlendMode(pics[0].texture, SDL_BLENDMODE_NONE);
    SDL_RenderCopy(renderer, pics[0].texture, NULL, &dstRect);
    SDL_RenderPresent(renderer);

    firstFrame = false;
    lastDraw = SDL_GetTicks();

    return 0;
}

int GAMEtick(int tpf)
{
    INITFAIL();

    SDL_Event event;
    while (1) {
        int sdlTime = SDL_GetTicks();
        int timeToWait = min(0, lastDraw+tpf-sdlTime);

        int haveEvent = !firstFrame && (SDL_PollEvent(&event) || SDL_WaitEventTimeout(&event, timeToWait));
        if (!haveEvent) {
            CHAIN( GAMEpresent() );
            return EventDraw;
        }
        switch (event.type) {
            case SDL_KEYDOWN: case SDL_KEYUP: {
                int base = EventConfused;
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: base = EventEscPress;   break;
                    case SDLK_LEFT:   base = EventLeftPress;  break;
                    case SDLK_RIGHT:  base = EventRightPress; break;
                    case SDLK_UP:     base = EventUpPress;    break;
                    case SDLK_DOWN:   base = EventDownPress;  break;
                    default: break;
                }
                if (base != EventConfused)
                    return base + (event.type == SDL_KEYUP ? 1 : 0);
                break; // Still here? Then the event was unrecognized.
            }
            default: break;
        }
    }
}