// Phase 1 mType wrapper around the __native__sdl_* natives exposed by
// mtype-sdl-imgui-plugin. Window and Renderer are thin wrappers over the
// int handles minted by the plugin's HandleRegistry; calling .destroy()
// on either is safe to skip if you also call Sdl.quit() at shutdown.

class Window {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function destroy(): void {
        __native__sdl_destroy_window(this.handle);
    }
}

class Renderer {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function destroy(): void {
        __native__sdl_destroy_renderer(this.handle);
    }

    public function setDrawColor(int r, int g, int b, int a): void {
        __native__sdl_set_render_draw_color(this.handle, r, g, b, a);
    }

    public function clear(): void {
        __native__sdl_render_clear(this.handle);
    }

    public function present(): void {
        __native__sdl_render_present(this.handle);
    }
}

class Sdl {
    // Initialise SDL3 video + events. Throws SdlError on failure.
    public static function init(): bool {
        return __native__sdl_init();
    }

    public static function quit(): void {
        __native__sdl_quit();
    }

    public static function createWindow(string title, int width, int height): Window {
        return new Window(__native__sdl_create_window(title, width, height));
    }

    public static function createRenderer(Window window): Renderer {
        return new Renderer(__native__sdl_create_renderer(window.handle));
    }

    // Drains the next pending event into the plugin's event stash.
    // Returns the SDL event type code (0 if no event was pending).
    public static function pollEvent(): int {
        return __native__sdl_poll_event();
    }

    // Constant for SDL_EVENT_QUIT (resolved at runtime; differs SDL2 vs SDL3).
    public static function quitEventId(): int {
        return __native__sdl_event_quit_id();
    }

    // Raw pointer to the most-recently-polled SDL_Event, for handing to
    // ImGui.processEventPtr. Treat as opaque on the .mt side.
    public static function lastEventPtr(): int {
        return __native__sdl_event_ptr();
    }

    public static function lastError(): string {
        return __native__sdl_get_error();
    }

    public static function delay(int millis): void {
        __native__sdl_delay(millis);
    }

    // Phase 2 — event-type id constants. Compare against pollEvent()'s return.
    public static function mouseMotionEventId():     int { return __native__sdl_event_mouse_motion_id(); }
    public static function mouseButtonDownEventId(): int { return __native__sdl_event_mouse_button_down_id(); }
    public static function mouseButtonUpEventId():   int { return __native__sdl_event_mouse_button_up_id(); }
    public static function mouseWheelEventId():      int { return __native__sdl_event_mouse_wheel_id(); }
    public static function keyDownEventId():         int { return __native__sdl_event_key_down_id(); }
    public static function keyUpEventId():           int { return __native__sdl_event_key_up_id(); }
    public static function textInputEventId():       int { return __native__sdl_event_text_input_id(); }
}

// Read accessors for the most recently polled event. Each accessor returns
// 0 / "" when the last event isn't of the matching type — gate on the
// event-type id constant from Sdl first.
class Event {
    // Mouse — valid for motion / button / wheel events as noted.
    public static function mouseX():         float  { return __native__sdl_event_mouse_x(); }
    public static function mouseY():         float  { return __native__sdl_event_mouse_y(); }
    public static function mouseButton():    int    { return __native__sdl_event_mouse_button(); }
    public static function mouseClicks():    int    { return __native__sdl_event_mouse_clicks(); }
    public static function wheelY():         float  { return __native__sdl_event_wheel_y(); }

    // Key — valid for key down/up events.
    // scancode: layout-independent (SDL_SCANCODE_*).
    // keycode:  layout-dependent character (SDL_KEYCODE_*).
    public static function keyScancode():    int    { return __native__sdl_event_key_scancode(); }
    public static function keyKeycode():     int    { return __native__sdl_event_key_keycode(); }
    public static function keyMod():         int    { return __native__sdl_event_key_mod(); }
    public static function keyRepeat():      bool   { return __native__sdl_event_key_repeat(); }

    // Text — valid for text input events.
    public static function text():           string { return __native__sdl_event_text(); }
}
