The following are the commands for MSYS2 - UCRT64 instances on Windows for building this operating system:

Download the official installer from https://www.msys2.org/ 

```bash
pacman -Syu
```
Note: If the terminal asks to close the window to complete update, do it and reopen the terminal.

```bash
pacman -S mingw-w64-ucrt-x86_64-arm-none-eabi-gcc binutils
pacman -S mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-qemu git
```

After installation, verify the tools:

```bash
# Check compiler version
arm-none-eabi-gcc --version

# Check QEMU for AArch64
qemu-system-aarch64 --version
```

Now check the directory structure in the repository: We have a directory called

```bash
AetherWebOS
```

This is the work directory with all the source code. We do all the work in this repository.