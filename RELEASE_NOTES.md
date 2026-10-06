# Explorer Dock v0.2.3

First public release of Explorer Dock.

## Highlights

- Embed a native Windows Explorer browser directly into OBS Studio.
- Create multiple independent Explorer docks.
- Back, forward, parent-folder and refresh navigation.
- Editable path bar with Open action.
- Native Windows icons, thumbnails, context menus and drag-and-drop behavior.
- Remembers the last folder used by each dock.
- German and English UI support.
- Built for OBS Studio 32.x on Windows x64.

## Installation

1. Close OBS Studio.
2. Download `Explorer-Dock-v0.2.3-Windows-x64.zip`.
3. Extract the ZIP into your OBS Studio installation directory so that `obs-plugins` and `data` merge with the existing folders.
4. Start OBS Studio.
5. Open `Tools -> Windows Explorer Docks...` and create your dock.

## Known limitation

The embedded file view is provided by the Windows Shell and therefore follows Windows styling rather than the active OBS theme.

## AI-assisted development disclosure

Development was substantially assisted by OpenAI ChatGPT for implementation, debugging, build setup, packaging and documentation. Builds were validated through GitHub Actions and functionality was manually tested in OBS Studio by the project owner.
