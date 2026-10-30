# Native atlas investigation

## Observed failure

The internal tag list and native `IsTaggedinFocusMode` flags can both contain 17 tagged entities. The camera's Stealth controller also reports a visible tag widget, nonzero scale and opacity 1.0. In the failing 4K reproduction, the root widget instead remains parented to `WorldMappins_QueueHolder` after a manual untag/retag. Its intended visible parent is `StealthMappins_VirtualWindow`.

The same test at 1920 x 1080 works without removing an NPC tag. At 3840 x 2160, enlarging only the Stealth atlas restores repeated manual tagging. This identifies atlas exhaustion as the cause of the reported reproduction, rather than a fixed limit of 16 internally stored tags.

## Native parameters

The verified build's Stealth pool creation requests 32 slots, an atlas of 1024 x 1024 pixels, and logical tiles of 256 x 256. Its virtual window is sized for a 3840 x 2160 coordinate space. Slot rectangles are scaled against the output size. Allocation stops when the atlas cannot fit another rectangle.

At 4K, a 1024-pixel square fits 4 x 4 tiles of 256 pixels: 16 slots. At 1080p the scaled tiles are smaller. The 2048-pixel test atlas has enough space for the 32 requested slots at 4K. It is not an unlimited-marker implementation and does not establish an arbitrary-resolution guarantee.

## Verified code location

- Game: 2.50, file version 3.0.88.46235.
- PE timestamp: `0x6ad55daf`; image size: `0x581b000`.
- Stealth pool parameters: RVA `0x2213510`.
- Verified instruction sequence: RVA `0x221352e`.
- Width immediate: RVA `0x2213531`.
- Height immediate: RVA `0x2213538`.
- Original immediate bytes: `00 00 80 44` (1024.0f).
- Patched immediate bytes: `00 00 00 45` (2048.0f).

`src/atlas_patch.c` verifies the surrounding opcodes and the unchanged 256-pixel tile constants before writing either value. `src/plugin.c` additionally checks the executable build, executable section, unique signature and expected RVA. The patch is applied to the current process' loaded executable; there is no disk patcher.

The request for 32 slots and the standard-marker pool remain unchanged. No original game executable, asset or full disassembly is distributed with this repository.

## Validation boundaries

The core tests cover exact patching/restoration, duplicate and missing matches, truncated buffers and unexpected changes. The DLL tests verify the exported API, actual page-protection restoration, strict build checks, unexpected RVAs, repeated loading and preservation of foreign modifications. The import redirection used by the tests is confined to the test DLL inside the Python test process.

The successful game test was one 4K camera/NPC reproduction. Future executable builds must be investigated and tested separately. Do not remove the build checks merely to make an unknown game version load.
