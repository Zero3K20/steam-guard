# tf is this?
steam-guard is a simple tool based on [SteamAuth](https://github.com/geel9/SteamAuth) that can be used to generate Steam Guard codes.

I made this because
1. I was learning C.
2. I was bored.
3. I spent a few hours trying to compile [SDA](https://github.com/Jessecar96/SteamDesktopAuthenticator) on Linux (to no avail) and got upset.
4. I needed a very simple way of getting Steam Guard codes and writing a node.js script wasn't an option.

# Compiling
1. Make sure you have `make`, `gcc, `libcurl4-openssl-dev` and `libssl-dev` installed
2. Run `make`

## Visual Studio 2019 (Windows)
1. Open `steam-guard.sln` in Visual Studio 2019.
2. Build the `steam-guard` project (`Debug` or `Release`, `x86` or `x64`).
3. The Windows build uses WinHTTP for server-time requests and legacy-compatible Windows crypto APIs instead of cURL/OpenSSL.
4. The Visual Studio project is configured to target Windows XP/Server 2003 (and newer) APIs.
5. Use `x86` builds on 32-bit Windows XP/Server 2003 systems (`x64` builds require 64-bit Windows).
