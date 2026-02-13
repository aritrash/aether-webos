#include <stdint.h>

#include "uart.h"
#include "drivers/pcie.h"
#include "drivers/usb/xhci.h"


/* MMU mapping function (from memory subsystem) */
extern void* ioremap(uint64_t phys_addr, uint64_t size);


/* ================================
   xHCI Extended Capability
   ================================ */

#define XHCI_EXT_CAP_LEGACY   1

#define USB_BIOS_OWNED   (1 << 16)
#define USB_OS_OWNED     (1 << 24)

#define USBCMD_HCRST     (1 << 1)


/* ================================
   Small Delay
   ================================ */

static void delay(uint32_t count)
{
    while (count--)
        asm volatile("nop");
}


/* ================================
   Print Hex (Debug)
   ================================ */

static void print_hex(uint32_t val)
{
    char hex[] = "0123456789ABCDEF";

    uart_puts("0x");

    for (int i = 28; i >= 0; i -= 4)
        uart_putc(hex[(val >> i) & 0xF]);
}


/* ================================
   BIOS → OS Handshake
   ================================ */

static void xhci_claim_ownership(struct xhci_cap_regs *cap)
{
    uint32_t hcc = cap->hcc_params1;

    /* Extended capability pointer */
    uint32_t offset = (hcc >> 16) << 2;

    if (!offset)
        return;


    volatile uint32_t *ext =
        (volatile uint32_t *)((uint8_t*)cap + offset);


    while (ext) {

        uint32_t header = *ext;

        uint8_t cap_id   = header & 0xFF;
        uint8_t next_ptr = (header >> 8) & 0xFF;


        /* USB Legacy Support */
        if (cap_id == XHCI_EXT_CAP_LEGACY) {

            uart_puts("[USB] Legacy Support found\n");

            volatile uint32_t *ctrl = ext + 1;

            /* Request OS ownership */
            *ctrl |= USB_OS_OWNED;

            /* Wait for BIOS release */
            while (*ctrl & USB_BIOS_OWNED);

            uart_puts("[USB] BIOS ownership released\n");
            return;
        }


        if (!next_ptr)
            break;

        ext = (volatile uint32_t *)
              ((uint8_t*)cap + (next_ptr << 2));
    }
}


/* ================================
   Main xHCI Init
   ================================ */

int xhci_hc_init(void)
{
    uart_puts("\n[USB] Initializing xHCI...\n");


    /* --------------------------------
       1. Find Controller via PCIe
       -------------------------------- */

    uint64_t bar0 = 0;

    if (pcie_find_xhci(&bar0) != 0) {

        uart_puts("[USB] ERROR: xHCI not found\n");
        return -1;
    }


    uart_puts("[USB] BAR0 = ");
    print_hex((uint32_t)bar0);
    uart_puts("\n");


    /* --------------------------------
       2. Map BAR
       -------------------------------- */

    struct xhci_cap_regs *cap =
        (struct xhci_cap_regs *)ioremap(bar0, 0x4000);

    if (!cap) {

        uart_puts("[USB] ERROR: ioremap failed\n");
        return -1;
    }


    /* --------------------------------
       3. Read Info
       -------------------------------- */

    uart_puts("[USB] Version: ");
    print_hex(cap->hci_version);
    uart_puts("\n");


    uint32_t hcs1 = cap->hcs_params1;

    uint32_t slots = hcs1 & 0xFF;
    uint32_t ports = (hcs1 >> 24) & 0xFF;


    uart_puts("[USB] Slots = ");
    print_hex(slots);

    uart_puts("  Ports = ");
    print_hex(ports);

    uart_puts("\n");


    /* --------------------------------
       4. Claim Ownership
       -------------------------------- */

    xhci_claim_ownership(cap);


    /* --------------------------------
       5. Reset Controller
       -------------------------------- */

    struct xhci_op_regs *op =
        (struct xhci_op_regs *)
        ((uint8_t*)cap + cap->cap_length);


    uart_puts("[USB] Resetting controller...\n");

    /* Stop controller */
    op->usb_cmd &= ~1;

    delay(100000);


    /* Issue reset */
    op->usb_cmd |= USBCMD_HCRST;

    while (op->usb_cmd & USBCMD_HCRST);


    /* Wait until not halted */
    while (op->usb_sts & 1);


    uart_puts("[OK] USB: xHCI Ready\n");


    return 0;
}
