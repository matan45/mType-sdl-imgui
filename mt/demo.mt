// SDL3 + ImGui demo (Phase 1 + Phase 2).
//
// Demonstrates the runtime plugin loader, the Window/Renderer wrappers,
// the full event-loop pattern, and the widget set added in Phase 2:
// sliders, checkbox, combo, text input, color picker, and event
// extraction (mouse position, keyboard, text input).
//
// Run from a working directory that contains mtype_sdl_imgui.dll
// (and SDL3.dll alongside it). Adjust the load path if needed.

import * from "Sdl.mt";
import * from "ImGui.mt";

__plugin_load("mt/mtype_sdl_imgui.dll");

Sdl::init();

Window window = Sdl::createWindow("mType + SDL3 + ImGui demo", 1280, 720);
Renderer renderer = Sdl::createRenderer(window);

ImGuiContext gui = ImGui::createContext();
ImGui::enableDocking();   // Phase 3 — must run BEFORE the first newFrame()
ImGui::initSdl3ForRenderer(window, renderer);
ImGui::initSdl3Renderer(renderer);

// --- demo state ---
int     clickCount = 0;
float   sliderValue = 0.5;
int     volume      = 50;
bool    enableThing = true;
int     selectedIdx = 0;
string  inputBuffer = "type here";
float   colorR = 0.85;
float   colorG = 0.45;
float   colorB = 0.20;

// Last observed event details (populated by the event loop).
float lastMouseX  = 0.0;
float lastMouseY  = 0.0;
int   lastKeycode = 0;
string lastTyped  = "";

string[] comboItems = new string[3];
comboItems[0] = "Apples";
comboItems[1] = "Bananas";
comboItems[2] = "Cherries";

// Phase 3 demo state — splitter pane sizes (left/right widths).
float leftPaneW  = 200.0;
float rightPaneW = 300.0;

bool running = true;

while (running) {
    int eventType = Sdl::pollEvent();
    while (eventType != 0) {
        ImGui::processSdlEvent();

        if (eventType == Sdl::quitEventId()) {
            running = false;
        } else if (eventType == Sdl::mouseMotionEventId()) {
            lastMouseX = Event::mouseX();
            lastMouseY = Event::mouseY();
        } else if (eventType == Sdl::keyDownEventId()) {
            lastKeycode = Event::keyKeycode();
        } else if (eventType == Sdl::textInputEventId()) {
            lastTyped = Event::text();
        }

        eventType = Sdl::pollEvent();
    }

    ImGui::newFrame();

    // Phase 3 — make the entire platform window a dockspace, so the two
    // demo windows below can be docked / tabbed / split via drag-and-drop.
    ImGui::dockSpaceOverViewport();

    if (ImGui::begin("mType + SDL3 + ImGui — Phase 2 demo")) {
        ImGui::text("Phase 1: text + button");
        ImGui::text("Clicks so far: " + clickCount);
        if (ImGui::button("Click me")) {
            clickCount = clickCount + 1;
        }
        ImGui::sameLine();
        if (ImGui::button("Quit")) {
            running = false;
        }

        ImGui::separator();
        ImGui::text("Phase 2: sliders + checkbox + combo");
        sliderValue = ImGui::sliderFloat("alpha", sliderValue, 0.0, 1.0);
        volume      = ImGui::sliderInt("volume", volume, 0, 100);
        enableThing = ImGui::checkbox("enable thing", enableThing);
        selectedIdx = ImGui::combo("fruit", selectedIdx, comboItems);
        ImGui::bulletText("selected: " + comboItems[selectedIdx]);

        ImGui::separator();
        ImGui::text("Phase 2: text input");
        inputBuffer = ImGui::inputText("name", inputBuffer, 256);
        ImGui::bulletText("you typed: " + inputBuffer);

        ImGui::separator();
        ImGui::text("Phase 2: color picker");
        float[] rgb = ImGui::colorEdit3("clear color", colorR, colorG, colorB);
        colorR = rgb[0];
        colorG = rgb[1];
        colorB = rgb[2];

        ImGui::separator();
        ImGui::text("Phase 2: raw events");
        ImGui::bulletText("mouse: " + lastMouseX + ", " + lastMouseY);
        ImGui::bulletText("last keycode: " + lastKeycode);
        ImGui::bulletText("last text input: " + lastTyped);
    }
    ImGui::end();

    // ----- Phase 3: layout demo (separate window — try docking it!) ----
    if (ImGui::begin("Phase 3 layout")) {
        if (ImGui::beginTabBar("##phase3tabs")) {

            if (ImGui::beginTabItem("Table")) {
                ImGui::text("3-column table:");
                if (ImGui::beginTable("##t1", 3)) {
                    ImGui::tableSetupColumn("name");
                    ImGui::tableSetupColumn("kind");
                    ImGui::tableSetupColumn("count");
                    ImGui::tableHeadersRow();

                    ImGui::tableNextRow(); ImGui::tableNextColumn(); ImGui::text("apples");
                    ImGui::tableNextColumn();                          ImGui::text("fruit");
                    ImGui::tableNextColumn();                          ImGui::text("12");

                    ImGui::tableNextRow(); ImGui::tableNextColumn(); ImGui::text("bread");
                    ImGui::tableNextColumn();                          ImGui::text("staple");
                    ImGui::tableNextColumn();                          ImGui::text("3");

                    ImGui::tableNextRow(); ImGui::tableNextColumn(); ImGui::text("milk");
                    ImGui::tableNextColumn();                          ImGui::text("dairy");
                    ImGui::tableNextColumn();                          ImGui::text("2 L");

                    ImGui::endTable();
                }
                ImGui::endTabItem();
            }

            if (ImGui::beginTabItem("Splitter + Child")) {
                ImGui::text("Drag the gray bar between the panels:");

                float[] sz = ImGui::splitter(true, 4.0,
                                             leftPaneW, rightPaneW,
                                             80.0, 80.0);
                leftPaneW  = sz[0];
                rightPaneW = sz[1];

                if (ImGui::beginChild("##left", leftPaneW, 200.0, true)) {
                    ImGui::text("Left pane");
                    ImGui::bulletText("width: " + leftPaneW);
                }
                ImGui::endChild();

                ImGui::sameLine();

                if (ImGui::beginChild("##right", rightPaneW, 200.0, true)) {
                    ImGui::text("Right pane");
                    ImGui::bulletText("width: " + rightPaneW);
                }
                ImGui::endChild();

                ImGui::endTabItem();
            }

            if (ImGui::beginTabItem("Docking tip")) {
                ImGui::bulletText("Drag this window's title bar onto the");
                ImGui::bulletText("Phase 2 window's tab area to dock them.");
                ImGui::bulletText("Drag a docked window's tab away to undock.");
                ImGui::endTabItem();
            }

            ImGui::endTabBar();
        }
    }
    ImGui::end();

    // Background clear with the user-picked color.
    int rr = (int)(colorR * 255.0);
    int gg = (int)(colorG * 255.0);
    int bb = (int)(colorB * 255.0);
    renderer.setDrawColor(rr, gg, bb, 255);
    renderer.clear();
    ImGui::render(renderer);
    renderer.present();
}

ImGui::shutdownSdl3();
gui.destroy();
renderer.destroy();
window.destroy();
Sdl::quit();
__plugin_unload("./bin/mType/Release/x64/mtype_sdl_imgui.dll");
