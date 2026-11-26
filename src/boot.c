#include <efi.h>
#include <efilib.h>
#include <stdint.h>

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} CB_CPUID_RESULT;

extern void cb_cpuid(uint32_t leaf, uint32_t subleaf, CB_CPUID_RESULT *out);

static void cb_print_cpu_vendor(void) {
    CB_CPUID_RESULT result = {0};
    CHAR16 vendor[13];

    cb_cpuid(0, 0, &result);

    vendor[0] = (CHAR16)((result.ebx >> 0) & 0xff);
    vendor[1] = (CHAR16)((result.ebx >> 8) & 0xff);
    vendor[2] = (CHAR16)((result.ebx >> 16) & 0xff);
    vendor[3] = (CHAR16)((result.ebx >> 24) & 0xff);
    vendor[4] = (CHAR16)((result.edx >> 0) & 0xff);
    vendor[5] = (CHAR16)((result.edx >> 8) & 0xff);
    vendor[6] = (CHAR16)((result.edx >> 16) & 0xff);
    vendor[7] = (CHAR16)((result.edx >> 24) & 0xff);
    vendor[8] = (CHAR16)((result.ecx >> 0) & 0xff);
    vendor[9] = (CHAR16)((result.ecx >> 8) & 0xff);
    vendor[10] = (CHAR16)((result.ecx >> 16) & 0xff);
    vendor[11] = (CHAR16)((result.ecx >> 24) & 0xff);
    vendor[12] = L'\\0';

    Print(L"CPU vendor: %s\\r\\n", vendor);
}

static void cb_wait_for_key(void) {
    EFI_STATUS status;
    EFI_INPUT_KEY key;
    UINTN event_index = 0;

    Print(L"Press any key to return to the firmware...\\r\\n");

    status = uefi_call_wrapper(
        ST->BootServices->WaitForEvent,
        3,
        1,
        &ST->ConIn->WaitForKey,
        &event_index
    );

    if (!EFI_ERROR(status)) {
        (void)uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);

    (void)uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

    Print(L"========================================\\r\\n");
    Print(L"             CarsonBoot                 \\r\\n");
    Print(L"       x86_64 UEFI development build    \\r\\n");
    Print(L"========================================\\r\\n\\r\\n");

    Print(L"[OK] UEFI application entry reached\\r\\n");
    Print(L"[OK] C runtime initialized\\r\\n");
    Print(L"[OK] x86_64 assembly routine linked\\r\\n");
    cb_print_cpu_vendor();

    Print(L"\\r\\nCurrent milestone: UEFI diagnostics\\r\\n");
    Print(L"Linux kernel loading is not implemented yet.\\r\\n\\r\\n");

    cb_wait_for_key();
    return EFI_SUCCESS;
}
