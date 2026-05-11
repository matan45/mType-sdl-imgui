/*
 * Phase 1 SDL3 bindings (MYT-289 follow-up).
 *
 * Native names follow the mType "__module_op" convention. Handles (windows,
 * renderers) flow as int64 ids minted by HandleRegistry; native pointers
 * never cross the C ABI boundary.
 *
 * Phase 1 surface (15-ish):
 *   __sdl_init / __sdl_quit
 *   __sdl_create_window / __sdl_destroy_window
 *   __sdl_create_renderer / __sdl_destroy_renderer
 *   __sdl_set_render_draw_color
 *   __sdl_render_clear / __sdl_render_present
 *   __sdl_poll_event              -> int (event type, 0 if no pending event)
 *   __sdl_event_quit_id           -> int (constant for SDL_EVENT_QUIT)
 *   __sdl_get_error               -> string
 *   __sdl_delay                   -> void (millis)
 *   __sdl_event_ptr               -> int (raw &g_lastEventBuffer for ImGui_ImplSDL3_ProcessEvent)
 */

#include "PluginGlobals.hpp"
#include "BindingHelpers.hpp"

#include <SDL3/SDL.h>
#include "stb_image.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

namespace sdlimgui
{
    namespace
    {
        constexpr const char* kEx = "SdlError";

        inline bool requireArgs(MTypeContext* ctx, int argc, int expected, const char* name)
        {
            return detail::requireArgs(ctx, argc, expected, name, kEx);
        }
        inline const char* getStr(const MTypeValue* v, size_t* outLen = nullptr)
        {
            return detail::getStr(v, outLen);
        }

        /* ---------------------------------------------------------------- */

        MTypeValue* nSdlInit(void*, MTypeContext* ctx, const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__sdl_init")) return g_host->makeNull(ctx);
            int rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) ? 0 : 1;
            if (rc != 0) {
                std::string m = std::string("SDL_Init failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
            }
            return g_host->makeBool(ctx, rc == 0);
        }

        MTypeValue* nSdlQuit(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            SDL_Quit();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlCreateWindow(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sdl_create_window")) return g_host->makeInt(ctx, 0);
            const char* title = getStr(args[0]);
            int w = static_cast<int>(g_host->getInt(args[1]));
            int h = static_cast<int>(g_host->getInt(args[2]));
            SDL_Window* win = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);
            if (!win) {
                std::string m = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_windows.insert(win));
        }

        MTypeValue* nSdlDestroyWindow(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_destroy_window")) return g_host->makeVoid(ctx);
            SDL_Window* win = g_windows.erase(g_host->getInt(args[0]));
            if (win) SDL_DestroyWindow(win);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlCreateRenderer(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_create_renderer")) return g_host->makeInt(ctx, 0);
            SDL_Window* win = g_windows.find(g_host->getInt(args[0]));
            if (!win) {
                g_host->raiseError(ctx, "SdlError", "__native__sdl_create_renderer: invalid window id");
                return g_host->makeInt(ctx, 0);
            }
            SDL_Renderer* r = SDL_CreateRenderer(win, nullptr);
            if (!r) {
                std::string m = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_renderers.insert(r));
        }

        MTypeValue* nSdlDestroyRenderer(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_destroy_renderer")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.erase(g_host->getInt(args[0]));
            if (r) SDL_DestroyRenderer(r);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlSetRenderDrawColor(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sdl_set_render_draw_color")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            SDL_SetRenderDrawColor(r,
                                   static_cast<Uint8>(g_host->getInt(args[1])),
                                   static_cast<Uint8>(g_host->getInt(args[2])),
                                   static_cast<Uint8>(g_host->getInt(args[3])),
                                   static_cast<Uint8>(g_host->getInt(args[4])));
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlRenderClear(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_render_clear")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (r) SDL_RenderClear(r);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlRenderPresent(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_render_present")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (r) SDL_RenderPresent(r);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlPollEvent(void*, MTypeContext* ctx,
                                   const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__sdl_poll_event")) return g_host->makeInt(ctx, 0);
            static_assert(sizeof(SDL_Event) <= SDL_EVENT_BUFFER_BYTES,
                          "SDL_Event grew beyond the plugin's stash buffer; bump SDL_EVENT_BUFFER_BYTES");
            SDL_Event* ev = reinterpret_cast<SDL_Event*>(g_lastEventBuffer);
            std::memset(g_lastEventBuffer, 0, SDL_EVENT_BUFFER_BYTES);
            if (!SDL_PollEvent(ev)) {
                return g_host->makeInt(ctx, 0);  /* 0 == no event */
            }
            return g_host->makeInt(ctx, static_cast<int64_t>(ev->type));
        }

        MTypeValue* nSdlEventQuitId(void*, MTypeContext* ctx,
                                     const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_QUIT));
        }

        MTypeValue* nSdlEventPtr(void*, MTypeContext* ctx,
                                  const MTypeValue* const*, int)
        {
            /* Raw address of the stash event for ImGui_ImplSDL3_ProcessEvent. */
            return g_host->makeInt(ctx, reinterpret_cast<int64_t>(g_lastEventBuffer));
        }

        MTypeValue* nSdlGetError(void*, MTypeContext* ctx,
                                  const MTypeValue* const*, int)
        {
            const char* err = SDL_GetError();
            return g_host->makeString(ctx, err ? err : "", err ? std::strlen(err) : 0);
        }

        MTypeValue* nSdlDelay(void*, MTypeContext* ctx,
                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_delay")) return g_host->makeVoid(ctx);
            SDL_Delay(static_cast<Uint32>(g_host->getInt(args[0])));
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 2: event accessors. Read from g_lastEventBuffer (populated
         * by __native__sdl_poll_event). Type-specific accessors return 0
         * if the last event isn't of the expected type — caller should
         * gate on the matching event-type id constant first.
         * ---------------------------------------------------------------- */

        SDL_Event* lastEvent()
        {
            return reinterpret_cast<SDL_Event*>(g_lastEventBuffer);
        }

        /* Event type constants (resolved at runtime; SDL3 uses different
         * values than SDL2 so we don't hardcode in mType). */
        MTypeValue* nSdlEventMouseMotionId(void*, MTypeContext* ctx,
                                             const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_MOUSE_MOTION));
        }
        MTypeValue* nSdlEventMouseButtonDownId(void*, MTypeContext* ctx,
                                                 const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_MOUSE_BUTTON_DOWN));
        }
        MTypeValue* nSdlEventMouseButtonUpId(void*, MTypeContext* ctx,
                                               const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_MOUSE_BUTTON_UP));
        }
        MTypeValue* nSdlEventMouseWheelId(void*, MTypeContext* ctx,
                                            const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_MOUSE_WHEEL));
        }
        MTypeValue* nSdlEventKeyDownId(void*, MTypeContext* ctx,
                                         const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_KEY_DOWN));
        }
        MTypeValue* nSdlEventKeyUpId(void*, MTypeContext* ctx,
                                       const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_KEY_UP));
        }
        MTypeValue* nSdlEventTextInputId(void*, MTypeContext* ctx,
                                           const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_TEXT_INPUT));
        }

        /* Mouse motion / button x,y are floats in SDL3. */
        MTypeValue* nSdlEventMouseX(void*, MTypeContext* ctx,
                                      const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            switch (ev->type) {
            case SDL_EVENT_MOUSE_MOTION:       return g_host->makeFloat(ctx, ev->motion.x);
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:    return g_host->makeFloat(ctx, ev->button.x);
            default: return g_host->makeFloat(ctx, 0.0);
            }
        }
        MTypeValue* nSdlEventMouseY(void*, MTypeContext* ctx,
                                      const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            switch (ev->type) {
            case SDL_EVENT_MOUSE_MOTION:       return g_host->makeFloat(ctx, ev->motion.y);
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:    return g_host->makeFloat(ctx, ev->button.y);
            default: return g_host->makeFloat(ctx, 0.0);
            }
        }
        MTypeValue* nSdlEventMouseButton(void*, MTypeContext* ctx,
                                           const MTypeValue* const*, int)
        {
            /* 1=left, 2=middle, 3=right, 4=x1, 5=x2 (SDL_BUTTON_*). */
            const SDL_Event* ev = lastEvent();
            if (ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev->type == SDL_EVENT_MOUSE_BUTTON_UP) {
                return g_host->makeInt(ctx, static_cast<int64_t>(ev->button.button));
            }
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nSdlEventMouseClicks(void*, MTypeContext* ctx,
                                           const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            if (ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev->type == SDL_EVENT_MOUSE_BUTTON_UP) {
                return g_host->makeInt(ctx, static_cast<int64_t>(ev->button.clicks));
            }
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nSdlEventWheelY(void*, MTypeContext* ctx,
                                      const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            if (ev->type == SDL_EVENT_MOUSE_WHEEL) {
                return g_host->makeFloat(ctx, ev->wheel.y);
            }
            return g_host->makeFloat(ctx, 0.0);
        }

        /* Key events. scancode is layout-independent; key (keycode) is
         * layout-dependent (the character the key produces). */
        MTypeValue* nSdlEventKeyScancode(void*, MTypeContext* ctx,
                                           const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            if (ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP) {
                return g_host->makeInt(ctx, static_cast<int64_t>(ev->key.scancode));
            }
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nSdlEventKeyKeycode(void*, MTypeContext* ctx,
                                          const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            if (ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP) {
                return g_host->makeInt(ctx, static_cast<int64_t>(ev->key.key));
            }
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nSdlEventKeyMod(void*, MTypeContext* ctx,
                                      const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            if (ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP) {
                return g_host->makeInt(ctx, static_cast<int64_t>(ev->key.mod));
            }
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nSdlEventKeyRepeat(void*, MTypeContext* ctx,
                                         const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            if (ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP) {
                return g_host->makeBool(ctx, ev->key.repeat ? 1 : 0);
            }
            return g_host->makeBool(ctx, 0);
        }

        /* Text input. SDL3 owns the buffer; valid until next event poll. */
        MTypeValue* nSdlEventText(void*, MTypeContext* ctx,
                                    const MTypeValue* const*, int)
        {
            const SDL_Event* ev = lastEvent();
            if (ev->type == SDL_EVENT_TEXT_INPUT && ev->text.text) {
                return g_host->makeString(ctx, ev->text.text, std::strlen(ev->text.text));
            }
            return g_host->makeString(ctx, "", 0);
        }

        /* ----------------------------------------------------------------
         * Phase 4: textures. stb_image loads PNG/JPG/BMP/GIF/PSD/TGA into
         * RGBA8 pixels; SDL_CreateTexture + SDL_UpdateTexture wraps it as
         * a renderer texture. Returned handle is the registry id; pass to
         * ImGui.image() or destroy via __native__sdl_destroy_texture.
         * ---------------------------------------------------------------- */

        MTypeValue* nSdlLoadTexture(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sdl_load_texture")) {
                return g_host->makeInt(ctx, 0);
            }
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) {
                g_host->raiseError(ctx, "SdlError",
                                   "__native__sdl_load_texture: invalid renderer id");
                return g_host->makeInt(ctx, 0);
            }
            const char* path = getStr(args[1]);

            int w = 0, h = 0, channels = 0;
            stbi_uc* pixels = stbi_load(path, &w, &h, &channels, STBI_rgb_alpha);
            if (!pixels) {
                std::string m = std::string("__native__sdl_load_texture: stbi_load failed for '")
                              + path + "': " + (stbi_failure_reason() ? stbi_failure_reason() : "unknown");
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeInt(ctx, 0);
            }

            SDL_Texture* tex = SDL_CreateTexture(r,
                SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
            if (!tex) {
                stbi_image_free(pixels);
                std::string m = std::string("__native__sdl_load_texture: SDL_CreateTexture failed: ")
                              + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            SDL_UpdateTexture(tex, nullptr, pixels, w * 4);
            SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
            stbi_image_free(pixels);

            return g_host->makeInt(ctx, g_textures.insert(tex));
        }

        MTypeValue* nSdlDestroyTexture(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_destroy_texture")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Texture* tex = g_textures.erase(g_host->getInt(args[0]));
            if (tex) SDL_DestroyTexture(tex);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlTextureWidth(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_texture_width")) {
                return g_host->makeInt(ctx, 0);
            }
            SDL_Texture* tex = g_textures.find(g_host->getInt(args[0]));
            if (!tex) return g_host->makeInt(ctx, 0);
            float w = 0, h = 0;
            SDL_GetTextureSize(tex, &w, &h);
            return g_host->makeInt(ctx, static_cast<int64_t>(w));
        }
        MTypeValue* nSdlTextureHeight(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_texture_height")) {
                return g_host->makeInt(ctx, 0);
            }
            SDL_Texture* tex = g_textures.find(g_host->getInt(args[0]));
            if (!tex) return g_host->makeInt(ctx, 0);
            float w = 0, h = 0;
            SDL_GetTextureSize(tex, &w, &h);
            return g_host->makeInt(ctx, static_cast<int64_t>(h));
        }

        /* Helper for ImGuiBindings — takes texture id, returns the
         * SDL_Texture* (or nullptr). Exposed via PluginGlobals. */

        /* ----------------------------------------------------------------
         * Phase 5: audio (fire-and-forget WAV playback). SDL3's audio API
         * is centered around streams; this helper opens a default audio
         * device, decodes a WAV, queues it once, and lets the device drain
         * the buffer naturally. Lifetime: device + stream are intentionally
         * leaked at process exit — fine for a v1 fire-and-forget API. For
         * looping / mixing / mp3 / ogg, vendor SDL_mixer in a future phase.
         * ---------------------------------------------------------------- */

        SDL_AudioDeviceID g_audioDevice = 0;

        MTypeValue* nSdlInitAudio(void*, MTypeContext* ctx,
                                    const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__sdl_init_audio")) {
                return g_host->makeBool(ctx, 0);
            }
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
                std::string m = std::string("SDL_InitSubSystem(AUDIO) failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, 1);
        }

        MTypeValue* nSdlPlayWav(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_play_wav")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* path = getStr(args[0]);

            SDL_AudioSpec spec{};
            Uint8* buf = nullptr;
            Uint32 len = 0;
            if (!SDL_LoadWAV(path, &spec, &buf, &len)) {
                std::string m = std::string("__native__sdl_play_wav: SDL_LoadWAV failed: ")
                              + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeBool(ctx, 0);
            }

            /* OpenAudioDeviceStream creates both device + stream and starts
             * paused. Caller must SDL_ResumeAudioStreamDevice to play. */
            SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
                SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
            if (!stream) {
                SDL_free(buf);
                std::string m = std::string("__native__sdl_play_wav: OpenAudioDeviceStream failed: ")
                              + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeBool(ctx, 0);
            }
            SDL_PutAudioStreamData(stream, buf, static_cast<int>(len));
            SDL_FlushAudioStream(stream);
            SDL_ResumeAudioStreamDevice(stream);

            /* Stream + buf are intentionally leaked: SDL drains the queue
             * on its own thread. For repeated playback, build a richer API
             * in a future revision (or vendor SDL_mixer). */
            return g_host->makeBool(ctx, 1);
        }

        /* ----------------------------------------------------------------
         * Phase 5: gamepads. Polled API (you can also subscribe to the
         * SDL_EVENT_GAMEPAD_* events; constants below).
         * ---------------------------------------------------------------- */

        MTypeValue* nSdlInitGamepads(void*, MTypeContext* ctx,
                                       const MTypeValue* const*, int)
        {
            if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
                std::string m = std::string("SDL_InitSubSystem(GAMEPAD) failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, 1);
        }

        MTypeValue* nSdlGamepadCount(void*, MTypeContext* ctx,
                                       const MTypeValue* const*, int)
        {
            int count = 0;
            SDL_JoystickID* ids = SDL_GetGamepads(&count);
            if (ids) SDL_free(ids);
            return g_host->makeInt(ctx, static_cast<int64_t>(count));
        }

        MTypeValue* nSdlOpenGamepad(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_open_gamepad")) {
                return g_host->makeInt(ctx, 0);
            }
            int wantedIdx = static_cast<int>(g_host->getInt(args[0]));

            int count = 0;
            SDL_JoystickID* ids = SDL_GetGamepads(&count);
            if (!ids || wantedIdx < 0 || wantedIdx >= count) {
                if (ids) SDL_free(ids);
                g_host->raiseError(ctx, "SdlError",
                                   "__native__sdl_open_gamepad: index out of range");
                return g_host->makeInt(ctx, 0);
            }
            SDL_Gamepad* pad = SDL_OpenGamepad(ids[wantedIdx]);
            SDL_free(ids);
            if (!pad) {
                std::string m = std::string("SDL_OpenGamepad failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_gamepads.insert(pad));
        }

        MTypeValue* nSdlCloseGamepad(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_close_gamepad")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Gamepad* pad = g_gamepads.erase(g_host->getInt(args[0]));
            if (pad) SDL_CloseGamepad(pad);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlGamepadAxis(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sdl_gamepad_axis")) {
                return g_host->makeInt(ctx, 0);
            }
            SDL_Gamepad* pad = g_gamepads.find(g_host->getInt(args[0]));
            int axisId = static_cast<int>(g_host->getInt(args[1]));
            if (!pad) return g_host->makeInt(ctx, 0);
            return g_host->makeInt(ctx,
                static_cast<int64_t>(SDL_GetGamepadAxis(pad, static_cast<SDL_GamepadAxis>(axisId))));
        }

        MTypeValue* nSdlGamepadButton(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sdl_gamepad_button")) {
                return g_host->makeBool(ctx, 0);
            }
            SDL_Gamepad* pad = g_gamepads.find(g_host->getInt(args[0]));
            int btnId = static_cast<int>(g_host->getInt(args[1]));
            if (!pad) return g_host->makeBool(ctx, 0);
            return g_host->makeBool(ctx,
                SDL_GetGamepadButton(pad, static_cast<SDL_GamepadButton>(btnId)) ? 1 : 0);
        }

        MTypeValue* nSdlEventGamepadButtonDownId(void*, MTypeContext* ctx,
                                                    const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_GAMEPAD_BUTTON_DOWN));
        }
        MTypeValue* nSdlEventGamepadButtonUpId(void*, MTypeContext* ctx,
                                                  const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_GAMEPAD_BUTTON_UP));
        }
        MTypeValue* nSdlEventGamepadAxisMotionId(void*, MTypeContext* ctx,
                                                    const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_GAMEPAD_AXIS_MOTION));
        }

        /* ----------------------------------------------------------------
         * Phase 5: haptic (rumble only). SDL3's high-level rumble works on
         * any opened gamepad that supports it; the lower-level SDL_Haptic
         * API is deferred.
         * ---------------------------------------------------------------- */

        MTypeValue* nSdlRumbleGamepad(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__sdl_rumble_gamepad")) {
                return g_host->makeBool(ctx, 0);
            }
            SDL_Gamepad* pad = g_gamepads.find(g_host->getInt(args[0]));
            if (!pad) return g_host->makeBool(ctx, 0);
            int low  = static_cast<int>(g_host->getInt(args[1]));
            int high = static_cast<int>(g_host->getInt(args[2]));
            int dur  = static_cast<int>(g_host->getInt(args[3]));
            return g_host->makeBool(ctx,
                SDL_RumbleGamepad(pad,
                    static_cast<Uint16>(std::clamp(low,  0, 0xFFFF)),
                    static_cast<Uint16>(std::clamp(high, 0, 0xFFFF)),
                    static_cast<Uint32>(dur)) ? 1 : 0);
        }

        /* ----------------------------------------------------------------
         * Phase 7-C: realtime input, timing, window props, render primitives.
         * ---------------------------------------------------------------- */

        /* Mouse state: returns float[3] = [x, y, buttonsBitmask]. SDL's
         * button mask uses SDL_BUTTON_MASK(n) = 1u << (n-1): left=1,
         * middle=2, right=4 (note: SDL3 numbers left=1, middle=2, right=3
         * — different ordering than SDL2). */
        MTypeValue* nSdlGetMouseState(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            float x = 0.0f, y = 0.0f;
            SDL_MouseButtonFlags buttons = SDL_GetMouseState(&x, &y);
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 3);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, x));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, y));
            g_host->arraySet(out, 2, g_host->makeFloat(ctx, static_cast<double>(buttons)));
            return out;
        }

        MTypeValue* nSdlIsScancodeDown(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_is_scancode_down")) {
                return g_host->makeBool(ctx, 0);
            }
            int sc = static_cast<int>(g_host->getInt(args[0]));
            int numkeys = 0;
            const bool* state = SDL_GetKeyboardState(&numkeys);
            if (!state || sc < 0 || sc >= numkeys) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, state[sc] ? 1 : 0);
        }

        /* Timing. SDL3 uses Uint64 for both ticks (ms since SDL_Init) and
         * the perf counter; we cast to int64 (mType has no uint64). */
        MTypeValue* nSdlGetTicks(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_GetTicks()));
        }
        MTypeValue* nSdlGetPerformanceCounter(void*, MTypeContext* ctx,
                                                const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_GetPerformanceCounter()));
        }
        MTypeValue* nSdlGetPerformanceFrequency(void*, MTypeContext* ctx,
                                                  const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_GetPerformanceFrequency()));
        }

        /* Window props. */
        MTypeValue* nSdlGetWindowSize(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_get_window_size")) {
                return g_host->makeNull(ctx);
            }
            SDL_Window* w = g_windows.find(g_host->getInt(args[0]));
            int ww = 0, hh = 0;
            if (w) SDL_GetWindowSize(w, &ww, &hh);
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_INT, 2);
            g_host->arraySet(out, 0, g_host->makeInt(ctx, ww));
            g_host->arraySet(out, 1, g_host->makeInt(ctx, hh));
            return out;
        }
        MTypeValue* nSdlSetWindowSize(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sdl_set_window_size")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Window* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) return g_host->makeVoid(ctx);
            SDL_SetWindowSize(w,
                static_cast<int>(g_host->getInt(args[1])),
                static_cast<int>(g_host->getInt(args[2])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSdlSetWindowTitle(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sdl_set_window_title")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Window* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) return g_host->makeVoid(ctx);
            SDL_SetWindowTitle(w, getStr(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSdlSetWindowFullscreen(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sdl_set_window_fullscreen")) {
                return g_host->makeBool(ctx, 0);
            }
            SDL_Window* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) return g_host->makeBool(ctx, 0);
            bool full = g_host->getBool(args[1]) != 0;
            return g_host->makeBool(ctx, SDL_SetWindowFullscreen(w, full) ? 1 : 0);
        }
        MTypeValue* nSdlShowCursor(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            SDL_ShowCursor();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSdlHideCursor(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            SDL_HideCursor();
            return g_host->makeVoid(ctx);
        }

        /* Renderer primitives. All coordinates are floats in SDL3. */
        MTypeValue* nSdlRenderLine(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sdl_render_line")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            SDL_RenderLine(r,
                static_cast<float>(g_host->getFloat(args[1])),
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
                static_cast<float>(g_host->getFloat(args[4])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSdlRenderRect(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sdl_render_rect")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            SDL_FRect rc {
                static_cast<float>(g_host->getFloat(args[1])),
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
                static_cast<float>(g_host->getFloat(args[4])),
            };
            SDL_RenderRect(r, &rc);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSdlRenderFillRect(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sdl_render_fill_rect")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            SDL_FRect rc {
                static_cast<float>(g_host->getFloat(args[1])),
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
                static_cast<float>(g_host->getFloat(args[4])),
            };
            SDL_RenderFillRect(r, &rc);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSdlRenderPoint(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sdl_render_point")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            SDL_RenderPoint(r,
                static_cast<float>(g_host->getFloat(args[1])),
                static_cast<float>(g_host->getFloat(args[2])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSdlRenderTexture(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 6, "__native__sdl_render_texture")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            SDL_Texture* t  = g_textures.find(g_host->getInt(args[1]));
            if (!r || !t) return g_host->makeVoid(ctx);
            SDL_FRect dst {
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
                static_cast<float>(g_host->getFloat(args[4])),
                static_cast<float>(g_host->getFloat(args[5])),
            };
            SDL_RenderTexture(r, t, nullptr, &dst);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSdlRenderTextureRotated(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 10, "__native__sdl_render_texture_rotated")) {
                return g_host->makeVoid(ctx);
            }
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            SDL_Texture* t  = g_textures.find(g_host->getInt(args[1]));
            if (!r || !t) return g_host->makeVoid(ctx);
            SDL_FRect dst {
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
                static_cast<float>(g_host->getFloat(args[4])),
                static_cast<float>(g_host->getFloat(args[5])),
            };
            double angle = g_host->getFloat(args[6]);
            SDL_FPoint center {
                static_cast<float>(g_host->getFloat(args[7])),
                static_cast<float>(g_host->getFloat(args[8])),
            };
            int flipFlags = static_cast<int>(g_host->getInt(args[9]));
            SDL_FlipMode flip = static_cast<SDL_FlipMode>(flipFlags & 3);
            SDL_RenderTextureRotated(r, t, nullptr, &dst, angle, &center, flip);
            return g_host->makeVoid(ctx);
        }
    }

    void registerSdlNatives(MTypeContext* ctx)
    {
        const auto reg = [&](const char* name, MTypeNativeFn fn) {
            g_host->registerFunction(ctx, name, fn, nullptr);
        };
        reg("__native__sdl_init",                  &nSdlInit);
        reg("__native__sdl_quit",                  &nSdlQuit);
        reg("__native__sdl_create_window",         &nSdlCreateWindow);
        reg("__native__sdl_destroy_window",        &nSdlDestroyWindow);
        reg("__native__sdl_create_renderer",       &nSdlCreateRenderer);
        reg("__native__sdl_destroy_renderer",      &nSdlDestroyRenderer);
        reg("__native__sdl_set_render_draw_color", &nSdlSetRenderDrawColor);
        reg("__native__sdl_render_clear",          &nSdlRenderClear);
        reg("__native__sdl_render_present",        &nSdlRenderPresent);
        reg("__native__sdl_poll_event",            &nSdlPollEvent);
        reg("__native__sdl_event_quit_id",         &nSdlEventQuitId);
        reg("__native__sdl_event_ptr",             &nSdlEventPtr);
        reg("__native__sdl_get_error",             &nSdlGetError);
        reg("__native__sdl_delay",                 &nSdlDelay);

        /* Phase 2 — event-type id constants */
        reg("__native__sdl_event_mouse_motion_id",    &nSdlEventMouseMotionId);
        reg("__native__sdl_event_mouse_button_down_id", &nSdlEventMouseButtonDownId);
        reg("__native__sdl_event_mouse_button_up_id", &nSdlEventMouseButtonUpId);
        reg("__native__sdl_event_mouse_wheel_id",     &nSdlEventMouseWheelId);
        reg("__native__sdl_event_key_down_id",        &nSdlEventKeyDownId);
        reg("__native__sdl_event_key_up_id",          &nSdlEventKeyUpId);
        reg("__native__sdl_event_text_input_id",      &nSdlEventTextInputId);

        /* Phase 2 — payload accessors */
        reg("__native__sdl_event_mouse_x",         &nSdlEventMouseX);
        reg("__native__sdl_event_mouse_y",         &nSdlEventMouseY);
        reg("__native__sdl_event_mouse_button",    &nSdlEventMouseButton);
        reg("__native__sdl_event_mouse_clicks",    &nSdlEventMouseClicks);
        reg("__native__sdl_event_wheel_y",         &nSdlEventWheelY);
        reg("__native__sdl_event_key_scancode",    &nSdlEventKeyScancode);
        reg("__native__sdl_event_key_keycode",     &nSdlEventKeyKeycode);
        reg("__native__sdl_event_key_mod",         &nSdlEventKeyMod);
        reg("__native__sdl_event_key_repeat",      &nSdlEventKeyRepeat);
        reg("__native__sdl_event_text",            &nSdlEventText);

        /* Phase 4 — textures */
        reg("__native__sdl_load_texture",          &nSdlLoadTexture);
        reg("__native__sdl_destroy_texture",       &nSdlDestroyTexture);
        reg("__native__sdl_texture_width",         &nSdlTextureWidth);
        reg("__native__sdl_texture_height",        &nSdlTextureHeight);

        /* Phase 5 — audio */
        reg("__native__sdl_init_audio",            &nSdlInitAudio);
        reg("__native__sdl_play_wav",              &nSdlPlayWav);

        /* Phase 5 — gamepad */
        reg("__native__sdl_init_gamepads",         &nSdlInitGamepads);
        reg("__native__sdl_gamepad_count",         &nSdlGamepadCount);
        reg("__native__sdl_open_gamepad",          &nSdlOpenGamepad);
        reg("__native__sdl_close_gamepad",         &nSdlCloseGamepad);
        reg("__native__sdl_gamepad_axis",          &nSdlGamepadAxis);
        reg("__native__sdl_gamepad_button",        &nSdlGamepadButton);
        reg("__native__sdl_event_gamepad_button_down_id", &nSdlEventGamepadButtonDownId);
        reg("__native__sdl_event_gamepad_button_up_id",   &nSdlEventGamepadButtonUpId);
        reg("__native__sdl_event_gamepad_axis_motion_id", &nSdlEventGamepadAxisMotionId);

        /* Phase 5 — haptic */
        reg("__native__sdl_rumble_gamepad",        &nSdlRumbleGamepad);

        /* Phase 7-C — realtime input */
        reg("__native__sdl_get_mouse_state",       &nSdlGetMouseState);
        reg("__native__sdl_is_scancode_down",      &nSdlIsScancodeDown);

        /* Phase 7-C — timing */
        reg("__native__sdl_get_ticks",                 &nSdlGetTicks);
        reg("__native__sdl_get_performance_counter",   &nSdlGetPerformanceCounter);
        reg("__native__sdl_get_performance_frequency", &nSdlGetPerformanceFrequency);

        /* Phase 7-C — window props */
        reg("__native__sdl_get_window_size",       &nSdlGetWindowSize);
        reg("__native__sdl_set_window_size",       &nSdlSetWindowSize);
        reg("__native__sdl_set_window_title",      &nSdlSetWindowTitle);
        reg("__native__sdl_set_window_fullscreen", &nSdlSetWindowFullscreen);
        reg("__native__sdl_show_cursor",           &nSdlShowCursor);
        reg("__native__sdl_hide_cursor",           &nSdlHideCursor);

        /* Phase 7-C — renderer primitives */
        reg("__native__sdl_render_line",            &nSdlRenderLine);
        reg("__native__sdl_render_rect",            &nSdlRenderRect);
        reg("__native__sdl_render_fill_rect",       &nSdlRenderFillRect);
        reg("__native__sdl_render_point",           &nSdlRenderPoint);
        reg("__native__sdl_render_texture",         &nSdlRenderTexture);
        reg("__native__sdl_render_texture_rotated", &nSdlRenderTextureRotated);
    }
}
