# Doom!
This is Doom for M5 Cardputer

Cardputer does not have PSRAM, so things get tricky

Is mostly a port of [prBoom Port to GBA](https://github.com/doomhack/GBADoom) by doomhack

HAL was taken from [M5 Official User Demo](https://github.com/m5stack/M5Cardputer-UserDemo)

Some parts collected from [Retro-Go](https://github.com/ducalex/retro-go) project

Can work with original WADs after their conversion with GbaWadUtil, also lended from doomhack.

WAD is compiled into binary itself

Built against
[ESP-IDF v4.4.6](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/index.html)

Save-load works with SD card

Save-load crashes without SD card

Sometimes crashes randomly

#### Controls
 - Forward: ;
 - Backward: .
 - Left: l
 - Right: '
 - Fire: opt
 - Strafe left: ctrl
 - Strafe right: alt
 - Use: fn
 - Map: tab
 - Weapon: 1-7

#### Cheats
 - iddqd - god mode
 - idkfa - keys, weapons, armor
 - idc<x><y> - jump to level E<x>M<x> (eg. idc14 jumps to E1M4)
   
#### Done
 - User input
 - Sounds (SFX)
 - Music
 - Save & Load
 - Optimize user input
 - Cheats!

#### To do
 - Load WAD from SD card (somehow, mmap?)
 - Fix minor music issues

#### Build

```bash
git clone https://github.com/romalik/m5cardputer_doom
```
```bash
idf.py build
```
```bash
idf.py flash -p /dev/ttyACM0
```


#### Wad Util

```bash
mkdir build
cd build
qmake ../GbaWadUtil.pro
make
cd ..
 # place doom1.wad file somewhere near here
 # gbadoom.wad is needed to cover missing assets
./build/GbaWadUtil -in doom1.wad -out gdoom1.wad -cfile doom1.c -pwad ./gbadoom.wad
 # place doom1.c to main/doom/source/iwad/ and recompile the firmware
```
