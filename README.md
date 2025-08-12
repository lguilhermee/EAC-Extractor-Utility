![EAC Decrypt and Extract Utility](https://github.com/lguilhermee/EAC-Extractor-Utility/blob/main/logo.png)
# EAC Decrypt and Extract Utility v2.0

A powerful Windows utility to decrypt and extract files from EAC (Easy Anti-Cheat) modules. Now featuring an interactive console menu, web-based module downloading, and support for multiple games!

## 🚀 Features

- **Interactive Console Menu**: User-friendly interface for easy navigation
- **Web-Based Module Downloader**: Download EAC modules directly without manual extraction
- **Local File Extraction**: Extract from existing EAC.bin files
- **Game Configuration System**: JSON-based configuration for multiple games
- **Enhanced Pattern Scanner**: Improved accuracy for module detection
- **Automatic File Extraction**: Extracts launcher, driver, and usermode components

## 📋 Requirements

- Windows Operating System (Windows 10/11 recommended)
- Visual C++ Redistributables
- Internet connection (for web-based downloading)

## 🎮 Usage

### Method 1: Interactive Mode (Recommended)
Simply run the executable and follow the interactive menu:
```bash
DecryptEACPayload.exe
```

The menu will guide you through:
1. Choosing between web download or local file extraction
2. Selecting a game from the configuration
3. Automatic processing and extraction

### Method 2: Command Line
For direct file extraction:
```bash
DecryptEACPayload.exe <path_to_EAC.bin>
```

## 🔍 Finding Game IDs

To add new games or use custom configurations, you'll need the `productId` and `deploymentId`:

1. **Navigate to your game's EAC folder**:
   ```
   ..\[GameName]\EasyAntiCheat\
   ```
   Example: `..\Throne and Liberty\EasyAntiCheat\`

2. **Open `Settings.json`** with any text editor

3. **Look for the configuration**:
   ```json
   {
       "deploymentid": "278b7d6708d198db6305102c77abdf50",
       "executable": "TL\\Binaries\\Win64\\TL.exe",
       "hide_bootstrapper": "true",
       "hide_gui": "true",
       "productid": "21bf5373a3f1ebc9d1cbe14de40871b0",
       "requested_splash": "EasyAntiCheat/SplashScreen.png",
       "sandboxid": "9e11c91c1a3047c8e03369eb271f6f50",
       "title": "TL",
       "wait_for_game_process_exit": "false"
   }
   ```

4. **Copy the `productid` and `deploymentid`** values

5. **Add to `games.json`** or use in the console menu

## 🤝 Contributing

We welcome contributions! You can help by:

- **Adding new games**: Submit a pull request to update `games.json` with new game configurations
- **Reporting bugs**: Open an issue on GitHub
- **Improving documentation**: Help other users with better guides

### Adding a New Game to games.json

```json
{
    "name": "Your Game Name",
    "productId": "productid_from_settings.json",
    "deploymentId": "deploymentid_from_settings.json"
}
```

Submit a pull request with your additions!

## 📁 Output Files

The utility extracts the following components:

- **EAC_Launcher.dll** - The main launcher component
- **EAC_Driver.sys** - Kernel-mode driver
- **EAC_UserMode.dll** - User-mode module

Files are saved in the current working directory or a specified output folder.

## 🔧 Technical Details

### How EAC Modules Work

Easy Anti-Cheat operates through a multi-stage process:

1. **Module Download**: The launcher downloads the encrypted EAC.bin module
2. **Decryption**: The binary is decrypted and loaded into memory
3. **Component Extraction**: Three main components are extracted:
   - Launcher DLL for initialization
   - Kernel driver
   - Usermode DLL
4. **Driver Loading**: The kernel driver is loaded for deep system integration
5. **Process Protection**: The usermode DLL is mapped into the game process

### Module Structure

Each EAC.bin contains:
- Encrypted launcher library code
- Encrypted usermode library code
- Encrypted kernel driver code

## 🙏 Acknowledgments

- Special thanks to [iPower](https://github.com/iPower) for their valuable insights
- Community contributors who maintain the game configuration list

## ⚠️ Legal Notice

This tool is for **educational and research purposes only**. Users are responsible for:
- Compliance with all applicable laws
- Respecting game Terms of Service
- Using the tool responsibly and ethically

**We do not encourage or support cheating in online games.**

---

If you find this tool useful, consider ⭐ starring the repository and contributing to the games.json list!
