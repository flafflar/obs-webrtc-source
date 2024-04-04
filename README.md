# obs-webrtc-source
Have anyone stream their screen directly to OBS from their browser,
with no extra installs needed.

This plugin creates an HTTP server that anyone in the same local network can
access. The website it serves uses [WebRTC](https://webrtc.org/), an API for
streaming audio and video that is included in all major browsers since 2018. The
plugin receives the stream from someone's browser and displays it in OBS.

## Installation
Download the zip files for your operating system at
[Releases](https://github.com/flafflar/obs-webrtc-source/releases).

Unzip the appropriate file for your operating system at OBS's plugin directory.
You can find that directory in
[OBS Plugins Guide](https://obsproject.com/kb/plugins-guide).

## Bugs
This plugin is still in beta, so bugs are expected to exist. If you find a bug,
please report it in [Issues](https://github.com/flafflar/obs-webrtc-source/issues).