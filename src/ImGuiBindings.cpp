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

        /* ----------------------------------------------------------------
         * Phase 3: Tables. Preferred over the deprecated Columns API.
         * Pattern:
         *   if (beginTable("id", 3)) {
         *     setupColumn("a"); setupColumn("b"); setupColumn("c");
         *     headersRow();
         *     for each row { nextRow(); nextColumn(); text(...); ... }
         *     endTable();
         *   }
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiBeginTable(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_begin_table")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* id = getStr(args[0]);
            int cols = static_cast<int>(g_host->getInt(args[1]));
            ImGuiTableFlags flags = ImGuiTableFlags_Borders
                                  | ImGuiTableFlags_RowBg
                                  | ImGuiTableFlags_Resizable;
            return g_host->makeBool(ctx, ImGui::BeginTable(id, cols, flags) ? 1 : 0);
        }
        MTypeValue* nImGuiEndTable(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndTable();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTableSetupColumn(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_table_setup_column")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::TableSetupColumn(getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTableHeadersRow(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::TableHeadersRow();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTableNextRow(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::TableNextRow();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTableNextColumn(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::TableNextColumn() ? 1 : 0);
        }
        MTypeValue* nImGuiTableSetColumnIndex(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_table_set_column_index")) {
                return g_host->makeBool(ctx, 0);
            }
            int idx = static_cast<int>(g_host->getInt(args[0]));
            return g_host->makeBool(ctx, ImGui::TableSetColumnIndex(idx) ? 1 : 0);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Child windows.
         * Pattern:
         *   if (beginChild("id", w, h, true)) {  // last arg = border
         *     // ...content...
         *   }
         *   endChild();   // ALWAYS call, even if begin returned false.
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiBeginChild(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_begin_child")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* id = getStr(args[0]);
            ImVec2 size(static_cast<float>(g_host->getFloat(args[1])),
                        static_cast<float>(g_host->getFloat(args[2])));
            bool border = g_host->getBool(args[3]) != 0;
            ImGuiChildFlags childFlags = border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;
            return g_host->makeBool(ctx, ImGui::BeginChild(id, size, childFlags) ? 1 : 0);
        }
        MTypeValue* nImGuiEndChild(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndChild();
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Tab bars.
         * Pattern:
         *   if (beginTabBar("##bar")) {
         *     if (beginTabItem("one")) { … endTabItem(); }
         *     if (beginTabItem("two")) { … endTabItem(); }
         *     endTabBar();
         *   }
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiBeginTabBar(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_tab_bar")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::BeginTabBar(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiEndTabBar(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndTabBar();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiBeginTabItem(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_tab_item")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::BeginTabItem(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiEndTabItem(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndTabItem();
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Docking. Requires ImGuiConfigFlags_DockingEnable on the
         * IO before any NewFrame; the engine-side wrapper exposes
         * enableDocking() to set it once at startup.
         *
         * dockSpaceOverViewport() is the easy mode: makes the entire
         * platform window a dockspace. Subsequent regular Begin() windows
         * automatically become dockable into it.
         *
         * dockSpace(id, w, h) spawns a dockspace inside an existing window
         * (use after a normal Begin()).
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiEnableDocking(void*, MTypeContext* ctx,
                                          const MTypeValue* const*, int)
        {
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiDockSpaceOverViewport(void*, MTypeContext* ctx,
                                                  const MTypeValue* const*, int)
        {
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiDockSpace(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_dock_space")) {
                return g_host->makeVoid(ctx);
            }
            const char* id = getStr(args[0]);
            ImVec2 size(static_cast<float>(g_host->getFloat(args[1])),
                        static_cast<float>(g_host->getFloat(args[2])));
            ImGui::DockSpace(ImGui::GetID(id), size);
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Splitter (hand-rolled — no imgui_internal dependency).
         *
         * Creates an InvisibleButton sized to (thickness × full available
         * height) for vertical splits, or (full width × thickness) for
         * horizontal. While dragged, redistributes the delta between
         * size1 and size2, clamping at min1/min2.
         *
         * Returns float[2] = [newSize1, newSize2]. The bool change-flag
         * is also written to g_lastWidgetChanged so widgetChanged() works
         * for splitters too.
         *
         * Caller pattern:
         *   float[] sz = ImGui.splitter(true, 4.0, leftW, rightW, 100.0, 100.0);
         *   leftW = sz[0]; rightW = sz[1];
         *   if (ImGui.beginChild("##L", leftW, 0.0, false)) { ... } ImGui.endChild();
         *   ImGui.sameLine();
         *   if (ImGui.beginChild("##R", rightW, 0.0, false)) { ... } ImGui.endChild();
         * ---------------------------------------------------------------- */

        /* ----------------------------------------------------------------
         * Phase 4: textures (Image), clipboard, fonts.
         * ---------------------------------------------------------------- */

        /* ImGui::Image takes an ImTextureID. With the SDL3 renderer
         * backend, ImTextureID is the SDL_Texture* (cast as needed).
         * Caller passes the texture id minted by __native__sdl_load_texture. */
        MTypeValue* nImGuiImage(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_image")) {
                return g_host->makeVoid(ctx);
            }
            int64_t texId = g_host->getInt(args[0]);
            SDL_Texture* tex = g_textures.find(texId);
            if (!tex) return g_host->makeVoid(ctx);
            ImVec2 size(static_cast<float>(g_host->getFloat(args[1])),
                        static_cast<float>(g_host->getFloat(args[2])));
            ImGui::Image(reinterpret_cast<ImTextureID>(tex), size);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSetClipboardText(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_set_clipboard_text")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::SetClipboardText(getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiGetClipboardText(void*, MTypeContext* ctx,
                                             const MTypeValue* const*, int)
        {
            const char* s = ImGui::GetClipboardText();
            if (!s) return g_host->makeString(ctx, "", 0);
            return g_host->makeString(ctx, s, std::strlen(s));
        }

        /* Fonts. AddFontFromFileTTF must be called BEFORE the first
         * NewFrame for the font to be available; the SDL3 renderer
         * backend rebuilds its font texture lazily on the next frame.
         * Returns the font handle (or 0 on failure). */
        MTypeValue* nImGuiAddFontFromFile(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_add_font_from_file")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* path = getStr(args[0]);
            float size = static_cast<float>(g_host->getFloat(args[1]));
            ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path, size);
            if (!font) {
                std::string m = std::string("__native__imgui_add_font_from_file: failed to load '")
                              + path + "'";
                g_host->raiseError(ctx, "ImGuiError", m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_fonts.insert(font));
        }
        MTypeValue* nImGuiPushFont(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_push_font")) {
                return g_host->makeVoid(ctx);
            }
            ImFont* font = g_fonts.find(g_host->getInt(args[0]));
            if (font) ImGui::PushFont(font);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiPopFont(void*, MTypeContext* ctx,
                                    const MTypeValue* const*, int)
        {
            ImGui::PopFont();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSplitter(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 6, "__native__imgui_splitter")) {
                return g_host->makeNull(ctx);
            }
            bool vertical = g_host->getBool(args[0]) != 0;
            float thickness = static_cast<float>(g_host->getFloat(args[1]));
            float size1 = static_cast<float>(g_host->getFloat(args[2]));
            float size2 = static_cast<float>(g_host->getFloat(args[3]));
            float min1  = static_cast<float>(g_host->getFloat(args[4]));
            float min2  = static_cast<float>(g_host->getFloat(args[5]));

            ImVec2 btnSize = vertical ? ImVec2(thickness, ImGui::GetContentRegionAvail().y)
                                       : ImVec2(ImGui::GetContentRegionAvail().x, thickness);

            /* Style the invisible button to look like Separator. */
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImGui::GetStyleColorVec4(ImGuiCol_Separator));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImGui::GetStyleColorVec4(ImGuiCol_SeparatorHovered));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                ImGui::GetStyleColorVec4(ImGuiCol_SeparatorActive));
            ImGui::Button(vertical ? "##vsplitter" : "##hsplitter", btnSize);
            ImGui::PopStyleColor(3);

            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(vertical ? ImGuiMouseCursor_ResizeEW
                                                : ImGuiMouseCursor_ResizeNS);
            }

            bool changed = false;
            if (ImGui::IsItemActive()) {
                float delta = vertical ? ImGui::GetIO().MouseDelta.x
                                        : ImGui::GetIO().MouseDelta.y;
                if (delta != 0.0f) {
                    float ns1 = std::max(size1 + delta, min1);
                    float ns2 = std::max(size2 - delta, min2);
                    /* Only commit if both clamps were honored. */
                    if (ns1 + ns2 == size1 + size2) {
                        size1 = ns1;
                        size2 = ns2;
                        changed = true;
                    }
                }
            }
            g_lastWidgetChanged = changed;

            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 2);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, size1));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, size2));
            return out;
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

        /* Phase 3 — Tables */
        reg("__native__imgui_begin_table",             &nImGuiBeginTable);
        reg("__native__imgui_end_table",               &nImGuiEndTable);
        reg("__native__imgui_table_setup_column",      &nImGuiTableSetupColumn);
        reg("__native__imgui_table_headers_row",       &nImGuiTableHeadersRow);
        reg("__native__imgui_table_next_row",          &nImGuiTableNextRow);
        reg("__native__imgui_table_next_column",       &nImGuiTableNextColumn);
        reg("__native__imgui_table_set_column_index",  &nImGuiTableSetColumnIndex);

        /* Phase 3 — Child windows */
        reg("__native__imgui_begin_child",             &nImGuiBeginChild);
        reg("__native__imgui_end_child",               &nImGuiEndChild);

        /* Phase 3 — Tab bars */
        reg("__native__imgui_begin_tab_bar",           &nImGuiBeginTabBar);
        reg("__native__imgui_end_tab_bar",             &nImGuiEndTabBar);
        reg("__native__imgui_begin_tab_item",          &nImGuiBeginTabItem);
        reg("__native__imgui_end_tab_item",            &nImGuiEndTabItem);

        /* Phase 3 — Docking */
        reg("__native__imgui_enable_docking",          &nImGuiEnableDocking);
        reg("__native__imgui_dock_space_over_viewport", &nImGuiDockSpaceOverViewport);
        reg("__native__imgui_dock_space",              &nImGuiDockSpace);

        /* Phase 3 — Splitter */
        reg("__native__imgui_splitter",                &nImGuiSplitter);

        /* Phase 4 — image, clipboard, fonts */
        reg("__native__imgui_image",                   &nImGuiImage);
        reg("__native__imgui_set_clipboard_text",      &nImGuiSetClipboardText);
        reg("__native__imgui_get_clipboard_text",      &nImGuiGetClipboardText);
        reg("__native__imgui_add_font_from_file",      &nImGuiAddFontFromFile);
        reg("__native__imgui_push_font",               &nImGuiPushFont);
        reg("__native__imgui_pop_font",                &nImGuiPopFont);
    }
}
