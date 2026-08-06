# Soundboard

C++ soundboard for Windows.
Note: app requires Vcredits to work.
## Features
- Plays audio to mic and headphones simultaneously
- Independent volume sliders
- Global hotkeys
- Hotkey pass-through toggle

## Installation
The application is available in two formats from the latest release:

**Standard Installer:**
1. Download and run `Soundboard_Setup.exe`.
2. Follow the installation wizard.
*(Settings are saved to `%APPDATA%\Soundboard` and audio files are loaded from your `Music\Soundboard` folder).*

**Portable Version:**
1. Download and extract `Soundboard_Portable.zip`.
2. Run `Soundboard.exe` directly from the extracted folder.
*(Settings and audio files are kept entirely within the extracted folder).*

## Usage
1. Put audio files in a `sounds/` folder next to the executable, or click `ADD SOUND`.
2. Click `EDIT HOTKEYS: OFF` to turn it ON. Click a sound button. Press a key combination. Click Save. Turn Edit Mode OFF.
3. Install a Virtual Audio Cable (e.g., VB-Cable).
4. Select `CABLE Input` from the dropdown in the app.
5. Set your microphone input in other apps to `CABLE Output`.

## Build
```cmd
cmake .
cmake --build .
```

## Credits
- [miniaudio](https://github.com/mackron/miniaudio) - Audio playback and routing library.
