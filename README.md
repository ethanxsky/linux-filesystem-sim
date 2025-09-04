# Linux File System Simulator

## Overview

This project is a command-line application that simulates a simplified Linux file system. It allows users to interact with a virtual file system in memory, performing basic file operations such as creating and deleting files and directories, reading and writing to files, and navigating the directory structure. The simulator provides a hands-on experience with core file system concepts like inodes, data blocks, and directory entries.

---

## Project Status

This project is a work in progress and is not yet a complete file system simulator. Basic file system functions and command-line parsing are functional, but some commands may not yet be fully implemented or may contain bugs. This simulator is not currently a fully-featured application.

---

## Features (WIP)

* **File System Management:**
  * Create a new file system with a specified number of inodes and data blocks.
  * Save the current file system to a binary file.
  * Load a file system from a saved binary file.
* **Navigation & Inspection:**
  * Change the current working directory (`cd`).
  * List the contents of a directory (`ls`).
  * Display the hierarchical tree structure of the file system (`tree`).
  * Display detailed information about the file system's structure (`fs`).
  * View available inodes and data blocks (`available`).
* **File & Directory Operations:**
  * Create new empty data files (`newfile`).
  * Create new directories (`newdir`).
  * Remove data files (`rmfile`).
  * Remove directories (`rmdir`).
* **File I/O:**
  * Write content to a file (`write`).
  * View the content of a file (`cat`).
  * Overwrite a section of a file with specific data (`patch`).
  * Fill a file with a specific byte value (`dump`).

---

## Building and Running the Simulator

### Prerequisites
* A C++ compiler (e.g., `g++`).
* `make` utility.

### Compilation
Navigate to the project's root directory and run:

```bash
cmake -S . -B build
cmake --build build
```

This will compile the source files and create an executable named `terminal` in the `build` directory.

### Running the Simulator
To start the interactive terminal, run:

```bash
./build/terminal
```

Once started, you can type `help` to see a list of all available commands and their descriptions.

---

## Commands

Below is a summary of supported commands. For more detailed information, type `help` in the terminal.

| Command | Description |
|---------|-------------|
| `new <inodes> <dblocks>` | Creates a new file system. |
| `load <path>` | Loads a file system from a binary file. |
| `save <path>` | Saves the current file system to a binary file. |
| `ls <path>` | Lists the contents of a directory. |
| `tree <path>` | Displays the file system tree. |
| `fs` | Displays detailed file system information. |
| `available` | Shows free inodes and data blocks. |
| `cd <path>` | Changes the current directory. |
| `newdir <path>` | Creates a new directory. |
| `newfile <path> <perms>` | Creates a new file. |
| `rmfile <path>` | Removes a file. |
| `rmdir <path>` | Removes a directory. |
| `write <path>` | Writes content to a file. |
| `cat <path>` | Prints file content. |
| `dump <path> <bytes>` | Dumps specific bytes into a file. |
| `patch <path> <offset> <bytes> <value>` | Overwrites a section of a file. |
| `help` | Displays the list of commands. |
| `exit` | Exits the simulator. |

---
