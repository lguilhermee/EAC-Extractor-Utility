![EAC Decrypt and Extract Utility](https://github.com/lguilhermee/EAC-Extractor-Utility/blob/main/logo.png)
# EAC Decrypt and Extract Utility


This project provides a utility to decrypt and extract files from an EAC (Easy Anti-Cheat) binary. The utility is designed to run on Windows.

## Requirements

- Windows Operating System
- Download EAC.Bin from EAC

## Description

This utility performs the following steps:

1. Loads an encrypted file specified as a command-line argument.
2. Decrypts the loaded file using a custom decryption algorithm.
3. Retrieves the current directory path.
4. Saves the decrypted file with a specific name in the current directory.
5. Loads the decrypted file as a library.
6. Searches for specific patterns within the loaded library to determine the start and size of data.
7. Copies encrypted data based on the identified patterns into separate vectors.
8. Decrypts the data in the vectors.
9. Saves the decrypted data as separate files in the current directory.

## Usage

Command-line usage: `EAC_Decrypt_Extract.exe <EAC.Bin>`

## Obtaining the EAC.Bin

You can download the EAC.Bin from the EAC servers. To do this, you will need to use an HTTP Debugger to acquire the link. The link format typically looks like this:

`https://modules-cdn.eac-prod.on.epicgames.com/modules/67d7e4253ad3477497a2ff44ddbd3f98/0ccb68be6228412ab45962992f0f6e7e/`


In this example:
- `67d7e4253ad3477497a2ff44ddbd3f98` is the gameid
- `0ccb68be6228412ab45962992f0f6e7e` is the uuid

Please note that these ids will differ for each game.

## Overview of EAC Operation

Easy Anti-Cheat (EAC) operates through a series of sequential steps:

1. EAC begins by executing the Launcher, whose primary responsibility is downloading and decrypting the EAC.Bin. It's important to note that the EAC.Bin already contains the Launcher, the usermode DLL, and the driver.sys.
2. The decrypted binary is loaded into memory where it performs several initialization checks. During this stage, EAC decrypts both the EAC_Usermode.dll and EAC_Driver.sys.
3. Following this, the usermode launcher loads the driver.sys.
4. The driver performs additional checks. If these checks pass, the driver loads the usermode DLL into the process by mapping it.
5. EAC continues to run until it is terminated.

This simplified description is intended to provide a general understanding of the process. The actual operation of EAC involves complex and advanced measures.

## Results

The utility generates the following files as a result of its operation:

1. EAC_Launcher.dll
2. EAC_Driver.sys
3. EAC_UserMode.dll

## Acknowledgments

Special thanks to [iPower](https://github.com/iPower) for their valuable insights.

---

Remember to always use this tool responsibly and respect the Terms Of Service of the games and services you are interacting with.
