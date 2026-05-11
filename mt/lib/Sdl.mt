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

    // Phase 5 — gamepad event-type ids.
    public static function gamepadButtonDownEventId(): int { return __native__sdl_event_gamepad_button_down_id(); }
    public static function gamepadButtonUpEventId():   int { return __native__sdl_event_gamepad_button_up_id(); }
    public static function gamepadAxisMotionEventId(): int { return __native__sdl_event_gamepad_axis_motion_id(); }
}

// ----------------------------------------------------------------------
// Phase 4 — textures.
// ----------------------------------------------------------------------

class Texture {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function destroy(): void {
        __native__sdl_destroy_texture(this.handle);
    }

    public function width():  int { return __native__sdl_texture_width(this.handle); }
    public function height(): int { return __native__sdl_texture_height(this.handle); }
}

class Textures {
    // Load PNG / JPG / BMP / TGA via stb_image into a renderer-bound
    // texture. Throws SdlError on decode/upload failure.
    public static function load(Renderer renderer, string path): Texture {
        return new Texture(__native__sdl_load_texture(renderer.handle, path));
    }
}

// ----------------------------------------------------------------------
// Phase 5 — audio.
//
// v1 surface: open default audio device on init, fire-and-forget WAV
// playback. For looping / mp3 / ogg / mixing, vendor SDL_mixer in a
// future revision.
// ----------------------------------------------------------------------

class Audio {
    public static function init(): bool { return __native__sdl_init_audio(); }

    // Decode a WAV from disk and play it once. Caller doesn't manage
    // the buffer's lifetime — SDL drains it on its own thread.
    public static function playWav(string path): bool {
        return __native__sdl_play_wav(path);
    }
}

// ----------------------------------------------------------------------
// Phase 5 — gamepad + haptic.
//
// Axis ids map to SDL_GamepadAxis (LEFTX=0, LEFTY=1, RIGHTX=2, RIGHTY=3,
// LEFT_TRIGGER=4, RIGHT_TRIGGER=5).
// Button ids map to SDL_GamepadButton (SOUTH=0/A, EAST=1/B, WEST=2/X,
// NORTH=3/Y, BACK=4, GUIDE=5, START=6, LEFT_STICK=7, RIGHT_STICK=8,
// LEFT_SHOULDER=9, RIGHT_SHOULDER=10, DPAD_UP=11, DPAD_DOWN=12,
// DPAD_LEFT=13, DPAD_RIGHT=14).
// ----------------------------------------------------------------------

class Gamepad {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function close(): void { __native__sdl_close_gamepad(this.handle); }

    public function axis(int axisId):    int  { return __native__sdl_gamepad_axis(this.handle, axisId); }
    public function button(int buttonId): bool { return __native__sdl_gamepad_button(this.handle, buttonId); }

    // Rumble both motors. Strengths in [0, 65535]. duration in ms.
    public function rumble(int low, int high, int durationMs): bool {
        return __native__sdl_rumble_gamepad(this.handle, low, high, durationMs);
    }
}

class Gamepads {
    // Init the gamepad subsystem. Call once before count/open.
    public static function init(): bool { return __native__sdl_init_gamepads(); }

    public static function count(): int { return __native__sdl_gamepad_count(); }

    public static function open(int idx): Gamepad {
        return new Gamepad(__native__sdl_open_gamepad(idx));
    }
}
