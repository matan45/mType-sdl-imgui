# mtype-imgui-sdl

Standalone runtime plugin for [mType](https://github.com/matan45/mType)
that exposes **SDL3** + **ImGui** (docking branch) to mType scripts.

This repo ships:
- A C/C++ shared library (`mtype_sdl_imgui.dll` / `.so` / `.dylib`) loaded
  at runtime via `__plugin_load`.
- mType wrapper classes (`Sdl.mt`, `ImGui.mt`) that hide the raw native
  names behind ergonomic OO APIs.
- A working demo (`mt/demo.mt`).

The plugin links **nothing from the mType engine** — only the C ABI
declared in `include/PluginHostApi.h` (a vendored copy from the engine).
A plugin built with one toolchain loads cleanly into an mType built with
another.

## Layout

```
mtype-imgui-sdl/
├── CMakeLists.txt           # the build
├── include/
│   └── PluginHostApi.h      # vendored from mType — see "ABI sync" below
├── src/                     # plugin C++ sources
│   ├── HandleRegistry.hpp
│   ├── PluginGlobals.hpp
│   ├── PluginEntry.cpp
│   ├── SdlBindings.cpp
│   └── ImGuiBindings.cpp
├── mt/                      # mType-side library + demo
│   ├── Sdl.mt
│   ├── ImGui.mt
│   └── demo.mt
├── vendor/
│   ├── SDL3/                # https://github.com/libsdl-org/SDL (--depth=1)
│   └── imgui/               # https://github.com/ocornut/imgui --branch docking
└── PLUGIN_NOTES.md          # internal: handle registry, phase roadmap
```

All three vendor directories are **git submodules**. Initialize them after cloning the repo:

```
git clone https://github.com/<you>/mtype-imgui-sdl.git
cd mtype-imgui-sdl
git submodule update --init --recursive
```

Or in one shot:

```
git clone --recursive https://github.com/<you>/mtype-imgui-sdl.git
```

Submodule sources:

| Path           | URL                                       | Notes |
|----------------|-------------------------------------------|---|
| `vendor/SDL3`  | https://github.com/libsdl-org/SDL.git     | Default branch (SDL3) — shallow checkout. |
| `vendor/imgui` | https://github.com/ocornut/imgui.git      | `docking` branch — full checkout (shallow + branch combo isn't reliably supported by `git submodule add`). |
| `vendor/stb`   | https://github.com/nothings/stb.git       | Shallow. We only consume `stb_image.h`. |

To update a submodule to its latest commit on the configured branch:

```
git -C vendor/SDL3  pull origin main
git -C vendor/imgui pull origin docking
git -C vendor/stb   pull origin master
git add vendor/SDL3 vendor/imgui vendor/stb && git commit -m "bump vendors"
```

## Build

CMake handles everything — SDL3 builds as a sub-project, ImGui sources
compile into the plugin's TU, and `SDL3.dll` is copied next to the
plugin's output on Windows.

```
cmake -B build -A x64                          (Windows)
cmake -B build -DCMAKE_BUILD_TYPE=Release      (Linux/macOS)
cmake --build build --config Release --parallel
```

Output:

| OS | Path |
|---|---|
| Windows | `build/Release/mtype_sdl_imgui.dll` (+ `SDL3.dll` next to it) |
| Linux   | `build/mtype_sdl_imgui.so` (+ `libSDL3.so.0` next to it) |
| macOS   | `build/mtype_sdl_imgui.dylib` (+ `libSDL3.0.dylib` next to it) |

## Run the demo

The demo loads the plugin from a literal CWD-relative path. Easiest setup:

1. Copy the build artifacts (`mtype_sdl_imgui.*` + the SDL3 shared lib)
   next to your `mType.exe`, or wherever your CWD will be when you run.
2. From that CWD:

```
mType.exe path\to\mtype-imgui-sdl\mt\demo.mt
```

You should see a 1280×720 window with a click counter, "Click me", and
"Quit" buttons. The default `demo.mt` looks for the plugin at
`./bin/mType/Release/x64/mtype_sdl_imgui.dll` — adjust to wherever you
placed the DLL relative to your CWD.

## ABI sync

`include/PluginHostApi.h` is a snapshot of the engine's plugin C ABI.
The header declares `MTYPE_PLUGIN_ABI_VERSION` (currently `2`); if the
engine bumps this, the plugin must be recompiled against the new header
or the loader will reject it with `PluginError: 'X' register returned 1`.

When the engine bumps:
1. Replace `include/PluginHostApi.h` with the new copy from
   `mType/plugin/PluginHostApi.h` in the engine repo.
2. Rebuild.
3. If the vtable layout grew, your existing bindings still work —
   `MTypePluginHost`'s new fields are just unused.

## Naming convention

mType's compiler validates every called function against its registry at
compile time. Plugin natives are registered at **runtime** by
`__plugin_load`, after compilation — so they wouldn't pass the check.
The engine's escape hatch is the `__native__` prefix: any name with this
prefix is treated as runtime-resolved and skipped from compile-time
existence checks.

That's why every native registered here uses
`__native__sdl_*` / `__native__imgui_*`. If you add new bindings, follow
the same convention or scripts that call them won't compile.

## Adding bindings

The core file pattern (see `src/SdlBindings.cpp` for full examples):

```cpp
MTypeValue* nMyOp(void*, MTypeContext* ctx,
                   const MTypeValue* const* args, int argc) {
    if (!requireArgs(ctx, argc, /*expected=*/N, "__native__module_op")) {
        return g_host->makeVoid(ctx);
    }
    // Use g_host->getInt/getFloat/getString to read args.
    // Use g_windows.find / g_renderers.find for handle lookups.
    // Call SDL3 / ImGui.
    // Return via g_host->makeXxx.
}

// In registerXxxNatives:
reg("__native__module_op", &nMyOp);
```

Then add a wrapper in `mt/Sdl.mt` or `mt/ImGui.mt`. See `PLUGIN_NOTES.md`
for the phase roadmap (mouse/keyboard, layout, textures, audio).

## License

This repo's source is MIT. SDL3 is zlib. ImGui is MIT. See each
vendored project's LICENSE file.
