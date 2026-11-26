# CarsonBoot

CarsonBoot is an experimental x86_64 UEFI boot manager written in C and GNU Assembly.

> **Experimental software:** CarsonBoot reads UEFI Boot#### entries and attempts to chainload selected EFI applications. Firmware behavior varies. Keep a known-good firmware boot option available and test in a virtual machine first. The current build is unsigned; Secure Boot may refuse to launch it unless it is signed with a key trusted by the firmware.

## Current features

- UEFI application startup and CPU vendor diagnostics
- Reads active firmware entries from UEFI BootOrder
- Keyboard-driven boot menu (Up/Down, Enter, Esc, and 1-9)
- Five-second automatic boot using the saved CarsonBoot default selection
- Saves the selected firmware entry in a CarsonBoot UEFI variable
- Chainloads existing EFI applications through UEFI LoadImage/StartImage
- Debian package includes an opt-in UEFI installer helper

CarsonBoot does **not** yet discover operating systems by scanning disks, load Linux kernels directly, or provide a recovery shell. It chainloads EFI applications already registered in the firmware's BootOrder.

## Build dependencies

On Debian or Ubuntu:

```sh
sudo apt update
sudo apt install build-essential binutils gnu-efi dpkg-dev
```

## Build

```sh
make
```

The output EFI application is `build/BOOTX64.EFI`.

The Makefile uses the standard Debian/Ubuntu GNU-EFI paths. Override `EFI_INCLUDE`, `EFI_LIB`, `EFI_CRT`, and `EFI_LDS` if GNU-EFI is installed elsewhere.

## Build a Debian package

After building the EFI application:

```sh
bash packaging/build-deb.sh
```

The package is written to `build/deb/carsonboot_0.1.0_amd64.deb`. It installs:
- `/usr/lib/carsonboot/BOOTX64.EFI`
- `/usr/sbin/carsonboot-install-uefi`

Installing the .deb **does not** copy files to the EFI System Partition or change firmware settings. The installer is deliberately opt-in.

## Make CarsonBoot the default firmware entry

First boot Linux in UEFI mode, mount the EFI System Partition (ESP), and identify its parent disk and partition number. Install `efibootmgr` if needed.

Run a dry run first:

```sh
sudo carsonboot-install-uefi --esp-mount /boot/efi --disk /dev/sda --part 1
```

Replace the example mount, disk, and partition with the actual ESP details for the machine. The dry run validates the mount and prints the planned changes without changing anything.

Only after verifying those values, apply:

```sh
sudo carsonboot-install-uefi --esp-mount /boot/efi --disk /dev/sda --part 1 --apply
```

The helper copies CarsonBoot to `EFI/CarsonBoot/BOOTX64.EFI`, backs up an existing file at that path, creates a UEFI NVRAM entry, and places it first in BootOrder while retaining the previous entries.

**Important:** Making CarsonBoot the first firmware entry means it runs before Windows Boot Manager or GRUB. If chainloading fails, use the motherboard/VM firmware boot menu to select the original entry. Do not remove the original boot entries. The helper does not repartition or format disks, but `--apply` does write to the ESP and firmware NVRAM.

To return to the previous default, use `sudo efibootmgr -v` and restore the previous BootOrder shown by the installer.

## Test in VirtualBox first

1. Create a VM using **UEFI** firmware (enable EFI in the VM settings).
2. Use a disposable VM with a working OS EFI boot entry.
3. Build and copy `BOOTX64.EFI` to a FAT EFI System Partition under `EFI/CarsonBoot/BOOTX64.EFI`.
4. Create a firmware boot entry for that path, or use the packaged helper inside a disposable Linux VM.
5. Verify that CarsonBoot displays the existing firmware boot entries and can chainload the selected one.

Do not test the installer against the host PC's ESP first.

## Technology

- C for UEFI application logic
- x86_64 GNU Assembly for architecture-specific routines
- GNU-EFI for building a PE/COFF UEFI application
- x86_64 UEFI firmware

## License

GPL-3.0-only. See [LICENSE](LICENSE).
