// Focused example: an ImGui window with a working X close button.
//
// Three windows are created. Each is opened/closed independently:
//   - The X close button on a window's title bar flips its state to false.
//   - The "Show window N" checkboxes in the control panel flip them back.
//
// Run from a working directory with mtype_sdl_imgui.dll + SDL3.dll.

import * from "../lib/Sdl.mt";
import * from "../lib/ImGui.mt";

__plugin_load("mt/mtype_sdl_imgui.dll");

Sdl::init();

Window window = Sdl::createWindow("Closable-window demo", 900, 600);
Renderer renderer = Sdl::createRenderer(window);

ImGuiContext gui = ImGui::createContext();
ImGui::initSdl3ForRenderer(window, renderer);
ImGui::initSdl3Renderer(renderer);

// Per-window open state.
bool helloOpen   = true;
bool detailsOpen = true;
bool aboutOpen   = false;  // start hidden

bool running = true;

while (running) {
    int eventType = Sdl::pollEvent();
    while (eventType != 0) {
        ImGui::processSdlEvent();
        if (eventType == Sdl::quitEventId()) {
            running = false;
        }
        eventType = Sdl::pollEvent();
    }

    ImGui::newFrame();

    // ---- Always-present control panel (no X button — uses Begin not BeginClosable).
    if (ImGui::begin("Controls")) {
        ImGui::text("Tick a checkbox to reopen a window after closing it:");
        helloOpen   = ImGui::checkbox("Show 'Hello'",   helloOpen);
        detailsOpen = ImGui::checkbox("Show 'Details'", detailsOpen);
        aboutOpen   = ImGui::checkbox("Show 'About'",   aboutOpen);
        ImGui::separator();
        if (ImGui::button("Quit")) {
            running = false;
        }
    }
    ImGui::end();

    // ---- Window 1 — closable. Standard pattern.
    if (helloOpen) {
        bool[] s = ImGui::beginClosable("Hello", helloOpen);
        if (s[0]) {
            ImGui::text("This window has an X close button on its title bar.");
            ImGui::bulletText("Click the X to close it.");
            ImGui::bulletText("Reopen it from the Controls panel.");
        }
        ImGui::end();
        helloOpen = s[1];  // pick up the new open state
    }

    // ---- Window 2 — closable, with a few widgets inside.
    if (detailsOpen) {
        bool[] s = ImGui::beginClosable("Details", detailsOpen);
        if (s[0]) {
            ImGui::text("X-close still works while widgets are present.");
            ImGui::separator();
            if (ImGui::button("Click me")) {
                // ...
            }
            ImGui::sameLine();
            if (ImGui::button("Or me")) {
                // ...
            }
        }
        ImGui::end();
        detailsOpen = s[1];
    }

    // ---- Window 3 — starts hidden. Tick "Show 'About'" to see it.
    if (aboutOpen) {
        bool[] s = ImGui::beginClosable("About", aboutOpen);
        if (s[0]) {
            ImGui::text("Closable windows can start either open or closed.");
            ImGui::bulletText("Initial state is whatever the bool starts at.");
            ImGui::bulletText("X-click sets it to false; checkbox sets it to true.");
        }
        ImGui::end();
        aboutOpen = s[1];
    }

    renderer.setDrawColor(30, 30, 30, 255);
    renderer.clear();
    ImGui::render(renderer);
    renderer.present();
}

ImGui::shutdownSdl3();
gui.destroy();
renderer.destroy();
window.destroy();
Sdl::quit();
__plugin_unload("mt/mtype_sdl_imgui.dll");
