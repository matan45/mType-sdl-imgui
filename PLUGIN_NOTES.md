# mtype-sdl-imgui-plugin

A runtime-loaded native plugin (`mtype_sdl_imgui.dll` / `.so` / `.dylib`)
exposing SDL3 + ImGui (docking branch) to mType scripts.

This plugin uses only `mType/plugin/PluginHostApi.h` — it does **not** link
the engine. Loaded at runtime by `__plugin_load("./bin/.../mtype_sdl_imgui.dll")`.

## Phase status

Roadmap below.

| Phase | Surface | Status |
|---|---|---|
| 1 | init/quit · window · renderer · poll/quit event · ImGui context+backends · `Begin`/`End` · `Text` · `Button` | implemented |
| 2 | mouse / keyboard / text-input event extraction · slider/checkbox/combo/input · color picker · `SameLine`/`Separator`/`Spacing`/`BulletText` | implemented |
| 3 | layout: tables · child windows · tabs · docking · hand-rolled splitter | implemented |
| 4 | textures (PNG/JPG/BMP/TGA via stb_image) · `Image` widget · TTF fonts · clipboard | implemented |
| 5 | audio (fire-and-forget WAV via SDL3 audio streams) · gamepad polling+events · rumble | implemented |

Each phase adds ~20–30 native functions and the corresponding mType wrapper
methods. Phases are independent — Phase 2 can land before Phase 3 etc.

**Phase 2 detail** (added 2026-05):

- SDL: event-type id constants (`mouseMotionEventId`, `mouseButtonDownEventId`, `mouseButtonUpEventId`, `mouseWheelEventId`, `keyDownEventId`, `keyUpEventId`, `textInputEventId`) plus an `Event` static class with `mouseX`/`mouseY`/`mouseButton`/`mouseClicks`/`wheelY`/`keyScancode`/`keyKeycode`/`keyMod`/`keyRepeat`/`text` payload accessors.
- ImGui inputs: `sliderFloat`, `sliderInt`, `checkbox`, `combo(label, idx, string[])`, `inputText(label, current, maxCap)`, `colorEdit3` (returns `float[3]`), plus `widgetChanged()` to detect "did the last widget mutate this frame" (separate signal because returned-value comparison can't tell when a slider is dragged back to its starting value within one frame).
- ImGui layout: `sameLine`, `separator`, `spacing`, `bulletText`.

**Phase 3 detail** (added 2026-05):

- ImGui Tables (preferred over deprecated Columns): `beginTable(id, columns)`, `endTable`, `tableSetupColumn(label)`, `tableHeadersRow`, `tableNextRow`, `tableNextColumn` (→ bool), `tableSetColumnIndex(idx)` (→ bool). The plugin defaults to `Borders | RowBg | Resizable` flags — change `nImGuiBeginTable` if you need different defaults.
- ImGui Child windows: `beginChild(id, w, h, border)` (pass 0.0 to take available space in that axis), `endChild`. Always call endChild even when begin returned false.
- ImGui Tab bars: `beginTabBar(id)` / `endTabBar`, `beginTabItem(label)` / `endTabItem`.
- ImGui Docking (docking branch only): `enableDocking()` sets `ImGuiConfigFlags_DockingEnable` on `io.ConfigFlags` — call once at startup BEFORE the first `newFrame`. `dockSpaceOverViewport()` makes the entire platform window a dockspace; `dockSpace(id, w, h)` spawns one inside an existing window. Subsequent `Begin()` windows become dockable.
- ImGui Splitter: hand-rolled (no `imgui_internal.h`) using `InvisibleButton` + `IO::MouseDelta`. Returns `float[2] = [newSize1, newSize2]`. Cursor changes to N-S/W-E resize on hover. Drag is clamped at min1/min2 — if either pane is at its minimum, no further movement in that direction.

**Phase 4 detail** (added 2026-05):

- **Textures**: vendored single-header `stb_image.h` (under `vendor/stb/`, downloaded from `nothings/stb`). The implementation define lives in `PluginEntry.cpp` so exactly one TU compiles it. `Textures::load(renderer, path)` decodes PNG/JPG/BMP/TGA/PSD/GIF to RGBA8, creates an `SDL_Texture` via `SDL_CreateTexture` + `SDL_UpdateTexture`, and returns a `Texture` wrapper (linear scale-mode by default). `ImGui::image(texture, w, h)` draws it. Texture handles live in `g_textures: HandleRegistry<SDL_Texture>`.
- **Fonts**: `ImGui::addFontFromFile(path, sizePixels)` wraps `ImGui::GetIO().Fonts->AddFontFromFileTTF`. Returns a `Font` wrapper. **Must be called BEFORE the first `newFrame()` of any frame the font is going to render in** — the SDL3 renderer backend rebuilds its font texture lazily on next frame. `pushFont(font)` / `popFont()` scope a section of widgets to the font.
- **Clipboard**: `ImGui::setClipboardText(s)` / `getClipboardText()` delegate to ImGui's wrapper, which dispatches through the SDL3 platform backend.

**Phase 5 detail** (added 2026-05):

- **Audio (minimal v1)**: `Audio::init()` initialises `SDL_INIT_AUDIO`. `Audio::playWav(path)` decodes a WAV via `SDL_LoadWAV`, opens a default playback device with `SDL_OpenAudioDeviceStream`, queues + flushes the buffer, and resumes the stream. The stream + buffer are intentionally leaked (SDL drains on its own thread). For looping / mp3 / ogg / mixing, vendor SDL_mixer in a future phase and add a richer API.
- **Gamepad**: `Gamepads::init()` initialises `SDL_INIT_GAMEPAD`. `Gamepads::count()` returns the connected count via `SDL_GetGamepads`. `Gamepads::open(idx)` opens the Nth gamepad and returns a `Gamepad` wrapper. `Gamepad::axis(axisId)` reads an axis (-32768..32767), `Gamepad::button(btnId)` reads a button (bool). Axis/button id constants are documented inline in `Sdl.mt`. Event-type constants exposed: `gamepadButtonDownEventId`, `gamepadButtonUpEventId`, `gamepadAxisMotionEventId`.
- **Haptic**: `Gamepad::rumble(low, high, durationMs)` wraps `SDL_RumbleGamepad` (low/high motor strengths in [0, 65535]). The lower-level `SDL_Haptic` API is deferred.

## Vendoring

The plugin's premake project is **gated** on `vendor/SDL3/` and
`vendor/imgui/` existing. If they're absent, premake silently skips the
project (so the rest of the workspace generates cleanly) and the runtime
test that loads `mtype_sdl_imgui.dll` will fail with a clear "file not
found" error. Do this once per checkout:

### 1. Vendor SDL3

Clone the SDL3 source (shallow clone keeps the checkout small):

```
git clone --depth=1 https://github.com/libsdl-org/SDL.git vendor/SDL3
```

Then build it once via its own CMake to produce the import lib + DLL:

```
cd vendor/SDL3
cmake -B build -A x64 -DSDL_TEST_LIBRARY=OFF
cmake --build build --config Release --parallel
```

Output (Windows):

```
vendor/SDL3/
├── include/SDL3/SDL.h     (headers, used at compile time)
└── build/Release/
    ├── SDL3.dll           (used at runtime — copy next to mType.exe)
    └── SDL3.lib           (import lib, linked by the plugin)
```

The premake project's `libdirs` already points at
`vendor/SDL3/build/Release` (and `…/Debug` for Debug builds), so once
this CMake build is done the workspace links cleanly.

Linux/macOS: same commands minus the `-A x64` flag. Output ends up at
`vendor/SDL3/build/libSDL3.so` (or `.dylib`).

### 2. Vendor ImGui (docking branch)

```
git clone --depth=1 --branch docking https://github.com/ocornut/imgui vendor/imgui
```

After cloning, the layout should be:

```
vendor/imgui/
├── imgui.h
├── imgui.cpp
├── imgui_demo.cpp
├── imgui_draw.cpp
├── imgui_tables.cpp
├── imgui_widgets.cpp
└── backends/
    ├── imgui_impl_sdl3.h
    ├── imgui_impl_sdl3.cpp
    ├── imgui_impl_sdlrenderer3.h
    └── imgui_impl_sdlrenderer3.cpp
```

Confirm the SDL3 backend files are present — older ImGui clones may
default to a non-docking branch that lacks them.

## Building

After vendoring:

```
runPremake.bat
```

Open `Interpreter.sln` and build the `mtype-sdl-imgui-plugin` project
(or build the whole solution). Output:

```
bin/mType/Release/x64/mtype_sdl_imgui.dll
bin/mType/Release/x64/SDL3.dll              ← copy this in manually
```

**Important on Windows**: `SDL3.dll` is loaded by the OS when
`mtype_sdl_imgui.dll` is loaded. Copy `vendor/SDL3/build/Release/SDL3.dll`
next to `mType.exe` (i.e. into `bin/mType/Release/x64/`) before running
the demo. premake doesn't do this automatically — keep it explicit so
reproducible builds don't accidentally rely on a stale system DLL.

```
copy vendor\SDL3\build\Release\SDL3.dll bin\mType\Release\x64\
```

## Running the demo

```
bin\mType\Release\x64\mType.exe examples\sdl-imgui-demo\demo.mt
```

You should see a 1280×720 window with one ImGui frame: a label, a click
counter, a "Click me" button, and a "Quit" button. Closing the window or
clicking Quit terminates cleanly.

## Adding bindings (Phase 2 and later)

Each new native function follows this shape:

```cpp
// In SdlBindings.cpp or ImGuiBindings.cpp:
MTypeValue* nMyNewBinding(void*, MTypeContext* ctx,
                           const MTypeValue* const* args, int argc) {
    if (!requireArgs(ctx, argc, /*expected=*/N, "__native__module_name")) return g_host->makeVoid(ctx);
    // Pull args via g_host->getInt / getFloat / getString / getBool.
    // Look up handles via g_windows.find / g_renderers.find / etc.
    // Call the SDL/ImGui function. Return via g_host->makeXxx.
}

// In registerSdlNatives / registerImGuiNatives:
reg("__native__module_name", &nMyNewBinding);
```

**Naming**: every plugin native must be registered under a `__native__`-
prefixed name. The mType compiler validates all called functions at
compile time; built-in natives are present then but plugin natives aren't.
The `__native__` prefix is the explicit opt-out that tells the compiler
the function resolves at runtime. Without it, every script that calls
your plugin's functions fails with "Function 'X' not found".

Then add a method to the corresponding mType wrapper class
(`Sdl.mt`/`ImGui.mt`) that calls `__module_name(...)` and returns a typed
result. Update the phase table at the top of this file when a phase is
complete.

## Cross-toolchain notes

The plugin uses `MTYPE_PLUGIN_ABI_VERSION` (currently 2) declared in
`PluginHostApi.h`. If the engine bumps it (e.g. adding more vtable
members), the plugin must be recompiled. The loader rejects mismatched
versions with a clear `PluginError` rather than crashing.
