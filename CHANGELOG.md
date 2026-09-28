# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-09-28

### Added

- Device pairing via ECDH key exchange with a 6-digit confirmation PIN
- Desktop notifications for incoming pairing requests on Linux
- Terminal prompt to accept or reject pairing requests
- Automatic reconnection for previously paired devices
- Temporary blocklist for rejected devices to prevent notification spam
- Display of device names in the Android app and on the PC

### Changed

- Reorganized Java and C++ projects
- Updated dependencies
- Updated and cleaned up README.md
- Connections now require device authorization before audio streaming starts
- Added OpenSSL and libnotify to the Linux system requirements
- Continuous background mDNS service discovery for more reliable connections

## [1.0.0] - 2026-06-16

### Added

- Initial release
