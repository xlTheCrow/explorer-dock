# Changelog

All notable changes to Explorer Dock are documented here.

## [0.2.3] - 2026-10-06

### Added
- Native Qt toolbar icons for Back, Forward, Up and Refresh.
- Accessible names and tooltips for navigation controls.
- Versioned Windows release package naming.

### Changed
- Public project name changed to Explorer Dock.
- Windows package layout now matches the OBS directory structure directly:
  - `obs-plugins/64bit/`
  - `data/obs-plugins/obs-explorer-dock/`
- Improved German and English interface labels.
- Added built-in German fallback strings so raw localization keys are never shown if locale loading fails.
- Improved width of the Open / Öffnen button.

### Fixed
- OBS 32 plugin loading compatibility.
- Duplicate OBS module symbols during linking.
- Qt signal connection compilation issues under MSVC.

## [0.2.2]
- Added robust UI localization fallbacks.

## [0.2.1]
- Polished German and English UI labels.

## [0.2.0]
- Updated build baseline for OBS Studio 32.x and added OBS 32 plugin metadata.

## [0.1.0]
- Initial development build.
