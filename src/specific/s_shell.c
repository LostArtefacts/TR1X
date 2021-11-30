#include "specific/s_shell.h"

#include "config.h"
#include "game/gamebuf.h"
#include "game/input.h"
#include "game/music.h"
#include "game/random.h"
#include "game/shell.h"
#include "global/vars_platform.h"
#include "log.h"
#include "memory.h"
#include "specific/s_hwr.h"

#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <time.h>

static int m_ArgCount = 0;
static char **m_ArgStrings = NULL;
static bool m_Fullscreen = true;
static SDL_Window *m_Window = NULL;

static void S_Shell_ShowFatalError(const char *message);
static void S_Shell_PostWindowResize();

void S_Shell_SeedRandom()
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    Random_SeedControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    Random_SeedDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}

static void S_Shell_ShowFatalError(const char *message)
{
    LOG_ERROR("%s", message);
    MessageBoxA(
        0, message, "Tomb Raider Error", MB_SETFOREGROUND | MB_ICONEXCLAMATION);
    S_Shell_TerminateGame(1);
}

static void S_Shell_PostWindowResize()
{
    int width;
    int height;
    SDL_GetWindowSize(m_Window, &width, &height);
    HWR_SetViewport(width, height);
}

void S_Shell_ToggleFullscreen()
{
    m_Fullscreen = !m_Fullscreen;
    HWR_SetFullscreen(m_Fullscreen);
    SDL_SetWindowFullscreen(
        m_Window, m_Fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_ShowCursor(m_Fullscreen ? SDL_DISABLE : SDL_ENABLE);
    S_Shell_PostWindowResize();
}

void S_Shell_TerminateGame(int exit_code)
{
    HWR_Shutdown();
    if (m_Window) {
        SDL_DestroyWindow(m_Window);
    }
    SDL_Quit();
    exit(exit_code);
}

void S_Shell_SpinMessageLoop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            S_Shell_TerminateGame(0);
            break;

        case SDL_KEYDOWN: {
            const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
            if (keyboard_state[SDL_SCANCODE_LALT]
                && keyboard_state[SDL_SCANCODE_RETURN]) {
                S_Shell_ToggleFullscreen();
            }
            break;
        }

        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                Music_SetVolume(g_Config.music_volume);
                break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
                Music_SetVolume(0);
                break;

            case SDL_WINDOWEVENT_RESIZED: {
                HWR_SetViewport(event.window.data1, event.window.data2);
                break;
            }
            }
            break;
        }
    }
}

int main(int argc, char **argv)
{
    Log_Init();

    m_ArgCount = argc;
    m_ArgStrings = argv;

    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        S_Shell_ExitSystemFmt("Cannot initialize SDL: %s", SDL_GetError());
        return 1;
    }

    m_Window = SDL_CreateWindow(
        "Tomb1Main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
            | SDL_WINDOW_RESIZABLE);
    if (!m_Window) {
        S_Shell_ShowFatalError("System Error: cannot create window");
        return 1;
    }

    S_Shell_PostWindowResize();

    SDL_ShowCursor(SDL_DISABLE);

    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(m_Window, &wm_info);
    g_TombModule = wm_info.info.win.hinstance;
    g_TombHWND = wm_info.info.win.window;

    if (!g_TombHWND) {
        S_Shell_ShowFatalError("System Error: cannot create window");
        return 1;
    }

    g_GLRage = LoadLibrary("glrage.dll");
    if (!g_GLRage) {
        S_Shell_ShowFatalError("Cannot find glrage.dll");
        return false;
    }

    Shell_Main();

    S_Shell_TerminateGame(0);
    return 0;
}

void S_Shell_ExitSystem(const char *message)
{
    while (g_Input.select) {
        Input_Update();
    }
    GameBuf_Shutdown();
    HWR_Shutdown();
    S_Shell_ShowFatalError(message);
}

void S_Shell_ExitSystemFmt(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char message[150];
    vsnprintf(message, 150, fmt, va);
    va_end(va);
    S_Shell_ExitSystem(message);
}

bool S_Shell_GetCommandLine(int *arg_count, char ***args)
{
    *arg_count = m_ArgCount;
    *args = Memory_Alloc(m_ArgCount * sizeof(char *));
    for (int i = 0; i < m_ArgCount; i++) {
        (*args)[i] = Memory_Alloc(strlen(m_ArgStrings[i]) + 1);
        strcpy((*args)[i], m_ArgStrings[i]);
    }
    return true;
}

void *S_Shell_GetWindowHandle()
{
    return (void *)m_Window;
}
