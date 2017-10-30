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

#include "hil/nvme/namespace.hh"

#include "hil/nvme/subsystem.hh"

namespace SimpleSSD {

namespace HIL {

namespace NVMe {

Namespace::Namespace(Subsystem *p, ConfigData *c)
    : pParent(p),
      pCfgdata(c),
      nsid(NSID_NONE),
      attached(false),
      allocated(false) {}

bool Namespace::submitCommand(SQEntryWrapper &req, CQEntryWrapper &resp,
                              uint64_t &tick) {
  uint64_t beginAt = tick;

  // Admin commands
  if (req.sqID == 0) {
    switch (req.entry.dword0.opcode) {
      case OPCODE_GET_LOG_PAGE:
        getLogPage(req, resp, beginAt);
        break;
      default:
        resp.makeStatus(true, false, TYPE_GENERIC_COMMAND_STATUS,
                        STATUS_INVALID_OPCODE);
        break;
    }
  }

  // NVM commands
  else {
    switch (req.entry.dword0.opcode) {
      case OPCODE_FLUSH:
        flush(req, resp, beginAt);
        break;
      case OPCODE_WRITE:
        write(req, resp, beginAt);
        break;
      case OPCODE_READ:
        read(req, resp, beginAt);
        break;
      case OPCODE_DATASET_MANAGEMEMT:
        datasetManagement(req, resp, beginAt);
        break;
      default:
        resp.makeStatus(true, false, TYPE_GENERIC_COMMAND_STATUS,
                        STATUS_INVALID_OPCODE);
        break;
    }
  }

  return true;
}

void Namespace::setData(uint32_t id, Information *data,
                        std::list<LBARange> &ranges) {
  nsid = id;
  memcpy(&info, data, sizeof(Information));

  allocated = true;
}

void Namespace::attach(bool attach) {
  attached = attach;
}

uint32_t Namespace::getNSID() {
  return nsid;
}

Namespace::Information *Namespace::getInfo() {
  return &info;
}

void Namespace::getLBARange(std::list<LBARange> &list) {
  list = lbaRanges;
}

void Namespace::getLogPage(SQEntryWrapper &req, CQEntryWrapper &resp,
                           uint64_t &tick) {
  uint16_t numdl = (req.entry.dword10 & 0xFFFF0000) >> 16;
  uint16_t lid = req.entry.dword10 & 0xFFFF;
  uint16_t numdu = req.entry.dword11 & 0xFFFF;
  uint32_t lopl = req.entry.dword12;
  uint32_t lopu = req.entry.dword13;

  uint32_t req_size = (((uint32_t)numdu << 16 | numdl) + 1) * 4;
  uint64_t offset = ((uint64_t)lopu << 32) | lopl;

  PRPList PRP(pCfgdata, req.entry.data1, req.entry.data2, (uint64_t)req_size);

  switch (lid) {
    case LOG_SMART_HEALTH_INFORMATION:
      if (req.entry.namespaceID == nsid) {
        PRP.write(offset, 512, health.data, tick);
      }
      else {
        resp.makeStatus(true, false, TYPE_COMMAND_SPECIFIC_STATUS,
                        STATUS_NAMESPACE_NOT_ATTACHED);
      }

      break;
    default:
      resp.makeStatus(true, false, TYPE_COMMAND_SPECIFIC_STATUS,
                      STATUS_INVALID_LOG_PAGE);
      break;
  }
}

void Namespace::flush(SQEntryWrapper &req, CQEntryWrapper &resp,
                      uint64_t &tick) {
  bool err = false;

  if (!attached) {
    err = true;
    resp.makeStatus(true, false, TYPE_COMMAND_SPECIFIC_STATUS,
                    STATUS_NAMESPACE_NOT_ATTACHED);
  }

  if (!err) {
    pParent->flush(tick);
    //
    // DPRINTF(NVMeAll, "NVM     | FLUSH | NSID %-5d| Tick %" PRIu64 "\n", nsid,
    //         cmdDelay);
    // DPRINTF(NVMeBreakdown, "N%u|2|F|I%d|F%" PRIu64 "\n", nsid,
    //         req.entry.dword0.CID, cmdDelay);
  }
}

void Namespace::write(SQEntryWrapper &req, CQEntryWrapper &resp,
                      uint64_t &tick) {
  bool err = false;

  uint64_t slba = ((uint64_t)req.entry.dword11 << 32) | req.entry.dword10;
  uint16_t nlb = (req.entry.dword12 & 0xFFFF) + 1;

  if (!attached) {
    err = true;
    resp.makeStatus(true, false, TYPE_COMMAND_SPECIFIC_STATUS,
                    STATUS_NAMESPACE_NOT_ATTACHED);
  }
  if (nlb == 0) {
    err = true;
    // TODO: warn("nvme_namespace: host tried to write 0 blocks\n");
  }

  if (!err) {
    PRPList PRP(pCfgdata, req.entry.data1, req.entry.data2,
                (uint64_t)nlb * info.lbaSize);

    pParent->write(slba, nlb, PRP, tick);

    // DPRINTF(NVMeBreakdown, "N%u|2|W|I%d|F%" PRIu64 "|D%" PRIu64 "\n", nsid,
    //         req.entry.dword0.CID, totalReq, DMA);
    //
    // cmdDelay = MAX(DMA, totalReq);
    //
    // DPRINTF(NVMeAll, "NVM     | WRITE | NSID %-5d| Tick %" PRIu64 "\n", nsid,
    //         cmdDelay);
  }
}

void Namespace::read(SQEntryWrapper &req, CQEntryWrapper &resp,
                     uint64_t &tick) {
  bool err = false;

  uint64_t slba = ((uint64_t)req.entry.dword11 << 32) | req.entry.dword10;
  uint16_t nlb = (req.entry.dword12 & 0xFFFF) + 1;
  // bool fua = req.entry.dword12 & 0x40000000;

  if (!attached) {
    err = true;
    resp.makeStatus(true, false, TYPE_COMMAND_SPECIFIC_STATUS,
                    STATUS_NAMESPACE_NOT_ATTACHED);
  }
  if (nlb == 0) {
    err = true;
    // TODO: warn("nvme_namespace: host tried to read 0 blocks\n");
  }

  if (!err) {
    PRPList PRP(pCfgdata, req.entry.data1, req.entry.data2,
                (uint64_t)nlb * info.lbaSize);

    pParent->write(slba, nlb, PRP, tick);
    // DPRINTF(NVMeBreakdown, "N%u|2|R|I%d|F%" PRIu64 "|D%" PRIu64 "\n", nsid,
    //         req.entry.dword0.CID, cmdDelay, DMA);
    //
    // DPRINTF(NVMeAll, "NVM     | READ  | NSID %-5d| Tick %" PRIu64 "\n", nsid,
    //         cmdDelay);
  }
}

void Namespace::datasetManagement(SQEntryWrapper &req, CQEntryWrapper &resp,
                                  uint64_t &tick) {
  bool err = false;

  int nr = (req.entry.dword10 & 0xFF) + 1;
  bool ad = req.entry.dword11 & 0x04;

  if (!attached) {
    err = true;
    resp.makeStatus(true, false, TYPE_COMMAND_SPECIFIC_STATUS,
                    STATUS_NAMESPACE_NOT_ATTACHED);
  }
  if (!ad) {
    err = true;
    // Just ignore
  }

  if (!err) {
    static DatasetManagementRange range;
    PRPList PRP(pCfgdata, req.entry.data1, req.entry.data2, (uint64_t)0x1000);

    for (int i = 0; i < nr; i++) {
      PRP.read(i * 0x10, 0x10, range.data, tick);
      pParent->trim(range.slba, range.nlb, tick);
    }

    // DPRINTF(NVMeAll, "NVM     | TRIM  | NSID %-5d| Tick %" PRIu64 "\n", nsid,
    //         cmdDelay);
    // DPRINTF(NVMeBreakdown, "N%u|2|T|I%d|F%" PRIu64 "\n", nsid,
    //         req.entry.dword0.CID, cmdDelay);
  }
}

}  // namespace NVMe

}  // namespace HIL

}  // namespace SimpleSSD
