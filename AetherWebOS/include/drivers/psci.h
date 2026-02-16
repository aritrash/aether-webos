#ifndef PSCI_H
#define PSCI_H

#include <stdint.h>

/**
 * PSCI Function IDs for ARMv8
 * 0x84000000 series is for 32-bit (SMC32/HVC32)
 * 0xC4000000 series is for 64-bit (SMC64/HVC64)
 */

/* Standard PSCI v0.2+ Function IDs */
#define PSCI_VERSION               0x84000000
#define PSCI_SYSTEM_OFF            0x84000008
#define PSCI_SYSTEM_RESET          0x84000009
#define PSCI_FEATURES              0x8400000A

/**
 * psci_system_off: Gracefully powers down the hardware.
 * Note: This call does not return.
 */
void psci_system_off(void);

/**
 * psci_system_reset: Reboots the hardware.
 * Note: This call does not return.
 */
void psci_system_reset(void);

#endif /* PSCI_H */