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

namespace SimpleSSD {

namespace HIL {

namespace NVMe {

LBARange::_LBARange() : slba(0), nlblk(0) {}

LBARange::_LBARange(uint64_t s, uint64_t n) : slba(s), nlblk(n) {}

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
      case OPCODE_IDENTIFY:
        identify(req, resp, beginAt);
        break;
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

  // TODO calculate lbaratio
  // TODO set rawInfo

  allocated = true;
}

void Namespace::attach(bool attach) {
  attached = attach;
}

uint32_t Namespace::getNSID() {
  return nsid;
}

void Namespace::identify(SQEntryWrapper &req, CQEntryWrapper &resp,
                         uint64_t &tick) {
  bool err = false;

  uint8_t cns = req.entry.dword10 & 0xFF;

  PRPList PRP(pCfgdata->pDmaEngine, pCfgdata->memoryPageSize, req.entry.data1,
              req.entry.data2, (uint64_t)0x1000);

  switch (cns) {
    case CNS_IDENTIFY_NAMESPACE:
      if (!attached) {
        err = true;
        resp.makeStatus(true, false, TYPE_COMMAND_SPECIFIC_STATUS,
                        STATUS_NAMESPACE_NOT_ATTACHED);
      }

      break;
    case CNS_IDENTIFY_ALLOCATED_NAMESPACE:
      // Do nothing
      break;
    default:
      // TODO panic("nvme_namespace: Invalid CNS in identify namespace\n");
      break;
  }

  if (!err) {
    PRP.write(0, 0x1000, rawInfo, tick);
  }
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

  PRPList PRP(pCfgdata->pDmaEngine, pCfgdata->memoryPageSize, req.entry.data1,
              req.entry.data2, (uint64_t)req_size);

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
    // std::vector<Addr> list;
    //
    // cache->flush(list, cmdDelay);
    //
    // Tick last, max = 0;
    //
    // for (auto iter = list.begin(); iter != list.end(); iter++) {
    //   last = ftl->write(*iter, 1);
    //   max = MAX(last, max);
    // }
    //
    // cmdDelay += max;
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

  uint64_t slpn;
  uint16_t nlp, off;

  slpn = slba / lbaratio;
  off = slba % lbaratio;
  nlp = (nlb + lbaratio - 1) / lbaratio;

  // DPRINTF(NVMeAll, "NVM     | WRITE | NSID %-5d| LBA %016" PRIX64 " + %X\n",
  //         nsid, slba, nlb);
  // DPRINTF(NVMeAll, "NVM     | WRITE | NSID %-5d| LPN %016" PRIX64 " + %X\n",
  //         nsid, slpn, nlp);

  if (!err) {
    // std::vector<Addr> list;
    // Tick lastReq;
    // Tick lastMax;
    // Tick totalReq = 0;
    // Tick DMA;
    //
    // PRPList PRP(pParent->getParent(), cmd.entry.DPTR1, cmd.entry.DPTR2,
    //             (size_t)nlb * blockSize);
    //
    // if (disk && nlb > 0) {
    //   uint8_t *data;
    //   uint16_t written;
    //
    //   data = (uint8_t *)malloc(nlb * blockSize);
    //   if (data) {
    //     DMA = PRP.read(0, nlb * blockSize, data);
    //     written = disk->write(slba + offset, nlb, data);
    //
    //     if (written != nlb) {
    //       panic("nvme_namespace: Failed to write disk image\n");
    //     }
    //
    //     free(data);
    //   }
    //   else {
    //     panic("nvme_namespace: Memory allocation failed\n");
    //   }
    // }
    // else {
    //   DMA = PRP.read(0, nlb * blockSize, NULL);
    // }
    //
    // for (uint16_t i = 0; i < nlp; i++) {
    //   lastReq = 0;
    //   lastMax = 0;
    //
    //   if (cache->setData(slpn + i, lastReq)) {
    //     lastMax = lastReq;
    //   }
    //   else if (!pParent->nvmeConfig.WriteCaching) {
    //     lastReq = ftl->write(slpn + i, 1);
    //     lastMax = MAX(lastReq, lastMax);
    //   }
    //   else {
    //     cache->getEvictList(list);
    //
    //     for (auto iter = list.begin(); iter != list.end(); iter++) {
    //       lastReq = ftl->write(*iter, 1);
    //       lastMax = MAX(lastReq, lastMax);
    //     }
    //   }
    //
    //   totalReq = MAX(lastMax, totalReq);
    // }
    //
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

  uint64_t slpn;
  uint16_t nlp, off;

  slpn = slba / lbaratio;
  off = slba % lbaratio;
  nlp = (nlb + off + lbaratio - 1) / lbaratio;

  // DPRINTF(NVMeAll, "NVM     | READ  | NSID %-5d| LBA %016" PRIX64 " + %X\n",
  //         nsid, slba, nlb);
  // DPRINTF(NVMeAll, "NVM     | READ  | NSID %-5d| LPN %016" PRIX64 " + %X\n",
  //         nsid, slpn, nlp);

  if (!err) {
    // Tick lastReq;
    // Tick totalReq = 0;
    // Tick prefetchTotal = 0;
    // Tick DMA;
    //
    // PRPList PRP(pParent->getParent(), cmd.entry.DPTR1, cmd.entry.DPTR2,
    //             (size_t)nlb * blockSize);
    //
    // if (disk && nlb > 0) {
    //   uint8_t *data;
    //   uint16_t read;
    //
    //   data = (uint8_t *)malloc(nlb * blockSize);
    //   if (data) {
    //     read = disk->read(slba + offset, nlb, data);
    //
    //     if (read != nlb) {
    //       panic("nvme_namespace: Failed to read disk image\n");
    //     }
    //
    //     DMA = PRP.write(0, nlb * blockSize, data);
    //     free(data);
    //   }
    //   else {
    //     panic("nvme_namespace: Memory allocation failed\n");
    //   }
    // }
    // else {
    //   DMA = PRP.write(0, nlb * blockSize, NULL);
    // }
    //
    // if (pParent->nvmeConfig.ReadPrefetch) {
    //   fua = false;
    //
    //   if (lastlpn == slpn) {
    //     if (slpn + nlp >= lastPrefetch) {
    //       Addr begin = MAX(lastPrefetch, slpn);
    //       Addr end = lastPrefetch + superPageSize;
    //
    //       Tick prefetchReq;
    //
    //       for (; begin < end; begin++) {
    //         if (!cache->getData(begin, prefetchReq)) {
    //           prefetchReq += ftl->read(begin, 1);
    //         }
    //         prefetchTotal = MAX(prefetchReq, prefetchTotal);
    //       }
    //
    //       lastPrefetch = end;
    //     }
    //   }
    // }
    //
    // for (uint16_t i = 0; i < nlp; i++) {
    //   lastReq = 0;
    //
    //   if (!cache->getData(slpn + i, lastReq) || fua) {
    //     lastReq += ftl->read(slpn + i, 1);
    //   }
    //
    //   totalReq = MAX(lastReq, totalReq);
    // }
    //
    // cmdDelay = max(prefetchTotal, totalReq);
    //
    // DPRINTF(NVMeBreakdown, "N%u|2|R|I%d|F%" PRIu64 "|D%" PRIu64 "\n", nsid,
    //         req.entry.dword0.CID, cmdDelay, DMA);
    //
    // if (cmdDelay > 1000000000000) {
    //   printf("NAND time is too large!!\n");
    //   printf(" Now: %" PRIu64 "\n", curTick());
    //   printf(" Total Req: %" PRIu64 "\n", totalReq);
    //   printf(" Prefetch: %" PRIu64 "\n", prefetchTotal);
    //   printf(" DMA: %" PRIu64 "\n", DMA);
    //   panic("Debug ME!");
    // }
    //
    // cmdDelay = max(DMA, cmdDelay);
    //
    // DPRINTF(NVMeAll, "NVM     | READ  | NSID %-5d| Tick %" PRIu64 "\n", nsid,
    //         cmdDelay);
  }
}

void Namespace::datasetManagement(SQEntryWrapper &req, CQEntryWrapper &resp,
                           uint64_t &tick) {
  bool err = false;

  // int nr = (req.entry.dword10 & 0xFF) + 1;
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
    // uint64_t slpn;
    // uint16_t nlp, off;
    // PRPList PRP(pParent->getParent(), cmd.entry.DPTR1, cmd.entry.DPTR2,
    //             (size_t)0x1000);
    // static Range range;
    //
    // for (int i = 0; i < nr; i++) {
    //   cmdDelay += PRP.read(i * 0x10, 0x10, range.uiData);
    //
    //   slpn = (range.slba + offset) / lbaratio;
    //   off = range.slba % lbaratio;
    //   nlp = (range.nlb + off + lbaratio - 1) / lbaratio;
    //
    //   cmdDelay = ftl->trim(slpn, nlp);
    // }
    //
    // DPRINTF(NVMeAll, "NVM     | TRIM  | NSID %-5d| Tick %" PRIu64 "\n", nsid,
    //         cmdDelay);
    // DPRINTF(NVMeBreakdown, "N%u|2|T|I%d|F%" PRIu64 "\n", nsid,
    //         req.entry.dword0.CID, cmdDelay);
  }
}

}  // namespace NVMe

}  // namespace HIL

}  // namespace SimpleSSD
