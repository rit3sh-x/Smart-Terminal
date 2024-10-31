# Smart Terminal
## _A multi functional File Explorer_


✨It is a UNIX  / LINUX ased, File Explorer.✨

## Features

- Basic File Explorer features like, Creating files and Folders.
- Text Editor, with basic features.
- Torrent Downloader.
- File searching and Autocomplete.
- P2P File sharing and receiving using TCP Sockets.


> This repository is under development at the moment,
> we've built a UI, using curses module.
> There may be errors in this Repository,
> thank you for trying this repository.


## Tech

Our Project uses a number of external C++ Libraries to work properly:

- Ncurses - UI Library for C++.
- LibTorrent - Torrent features implementation.
- OpenSSL - Hashing and Encryption purposes.


## Installation

This Repository Requires C++ to run, (Ideally C++17).

Install the dependencies and compile.

```sh
sudo apt update
sudo apt install libssl-dev libtorrent-dev libncurses5-dev libncursesw5-dev
```

For Compiling...

```sh
g++ -o main main.cpp -lncurses -lboost_system -lboost_filesystem -ltorrent-rasterbar -pthread -lssl -lcrypto --std=c++17
```

## Commands

These are commands for the Program...
- File Explorer
```sh
Enter - Enters Directories / Edit Files / Start Torrent Download
q - Quit
```

- Text Editor
```sh
i - Insert Mode
esc - Command Mode
!q - Quit Without Saving
!wq - Quit With Saving (Saving Create version Copy)
Ctrl+H - See Version History
```
- Torrent Downloading
```sh
Enter - Entering on .torrent file start download
esc - Exit with Pause
```
- File Sharing
```sh
Ctrl+S - Send selected file
Ctrl+R - Receive in Current directory
```
> Use session ID given by receiver to establish connection. Make sure theres no firewall and both Sender and Receiver are connected to same Network.

- File Searching
```sh
Ctrl+F - Search for files in Current directory
```