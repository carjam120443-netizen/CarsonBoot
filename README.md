# CarsonBoot
# why have a bootloader that can brick a device when you can have a better one? #

CarsonBoot is an experimental x86_64 UEFI boot manager written in C and GNU Assembly.

> **Early prototype:** this version is a UEFI diagnostic application. It does not yet load Linux kernels or boot installed operating systems.

## Technology

- C for UEFI application logic
- x86_64 GNU Assembly for architecture-specific routines
- GNU-EFI for building a PE/COFF UEFI application
- x86_64 UEFI firmware

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

The package is written to `build/deb/carsonboot_0.1.0_amd64.deb`. It installs the EFI executable to `/usr/lib/carsonboot/BOOTX64.EFI`.

**The .deb does not write to an EFI System Partition, create firmware boot entries, or change the machine's boot configuration.** It only places the file on the Linux filesystem. This is intentional while CarsonBoot is experimental.

## GitHub Actions

Every push or pull request targeting `main`, version tag matching `v*`, or manual workflow dispatch runs the UEFI build and Debian packaging steps. The workflow uploads both `BOOTX64.EFI` and the `.deb` as downloadable build artifacts.

## Test in VirtualBox

1. Create a VM using **UEFI** firmware (enable EFI in the VM settings).
2. Prepare a FAT32 EFI System Partition or FAT image.
3. Copy the application to `EFI/BOOT/BOOTX64.EFI`.
4. Attach the image to the VM and boot it.

Do not install this prototype to a physical machine's EFI System Partition. It is an early diagnostic application and does not yet provide an operating-system boot path.

## Planned milestones

1. UEFI application startup and diagnostics
2. Keyboard-driven boot menu
3. Configuration file support
4. Safe chainloading of other EFI applications
5. Linux EFI-stub kernel and initramfs support
6. OS discovery and Debian packaging

## License

GPL-3.0-only. See [LICENSE](LICENSE).
