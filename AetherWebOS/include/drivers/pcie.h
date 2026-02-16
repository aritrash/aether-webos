#ifndef PCIE_H
#define PCIE_H

#include <stdint.h>
#include "config.h"

/* --- PCI Class Codes --- */
#define PCI_CLASS_MASS_STORAGE    0x01
#define PCI_CLASS_NETWORK         0x02
#define PCI_CLASS_DISPLAY         0x03
#define PCI_CLASS_BRIDGE          0x06
#define PCI_CLASS_SERIAL_BUS      0x0C

/* --- Subclasses & Interfaces --- */
#define PCI_SUBCLASS_USB          0x03
#define PCI_IF_XHCI               0x30

// Combined Class Code for xHCI: Class 0x0C, Subclass 0x03, Prog IF 0x30
#define PCI_CLASS_CODE_XHCI       0x0C0330

/* --- Registry Definitions --- */

typedef void (*pci_init_fn)(uint32_t bus, uint32_t dev, uint32_t func);

struct pci_driver {
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t class_code; // Format: (Class << 16 | Subclass << 8 | Interface)
    pci_init_fn init;
    const char *name;
};

/* --- Existing Peripheral Mappings --- */
#define PCIE_RC_REVISION     ((volatile uint32_t*)(PCIE_REG_BASE + 0x406c))
#define PCIE_STATUS           ((volatile uint32_t*)(PCIE_REG_BASE + 0x4068))
#define PCIE_INTR2_CPU_MASK   ((volatile uint32_t*)(PCIE_REG_BASE + 0x4310))

/* --- Function Prototypes --- */
void pcie_init(void);
void pcie_probe_device(uint32_t bus, uint32_t dev, uint32_t func);
uint32_t pcie_read_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg);
void pcie_write_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg, uint32_t val);
void pcie_dump_header(uint32_t bus, uint32_t dev, uint32_t func);
uint32_t get_total_pci_devices();

#endif