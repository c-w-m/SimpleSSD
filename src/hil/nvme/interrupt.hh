/**
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

#ifndef __SNL_NVME_INTERRUPT__
#define __SNL_NVME_INTERRUPT__

#include <cinttypes>

#include "base/types.hh"
#include "src/config.hh"
#include "src/snl/nvme/def.hh"

namespace SimpleSSD {

namespace NVMe {

class Interrupt {
 private:
  union MSIXTable {
    uint8_t data[16];
    struct {
      uint32_t addrLow;
      uint32_t addrHigh;
      uint32_t messageData;
      uint32_t vectorControl;
    };
  };

  union MSIXPBAEntry {
    uint8_t data[8];
    uint64_t mask;
  };

  INTERRUPT_MODE mode;

  uint32_t interruptStatus;
  uint32_t interruptStatusOld;

  uint16_t allocatedVectors;

  Addr tableBaseAddr;
  int tableSize;
  Addr pbaBaseAddr;
  int pbaSize;

  MSIXTable *pMSIXTable;
  MSIXPBAEntry *pMSIXPBAEntry;

 public:
  Interrupt();
};

}  // namespace NVMe

}  // namespace SimpleSSD

#endif
