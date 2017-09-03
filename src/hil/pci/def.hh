/*
 * Copyright (C) 2017 CAMELab
 *
 * This file is part of SimpleSSD.
 *
 * SimpleSSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSSD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file "def.hh"
 *
 * Definition for PCI or PCI Express structures.
 */

#ifndef __HIL_PCI_DEF__
#define __HIL_PCI_DEF__

#include <cinttypes>

namespace SimpleSSD {

namespace PCI {

/** Interrupt modes used in PCI/PCIe */
typedef enum { INTERRUPT_PIN, INTERRUPT_MSI, INTERRUPT_MSIX } INTERRUPT_MODE;

/** Size of capabilities used in PCI/PCIe */
typedef enum {
  SIZE_CONFIG = 64,
  SIZE_PMCAP = 6,
  SIZE_MSI = 24,
  SIZE_MSIX = 12,
  SIZE_PXCAP = 60,

  SIZE_MSIX_TABLE = 16,
  SIZE_MSIX_PBA = 8,
} PCI_SIZE;

/** Capability ID used in PCI/PCIe */
typedef enum {
  ID_PMCAP = 0x01,
  ID_MSI = 0x05,
  ID_MSIX = 0x11,
  ID_PXCAP = 0x10,
} PCI_CAP_ID;

/** PCI Configuration structure */
union Config {
  uint8_t data[64];
  struct {
    uint16_t vendorID;
    uint16_t deviceID;
    uint16_t command;
    uint16_t deviceStatus;
    uint32_t classCode;
    uint8_t cacheLine;
    uint8_t latencyTimer;
    uint8_t headerType;
    uint8_t buitInSelfTest;
    uint32_t baseAddress[6];
    uint32_t cardbus;
    uint16_t subsystemVendorID;
    uint16_t subsystemID;
    uint32_t expansionROM;
    uint8_t capabilityPointer;
    uint8_t reserved[7];
    uint8_t interruptLine;
    uint8_t interruptPin;
    uint8_t minimumGrant;
    uint8_t maximumLatency;
  };

  Config();
};

/** PCI Power Management Capability structure */
union PCIPowerManagement {
  uint8_t data[6];
  struct {
    uint8_t capabilityID;
    uint8_t nextCapability;
    uint16_t capabilities;
    uint16_t controlStatus;
  };

  PCIPowerManagement();
};

/** MSI Capability structure */
union MSI {
  uint8_t data[24];
  struct {
    uint8_t capabilityID;
    uint8_t nextCapability;
    uint16_t messageControl;
    uint32_t address;
    uint32_t addressHigh;
    uint32_t messageData;
    uint32_t maskBits;
    uint32_t pendingBits;
  };

  MSI();
};

/** MSI-X Capability structure */
union MSIX {
  uint8_t data[12];
  struct {
    uint8_t capabilityID;
    uint8_t nextCapability;
    uint16_t messageControl;
    uint32_t tableOffset;
    uint32_t pbaOffset;
  };

  MSIX();
};

/** MSI-X Table Entry structure */
union MSIXTableEntry {
  uint8_t data[16];
  struct {
    uint32_t addrresLow;
    uint32_t addrresHigh;
    uint32_t messageData;
    uint32_t vectorControl;
  };

  MSIXTableEntry();
};

/** MSI-X PBA Entry structure */
union MSIXPBAEntry {
  uint8_t data[8];
  uint64_t mask;

  MSIXPBAEntry();
};

/** PCI Express Capability structure */
union PCIExpress {
  uint8_t data[60];
  struct {
    uint8_t capabilityID;
    uint8_t nextCapability;
    uint16_t capabilityRegister;
    uint32_t deviceCapabilities;
    uint16_t deviceControl;
    uint16_t deviceStatus;
    uint32_t linkCapabilities;
    uint16_t linkControl;
    uint16_t linkStatus;
    uint32_t slotCapabilities;
    uint16_t slotControl;
    uint16_t slotStatus;
    uint16_t rootControl;
    uint16_t rootCapabilities;
    uint32_t rootStatus;
    uint32_t deviceCapabilities2;
    uint16_t deviceControl2;
    uint16_t deviceStatus2;
    uint32_t linkCapabilities2;
    uint16_t linkControl2;
    uint16_t linkStatus2;
    uint32_t slotCapabilities2;
    uint16_t slotControl2;
    uint16_t slotStatus2;
  };

  PCIExpress();
};

}  // namespace PCI

}  // namespace SimpleSSD

#endif
