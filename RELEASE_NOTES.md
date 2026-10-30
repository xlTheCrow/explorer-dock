# Explorer Dock v0.2.4

This release provides two separate Windows x64 packages so users can choose the package that matches their OBS Studio major version.

## Downloads

- **OBS Studio 32.x:** `Explorer-Dock-v0.2.4-OBS32-Windows-x64.zip`
- **OBS Studio 33.0.x:** `Explorer-Dock-v0.2.4-OBS33-Windows-x64.zip`

Do not install both packages at the same time.

## Highlights

- Separate OBS 32.0.2 and OBS 33.0.1 build targets.
- OBS 32 package retains the legacy `obs-plugins/64bit` and `data/obs-plugins` folder structure.
- OBS 33 package uses the new `plugins/obs-explorer-dock` folder structure.
- Independent CI builds and versioned ZIP packages for both OBS targets.
- Native Windows Explorer browser docks, multiple docks, navigation, thumbnails, context menus, drag-and-drop and saved folder locations remain available.

## Installation

### OBS Studio 32.x

Extract the contents of the OBS32 ZIP into your OBS Studio installation directory.

### OBS Studio 33.x

Copy the complete `obs-explorer-dock` folder from the OBS33 ZIP into:

`%ProgramData%\obs-studio\plugins`

See the README for detailed installation guidance.

## Compatibility note

The OBS 33 package is compiled against OBS Studio 33.0.1. CI validates the build and package layout; manual runtime testing in OBS 33 is still required before claiming full runtime verification. OBS 32 package is built against OBS Studio 32.0.2.

## Known limitation

The embedded file view is provided by the Windows Shell and therefore follows Windows styling rather than the active OBS theme.

## AI-assisted development disclosure

Development was substantially assisted by OpenAI ChatGPT for implementation, debugging, build setup, packaging and documentation.
