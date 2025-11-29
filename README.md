# GcToolKit

![screenshot](https://git.silica.codes/Estradrive/GcToolKit/raw/branch/main/images/screenshot1.jpg)

Tool to create 1:1 backups of PSV Game Cartridges
as well as the per-cartridge keys obtained from CMD56 data.

# Features

## Backup entire gamecart
This can backup game cartridges in the following formats:  
- VCI (Vita Cartridge Image)
- PSV (PSVGameSD Format)
- IMG (Raw Image File)

## Features
Backup an entire gamecar the formats supported by this are:

- Vita Cartridge Image (.vci)
- Triimmed Vita Cartridge Image (.trim.vci)
- PSVGameSD (.psv)
- Trimmed PSVGameSD (.psv)
- Raw (.img)

it can also format, restore and backup the "grw0" & "mediaid" section of any games that use it;
these options can sometimes be used in some cases to unbrick vita games- or just to make a more overpowered backup of your save files

it is also has an option to view information about your gamecarts such as manufacturer or serial number.
![gc info](https://git.silica.codes/OEstriot/GcToolKit/raw/branch/main/images/gcinfo.jpg)

## Backup locations

GCToolKit can backup GameCarts to the following locations:

- An Offical Sony Memory Card
- PSVSD (3G modem replacement)
- A ExFAT formatted USB Drive connected to PlayStation Vita TV.
- A ExFAT formatted USB Drive connected to Vita 2000 via OTG cable.
- A ExFAT formatted USB Drive connected to Vita 1000 via Accessory Port.
- ``host0:`` on Vita Development Kit.
- A Computer listening on same local network.

Vita GameCart's are always either 2gb or 4gb in size, 
for this reason the 1gb internal storage on a Vita 2k- cannot be used.

# Network Backup
GCToolKit allows to save a VCI of a game over the local network;

to do this requires running the program "GcNetworkBackup" program running on your computer;

this feature is useful if you don't have a memory card or otherwise, do not have an avalible storage device

- [Windows](https://silica.codes/OEstriot/GcToolKit/releases/download/v1.6/GcNetworkBackup.exe) [(mirror)](https://github.com/oestriot/GcToolKit/releases/download/v1.6/GcNetworkBackup.exe)
- [Linux](https://silica.codes/OEstriot/GcToolKit/releases/download/v1.6/GcNetworkBackup) [(mirror)](https://github.com/oestriot/GcToolKit/releases/download/v1.6/GcNetworkBackup)

the source code for it is in the "pc" folder of this repoistory.

# USB OTG Backup

This program allows backup vita GCs with a USB device connected via an OTG cable

however this only works with OTG cables with an external power source; or "Y-Cable"
for example this one for the [Amazon Fire Stick](https://www.amazon.com/ANDTOBO-Micro-Adapter-Power-Devices/dp/B083M1S6QT).

![otg setup](https://git.silica.codes/OEstriot/GcToolKit/raw/branch/main/images/psvita_otg_example.jpg)

# YAMT Notice:
- This tool now supports working even with YAMT installed,
you can hot-swap a SD2VITA for a game cartridge- however doing so will require a reboot to get the sd2vita to work again.

# Credits
-  <sup>The Crystal System</sup> Li- Programming, VCI Format, Graphics, Reverse engineering, GC CMD56 Research
- olebeck - CMD56 helps
- Robots System - Selecting music, choosing port numbers, ~~emotional support~~
- Princess of Sleeping - ExFAT Format code, CMD56 helps
- dots_tb - original research into USB OTG, and Accessory Port
- EA Games 1997 - BGM Music from Dungeon Keeper 1 https://www.youtube.com/watch?v=RXfUV_z7i0c

# Difference between .vci and .psv formats (why a new format?)

The main difference is how the keys stored. 
in psvgamesd, the result of gc_auth_mgr_sm function 0x20 is stored,
this is the key required to decrypt the .RIF file
however this key is *actually* derived from the result of SHA256 hash functions
of some constants exchanged in packet20 and packet18 of gc authentication.
in .VCI the *input* to the SHA256 function are included instead.
SHA256 is a one-way function and so you cannot go backwards from 
the data captured in psvgamesd to the packet20 and packet18 constants.

main advantage is that with VCI it would be thereotically possible to create a vita flash cartridge. .

this also means that .VCI can be easily converted to .PSV, but .PSV cannot be converted back to VCI without keys
for tools to convert to/from VCI format to others, see: https://silica.codes/OEstriot/VCI-TOOLS

-- 

for better understanding, here is a complete flowchart of how NpDrm works for physical games, 
as well as a comparison between different backup formats over the years; 
and an overview on how the vita decrypts game data in general;


![gc authentication diagram](https://git.silica.codes/OEstriot/GcToolKit/raw/branch/main/images/diagram.png)

# Building

to build the plugin using VitaSDK just run:

```
mkdir build
cd build
cmake ..
make
```

you can also build it with debug logging enabled by passing ```-DCMAKE_BUILD_TYPE=DEBUG``` to the cmake command.
or you can use ```-DCMAKE_BUILD_TYPE=RELEASE``` to build a release verison,

the windows or linux version can be build with ```-DBUILD_PC=1``` 
