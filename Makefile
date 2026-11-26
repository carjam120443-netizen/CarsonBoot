ARCH := x86_64
CC := gcc
LD := ld
OBJCOPY := objcopy

EFI_INCLUDE ?= /usr/include/efi
EFI_LIB ?= /usr/lib
EFI_CRT ?= $(EFI_LIB)/crt0-efi-$(ARCH).o
EFI_LDS ?= $(EFI_LIB)/elf_$(ARCH)_efi.lds

CFLAGS := -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) \
          -fpic -ffreestanding -fno-stack-protector -fno-stack-check \
          -fshort-wchar -mno-red-zone -maccumulate-outgoing-args \
          -Wall -Wextra -Werror
ASFLAGS := -fpic -mno-red-zone
LDFLAGS := -shared -Bsymbolic -L$(EFI_LIB) -T$(EFI_LDS) -znocombreloc

BUILD_DIR := build
OBJECTS := $(BUILD_DIR)/boot.o $(BUILD_DIR)/cpuid.o

.PHONY: all clean check-deps

all: check-deps $(BUILD_DIR)/BOOTX64.EFI

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.o: src/boot.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cpuid.o: src/arch/x86_64/cpuid.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/carsonboot.so: $(OBJECTS)
	$(LD) $(LDFLAGS) $(EFI_CRT) $(OBJECTS) -lefi -lgnuefi -o $@

$(BUILD_DIR)/BOOTX64.EFI: $(BUILD_DIR)/carsonboot.so
	$(OBJCOPY) \
		-j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $< $@

check-deps:
	@test -f "$(EFI_CRT)" || (echo "Missing GNU-EFI CRT object: $(EFI_CRT)"; exit 1)
	@test -f "$(EFI_LDS)" || (echo "Missing GNU-EFI linker script: $(EFI_LDS)"; exit 1)
	@command -v $(CC) >/dev/null
	@command -v $(LD) >/dev/null
	@command -v $(OBJCOPY) >/dev/null

clean:
	rm -rf $(BUILD_DIR)
