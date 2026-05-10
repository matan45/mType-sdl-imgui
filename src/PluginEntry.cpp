#include "PluginGlobals.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

/* stb_image implementation lives here so exactly one TU compiles it.
 * Do NOT add STB_IMAGE_STATIC — SdlBindings.cpp also includes the header
 * (declarations only) and needs the symbols to be linker-visible. */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace sdlimgui
{
    const MTypePluginHost* g_host = nullptr;
    HandleRegistry<SDL_Window>   g_windows;
    HandleRegistry<SDL_Renderer> g_renderers;
    HandleRegistry<SDL_Texture>  g_textures;
    HandleRegistry<SDL_Gamepad>  g_gamepads;
    HandleRegistry<ImGuiContext> g_imguiContexts;
    HandleRegistry<ImFont>       g_fonts;
    unsigned char g_lastEventBuffer[SDL_EVENT_BUFFER_BYTES] = {};
}

extern "C" MTYPE_PLUGIN_EXPORT
int mtype_plugin_register(uint32_t hostAbiVersion,
                          const MTypePluginHost* host,
                          MTypeContext* registrationCtx)
{
    if (hostAbiVersion != MTYPE_PLUGIN_ABI_VERSION) {
        return 1;
    }
    sdlimgui::g_host = host;

    sdlimgui::registerSdlNatives(registrationCtx);
    sdlimgui::registerImGuiNatives(registrationCtx);
    return 0;
}
