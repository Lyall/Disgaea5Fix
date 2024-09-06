# Disgaea 5 Complete Fix
[![Patreon-Button](https://github.com/user-attachments/assets/ef86cee1-222a-444c-a2c0-709578a15732)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/Disgaea5Fix/total.svg)](https://github.com/Lyall/Disgaea5Fix/releases)

This is a fix for Disgaea 5 Complete that adds ultrawide/narrower support.

## Features
- Adds support for non-16:9 resolutions.
- Correct aspect ratio at any resolution.
- Correct FOV at any resolution.
- Centered and scaled 16:9 HUD.

## Installation
- Grab the latest release of Disgaea5Fix from [here.](https://github.com/Lyall/Disgaea5Fix/releases)
- Extract the contents of the release zip in to the the game folder.<br />(e.g. "**steamapps\common\Disgaea 5 Complete**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the launch options.

## Configuration
- See **Disgaea5Fix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

## Screenshots

| ![ezgif-3-14e41dced4](https://github.com/user-attachments/assets/12c66131-19bf-490c-9648-193c00f80a36)  |
|:--:|
| Gameplay |

## Credits
Thanks to Alex Fierro for commissioning this fix! <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
