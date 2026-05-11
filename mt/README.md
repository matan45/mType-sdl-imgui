# sdl-imgui-demo

Minimal SDL3 + ImGui app written in mType, driven by the
`mtype-sdl-imgui-plugin` runtime plugin.

## Prerequisites

Build the plugin first. See
[`mType/sdlImguiPlugin/README.md`](../../mType/sdlImguiPlugin/README.md)
for vendoring (SDL3 + ImGui docking branch under `vendor/`) and build
steps.

After building, you should have:

```
bin/mType/Release/x64/mtype_sdl_imgui.dll
bin/mType/Release/x64/SDL3.dll              (Windows: copied manually)
```

## Run

From the project root:

```
bin\mType\Release\x64\mType.exe examples\sdl-imgui-demo\demo.mt
```

You should see a 1280×720 window with an ImGui panel containing a click
counter and two buttons. Click "Quit" or close the window to exit.

Additional demos:

```
bin\mType\Release\x64\mType.exe examples\sdl-imgui-demo\demo_input.mt
bin\mType\Release\x64\mType.exe examples\sdl-imgui-demo\demo_menus.mt
bin\mType\Release\x64\mType.exe examples\sdl-imgui-demo\demo_realtime.mt
bin\mType\Release\x64\mType.exe examples\sdl-imgui-demo\demo_load_image.mt
```

## Files

| File | Purpose |
|---|---|
| `lib/Sdl.mt`   | mType wrappers around the `__sdl_*` natives. Window, Renderer, Texture, Audio, Gamepad, plus the Phase 7-C `Timing`, `Mouse`, `Keyboard`, `Scancode` static classes for realtime input + timing. |
| `lib/ImGui.mt` | mType wrappers around the `__imgui_*` natives. `ImGui` static class (begin/end, widgets, popups, tables, tabs, docking, menus, tooltips, trees, numeric inputs, layout precision, interactivity queries) plus the `ImGuiKey` constant-getter class. |
| `demo/demo.mt`           | Phases 1–3 demo — click counter, sliders, combo, color picker, tables, tabs, splitter, docking. |
| `demo/demo_close.mt`     | Closable-window pattern (X button + `beginClosable`). |
| `demo/demo_popup_style.mt` | Phase 6 — popups (modal / context) and per-section style overrides. |
| `demo/demo_input.mt`     | Phase 7-A — `isItemHovered`, `getMousePos`, `isKeyPressed`, `pushID` over a 4×4 grid, ProgressBar. |
| `demo/demo_menus.mt`     | Phase 7-B — main menu bar, hover-tooltips, collapsing header + tree nodes, `inputInt/inputText/inputTextMultiline`, `dragFloat`. |
| `demo/demo_realtime.mt`  | Phase 7-C — WASD-moves-a-rect game loop driven by `Keyboard::isDown`, mouse crosshair via `Mouse::state`, SDL renderer primitives (line / fillRect), F11 fullscreen, live FPS in window title. |
| `demo/demo_load_image.mt` | Exercises `Textures::load` (the stb_image file-decode path). Synthesises a BMP on disk via `Textures::writeCheckerboardBmp` so the demo runs without a shipped asset, then loads it through the regular `Textures::load` pipeline and draws it three ways: `ImGui::image`, `renderer.renderTexture`, and `renderer.renderTextureRotated`. Swap the path in the script to load your own PNG / JPG / BMP / TGA. |
