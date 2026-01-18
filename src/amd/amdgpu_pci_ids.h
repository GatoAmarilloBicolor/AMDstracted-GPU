#ifndef AMDGPU_PCI_IDS_H
#define AMDGPU_PCI_IDS_H

#include "hal/hal.h"
#include <stdint.h>

/*
 * 🌀 HIT Edition: The Global GPU ID List
 *
 * Instead of hardcoding names, we use this table to look up
 * what kind of chip we're actually talking to.
 */

struct amd_pci_info {
  uint16_t device_id;
  enum amd_asic_type asic_type;
  const char *name;
  uint32_t vram_mb_default;
  uint32_t clock_mhz;
};

static const struct amd_pci_info amd_pci_table[] = {
    {0x9806, AMD_ASIC_WRESTLER, "Radeon HD 7290 (Wrestler)", 512, 400},
    {0x6810, AMD_ASIC_NAVI10, "Radeon RX 5700 XT (Navi10)", 8192, 1710},
    {0x7310, AMD_ASIC_NAVI10, "Radeon RX 5700 (Navi10)", 8192, 1625},
    {0x9802, AMD_ASIC_WRESTLER, "Radeon HD 6310 (Wrestler)", 384, 280},
    {0x6898, AMD_ASIC_EVERGREEN, "Radeon HD 5870 (Cypress)", 1024, 850},
    {0x6718, AMD_ASIC_NI, "Radeon HD 6970 (Cayman)", 2048, 880},
    {0x9400, AMD_ASIC_R600, "Radeon HD 2900 XT (R600)", 512, 740},
    {0, AMD_ASIC_NAVI10, "Generic AMD GPU", 1024, 1000} // Fallback
};

#endif
