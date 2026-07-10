# Archipelago Build Notes

The Linux Archipelago client must be built with a fully 64-bit (`x86_64`)
audio dependency chain. Mixing a 64-bit executable with a 32-bit audio
library is known to crash during audio initialization.

Before distributing a build, verify the executable and its SDL2, Ogg,
Vorbis, Opus, and Opusfile dependencies with `file`, `readelf`, and `ldd`.
All linked audio libraries must resolve to 64-bit builds.
