// Focused example: ImGui popups (modal + non-modal + context menu) and
// styles (theme switching + per-widget color/var overrides).

import * from "../lib/Sdl.mt";
import * from "../lib/ImGui.mt";

__plugin_load("mt/mtype_sdl_imgui.dll");

Sdl::init();

Window window = Sdl::createWindow("Popups + Styles demo", 900, 600);
Renderer renderer = Sdl::createRenderer(window);

ImGuiContext gui = ImGui::createContext();
ImGui::initSdl3ForRenderer(window, renderer);
ImGui::initSdl3Renderer(renderer);
ImGui::styleDark();                  // initial theme

// State
bool   modalOpen   = true;             // for the modal popup's bool[2] return
int    themeIdx    = 0;                // 0=dark, 1=light, 2=classic
string themeName   = "dark";
int    contextHits = 0;
bool   running     = true;

while (running) {
    int eventType = Sdl::pollEvent();
    while (eventType != 0) {
        ImGui::processSdlEvent();
        if (eventType == Sdl::quitEventId()) { running = false; }
        eventType = Sdl::pollEvent();
    }

    ImGui::newFrame();

    if (ImGui::begin("Popups + Styles")) {

        // ---- Theme switcher ----
        ImGui::text("Current theme: " + themeName);
        if (ImGui::button("Cycle theme")) {
            themeIdx = (themeIdx + 1) % 3;
            if (themeIdx == 0) {
                ImGui::styleDark();
                themeName = "dark";
            } else if (themeIdx == 1) {
                ImGui::styleLight();
                themeName = "light";
            } else {
                ImGui::styleClassic();
                themeName = "classic";
            }
        }

        ImGui::separator();

        // ---- Per-widget styled section ----
        ImGui::text("Per-widget overrides (red Button, large rounding):");
        ImGui::pushStyleColor("Button",        0.80, 0.20, 0.20, 1.00);
        ImGui::pushStyleColor("ButtonHovered", 0.95, 0.30, 0.30, 1.00);
        ImGui::pushStyleColor("ButtonActive",  0.70, 0.10, 0.10, 1.00);
        ImGui::pushStyleVarFloat("FrameRounding", 12.0);
        if (ImGui::button("DON'T PRESS")) { /* user pressed it anyway */ }
        ImGui::popStyleVar(1);
        ImGui::popStyleColor(3);

        ImGui::separator();

        // ---- Non-modal popup (auto-dismiss on outside click) ----
        if (ImGui::button("Show non-modal popup")) {
            ImGui::openPopup("##nonmodal");
        }
        if (ImGui::beginPopup("##nonmodal")) {
            ImGui::text("Hi! I close when you click outside me,");
            ImGui::text("or when you click this button:");
            if (ImGui::button("Close")) { ImGui::closeCurrentPopup(); }
            ImGui::endPopup();
        }

        // ---- Modal popup (X button + bool[2] pattern) ----
        ImGui::sameLine();
        if (ImGui::button("Show modal popup")) {
            modalOpen = true;
            ImGui::openPopup("Confirm action");
        }
        bool[] m = ImGui::beginPopupModal("Confirm action", modalOpen);
        if (m[0]) {
            ImGui::text("This is a MODAL popup — input behind it is blocked.");
            ImGui::text("Use the X to dismiss, or the buttons below.");
            ImGui::separator();
            if (ImGui::button("OK")) {
                ImGui::closeCurrentPopup();
                modalOpen = false;
            }
            ImGui::sameLine();
            if (ImGui::button("Cancel")) {
                ImGui::closeCurrentPopup();
                modalOpen = false;
            }
            ImGui::endPopup();
        }
        modalOpen = m[1];   // pick up X-click

        ImGui::separator();

        // ---- Right-click context menu on a specific item ----
        ImGui::text("Right-click the box below for a context menu.");
        ImGui::button("(right-click me)");
        if (ImGui::beginPopupContextItem("##ctx_button")) {
            ImGui::text("Context menu hit count: " + contextHits);
            if (ImGui::button("Increment")) { contextHits = contextHits + 1; }
            if (ImGui::button("Reset"))     { contextHits = 0; }
            ImGui::endPopup();
        }

        ImGui::separator();
        if (ImGui::button("Quit")) { running = false; }
    }
    ImGui::end();

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
