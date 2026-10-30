# Tag Marker Atlas Fix

A small, standalone RED4ext plugin for the resolution-dependent tagging-marker bug in Cyberpunk 2077.

At 4K, an entity can remain highlighted through walls while its tag arrow disappears. The game still considers the entity tagged, but its marker can become stranded in `WorldMappins_QueueHolder`. This plugin enlarges the native Stealth marker atlas from **1024 x 1024** to **2048 x 2048**.

The game's request for **32 marker slots** is unchanged. This is an atlas-capacity fix, not unlimited tagging.

## Requirements and compatibility

- Windows x64 and RED4ext.
- Cyberpunk 2077 **2.50**, executable file version **3.0.88.46235**.
- Only the verified executable build in [docs/verified-build.json](docs/verified-build.json) is accepted.
- No CET or Lua mod is required.

## Installation

1. Close the game completely.
2. Extract the package's `red4ext` directory into your game directory.
3. Confirm this file exists:

   ```text
   red4ext/plugins/TagMarkerAtlasFix/TagMarkerAtlasFix.dll
   ```

4. Start the game. The RED4ext plugin log should contain:

   ```text
   Tag Marker Atlas Fix patch applied: Stealth 1024x1024 -> 2048x2048; requested slots remain 32.
   ```

To uninstall, close the game and remove `red4ext/plugins/TagMarkerAtlasFix`. The executable on disk is never modified.

## What the code does

- Checks the loaded executable's architecture, PE timestamp and image size.
- Searches the executable code section for exactly one verified instruction sequence and checks its RVA.
- Changes only two float immediates: the Stealth atlas width and height.
- Flushes the instruction cache and restores the previous page protection.
- Attempts to restore the original constants when the plugin unloads. It refuses to overwrite unexpected changes.

The DLL uses the public RED4ext plugin API and Windows' Kernel32 API. It has no C++ runtime dependency. It does not read saves, open another process, access the network, or write the game executable.

See [docs/technical-notes.md](docs/technical-notes.md) for the diagnosis and exact patch site.

## Build from source

Use **64-bit Windows**, Python **3.10 or newer**, and a Windows GNU-target C compiler such as Embarcadero `bcc64x` or MinGW-w64 GCC/Clang. The build was verified with `bcc64x` 7.80 / Clang 20.1.7. Other compatible compilers are supported by the command-line interface but have not yet been validated in this project. MSVC `cl` is not currently supported by this build script.

From the repository root:

```powershell
python build.py --compiler "C:\path\to\bcc64x.exe"
```

The script can also locate `bcc64x` on PATH or in its standard RAD Studio 37 installation directory.

The build runs **45 core checks** and **10 DLL integration tests** before creating:

```text
build/TagMarkerAtlasFix.dll
dist/TagMarkerAtlasFix-0.1.0.zip
```

The ZIP contains the DLL, README, licenses, attribution and a manifest with the DLL's SHA-256. No compiled binaries are tracked in Git.

The integration tests run the actual DLL against a synthetic image in the test's own process. They do not open or modify a running game. The build works from the sources in this repository; no other mod checkout or game files are needed.

## Validation and limits

The original reproduction used 16 automatically tagged NPCs plus one manually tagged camera:

| Configuration | Observed camera behavior |
| --- | --- |
| 3840 x 2160, original atlas | Marker can remain in the queue holder after untag/retag |
| 1920 x 1080, original atlas | Marker returns to the visible Stealth window |
| 3840 x 2160, enlarged atlas | Repeated untag/retag works; trace confirms the visible Stealth window |

This verifies that reproduction. It does not establish compatibility with every scene, device, resolution or game update. The separate standard-marker atlas and native tagging logic are unchanged. The requested Stealth marker count remains 32.

## Project layout

- `src/`: complete plugin and byte-patch implementation, with a minimal C translation of the public RED4ext API declarations.
- `tests/`: core tests and synthetic DLL lifecycle tests.
- `docs/`: diagnosis and verified build metadata.
- `third_party/`: retained RED4ext.SDK license.

## License

Project code is licensed under [MIT](LICENSE), copyright 2026 Shaydera. The translated public RED4ext.SDK declarations retain their separate [MIT attribution](third_party/RED4ext.SDK.LICENSE.txt); see [NOTICE](NOTICE).
