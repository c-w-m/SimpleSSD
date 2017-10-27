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

#include "hil/nvme/subsystem.hh"

#include <cmath>

namespace SimpleSSD {

namespace HIL {

namespace NVMe {

const uint32_t nLBAFormat = 4;
const uint32_t lbaFormat[nLBAFormat] = {
    0x02090000,  // 512B + 0, Good performance
    0x020A0000,  // 1KB + 0, Good performance
    0x010B0000,  // 2KB + 0, Better performance
    0x000C0000,  // 4KB + 0, Best performance
};
const uint32_t lbaSize[nLBAFormat] = {
    512,   // 512B
    1024,  // 1KB
    2048,  // 2KB
    4096,  // 4KB
};

Subsystem::Subsystem(Controller *ctrl, ConfigData *cfg)
    : pParent(ctrl),
      pDisk(nullptr),
      pCfgdata(cfg),
      conf(cfg->conf->nvmeConfig) {
  if (conf.readBoolean(NVME_ENABLE_DISK_IMAGE)) {
    if (conf.readBoolean(NVME_USE_COW_DISK)) {
      pDisk = new CoWDisk();
    }
    else {
      pDisk = new Disk();
    }

    // TODO
  }

  if (conf.readBoolean(NVME_ENABLE_DEFAULT_NAMESPACE)) {
    Namespace::Information info;
    uint32_t lba = (uint32_t)conf.readUint(NVME_LBA_SIZE);

    for (info.lbaFormatIndex = 0; info.lbaFormatIndex < nLBAFormat;
         info.lbaFormatIndex++) {
      if (lbaSize[info.lbaFormatIndex] == lba) {
        break;
      }
    }

    if (info.lbaFormatIndex == nLBAFormat) {
      // TODO: panic("Failed to setting LBA size (512B ~ 4KB)");
    }

    // TODO make one full-sized LBA range
    // TODO set data from FTL/NAND configuration
    Namespace *pNS = new Namespace(this, pCfgdata);
    pNS->setData(NSID_LOWEST, &info);
    pNS->attach(true);

    lNamespaces.push_back(pNS);
  }

  pHIL = new HIL(cfg->conf);
}

Subsystem::~Subsystem() {
  if (pDisk) {
    delete pDisk;
  }

  for (auto &iter : lNamespaces) {
    delete iter;
  }

  delete pHIL;
}

bool Subsystem::createNamespace(uint32_t nsid, Namespace::Information *info) {
  // TODO: calculate lba ranges
  // uint64_t lastOffset = 0;
  //
  // for (auto iter = lNamespaces.begin(); iter != lNamespaces.end(); iter++) {
  //   lastOffset = (*iter)->offset + (*iter)->nsData.NSZE;
  // }
  //
  // if (lastOffset + logical_blocks > totalLogicalBlocks) {
  //   return false;
  // }

  Namespace *pNS = new Namespace(this, pCfgdata);

  pNS->setData(nsid, info);

  lNamespaces.push_back(pNS);
  // DPRINTF(NVMeAll, "NS %-5d| CREATE | LBA Range %" PRIu64 " + %" PRIu64 "\n",
  //        nsid, lastOffset, logical_blocks);

  return true;
}

bool Subsystem::destroyNamespace(uint32_t nsid) {
  bool found = false;

  for (auto iter = lNamespaces.begin(); iter != lNamespaces.end(); iter++) {
    if ((*iter)->getNSID() == nsid) {
      found = true;

      // DPRINTF(NVMeAll, "NS %-5d| DELETE\n", nsid);

      delete *iter;

      lNamespaces.erase(iter);

      break;
    }
  }

  return found;
}

// TODO: for namespace management function
// if (flbas >= nLBAFormat) {
//   return false;
// }
//
// uint32_t lbasize = (uint32_t)powf(2, (lbaFormat[flbas] >> 16) & 0xFF);

}  // namespace NVMe

}  // namespace HIL

}  // namespace SimpleSSD
