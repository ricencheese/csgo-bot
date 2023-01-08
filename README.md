# slipperygg project 
[![C++](https://img.shields.io/badge/language-C%2B%2B-%23f34b7d.svg?style=plastic)](https://en.wikipedia.org/wiki/C%2B%2B) 
[![CS:GO](https://img.shields.io/badge/game-CS%3AGO-yellow.svg?style=plastic)](https://store.steampowered.com/app/730/CounterStrike_Global_Offensive/) 
[![Windows](https://img.shields.io/badge/platform-Windows-0078d7.svg?style=plastic)](https://en.wikipedia.org/wiki/Microsoft_Windows) 
[![x86](https://img.shields.io/badge/arch-x86-red.svg?style=plastic)](https://en.wikipedia.org/wiki/X86) 
[![License](https://img.shields.io/github/license/ricencheese/slipperygg.svg?style=plastic)](LICENSE)
[![Issues](https://img.shields.io/github/issues/ricencheese/slipperygg.svg?style=plastic)](https://github.com/ricencheese/slipperygg/issues)
<br>![Windows](https://github.com/ricencheese/slipperygg/workflows/Windows/badge.svg?branch=master&event=push)
![Linux](https://github.com/ricencheese/slipperygg/workflows/Linux/badge.svg?branch=master&event=push)

## Features
*   **█████bot** - Customisable █████/████████ ███bot.
*   **███████bot** - Shoots automatically when your crosshair is on someone.
*   **█████████** - ████ ██ ███ ██████ █████.
*   **████ ███** - Useful against other ███████s.
*   **Visuals** - Visual advantages.
*   **██████████ ███████** - ███ ██████ █████, ██████, ██████, ███ ████████ ███ ████ ██ ████ █████████! ████ ██ ██████-█████ ████.
*   **█████** - ████ ██ ████████ ███ ████████ ██████ ██ ███████?
*   **█████** - Menu style, layout, colours.
*   **Misc** - Other features.
*   **Config** - JSON config system.

### Build the cheat yourself!
Microsoft Visual Studio 2022 is required in order to compile slipperygg.
Alternatively, you can compile it for Linux to get an injectable `.so`. Just run the code below in a terminal to get it working.

    sudo apt-get update && sudo apt-get install -y libsdl2-dev libfreetype-dev clang++-12 g++-10 && cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_CXX_COMPILER=clang++-12 -D BUILD_TESTS=1 -S . -B Release && cmake --build Release -j $(nproc --all)

Change instruction set to `AVX` / `AVX2` / `AVX-512` / `SSE2` if your CPU supports either.
slippery.gg is by default set to `SSE2` instruction set for compatibility with majority of CPUs.

### Configuration Files
The config files are stored in `Documents\slippery.gg\`

## Acknowledgments
*   [ocornut](https://github.com/ocornut) and [contributors](https://github.com/ocornut/imgui/graphs/contributors) for creating and maintaining an amazing GUI library - [Dear imgui](https://github.com/ocornut/imgui).
*   [Zer0Mem0ry](https://github.com/Zer0Mem0ry) - For great tutorials on reverse engineering and game hacking.

## License
`> Copyright (c) 2022 ricencheese, bluebewwy` 
This project is licensed under the [MIT License](https://opensource.org/licenses/mit-license.php) - see the [LICENSE](https://github.com/ricencheese/slipperygg/blob/master/LICENSE) file for details.

## From the creators of the initial cheat:
*   [Osiris](https://github.com/danielkrupinski/Osiris) - A free open-source cross-platform stream-proof cheat for CS:GO.
*   [Anubis](https://github.com/danielkrupinski/Anubis) - Another free open-source cheat for CS:GO.
*   [GOESP](https://github.com/danielkrupinski/GOESP) - Yet again another free open-source cross-platform stream-proof cheat for CS:GO.
