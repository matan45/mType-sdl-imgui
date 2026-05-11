// Phase 7-A demo: ImGui interactivity queries (IsItemHovered, GetMousePos,
// IsKeyPressed) and the ID stack (PushID/PopID for a list of duplicate
// labels). Press Escape to exit — without going through SDL_EVENT_QUIT.

import * from "../lib/Sdl.mt";
import * from "../lib/ImGui.mt";

__plugin_load("mt/mtype_sdl_imgui.dll");

Sdl::init();

Window window = Sdl::createWindow("Phase 7-A: input queries", 900, 600);
Renderer renderer = Sdl::createRenderer(window);

ImGuiContext gui = ImGui::createContext();
ImGui::initSdl3ForRenderer(window, renderer);
ImGui::initSdl3Renderer(renderer);

int   escKey      = ImGuiKey::escape();
int   spaceKey    = ImGuiKey::space();
int   spaceDowns  = 0;
int   hoveredCell = -1;

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

    if (ImGui::begin("input demo")) {
        ImGui::text("Press Escape to exit. Press Space to count.");
        ImGui::text("Space presses: " + spaceDowns);
        ImGui::separator();

        float[] mp = ImGui::getMousePos();
        ImGui::text("Mouse (screen): " + mp[0] + ", " + mp[1]);

        ImGui::separator();
        ImGui::text("4x4 grid — every button has the same label.");
        ImGui::text("Without pushID, ImGui would conflate clicks.");

        int last = -1;
        int row = 0;
        while (row < 4) {
            int col = 0;
            while (col < 4) {
                int idx = row * 4 + col;
                ImGui::pushIDInt(idx);
                if (ImGui::button("##cell")) {
                    last = idx;
                }
                if (ImGui::isItemHovered()) {
                    hoveredCell = idx;
                }
                ImGui::popID();
                col = col + 1;
                if (col < 4) { ImGui::sameLine(); }
            }
            row = row + 1;
        }

        ImGui::separator();
        ImGui::text("Hovered cell: " + hoveredCell);
        if (last >= 0) {
            ImGui::text("Last clicked: " + last);
        }

        ImGui::separator();
        ImGui::text("Progress bar:");
        float frac = ((float)(spaceDowns % 50)) / 50.0;
        ImGui::progressBar(frac, -1.0, 0.0, "");
    }
    ImGui::end();

    if (ImGui::isKeyPressed(escKey)) {
        running = false;
    }
    if (ImGui::isKeyPressed(spaceKey)) {
        spaceDowns = spaceDowns + 1;
    }

    renderer.setDrawColor(20, 20, 30, 255);
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
