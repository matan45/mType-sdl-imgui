/*
 * Phase 1 ImGui (docking branch) bindings.
 *
 * Phase 1 surface (~10):
 *   __imgui_create_context / __imgui_destroy_context
 *   __imgui_sdl3_init_for_renderer
 *   __imgui_sdl3_renderer_init
 *   __imgui_sdl3_process_event_ptr
 *   __imgui_sdl3_shutdown_renderer / __imgui_sdl3_shutdown
 *   __imgui_new_frame
 *   __imgui_render
 *   __imgui_begin / __imgui_end
 *   __imgui_text
 *   __imgui_button -> bool
 */

#include "PluginGlobals.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace sdlimgui
{
    namespace
    {
        bool requireArgs(MTypeContext* ctx, int argc, int expected, const char* name)
        {
            if (argc != expected) {
                std::string m = std::string(name) + ": expected " + std::to_string(expected)
                              + " args, got " + std::to_string(argc);
                g_host->raiseError(ctx, "ImGuiError", m.c_str());
                return false;
            }
            return true;
        }

        const char* getStr(const MTypeValue* v, size_t* outLen = nullptr)
        {
            if (g_host->getTag(v) != MT_TAG_STRING) {
                if (outLen) *outLen = 0;
                return "";
            }
            return g_host->getString(v, outLen);
        }

        /* ---------------------------------------------------------------- */

        MTypeValue* nImGuiCreateContext(void*, MTypeContext* ctx,
                                         const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__imgui_create_context")) return g_host->makeInt(ctx, 0);
            ImGuiContext* c = ImGui::CreateContext();
            if (!c) {
                g_host->raiseError(ctx, "ImGuiError", "ImGui::CreateContext returned null");
                return g_host->makeInt(ctx, 0);
            }
            ImGui::SetCurrentContext(c);
            return g_host->makeInt(ctx, g_imguiContexts.insert(c));
        }

        MTypeValue* nImGuiDestroyContext(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_destroy_context")) return g_host->makeVoid(ctx);
            ImGuiContext* c = g_imguiContexts.erase(g_host->getInt(args[0]));
            if (c) ImGui::DestroyContext(c);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSdl3InitForRenderer(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_sdl3_init_for_renderer")) return g_host->makeBool(ctx, 0);
            SDL_Window*   w = g_windows.find(g_host->getInt(args[0]));
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[1]));
            if (!w || !r) {
                g_host->raiseError(ctx, "ImGuiError",
                                   "__native__imgui_sdl3_init_for_renderer: invalid window or renderer id");
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui_ImplSDL3_InitForSDLRenderer(w, r) ? 1 : 0);
        }

        MTypeValue* nImGuiSdl3RendererInit(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_sdl3_renderer_init")) return g_host->makeBool(ctx, 0);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) {
                g_host->raiseError(ctx, "ImGuiError",
                                   "__native__imgui_sdl3_renderer_init: invalid renderer id");
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui_ImplSDLRenderer3_Init(r) ? 1 : 0);
        }

        MTypeValue* nImGuiSdl3ShutdownRenderer(void*, MTypeContext* ctx,
                                                const MTypeValue* const*, int)
        {
            ImGui_ImplSDLRenderer3_Shutdown();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSdl3Shutdown(void*, MTypeContext* ctx,
                                        const MTypeValue* const*, int)
        {
            ImGui_ImplSDL3_Shutdown();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSdl3ProcessEventPtr(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_sdl3_process_event_ptr")) return g_host->makeVoid(ctx);
            int64_t addr = g_host->getInt(args[0]);
            const SDL_Event* ev = reinterpret_cast<const SDL_Event*>(addr);
            ImGui_ImplSDL3_ProcessEvent(ev);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiNewFrame(void*, MTypeContext* ctx,
                                    const MTypeValue* const*, int)
        {
            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiRender(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_render")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            ImGui::Render();
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), r);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiBegin(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin")) return g_host->makeBool(ctx, 0);
            const char* name = getStr(args[0]);
            return g_host->makeBool(ctx, ImGui::Begin(name) ? 1 : 0);
        }

        MTypeValue* nImGuiEnd(void*, MTypeContext* ctx,
                               const MTypeValue* const*, int)
        {
            ImGui::End();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiText(void*, MTypeContext* ctx,
                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_text")) return g_host->makeVoid(ctx);
            const char* s = getStr(args[0]);
            ImGui::TextUnformatted(s);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiButton(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_button")) return g_host->makeBool(ctx, 0);
            const char* label = getStr(args[0]);
            return g_host->makeBool(ctx, ImGui::Button(label) ? 1 : 0);
        }

        /* ----------------------------------------------------------------
         * Phase 2: input widgets. The C ABI can't pass C++ references, so
         * the in/out pattern from native ImGui is folded into:
         *
         *   in:  the current value (caller's copy)
         *   out: the new value (== current if widget did not mutate)
         *
         * Whether the widget actually changed this frame is recorded in
         * g_lastWidgetChanged and surfaced via __native__imgui_widget_changed.
         * Both pieces are needed: returned-value comparison can't tell when
         * a slider was dragged back to its starting value within one frame.
         * ---------------------------------------------------------------- */

        bool g_lastWidgetChanged = false;

        MTypeValue* nImGuiWidgetChanged(void*, MTypeContext* ctx,
                                          const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, g_lastWidgetChanged ? 1 : 0);
        }

        MTypeValue* nImGuiSliderFloat(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_slider_float")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            const char* label = getStr(args[0]);
            float v = static_cast<float>(g_host->getFloat(args[1]));
            float lo = static_cast<float>(g_host->getFloat(args[2]));
            float hi = static_cast<float>(g_host->getFloat(args[3]));
            g_lastWidgetChanged = ImGui::SliderFloat(label, &v, lo, hi);
            return g_host->makeFloat(ctx, static_cast<double>(v));
        }

        MTypeValue* nImGuiSliderInt(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_slider_int")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* label = getStr(args[0]);
            int v  = static_cast<int>(g_host->getInt(args[1]));
            int lo = static_cast<int>(g_host->getInt(args[2]));
            int hi = static_cast<int>(g_host->getInt(args[3]));
            g_lastWidgetChanged = ImGui::SliderInt(label, &v, lo, hi);
            return g_host->makeInt(ctx, static_cast<int64_t>(v));
        }

        MTypeValue* nImGuiCheckbox(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_checkbox")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* label = getStr(args[0]);
            bool v = g_host->getBool(args[1]) != 0;
            g_lastWidgetChanged = ImGui::Checkbox(label, &v);
            return g_host->makeBool(ctx, v ? 1 : 0);
        }

        /* Combo accepts items as a string[] mType array. The plugin builds
         * a single \\0-separated buffer that ImGui's Combo() expects.
         * Returns the new selected index. */
        MTypeValue* nImGuiCombo(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_combo")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* label = getStr(args[0]);
            int currentIdx = static_cast<int>(g_host->getInt(args[1]));
            const MTypeValue* itemsArr = args[2];

            std::string buf;
            int count = 0;
            if (g_host->getTag(itemsArr) == MT_TAG_ARRAY) {
                size_t n = g_host->arrayLen(itemsArr);
                count = static_cast<int>(n);
                for (size_t i = 0; i < n; ++i) {
                    MTypeValue* el = g_host->arrayGet(ctx, itemsArr, i);
                    size_t slen = 0;
                    const char* s = g_host->getString(el, &slen);
                    buf.append(s, slen);
                    buf.push_back('\0');
                }
                buf.push_back('\0');  /* second null terminates the items list */
            }
            g_lastWidgetChanged = ImGui::Combo(label, &currentIdx,
                                               count > 0 ? buf.c_str() : "\0\0",
                                               count);
            return g_host->makeInt(ctx, static_cast<int64_t>(currentIdx));
        }

        /* InputText. Plugin sizes its temp buffer to max(256, capCap) so
         * malicious capCap can't blow heap. Returns the modified string. */
        MTypeValue* nImGuiInputText(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_input_text")) {
                return g_host->makeString(ctx, "", 0);
            }
            const char* label = getStr(args[0]);
            size_t curLen = 0;
            const char* curStr = getStr(args[1], &curLen);
            int requestedCap = static_cast<int>(g_host->getInt(args[2]));

            constexpr size_t kMinCap = 256;
            constexpr size_t kMaxCap = 1 << 16;  /* 64 KiB hard ceiling */
            size_t cap = static_cast<size_t>(std::max(0, requestedCap));
            if (cap < kMinCap) cap = kMinCap;
            if (cap > kMaxCap) cap = kMaxCap;

            std::vector<char> buf(cap, 0);
            size_t copy = std::min(curLen, cap - 1);
            std::memcpy(buf.data(), curStr, copy);
            buf[copy] = '\0';

            g_lastWidgetChanged = ImGui::InputText(label, buf.data(), cap);
            return g_host->makeString(ctx, buf.data(), std::strlen(buf.data()));
        }

        /* ColorEdit3. Returns a 3-element float[] array (r,g,b in [0,1]). */
        MTypeValue* nImGuiColorEdit3(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_color_edit3")) {
                return g_host->makeNull(ctx);
            }
            const char* label = getStr(args[0]);
            float c[3] = {
                static_cast<float>(g_host->getFloat(args[1])),
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
            };
            g_lastWidgetChanged = ImGui::ColorEdit3(label, c);

            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 3);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, c[0]));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, c[1]));
            g_host->arraySet(out, 2, g_host->makeFloat(ctx, c[2]));
            return out;
        }

        /* Layout / formatting helpers. All void-returning. */
        MTypeValue* nImGuiSameLine(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::SameLine();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiSeparator(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::Separator();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiSpacing(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::Spacing();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiBulletText(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_bullet_text")) {
                return g_host->makeVoid(ctx);
            }
            const char* s = getStr(args[0]);
            ImGui::BulletText("%s", s);
            return g_host->makeVoid(ctx);
        }
    }

    void registerImGuiNatives(MTypeContext* ctx)
    {
        const auto reg = [&](const char* name, MTypeNativeFn fn) {
            g_host->registerFunction(ctx, name, fn, nullptr);
        };
        reg("__native__imgui_create_context",          &nImGuiCreateContext);
        reg("__native__imgui_destroy_context",         &nImGuiDestroyContext);
        reg("__native__imgui_sdl3_init_for_renderer",  &nImGuiSdl3InitForRenderer);
        reg("__native__imgui_sdl3_renderer_init",      &nImGuiSdl3RendererInit);
        reg("__native__imgui_sdl3_shutdown_renderer",  &nImGuiSdl3ShutdownRenderer);
        reg("__native__imgui_sdl3_shutdown",           &nImGuiSdl3Shutdown);
        reg("__native__imgui_sdl3_process_event_ptr",  &nImGuiSdl3ProcessEventPtr);
        reg("__native__imgui_new_frame",               &nImGuiNewFrame);
        reg("__native__imgui_render",                  &nImGuiRender);
        reg("__native__imgui_begin",                   &nImGuiBegin);
        reg("__native__imgui_end",                     &nImGuiEnd);
        reg("__native__imgui_text",                    &nImGuiText);
        reg("__native__imgui_button",                  &nImGuiButton);

        /* Phase 2 — input widgets */
        reg("__native__imgui_widget_changed",          &nImGuiWidgetChanged);
        reg("__native__imgui_slider_float",            &nImGuiSliderFloat);
        reg("__native__imgui_slider_int",              &nImGuiSliderInt);
        reg("__native__imgui_checkbox",                &nImGuiCheckbox);
        reg("__native__imgui_combo",                   &nImGuiCombo);
        reg("__native__imgui_input_text",              &nImGuiInputText);
        reg("__native__imgui_color_edit3",             &nImGuiColorEdit3);

        /* Phase 2 — layout helpers */
        reg("__native__imgui_same_line",               &nImGuiSameLine);
        reg("__native__imgui_separator",               &nImGuiSeparator);
        reg("__native__imgui_spacing",                 &nImGuiSpacing);
        reg("__native__imgui_bullet_text",             &nImGuiBulletText);
    }
}
