# Explorer Dock

Explorer Dock is a Windows-only plugin for OBS Studio that embeds a native Windows Explorer browser directly into an OBS dock.

It is designed for creators who want fast access to recordings, clips, thumbnails, project files or other folders without leaving OBS.

## Features

- Native Windows Explorer view inside OBS Studio
- Create multiple independent Explorer docks
- Back, forward, parent-folder and refresh navigation
- Editable path bar
- Native Windows file icons, thumbnails and context menus
- Drag and drop support through the Windows Shell view
- Remembers the last folder used by each dock
- German and English UI strings
- Built for OBS Studio 32.x on Windows x64

## Requirements

- Windows 10 or Windows 11, 64-bit
- OBS Studio 32.x, 64-bit

Other OBS versions may work, but are not currently guaranteed.

## Installation

1. Download the latest `Explorer-Dock-vX.Y.Z-Windows-x64.zip` release.
2. Close OBS Studio completely.
3. Extract the contents of the ZIP into your OBS Studio installation directory.
4. Confirm that the files end up in paths similar to:

```text
obs-plugins\64bit\obs-explorer-dock.dll
obs-plugins\64bit\obs-explorer-dock.pdb
data\obs-plugins\obs-explorer-dock\manifest.json
data\obs-plugins\obs-explorer-dock\locale\de-DE.ini
```

5. Start OBS Studio.
6. Open `Tools -> Windows Explorer Docks...`.
7. Create a dock, choose a start folder and click `Save & Apply` / `Speichern & übernehmen`.

## Usage

Each dock behaves like a lightweight Windows Explorer embedded in OBS. You can navigate folders, double-click files, use the Windows context menu and drag files to other applications where supported.

The path field at the top can also be edited manually. Press Enter or click `Open` / `Öffnen` to navigate to that path.

## Known limitations

- Windows only.
- The embedded file view is provided by the Windows Shell (`IExplorerBrowser`), so its visual appearance follows Windows rather than the active OBS theme.
- The plugin creates its own Explorer browser view. It does not embed an already-open Windows Explorer window or tab.
- Runtime behavior can vary slightly between Windows versions because parts of the UI are provided by the operating system.

## Reporting bugs

Please open a GitHub issue and include:

- OBS Studio version
- Windows version
- Explorer Dock version
- relevant OBS log lines
- steps to reproduce the problem

## AI-assisted development disclosure

Development of Explorer Dock was substantially assisted by OpenAI ChatGPT. AI assistance was used for C++ implementation, build-system setup, debugging, packaging and documentation. The resulting code was iteratively compiled and tested through GitHub Actions and manually tested in OBS Studio by the project owner.

## License

Explorer Dock is free software licensed under the GNU General Public License version 2. See [LICENSE](LICENSE).

## Disclaimer

Explorer Dock is an independent community project and is not affiliated with or endorsed by OBS Project.
