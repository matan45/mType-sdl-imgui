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

    // Phase 7-C — window props.
    public function size(): int[] { return __native__sdl_get_window_size(this.handle); }
    public function setSize(int w, int h): void {
        __native__sdl_set_window_size(this.handle, w, h);
    }
    public function setTitle(string title): void {
        __native__sdl_set_window_title(this.handle, title);
    }
    public function setFullscreen(bool full): bool {
        return __native__sdl_set_window_fullscreen(this.handle, full);
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

    // Phase 7-C — renderer primitives. All coordinates are floats.
    public function line(float x1, float y1, float x2, float y2): void {
        __native__sdl_render_line(this.handle, x1, y1, x2, y2);
    }
    public function rect(float x, float y, float w, float h): void {
        __native__sdl_render_rect(this.handle, x, y, w, h);
    }
    public function fillRect(float x, float y, float w, float h): void {
        __native__sdl_render_fill_rect(this.handle, x, y, w, h);
    }
    public function point(float x, float y): void {
        __native__sdl_render_point(this.handle, x, y);
    }
    // Draw a texture stretched to the given dest rect. Pass full-size
    // dst (w,h = tex.width(), tex.height()) for 1:1 rendering.
    public function renderTexture(Texture tex, float dx, float dy,
                                   float dw, float dh): void {
        __native__sdl_render_texture(this.handle, tex.handle, dx, dy, dw, dh);
    }
    // Rotated variant. angle in degrees clockwise; (cx, cy) is the
    // pivot in local coordinates (use dw/2, dh/2 for center). flipFlags:
    // 0=none, 1=horizontal, 2=vertical, 3=both.
    public function renderTextureRotated(Texture tex,
                                          float dx, float dy, float dw, float dh,
                                          float angle, float cx, float cy,
                                          int flipFlags): void {
        __native__sdl_render_texture_rotated(this.handle, tex.handle,
                                              dx, dy, dw, dh,
                                              angle, cx, cy, flipFlags);
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

// ----------------------------------------------------------------------
// Phase 7-C — realtime input + timing.
//
// Mouse/Keyboard read SDL's current input state directly (no event poll
// required). Timing exposes the ms-since-init counter plus the high-
// resolution perf counter for delta-time calculations.
// ----------------------------------------------------------------------

class Timing {
    public static function ticks(): int { return __native__sdl_get_ticks(); }
    public static function perfCounter():   int { return __native__sdl_get_performance_counter(); }
    public static function perfFrequency(): int { return __native__sdl_get_performance_frequency(); }
    // Seconds since SDL_Init, derived from the high-resolution counter.
    public static function seconds(): float {
        int c = __native__sdl_get_performance_counter();
        int f = __native__sdl_get_performance_frequency();
        if (f == 0) { return 0.0; }
        return ((float)c) / ((float)f);
    }
}

class Mouse {
    // Returns float[3] = [x, y, buttonsBitmask]. Bitmask uses SDL's
    // SDL_BUTTON_MASK(n) convention (left=bit0, middle=bit1, right=bit2).
    public static function state(): float[] { return __native__sdl_get_mouse_state(); }
    // True if the specified SDL_BUTTON_* button is currently pressed.
    // button: 1=left, 2=middle, 3=right, 4=x1, 5=x2.
    public static function isDown(int button): bool {
        float[] s = __native__sdl_get_mouse_state();
        int mask = 1;
        int i = 1;
        while (i < button) {
            mask = mask * 2;
            i = i + 1;
        }
        int b = (int)s[2];
        return (b & mask) != 0;
    }
    public static function show(): void { __native__sdl_show_cursor(); }
    public static function hide(): void { __native__sdl_hide_cursor(); }
}

class Keyboard {
    // True if the SDL scancode is currently pressed. See the Scancode
    // class for common values (or pass SDL_SCANCODE_* directly).
    public static function isDown(int scancode): bool {
        return __native__sdl_is_scancode_down(scancode);
    }
}

// Common SDL_Scancode values. Add more as needed — SDL has ~287 in
// total but these cover everything a small game/demo would need.
class Scancode {
    public static function a(): int { return 4; }
    public static function b(): int { return 5; }
    public static function c(): int { return 6; }
    public static function d(): int { return 7; }
    public static function e(): int { return 8; }
    public static function f(): int { return 9; }
    public static function g(): int { return 10; }
    public static function h(): int { return 11; }
    public static function i(): int { return 12; }
    public static function j(): int { return 13; }
    public static function k(): int { return 14; }
    public static function l(): int { return 15; }
    public static function m(): int { return 16; }
    public static function n(): int { return 17; }
    public static function o(): int { return 18; }
    public static function p(): int { return 19; }
    public static function q(): int { return 20; }
    public static function r(): int { return 21; }
    public static function s(): int { return 22; }
    public static function t(): int { return 23; }
    public static function u(): int { return 24; }
    public static function v(): int { return 25; }
    public static function w(): int { return 26; }
    public static function x(): int { return 27; }
    public static function y(): int { return 28; }
    public static function z(): int { return 29; }

    public static function n1(): int { return 30; }
    public static function n2(): int { return 31; }
    public static function n3(): int { return 32; }
    public static function n4(): int { return 33; }
    public static function n5(): int { return 34; }
    public static function n6(): int { return 35; }
    public static function n7(): int { return 36; }
    public static function n8(): int { return 37; }
    public static function n9(): int { return 38; }
    public static function n0(): int { return 39; }

    public static function enter():     int { return 40; }
    public static function escape():    int { return 41; }
    public static function backspace(): int { return 42; }
    public static function tab():       int { return 43; }
    public static function space():     int { return 44; }

    public static function f1():  int { return 58; }
    public static function f2():  int { return 59; }
    public static function f3():  int { return 60; }
    public static function f4():  int { return 61; }
    public static function f5():  int { return 62; }
    public static function f6():  int { return 63; }
    public static function f7():  int { return 64; }
    public static function f8():  int { return 65; }
    public static function f9():  int { return 66; }
    public static function f10(): int { return 67; }
    public static function f11(): int { return 68; }
    public static function f12(): int { return 69; }

    public static function right(): int { return 79; }
    public static function left():  int { return 80; }
    public static function down():  int { return 81; }
    public static function up():    int { return 82; }
}
