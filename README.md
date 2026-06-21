## ShadowPlay Codec Replacer
Audio codec replacer for NVIDIA ShadowPlay, allowing to get video recordings
with audio bitrate above 192 kbps.


## Demonstration
![alt text](https://raw.githubusercontent.com/Mugnum/ShadowPlayCodecReplacer/refs/heads/master/media/MediaInfo-1.png)
![alt text](https://raw.githubusercontent.com/Mugnum/ShadowPlayCodecReplacer/refs/heads/master/media/MediaInfo-2.png)


## Installation
- Download [latest release](https://github.com/Mugnum/ShadowPlayCodecReplacer/releases) and extract it's contents
to a permanent location, such as `%LOCALAPPDATA%\ShadowPlayAudioCodecReplacer\ShadowPlayAudioBridge.dll`.
- Install [WindHawk](https://windhawk.net/).
- Create new WindHawk mod: [shadowplay-audio-codec-replacer](https://github.com/Mugnum/ShadowPlayCodecReplacer/blob/master/scripts/shadowplay-audio-codec-replacer.hw.cpp).
- Compile and enable the mod.
- Restart nvcontainer.exe: [PowerShell script](https://github.com/Mugnum/ShadowPlayCodecReplacer/blob/master/scripts/Stop-NvContainer.ps1).


## Configuration
![alt text](https://raw.githubusercontent.com/Mugnum/ShadowPlayCodecReplacer/refs/heads/master/media/Windhawk-Settings.png)


## Third-party components
This project includes FDK-AAC headers and libfdk-aac-2.dll, which are
licensed separately. See [THIRD-PARTY-NOTICES.md](https://github.com/Mugnum/ShadowPlayCodecReplacer/blob/master/THIRD-PARTY-NOTICES.md)
and [LICENSE-FDK-AAC.txt](https://github.com/Mugnum/ShadowPlayCodecReplacer/blob/master/third-party/fdk-aac/LICENSE-FDK-AAC.txt).
