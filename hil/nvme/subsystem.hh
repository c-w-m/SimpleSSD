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

#ifndef __HIL_NVME_SUBSYSTEM__
#define __HIL_NVME_SUBSYSTEM__

#include "hil/hil.hh"
#include "hil/nvme/controller.hh"
#include "hil/nvme/namespace.hh"
#include "util/disk.hh"

namespace SimpleSSD {

namespace HIL {

namespace NVMe {

typedef union _HealthInfo {
  uint8_t data[0x200];
  struct {
    uint8_t status;
    uint16_t temperature;
    uint8_t availableSpare;
    uint8_t spareThreshold;
    uint8_t lifeUsed;
    uint8_t reserved[26];
    uint64_t readL;
    uint64_t readH;
    uint64_t writeL;
    uint64_t writeH;
    uint64_t readCommandL;
    uint64_t readCommandH;
    uint64_t writeCommandL;
    uint64_t writeCommandH;
  };

  _HealthInfo();
} HealthInfo;

class Subsystem {
 private:
  Controller *pParent;
  HIL *pHIL;
  Disk *pDisk;

  std::list<Namespace *> lNamespaces;

  ConfigData *pCfgdata;
  Config &conf;

  HealthInfo globalHealth;

  bool createNamespace(uint32_t, Namespace::Information *);
  bool destroyNamespace(uint32_t);

 public:
  Subsystem(Controller *, ConfigData *);
  ~Subsystem();

  uint64_t submitCommand(SQEntryWrapper &, uint64_t);
  uint64_t allocatedNVMCapacity();
  uint32_t validNamespaceCount();
};

}  // namespace NVMe

}  // namespace HIL

}  // namespace SimpleSSD

#endif
