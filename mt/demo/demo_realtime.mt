// Phase 7-C demo: realtime input + timing + renderer primitives.
//
// WASD moves the player rect (driven by Keyboard::isDown — no events
// needed). F11 toggles fullscreen. Esc quits. A grid + crosshair are
// drawn with the SDL renderer's line/rect/fillRect primitives, and
// the window title updates every ~500 ms with the live FPS.

import * from "../lib/Sdl.mt";

__plugin_load("mt/mtype_sdl_imgui.dll");

Sdl::init();

Window window = Sdl::createWindow("Phase 7-C: realtime", 800, 600);
Renderer renderer = Sdl::createRenderer(window);

// Player position and movement speed (px / second).
float px = 400.0;
float py = 300.0;
float speed = 300.0;

// Toggle state for F11 fullscreen.
bool fullscreen = false;
bool f11Prev = false;

// Frame-time tracking.
int   lastTick     = Timing::ticks();
int   fpsFrames    = 0;
int   fpsLastTick  = lastTick;
float fps          = 0.0;

// Scancodes resolved once.
int scW    = Scancode::w();
int scA    = Scancode::a();
int scS    = Scancode::s();
int scD    = Scancode::d();
int scEsc  = Scancode::escape();
int scF11  = Scancode::f11();

bool running = true;
while (running) {
    int eventType = Sdl::pollEvent();
    while (eventType != 0) {
        if (eventType == Sdl::quitEventId()) { running = false; }
        eventType = Sdl::pollEvent();
    }

    // Delta time.
    int now = Timing::ticks();
    float dt = ((float)(now - lastTick)) / 1000.0;
    lastTick = now;

    // Input — realtime keyboard state.
    if (Keyboard::isDown(scW)) { py = py - speed * dt; }
    if (Keyboard::isDown(scS)) { py = py + speed * dt; }
    if (Keyboard::isDown(scA)) { px = px - speed * dt; }
    if (Keyboard::isDown(scD)) { px = px + speed * dt; }
    if (Keyboard::isDown(scEsc)) { running = false; }

    // F11 — toggle fullscreen, edge-triggered.
    bool f11Now = Keyboard::isDown(scF11);
    if (f11Now && !f11Prev) {
        fullscreen = !fullscreen;
        window.setFullscreen(fullscreen);
    }
    f11Prev = f11Now;

    // Bounds-check player against current window size.
    int[] ws = window.size();
    if (px < 0.0)            { px = 0.0; }
    if (py < 0.0)            { py = 0.0; }
    if (px > ((float)ws[0]) - 32.0) { px = ((float)ws[0]) - 32.0; }
    if (py > ((float)ws[1]) - 32.0) { py = ((float)ws[1]) - 32.0; }

    // Background.
    renderer.setDrawColor(15, 15, 20, 255);
    renderer.clear();

    // 64-px grid.
    renderer.setDrawColor(40, 40, 55, 255);
    int x = 0;
    while (x < ws[0]) {
        renderer.line((float)x, 0.0, (float)x, (float)ws[1]);
        x = x + 64;
    }
    int y = 0;
    while (y < ws[1]) {
        renderer.line(0.0, (float)y, (float)ws[0], (float)y);
        y = y + 64;
    }

    // Mouse crosshair.
    float[] ms = Mouse::state();
    renderer.setDrawColor(120, 180, 255, 255);
    renderer.line(ms[0] - 8.0, ms[1],       ms[0] + 8.0, ms[1]);
    renderer.line(ms[0],       ms[1] - 8.0, ms[0],       ms[1] + 8.0);

    // Player.
    renderer.setDrawColor(220, 80, 60, 255);
    renderer.fillRect(px, py, 32.0, 32.0);
    renderer.setDrawColor(255, 255, 255, 255);
    renderer.rect(px, py, 32.0, 32.0);

    renderer.present();

    // FPS — refresh ~twice per second.
    fpsFrames = fpsFrames + 1;
    int elapsed = now - fpsLastTick;
    if (elapsed >= 500) {
        fps = ((float)fpsFrames) * 1000.0 / ((float)elapsed);
        fpsFrames = 0;
        fpsLastTick = now;
        window.setTitle("Phase 7-C: realtime — FPS " + ((int)fps));
    }
}

renderer.destroy();
window.destroy();
Sdl::quit();
__plugin_unload("mt/mtype_sdl_imgui.dll");
