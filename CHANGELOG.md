# Changelog

All notable changes to DecryptEACPayload will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2024-01-10

### Added
- **Web-based Module Downloader**: Implemented support for downloading EAC modules from multiple platforms, expanding compatibility beyond local extraction
- **Download Manager**: Introduced a robust download management system with progress tracking and error handling
- **Interactive Console Menu**: Added a user-friendly console interface for easier navigation and feature selection
- **Game Configuration System**: New JSON-based configuration system for managing game-specific settings and patterns

### Fixed
- **Pattern Scanner**: Resolved critical issues with pattern scanning algorithm, improving accuracy and reliability
- **Driver Decryption**: Fixed decryption routines for newer EAC driver versions, ensuring compatibility with latest anti-cheat implementations

### Changed
- **Extraction Methods**: Enhanced extraction capabilities to support both disk-based and web-based module retrieval
- **Code Architecture**: Major refactoring for improved maintainability, modularity, and performance
  - Separated concerns into dedicated modules (ConsoleUtils, GameConfig, ModuleDownloader)
  - Improved error handling and logging throughout the application
  - Enhanced memory management and resource cleanup

### Technical Improvements
- Restructured project architecture with clear separation of concerns
- Improved cross-platform compatibility
- Enhanced error messages and user feedback
- Optimized memory usage during large file operations

## [1.0.0] - Initial Release

### Features
- Basic EAC payload decryption
- Local module extraction
- Simple pattern scanning
