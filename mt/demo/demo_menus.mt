// Phase 7-B demo: main menu bar, tooltips, trees, numeric inputs, layout.
//
// Main menu bar with File / View; each item has a hover-tooltip. A
// collapsing header reveals a tree-node containing inputInt + inputText.
// dragFloat drives the size of a colored progress bar.

import * from "../lib/Sdl.mt";
import * from "../lib/ImGui.mt";

__plugin_load("mt/mtype_sdl_imgui.dll");

Sdl::init();

Window window = Sdl::createWindow("Phase 7-B: menus + trees + inputs", 1100, 700);
Renderer renderer = Sdl::createRenderer(window);

ImGuiContext gui = ImGui::createContext();
ImGui::initSdl3ForRenderer(window, renderer);
ImGui::initSdl3Renderer(renderer);

bool showDemoCheck = true;
int  port = 8080;
string host = "localhost";
float zoom = 0.5;
string notes = "Multi-line notes go here.\nDrag to resize.";

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

    if (ImGui::beginMainMenuBar()) {
        if (ImGui::beginMenu("File")) {
            if (ImGui::menuItem("New", "Ctrl+N")) { /* no-op */ }
            if (ImGui::isItemHovered()) { ImGui::setTooltip("Create a new document"); }
            if (ImGui::menuItem("Open", "Ctrl+O")) { /* no-op */ }
            if (ImGui::isItemHovered()) { ImGui::setTooltip("Open a document from disk"); }
            if (ImGui::menuItem("Quit", "Esc")) { running = false; }
            ImGui::endMenu();
        }
        if (ImGui::beginMenu("View")) {
            bool[] r = ImGui::menuItemCheck("Show demo", "", showDemoCheck);
            showDemoCheck = r[1];
            ImGui::endMenu();
        }
        ImGui::endMainMenuBar();
    }

    if (ImGui::begin("Phase 7-B demo")) {
        ImGui::textWrapped(
            "This window exercises tooltips, trees, numeric inputs, and "
            + "layout precision. Hover any menu item above for a tooltip.");
        ImGui::separator();

        if (ImGui::collapsingHeader("Advanced")) {
            ImGui::indent(12.0);
            if (ImGui::treeNode("Network")) {
                port = ImGui::inputInt("port", port);
                host = ImGui::inputText("host", host, 256);
                ImGui::labelText("endpoint", host + ":" + port);
                ImGui::treePop();
            }
            if (ImGui::treeNode("Notes")) {
                notes = ImGui::inputTextMultiline("##notes", notes, 1024, -1.0, 80.0);
                ImGui::treePop();
            }
            ImGui::unindent(12.0);
        }

        ImGui::separator();
        ImGui::text("Drag zoom to update the bar:");
        zoom = ImGui::dragFloat("zoom", zoom, 0.01);
        if (zoom < 0.0) { zoom = 0.0; }
        if (zoom > 1.0) { zoom = 1.0; }
        ImGui::progressBar(zoom, -1.0, 0.0, "");

        ImGui::separator();
        ImGui::textColored(0.7, 0.9, 0.5, 1.0, "Custom-colored text (textColored).");
        ImGui::textDisabled("Disabled text (textDisabled).");

        ImGui::separator();
        ImGui::text("Selectable list:");
        int row = 0;
        while (row < 5) {
            ImGui::pushIDInt(row);
            if (ImGui::selectable("Item " + row, false)) { /* no-op */ }
            ImGui::popID();
            row = row + 1;
        }
    }
    ImGui::end();

    renderer.setDrawColor(25, 25, 35, 255);
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
