#include <efi.h>
#include <efilib.h>
#include <stdint.h>

#define CB_MAX_OPTIONS 32
#define CB_OPTION_BYTES 4096
#define CB_DEFAULT_TIMEOUT 5
#define CB_VARIABLE_ATTRIBUTES (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

typedef struct {
    UINT16 number;
    CHAR16 description[96];
    UINT8 data[CB_OPTION_BYTES];
    UINTN data_size;
    UINTN device_path_offset;
} CB_BOOT_OPTION;

static EFI_GUID cb_variable_guid = {
    0x8f1d6c42, 0x7b31, 0x4a91,
    {0xa2, 0x50, 0x43, 0x42, 0x4f, 0x4f, 0x54, 0x01}
};
static CB_BOOT_OPTION cb_options[CB_MAX_OPTIONS];
static UINTN cb_option_count;

static UINT16 cb_read_u16(const UINT8 *p) {
    return (UINT16)(p[0] | ((UINT16)p[1] << 8));
}

static UINT32 cb_read_u32(const UINT8 *p) {
    return (UINT32)p[0] | ((UINT32)p[1] << 8) |
           ((UINT32)p[2] << 16) | ((UINT32)p[3] << 24);
}

static EFI_STATUS cb_read_variable(CHAR16 *name, EFI_GUID *guid,
                                   VOID *buffer, UINTN *size) {
    return uefi_call_wrapper(RT->GetVariable, 5, name, guid, NULL, size, buffer);
}

static BOOLEAN cb_is_boot_name(CHAR16 *name, UINT16 *number) {
    UINTN i;
    UINT16 value = 0;

    if (name[0] != L'B' || name[1] != L'o' || name[2] != L'o' ||
        name[3] != L't') return FALSE;

    for (i = 4; i < 8; ++i) {
        CHAR16 c = name[i];
        UINTN digit;
        if (c >= L'0' && c <= L'9') digit = (UINTN)(c - L'0');
        else if (c >= L'a' && c <= L'f') digit = (UINTN)(c - L'a' + 10);
        else if (c >= L'A' && c <= L'F') digit = (UINTN)(c - L'A' + 10);
        else return FALSE;
        value = (UINT16)((value << 4) | digit);
    }
    if (name[8] != L'\0') return FALSE;
    *number = value;
    return TRUE;
}

static BOOLEAN cb_add_boot_option(UINT16 number) {
    CHAR16 name[16];
    UINT8 raw[CB_OPTION_BYTES];
    UINTN size = sizeof(raw);
    UINTN desc_end;
    UINTN path_offset;
    UINTN path_length;
    UINTN i;
    UINT32 attributes;
    CB_BOOT_OPTION *option;
    EFI_STATUS status;

    if (cb_option_count >= CB_MAX_OPTIONS) return FALSE;
    SPrint(name, sizeof(name), L"Boot%04X", number);
    status = cb_read_variable(name, &EfiGlobalVariable, raw, &size);
    if (EFI_ERROR(status) || size < 8) return FALSE;

    attributes = cb_read_u32(raw);
    if ((attributes & LOAD_OPTION_ACTIVE) == 0) return FALSE;

    desc_end = 6;
    while (desc_end + 1 < size &&
           !(raw[desc_end] == 0 && raw[desc_end + 1] == 0)) {
        desc_end += 2;
    }
    if (desc_end + 1 >= size) return FALSE;

    path_offset = desc_end + 2;
    if (path_offset + 2 > size) return FALSE;
    path_length = cb_read_u16(raw + 4);
    if (path_length < sizeof(EFI_DEVICE_PATH_PROTOCOL) ||
        path_offset + path_length > size) return FALSE;

    option = &cb_options[cb_option_count];
    SetMem(option, sizeof(*option), 0);
    option->number = number;
    option->data_size = size;
    option->device_path_offset = path_offset;
    CopyMem(option->data, raw, size);

    for (i = 0; i < 95 && (6 + i * 2 + 1) < desc_end; ++i) {
        option->description[i] = (CHAR16)cb_read_u16(raw + 6 + i * 2);
    }
    option->description[i] = L'\0';
    ++cb_option_count;
    return TRUE;
}

static void cb_discover_boot_options(void) {
    UINT16 order[256];
    UINTN order_size = sizeof(order);
    UINTN i;
    UINT16 current = 0xffff;
    UINTN current_size = sizeof(current);
    EFI_STATUS status;
    CHAR16 current_name[] = L"BootCurrent";

    cb_option_count = 0;
    (void)cb_read_variable(current_name, &EfiGlobalVariable,
                           &current, &current_size);

    status = cb_read_variable(L"BootOrder", &EfiGlobalVariable,
                              order, &order_size);
    if (!EFI_ERROR(status)) {
        UINTN count = order_size / sizeof(UINT16);
        for (i = 0; i < count && i < 256; ++i) {
            if (order[i] != current) (void)cb_add_boot_option(order[i]);
        }
    }

    /* Firmware may not expose BootOrder in unusual test environments. */
    if (cb_option_count == 0) {
        CHAR16 variable_name[256];
        EFI_GUID vendor;
        UINTN name_size;
        UINT16 number;
        SetMem(variable_name, sizeof(variable_name), 0);
        SetMem(&vendor, sizeof(vendor), 0);
        for (;;) {
            name_size = sizeof(variable_name);
            status = uefi_call_wrapper(RT->GetNextVariableName, 3,
                                       &name_size, variable_name, &vendor);
            if (EFI_ERROR(status)) break;
            if (CompareGuid(&vendor, &EfiGlobalVariable) &&
                cb_is_boot_name(variable_name, &number) && number != current) {
                (void)cb_add_boot_option(number);
            }
        }
    }
}

static UINT16 cb_get_saved_default(void) {
    UINT16 value = 0xffff;
    UINTN size = sizeof(value);
    CHAR16 name[] = L"CarsonBootDefault";
    (void)cb_read_variable(name, &cb_variable_guid, &value, &size);
    return value;
}

static void cb_save_default(UINT16 number) {
    CHAR16 name[] = L"CarsonBootDefault";
    (void)uefi_call_wrapper(RT->SetVariable, 5, name, &cb_variable_guid,
                            CB_VARIABLE_ATTRIBUTES, sizeof(number), &number);
}

static UINTN cb_find_default(UINT16 number) {
    UINTN i;
    for (i = 0; i < cb_option_count; ++i) {
        if (cb_options[i].number == number) return i;
    }
    return 0;
}

static EFI_STATUS cb_launch_option(EFI_HANDLE parent, CB_BOOT_OPTION *option) {
    EFI_HANDLE child = NULL;
    EFI_STATUS status;
    EFI_DEVICE_PATH_PROTOCOL *path =
        (EFI_DEVICE_PATH_PROTOCOL *)(option->data + option->device_path_offset);

    Print(L"\r\nLaunching %s (Boot%04X)...\r\n",
          option->description, option->number);

    status = uefi_call_wrapper(BS->LoadImage, 6, FALSE, parent, path,
                               NULL, 0, &child);
    if (EFI_ERROR(status)) {
        Print(L"LoadImage failed: %r\r\n", status);
        return status;
    }

    status = uefi_call_wrapper(BS->StartImage, 3, child, NULL, NULL);
    if (EFI_ERROR(status)) Print(L"Started image returned: %r\r\n", status);
    (void)uefi_call_wrapper(BS->UnloadImage, 1, child);
    return status;
}

static void cb_draw_menu(UINTN selected, UINTN countdown) {
    UINTN i;
    (void)uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    Print(L"========================================\r\n");
    Print(L"               CarsonBoot               \r\n");
    Print(L"          UEFI Boot Manager              \r\n");
    Print(L"========================================\r\n\r\n");
    Print(L"Select an operating system:\r\n\r\n");
    for (i = 0; i < cb_option_count; ++i) {
        Print(i == selected ? L"  > " : L"    ");
        Print(L"%d. %s  [Boot%04X]\r\n", (UINT32)(i + 1),
              cb_options[i].description, cb_options[i].number);
    }
    Print(L"\r\nUp/Down: select   Enter: boot   Esc: firmware\r\n");
    if (countdown > 0) {
        Print(L"Default starts in %d second(s).\r\n", (UINT32)countdown);
    } else {
        Print(L"Automatic boot is disabled for this selection.\r\n");
    }
}

static EFI_STATUS cb_run_menu(EFI_HANDLE image_handle) {
    EFI_EVENT events[2];
    EFI_EVENT timer;
    EFI_STATUS status;
    EFI_INPUT_KEY key;
    UINTN selected = cb_find_default(cb_get_saved_default());
    UINTN ticks = CB_DEFAULT_TIMEOUT * 10;
    UINTN i;

    if (cb_option_count == 0) {
        Print(L"No active firmware Boot#### entries were found.\r\n");
        return EFI_NOT_FOUND;
    }

    status = uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER, 0,
                               NULL, NULL, &timer);
    if (EFI_ERROR(status)) return status;
    status = uefi_call_wrapper(BS->SetTimer, 3, timer, TimerPeriodic, 1000000);
    if (EFI_ERROR(status)) {
        (void)uefi_call_wrapper(BS->CloseEvent, 1, timer);
        return status;
    }

    events[0] = ST->ConIn->WaitForKey;
    events[1] = timer;
    cb_draw_menu(selected, CB_DEFAULT_TIMEOUT);

    for (;;) {
        UINTN event_index = 0;
        status = uefi_call_wrapper(BS->WaitForEvent, 3, 2, events, &event_index);
        if (EFI_ERROR(status)) break;

        if (event_index == 1) {
            if (ticks > 0) --ticks;
            if (ticks == 0) {
                cb_save_default(cb_options[selected].number);
                status = cb_launch_option(image_handle, &cb_options[selected]);
                break;
            }
            if (ticks % 10 == 0) cb_draw_menu(selected, ticks / 10);
            continue;
        }

        status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);
        if (EFI_ERROR(status)) continue;
        ticks = 0;

        if (key.ScanCode == SCAN_UP) {
            selected = selected == 0 ? cb_option_count - 1 : selected - 1;
        } else if (key.ScanCode == SCAN_DOWN) {
            selected = (selected + 1) % cb_option_count;
        } else if (key.UnicodeChar == L'\r') {
            cb_save_default(cb_options[selected].number);
            status = cb_launch_option(image_handle, &cb_options[selected]);
            break;
        } else if (key.UnicodeChar == 0x1b) {
            status = EFI_SUCCESS;
            break;
        } else {
            for (i = 0; i < cb_option_count && i < 9; ++i) {
                if (key.UnicodeChar == (CHAR16)(L'1' + i)) {
                    selected = i;
                    cb_draw_menu(selected, 0);
                    break;
                }
            }
        }
        cb_draw_menu(selected, 0);
    }

    (void)uefi_call_wrapper(BS->SetTimer, 3, timer, TimerCancel, 0);
    (void)uefi_call_wrapper(BS->CloseEvent, 1, timer);
    return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    EFI_STATUS status;
    InitializeLib(image_handle, system_table);
    (void)uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

    Print(L"========================================\r\n");
    Print(L"               CarsonBoot               \r\n");
    Print(L"          UEFI Boot Manager              \r\n");
    Print(L"========================================\r\n");
    Print(L"Scanning firmware boot entries...\r\n");

    cb_discover_boot_options();
    Print(L"Found %d active boot option(s).\r\n", (UINT32)cb_option_count);
    if (cb_option_count == 0) {
        Print(L"Nothing to chainload. Returning to firmware.\r\n");
        return EFI_NOT_FOUND;
    }

    status = cb_run_menu(image_handle);
    Print(L"\r\nCarsonBoot returned to firmware (%r).\r\n", status);
    Print(L"Press any key to return.\r\n");
    {
        EFI_INPUT_KEY key;
        UINTN index;
        (void)uefi_call_wrapper(BS->WaitForEvent, 3, 1,
                                &ST->ConIn->WaitForKey, &index);
        (void)uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);
    }
    return EFI_SUCCESS;
}
