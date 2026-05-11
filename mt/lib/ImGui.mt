// Phase 1 mType wrapper around the __native__imgui_* natives exposed by
// mtype-sdl-imgui-plugin. Tracks one active ImGui context as an int
// handle. The SDL3 backends are stateful singletons inside ImGui itself,
// so initSdl3ForRenderer / initSdl3Renderer don't return handles.

import * from "Sdl.mt";

class ImGuiContext {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function destroy(): void {
        __native__imgui_destroy_context(this.handle);
    }
}

class ImGui {
    public static function createContext(): ImGuiContext {
        return new ImGuiContext(__native__imgui_create_context());
    }

    // Bind ImGui's SDL3 platform + renderer backends to the supplied
    // window/renderer. Call createContext() first.
    public static function initSdl3ForRenderer(Window window, Renderer renderer): bool {
        return __native__imgui_sdl3_init_for_renderer(window.handle, renderer.handle);
    }

    public static function initSdl3Renderer(Renderer renderer): bool {
        return __native__imgui_sdl3_renderer_init(renderer.handle);
    }

    public static function shutdownSdl3(): void {
        __native__imgui_sdl3_shutdown_renderer();
        __native__imgui_sdl3_shutdown();
    }

    // Forward the most-recent SDL event into ImGui's input pipeline.
    // Call from the event loop after Sdl.pollEvent() returned non-zero.
    public static function processSdlEvent(): void {
        __native__imgui_sdl3_process_event_ptr(Sdl::lastEventPtr());
    }

    public static function newFrame(): void {
        __native__imgui_new_frame();
    }

    public static function render(Renderer renderer): void {
        __native__imgui_render(renderer.handle);
    }

    // Begin a top-level window. Always pair with end().
    public static function begin(string title): bool {
        return __native__imgui_begin(title);
    }

    public static function end(): void {
        __native__imgui_end();
    }

    // Top-level window WITH the X close button. Returns bool[2]:
    //   ret[0] = shouldDraw  (true if Begin's body should run; false when
    //                         the window is collapsed/clipped — but still
    //                         call end() either way)
    //   ret[1] = newOpen     (false on the frame the user clicks X)
    //
    // Pattern:
    //   if (myWindowOpen) {
    //     bool[] s = ImGui::beginClosable("Title", myWindowOpen);
    //     if (s[0]) { ... widgets ... }
    //     ImGui::end();
    //     myWindowOpen = s[1];
    //   }
    public static function beginClosable(string title, bool currentOpen): bool[] {
        return __native__imgui_begin_closable(title, currentOpen);
    }

    public static function text(string label): void {
        __native__imgui_text(label);
    }

    // Draw a labeled button. Returns true on the frame the user clicks it.
    public static function button(string label): bool {
        return __native__imgui_button(label);
    }

    // ------------------------------------------------------------------
    // Phase 2 — input widgets.
    //
    // Widgets that mutate a value follow the same pattern: pass the
    // current value, the call returns the (possibly modified) new value.
    // To detect "did the user touch this widget THIS frame" (and not
    // just "current != previous"), call ImGui.widgetChanged() after the
    // widget call — it's true on the frame the widget mutated its value.
    // ------------------------------------------------------------------

    public static function widgetChanged(): bool {
        return __native__imgui_widget_changed();
    }

    public static function sliderFloat(string label, float current, float min, float max): float {
        return __native__imgui_slider_float(label, current, min, max);
    }

    public static function sliderInt(string label, int current, int min, int max): int {
        return __native__imgui_slider_int(label, current, min, max);
    }

    public static function checkbox(string label, bool current): bool {
        return __native__imgui_checkbox(label, current);
    }

    // Combo dropdown. items is a string[] of options.
    // Returns the new selected index (== currentIdx if unchanged).
    public static function combo(string label, int currentIdx, string[] items): int {
        return __native__imgui_combo(label, currentIdx, items);
    }

    // Single-line text edit. maxCapacity is the buffer size the plugin
    // allocates (clamped to [256, 65536] internally). Returns the
    // current contents — equal to `current` on frames the user didn't
    // edit.
    public static function inputText(string label, string current, int maxCapacity): string {
        return __native__imgui_input_text(label, current, maxCapacity);
    }

    // RGB color edit. Each component is in [0,1]. Returns a new
    // float[3] — index 0=R, 1=G, 2=B.
    public static function colorEdit3(string label, float r, float g, float b): float[] {
        return __native__imgui_color_edit3(label, r, g, b);
    }

    // ------------------------------------------------------------------
    // Phase 2 — layout helpers.
    // ------------------------------------------------------------------

    public static function sameLine():  void { __native__imgui_same_line(); }
    public static function separator(): void { __native__imgui_separator(); }
    public static function spacing():   void { __native__imgui_spacing(); }
    public static function bulletText(string s): void { __native__imgui_bullet_text(s); }

    // ------------------------------------------------------------------
    // Phase 3 — layout: tables, child windows, tabs, docking, splitter.
    // ------------------------------------------------------------------

    // Tables. Pattern:
    //   if (ImGui::beginTable("id", 3)) {
    //     ImGui::tableSetupColumn("a"); ImGui::tableSetupColumn("b"); ImGui::tableSetupColumn("c");
    //     ImGui::tableHeadersRow();
    //     ImGui::tableNextRow(); ImGui::tableNextColumn(); ImGui::text("...");
    //     ImGui::endTable();
    //   }
    public static function beginTable(string id, int columns): bool {
        return __native__imgui_begin_table(id, columns);
    }
    public static function endTable(): void { __native__imgui_end_table(); }
    public static function tableSetupColumn(string label): void {
        __native__imgui_table_setup_column(label);
    }
    public static function tableHeadersRow(): void { __native__imgui_table_headers_row(); }
    public static function tableNextRow():    void { __native__imgui_table_next_row(); }
    public static function tableNextColumn(): bool { return __native__imgui_table_next_column(); }
    public static function tableSetColumnIndex(int idx): bool {
        return __native__imgui_table_set_column_index(idx);
    }

    // Child windows. Always call endChild even if beginChild returned false.
    // Pass 0.0 for w/h to take the available region in that axis.
    public static function beginChild(string id, float width, float height, bool border): bool {
        return __native__imgui_begin_child(id, width, height, border);
    }
    public static function endChild(): void { __native__imgui_end_child(); }

    // Tab bars.
    public static function beginTabBar(string id): bool {
        return __native__imgui_begin_tab_bar(id);
    }
    public static function endTabBar(): void { __native__imgui_end_tab_bar(); }
    public static function beginTabItem(string label): bool {
        return __native__imgui_begin_tab_item(label);
    }
    public static function endTabItem(): void { __native__imgui_end_tab_item(); }

    // Docking. Call enableDocking() once at startup BEFORE the first
    // newFrame(). Then either dockSpaceOverViewport() to make the platform
    // window a dockspace, or dockSpace(id, w, h) inside a regular window.
    public static function enableDocking(): void { __native__imgui_enable_docking(); }
    public static function dockSpaceOverViewport(): void { __native__imgui_dock_space_over_viewport(); }
    public static function dockSpace(string id, float width, float height): void {
        __native__imgui_dock_space(id, width, height);
    }

    // Splitter. Pass current size1/size2 (e.g. left-pane width, right-pane
    // width for a vertical splitter), plus minimum sizes. Returns float[2]
    // = [newSize1, newSize2]. If neither pane can shrink below its min,
    // sizes stay put.
    public static function splitter(bool vertical, float thickness,
                                     float size1, float size2,
                                     float min1, float min2): float[] {
        return __native__imgui_splitter(vertical, thickness, size1, size2, min1, min2);
    }

    // ------------------------------------------------------------------
    // Phase 4 — image, clipboard, fonts.
    // ------------------------------------------------------------------

    // Draw a Texture as an Image widget at the given size in pixels.
    public static function image(Texture texture, float width, float height): void {
        __native__imgui_image(texture.handle, width, height);
    }

    public static function setClipboardText(string text): void {
        __native__imgui_set_clipboard_text(text);
    }
    public static function getClipboardText(): string {
        return __native__imgui_get_clipboard_text();
    }

    // Add a TTF font. Must be called BEFORE the first newFrame() of any
    // frame the font is going to be used in (the SDL3 renderer backend
    // rebuilds its font texture lazily on next frame). Returns a Font
    // wrapper; pass it to pushFont/popFont to scope a section's text to
    // it.
    public static function addFontFromFile(string path, float sizePixels): Font {
        return new Font(__native__imgui_add_font_from_file(path, sizePixels));
    }
    public static function pushFont(Font font): void {
        __native__imgui_push_font(font.handle);
    }
    public static function popFont(): void { __native__imgui_pop_font(); }

    // ------------------------------------------------------------------
    // Phase 6 — popups.
    //
    // Two-step trigger pattern:
    //   if (ImGui::button("Open")) { ImGui::openPopup("##my_popup"); }
    //   if (ImGui::beginPopup("##my_popup")) {
    //       ImGui::text("hello");
    //       if (ImGui::button("Close")) { ImGui::closeCurrentPopup(); }
    //       ImGui::endPopup();
    //   }
    //
    // Modal popups support an X close button — same bool[2] convention as
    // beginClosable: ret[0] = shouldDraw, ret[1] = newOpen.
    //
    // Context popups auto-trigger on right-click (no openPopup needed).
    // ------------------------------------------------------------------

    public static function openPopup(string id): void { __native__imgui_open_popup(id); }
    public static function beginPopup(string id): bool {
        return __native__imgui_begin_popup(id);
    }
    public static function beginPopupModal(string title, bool currentOpen): bool[] {
        return __native__imgui_begin_popup_modal(title, currentOpen);
    }
    public static function beginPopupContextItem(string id): bool {
        return __native__imgui_begin_popup_context_item(id);
    }
    public static function beginPopupContextWindow(string id): bool {
        return __native__imgui_begin_popup_context_window(id);
    }
    public static function endPopup(): void { __native__imgui_end_popup(); }
    public static function closeCurrentPopup(): void { __native__imgui_close_current_popup(); }

    // ------------------------------------------------------------------
    // Phase 6 — styles.
    //
    // Theme switches (dark / light / classic) reset every color at once.
    // Per-section overrides use push/pop with names like "Button" or
    // "WindowBg" — keyed by name so .mt code doesn't have to track ImGui's
    // enum re-numberings between versions. See PLUGIN_NOTES.md for the
    // full name list, or check src/ImGuiBindings.cpp's resolveColorIdx.
    //
    // Each push must be balanced by a matching pop with the right count.
    // ------------------------------------------------------------------

    public static function styleDark():    void { __native__imgui_style_dark(); }
    public static function styleLight():   void { __native__imgui_style_light(); }
    public static function styleClassic(): void { __native__imgui_style_classic(); }

    public static function pushStyleColor(string name, float r, float g, float b, float a): void {
        __native__imgui_push_style_color(name, r, g, b, a);
    }
    public static function popStyleColor(int count): void {
        __native__imgui_pop_style_color(count);
    }
    public static function pushStyleVarFloat(string name, float value): void {
        __native__imgui_push_style_var_float(name, value);
    }
    public static function pushStyleVarVec2(string name, float x, float y): void {
        __native__imgui_push_style_var_vec2(name, x, y);
    }
    public static function popStyleVar(int count): void {
        __native__imgui_pop_style_var(count);
    }
}

class Font {
    public int handle;
    public constructor(int h) { this.handle = h; }
}
