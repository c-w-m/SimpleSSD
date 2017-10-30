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
      conf(cfg->pConfigReader->nvmeConfig),
      allocatedLogicalPages(0) {
  pHIL = new HIL(cfg->pConfigReader);

  pHIL->getLPNInfo(totalLogicalPages, logicalPageSize);

  if (conf.readBoolean(NVME_ENABLE_DISK_IMAGE)) {
    uint64_t diskSize;

    if (conf.readBoolean(NVME_USE_COW_DISK)) {
      pDisk = new CoWDisk();
    }
    else {
      pDisk = new Disk();
    }

    std::string filename = conf.readString(NVME_DISK_IMAGE_PATH);

    diskSize = pDisk->open(filename);

    if (diskSize == 0) {
      // TODO: panic("Failed to open disk image");
    }
    else if (diskSize != totalLogicalPages * logicalPageSize) {
      if (conf.readBoolean(NVME_STRICT_DISK_SIZE)) {
        // TODO: panic("Disk size not match");
      }
    }
  }

  if (conf.readBoolean(NVME_ENABLE_DEFAULT_NAMESPACE)) {
    Namespace::Information info;
    uint32_t lba = (uint32_t)conf.readUint(NVME_LBA_SIZE);
    uint32_t lbaRatio = logicalPageSize / lba;

    for (info.lbaFormatIndex = 0; info.lbaFormatIndex < nLBAFormat;
         info.lbaFormatIndex++) {
      if (lbaSize[info.lbaFormatIndex] == lba) {
        info.lbaSize = lbaSize[info.lbaFormatIndex];

        break;
      }
    }

    if (info.lbaFormatIndex == nLBAFormat) {
      // TODO: panic("Failed to setting LBA size (512B ~ 4KB)");
    }

    // Default namespace is full-sized
    allocatedLogicalPages = totalLogicalPages;

    // Fill Namespace information
    info.size = totalLogicalPages * lbaRatio;
    info.capacity = info.size;
    info.dataProtectionSettings = 0x00;
    info.namespaceSharingCapabilities = 0x00;

    if (!createNamespace(NSID_LOWEST, &info)) {
      // TODO: panic("Failed to create namespace");
    }
  }
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

bool Subsystem::submitCommand(SQEntryWrapper &req, CQEntryWrapper &resp,
                              uint64_t &tick) {
  bool processed = false;
  bool submit = true;
  uint64_t beginAt = tick;

  // TODO: change this
  tick += 1000000;  // 1us of command delay

  // Admin command
  if (req.sqID == 0) {
    switch (req.entry.dword0.opcode) {
      case OPCODE_DELETE_IO_SQUEUE:
        break;
      case OPCODE_ASYNC_EVENT_REQ:
        submit = false;
        break;
      default:
        // Invalid opcode
        break;
    }
  }

  // NVM commands or Namespace specific Admin commands
  if (!processed) {
    for (auto &iter : lNamespaces) {
      return iter->submitCommand(req, resp, beginAt);
    }
  }

  // Invalid namespace
  if (!processed) {
  }

  return submit;
}

void Subsystem::getNVMCapacity(uint64_t &total, uint64_t &used) {
  total = totalLogicalPages * logicalPageSize;
  used = allocatedLogicalPages * logicalPageSize;
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
