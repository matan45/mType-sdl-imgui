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
}
