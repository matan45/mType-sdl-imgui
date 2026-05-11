// Demo: Textures::load (the file-based path through stb_image).
//
// We want this demo to run out of the box without shipping a binary
// asset, so step 1 synthesises a checkerboard BMP on disk via
// Textures::writeCheckerboardBmp, then step 2 loads it back through
// the regular Textures::load entry point — exercising the real
// disk-read + stb_image decode + GPU upload path. The loaded handle
// is then drawn three ways:
//   1. ImGui::image (inside an ImGui window)
//   2. renderer.renderTexture (straight blit on the backbuffer)
//   3. renderer.renderTextureRotated (rotating blit)
//
// You can replace the assetPath with your own PNG / JPG / BMP / TGA
// and skip the synth step — Textures::load handles all of those.
//
// Esc quits.

import * from "../lib/Sdl.mt";
import * from "../lib/ImGui.mt";

__plugin_load("mt/mtype_sdl_imgui.dll");

Sdl::init();

Window window = Sdl::createWindow("Textures::load demo", 1000, 700);
Renderer renderer = Sdl::createRenderer(window);

ImGuiContext gui = ImGui::createContext();
ImGui::initSdl3ForRenderer(window, renderer);
ImGui::initSdl3Renderer(renderer);

// Step 1 — synthesise a 128x128 / 16-px checker BMP on disk.
string assetPath = "mt/demo/_demo_load_image.bmp";

// Step 2 — load it back through the regular file-based pipeline.
Texture loaded = Textures::load(renderer, assetPath);

float imguiSize = 192.0;
float blitScale = 2.0;
float rotAngle  = 0.0;
float rotSpeed  = 60.0;
int   escKey    = ImGuiKey::escape();
int   lastTick  = Timing::ticks();

bool running = true;
while (running) {
    int eventType = Sdl::pollEvent();
    while (eventType != 0) {
        ImGui::processSdlEvent();
        if (eventType == Sdl::quitEventId()) { running = false; }
        eventType = Sdl::pollEvent();
    }

    int now = Timing::ticks();
    float dt = ((float)(now - lastTick)) / 1000.0;
    lastTick = now;
    rotAngle = rotAngle + rotSpeed * dt;

    ImGui::newFrame();

    if (ImGui::begin("Textures::load controls")) {
        ImGui::text("Loaded from: " + assetPath);
        ImGui::text("Decoded size: " + loaded.width() + " x " + loaded.height());
        ImGui::separator();

        ImGui::text("Path 1 — ImGui::image (inside this window):");
        imguiSize = ImGui::dragFloat("imgui size", imguiSize, 1.0);
        if (imguiSize < 16.0)  { imguiSize = 16.0; }
        if (imguiSize > 512.0) { imguiSize = 512.0; }
        ImGui::image(loaded, imguiSize, imguiSize);

        ImGui::separator();
        ImGui::text("Path 2/3 — drawn directly on the backbuffer below.");
        blitScale = ImGui::dragFloat("blit scale", blitScale, 0.01);
        if (blitScale < 0.25) { blitScale = 0.25; }
        if (blitScale > 6.0)  { blitScale = 6.0; }
        rotSpeed  = ImGui::dragFloat("rotation speed (deg/s)", rotSpeed, 1.0);
    }
    ImGui::end();

    renderer.setDrawColor(20, 22, 28, 255);
    renderer.clear();

    float tw = (float)loaded.width();
    float th = (float)loaded.height();
    renderer.renderTexture(loaded, 540.0, 380.0, tw * blitScale, th * blitScale);

    float dw = tw * blitScale;
    float dh = th * blitScale;
    renderer.renderTextureRotated(loaded,
        540.0 + dw + 24.0, 380.0,
        dw, dh,
        rotAngle, dw * 0.5, dh * 0.5, 0);

    ImGui::render(renderer);
    renderer.present();

    if (ImGui::isKeyPressed(escKey)) { running = false; }
}

loaded.destroy();
ImGui::shutdownSdl3();
gui.destroy();
renderer.destroy();
window.destroy();
Sdl::quit();
__plugin_unload("mt/mtype_sdl_imgui.dll");
