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

#include "hil/nvme/controller.hh"

namespace SimpleSSD {

namespace NVMe {

RegisterTable::_RegisterTable() {
  memset(data, 0, 64);
}

Controller::Controller(Interface *intrface, Config *conf)
    : pParent(intrface), adminQueueInited(false), interruptMask(0) {
  pDmaEngine = new DMAScheduler(pParent, conf);

  // Allocate array for Command Queues
  ppCQueue = (CQueue **)calloc(conf->readUint(NVME_MAX_IO_CQUEUE) + 1,
                               sizeof(CQueue *));
  ppSQueue = (SQueue **)calloc(conf->readUint(NVME_MAX_IO_SQUEUE) + 1,
                               sizeof(SQueue *));

  // [Bits ] Name  : Description                     : Current Setting
  // [63:56] Reserved
  // [55:52] MPSMZX: Memory Page Size Maximum        : 2^14 Bytes
  // [51:48] MPSMIN: Memory Page Size Minimum        : 2^12 Bytes
  // [47:45] Reserved
  // [44:37] CSS   : Command Sets Supported          : NVM command set
  // [36:36] NSSRS : NVM Subsystem Reset Supported   : No
  // [35:32] DSTRD : Doorbell Stride                 : 0 (4 bytes)
  // [31:24] TO    : Timeout                         : 40 * 500ms
  // [23:19] Reserved
  // [18:17] AMS   : Arbitration Mechanism Supported : Weighted Round Robin
  // [16:16] CQR   : Contiguous Queues Required      : Yes
  // [15:00] MQES  : Maximum Queue Entries Supported : 4096 Entries
  registers.capabilities = 0x0020002028010FFF;
  registers.version = 0x00010201;  // NVMe 1.2.1

  cfgdata.conf = conf;
  cfgdata.maxQueueEntry = (registers.capabilities & 0xFFFF) + 1;

  pSubsystem = new Subsystem(this, &cfgdata);
}

Controller::~Controller() {
  delete pSubsystem;

  for (uint16_t i = 0; i < cfgdata.conf->readUint(NVME_MAX_IO_CQUEUE) + 1;
       i++) {
    if (ppCQueue[i]) {
      delete ppCQueue[i];
    }
  }

  for (uint16_t i = 0; i < cfgdata.conf->readUint(NVME_MAX_IO_SQUEUE) + 1;
       i++) {
    if (ppSQueue[i]) {
      delete ppSQueue[i];
    }
  }

  free(ppCQueue);
  free(ppSQueue);
}

uint64_t Controller::readRegister(uint64_t offset, uint64_t size,
                                  uint8_t *buffer, uint64_t tick) {
  registers.interruptMaskSet = interruptMask;
  registers.interruptMaskClear = interruptMask;

  memcpy(buffer, registers.data + offset, size);
  tick = pDmaEngine->read(0, size, nullptr, tick);

  /*
  TODO:
  switch (offset) {
    case REG_CONTROLLER_CAPABILITY:
    case REG_CONTROLLER_CAPABILITY + 4:   DPRINTF(NVMeAll, "BAR0    | READ  |
  Controller Capabilities\n");              break; case REG_VERSION:
  DPRINTF(NVMeAll, "BAR0    | READ  | Version\n"); break; case
  REG_INTERRUPT_MASK_SET:          DPRINTF(NVMeAll, "BAR0    | READ  | Interrupt
  Mask Set\n");                   break; case REG_INTERRUPT_MASK_CLEAR:
  DPRINTF(NVMeAll, "BAR0    | READ  | Interrupt Mask Clear\n"); break; case
  REG_CONTROLLER_CONFIG:           DPRINTF(NVMeAll, "BAR0    | READ  |
  Controller Configuration\n");             break; case REG_CONTROLLER_STATUS:
  DPRINTF(NVMeAll, "BAR0    | READ  | Controller Status\n"); break; case
  REG_NVM_SUBSYSTEM_RESET:         DPRINTF(NVMeAll, "BAR0    | READ  | NVM
  Subsystem Reset\n");                  break; case REG_ADMIN_QUEUE_ATTRIBUTE:
  DPRINTF(NVMeAll, "BAR0    | READ  | Admin Queue Attributes\n"); break; case
  REG_ADMIN_SQUEUE_BASE_ADDR: case REG_ADMIN_SQUEUE_BASE_ADDR + 4:
  DPRINTF(NVMeAll, "BAR0    | READ  | Admin Submission Queue Base Address\n");
  break; case REG_ADMIN_CQUEUE_BASE_ADDR: case REG_ADMIN_CQUEUE_BASE_ADDR + 4:
  DPRINTF(NVMeAll, "BAR0    | READ  | Admin Completion Queue Base Address\n");
  break; case REG_CMB_LOCATION:                DPRINTF(NVMeAll, "BAR0    | READ
  | Controller Memory Buffer Location\n");    break; case REG_CMB_SIZE:
  DPRINTF(NVMeAll, "BAR0    | READ  | Controller Memory Buffer Size\n"); break;
  }

  if (size == 4) {
    DPRINTF(NVMeDMA, "DMAPORT | READ  | DATA %08" PRIX32 "\n", *(uint32_t
  *)buffer);
  }
  else {
    DPRINTF(NVMeDMA, "DMAPORT | READ  | DATA %016" PRIX64 "\n", *(uint64_t
  *)buffer);
  }
  */

  return tick;
}

uint64_t Controller::writeRegister(uint64_t offset, uint64_t size,
                                   uint8_t *buffer, uint64_t tick) {
  uint32_t uiTemp32;
  uint64_t uiTemp64;

  tick = pDmaEngine->write(0, size, nullptr, tick);

  if (size == 4) {
    memcpy(&uiTemp32, buffer, 4);

    switch (offset) {
      case REG_INTERRUPT_MASK_SET:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Interrupt Mask Set\n");

        interruptMask |= uiTemp32;

        break;
      case REG_INTERRUPT_MASK_CLEAR:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Interrupt Mask Clear\n");

        interruptMask &= ~uiTemp32;

        break;
      case REG_CONTROLLER_CONFIG:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Controller Configuration\n");

        registers.configuration &= 0xFF00000E;
        registers.configuration |= (uiTemp32 & 0x00FFFFF1);

        // Update entry size
        sqstride = (int)powf(2.f, (registers.configuration & 0x000F0000) >> 16);
        cqstride = (int)powf(2.f, (registers.configuration & 0x00F00000) >> 20);

        // Update Memory Page Size
        cfgdata.memoryPageSizeOrder =
            ((registers.configuration & 0x780) >> 7) + 11;  // CC.MPS + 12 - 1
        cfgdata.memoryPageSize =
            (int)powf(2.f, cfgdata.memoryPageSizeOrder + 1);

        // Update Arbitration Mechanism
        arbitration = (registers.configuration & 0x00003800) >> 11;

        // Apply to admin queue
        if (ppCQueue[0]) {
          ppCQueue[0]->setBase(
              new PRPList(pDmaEngine, cfgdata.memoryPageSize,
                          registers.adminCQueueBaseAddress,
                          ppCQueue[0]->getSize() * cqstride, true),
              cqstride);
        }
        if (ppSQueue[0]) {
          ppSQueue[0]->setBase(
              new PRPList(pDmaEngine, cfgdata.memoryPageSize,
                          registers.adminSQueueBaseAddress,
                          ppSQueue[0]->getSize() * sqstride, true),
              sqstride);
        }

        // If EN = 1, Set CSTS.RDY = 1
        if (registers.configuration & 0x00000001) {
          registers.status |= 0x00000001;

          pParent->enableController(
              cfgdata.conf->readUint(NVME_QUEUE_INTERVAL));
        }
        // If EN = 0, Set CSTS.RDY = 0
        else {
          registers.status &= 0xFFFFFFFE;

          pParent->disableController();
        }

        break;
      case REG_CONTROLLER_STATUS:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Controller Status\n");

        // Clear NSSRO if set
        if (uiTemp32 & 0x00000010) {
          registers.status &= 0xFFFFFFEF;
        }

        break;
      case REG_NVM_SUBSYSTEM_RESET:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | NVM Subsystem Reset\n");

        registers.subsystemReset = uiTemp32;

        // FIXME: If NSSR is same as NVMe(0x4E564D65), do NVMe Subsystem reset
        // (when CAP.NSSRS is 1)
        break;
      case REG_ADMIN_QUEUE_ATTRIBUTE:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Admin Queue Attributes\n");

        registers.adminQueueAttributes &= 0xF000F000;
        registers.adminQueueAttributes |= (uiTemp32 & 0x0FFF0FFF);

        break;
      case REG_ADMIN_CQUEUE_BASE_ADDR:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Admin Completion Queue Base
        // Address | L\n");

        memcpy(&(registers.adminCQueueBaseAddress), buffer, 4);
        adminQueueInited++;

        break;
      case REG_ADMIN_CQUEUE_BASE_ADDR + 4:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Admin Completion Queue Base
        // Address | H\n");

        memcpy(((uint8_t *)&(registers.adminCQueueBaseAddress)) + 4, buffer, 4);
        adminQueueInited++;

        break;
      case REG_ADMIN_SQUEUE_BASE_ADDR:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Admin Submission Queue Base
        // Address | L\n");
        memcpy(&(registers.adminSQueueBaseAddress), buffer, 4);
        adminQueueInited++;

        break;
      case REG_ADMIN_SQUEUE_BASE_ADDR + 4:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Admin Submission Queue Base
        // Address | H\n");
        memcpy(((uint8_t *)&(registers.adminSQueueBaseAddress)) + 4, buffer, 4);
        adminQueueInited++;

        break;
      default:
        // panic("nvme_ctrl: Write on read only register\n");
        break;
    }

    // DPRINTF(NVMeDMA, "DMAPORT | WRITE | DATA %08" PRIX32 "\n", uiTemp32);
  }
  else if (size == 8) {
    memcpy(&uiTemp64, buffer, 8);

    switch (offset) {
      case REG_ADMIN_CQUEUE_BASE_ADDR:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Admin Completion Queue Base
        // Address\n");

        registers.adminCQueueBaseAddress = uiTemp64;
        adminQueueInited += 2;

        break;
      case REG_ADMIN_SQUEUE_BASE_ADDR:
        // DPRINTF(NVMeAll, "BAR0    | WRITE | Admin Submission Queue Base
        // Address\n");

        registers.adminSQueueBaseAddress = uiTemp64;
        adminQueueInited += 2;

        break;
      default:
        // panic("nvme_ctrl: Write on read only register\n");
        break;
    }

    // DPRINTF(NVMeDMA, "DMAPORT | WRITE | DATA %016" PRIX64 "\n", uiTemp64);
  }
  else {
    // panic("nvme_ctrl: Invalid read size(%d) on controller register\n", size);
  }

  if (adminQueueInited == 4) {
    uint16_t entrySize = 0;

    adminQueueInited = 0;

    entrySize = ((registers.adminQueueAttributes & 0x0FFF0000) >> 16) + 1;
    ppCQueue[0] = new CQueue(0, true, 0, entrySize);

    // DPRINTF(NVMeQueue, "CQ 0    | CREATE | Entry size %d\n", entrySize);

    entrySize = (registers.adminQueueAttributes & 0x0FFF) + 1;
    ppSQueue[0] = new SQueue(0, 0, 0, entrySize);

    // DPRINTF(NVMeQueue, "SQ 0    | CREATE | Entry size %d\n", entrySize);
  }

  return tick;
}

uint64_t Controller::ringCQHeadDoorbell(uint16_t qid, uint16_t head,
                                        uint64_t tick) {
  CQueue *pQueue = ppCQueue[qid];

  if (pQueue) {
    pQueue->setHead(head);
    /*
        DPRINTF(NVMeQueue, "CQ %-5d| Completion Queue Head Doorbell | Item count
       in queue %d | head %d | tail %d\n", qid, pQueue->getItemCount(),
       pQueue->getHead(), pQueue->getTail()); DPRINTF(NVMeBreakdown,
       "C%d|4|H%d|T%d\n", qid, pQueue->getHead(), pQueue->getTail());*/

    if (pQueue->interruptEnabled()) {
      clearInterrupt(pQueue->getInterruptVector());
    }
  }
}

uint64_t Controller::ringSQTailDoorbell(uint16_t qid, uint16_t tail,
                                        uint64_t tick) {
  SQueue *pQueue = ppSQueue[qid];

  if (pQueue) {
    pQueue->setTail(tail);
    /*
        DPRINTF(NVMeQueue, "SQ %-5d| Submission Queue Tail Doorbell | Item count
       in queue %d | head %d | tail %d\n", qid, pQueue->getItemCount(),
       pQueue->getHead(), pQueue->getTail()); DPRINTF(NVMeBreakdown,
       "S%d|0|H%d|T%d\n", qid, pQueue->getHead(), pQueue->getTail());*/
  }
}

void Controller::clearInterrupt(uint16_t interruptVector) {
  uint16_t notFinished = 0;

  // Check all queues associated with same interrupt vector are processed
  for (uint16_t i = 0; i < cfgdata.conf->readUint(NVME_MAX_IO_CQUEUE) + 1;
       i++) {
    if (ppCQueue[i]) {
      if (ppCQueue[i]->getInterruptVector() == interruptVector) {
        notFinished += ppCQueue[i]->getItemCount();
      }
    }
  }

  // Update interrupt
  updateInterrupt(interruptVector, notFinished > 0);
}

void Controller::updateInterrupt(uint16_t interruptVector, bool post) {
  pParent->updateInterrupt(interruptVector, post);
}

void Controller::submit(CQEntryWrapper &entry) {
  CQueue *pQueue = ppCQueue[entry.cqID];

  if (pQueue == NULL) {
    // panic("nvme_ctrl: Completion Queue not created! CQID %d\n", entry.cqid);
  }

  // Enqueue with delay
  auto iter = lCQFIFO.begin();

  for (; iter != lCQFIFO.end(); iter++) {
    if (iter->submitAt > entry.submitAt) {
      break;
    }
  }

  lCQFIFO.insert(iter, entry);
}

int Controller::createCQueue(uint16_t cqid, uint16_t size, uint16_t iv,
                             bool ien, bool pc, uint64_t prp1) {
  int ret = 1;  // Invalid Queue ID

  if (ppCQueue[cqid] == NULL) {
    ppCQueue[cqid] = new CQueue(iv, ien, cqid, size);
    ppCQueue[cqid]->setBase(new PRPList(pDmaEngine, cfgdata.memoryPageSize,
                                        prp1, size * cqstride, pc),
                            cqstride);

    ret = 0;

    // DPRINTF(NVMeQueue, "CQ %-5d| CREATE | Entry size %d | IV %04X | IEN %s |
    // PC %s\n", cqid, entrySize, iv, BOOLEAN_STRING(ien), BOOLEAN_STRING(pc));
  }

  return ret;
}

int Controller::createSQueue(uint16_t sqid, uint16_t cqid, uint16_t size,
                             uint8_t priority, bool pc, uint64_t prp1) {
  int ret = 1;  // Invalid Queue ID

  if (ppSQueue[sqid] == NULL) {
    if (ppCQueue[cqid] != NULL) {
      ppSQueue[sqid] = new SQueue(cqid, priority, sqid, size);
      ppSQueue[sqid]->setBase(new PRPList(pDmaEngine, cfgdata.memoryPageSize,
                                          prp1, size * sqstride, pc),
                              sqstride);

      ret = 0;

      // DPRINTF(NVMeQueue, "SQ %-5d| CREATE | Entry size %d | Priority %d | PC
      // %s\n", cqid, entrySize, priority, BOOLEAN_STRING(pc));
    }
    else {
      ret = 2;  // Invalid CQueue
    }
  }

  return ret;
}

int Controller::deleteCQueue(uint16_t cqid) {
  int ret = 0;  // Success

  if (ppCQueue[cqid] != NULL && cqid > 0) {
    for (uint16_t i = 1; i < cfgdata.conf->readUint(NVME_MAX_IO_CQUEUE) + 1; i++) {
      if (ppSQueue[i]) {
        if (ppSQueue[i]->getCQID() == cqid) {
          ret = 2;  // Invalid Queue Deletion
          break;
        }
      }
    }

    if (ret == 0) {
      delete ppCQueue[cqid];
      ppCQueue[cqid] = NULL;

      // DPRINTF(NVMeQueue, "CQ %-5d| DELETE\n", cqid);
    }
  }
  else {
    ret = 1;  // Invalid Queue ID
  }

  return ret;
}

int Controller::deleteSQueue(uint16_t sqid) {
  int ret = 0;  // Success

  if (ppSQueue[sqid] != NULL && sqid > 0) {
    // Create abort response
    uint16_t sqHead = ppSQueue[sqid]->getHead();
    uint16_t status = 0x8000 | (TYPE_GENERIC_COMMAND_STATUS << 9) |
                     (STATUS_ABORT_DUE_TO_SQ_DELETE << 1);

    // Abort all commands in SQueue
    for (auto iter = lSQFIFO.begin(); iter != lSQFIFO.end(); iter++) {
      if (iter->sqID == sqid) {
        CQEntryWrapper wrapper(*iter);
        wrapper.entry.dword2.sqHead = sqHead;
        wrapper.entry.dword3.status = status;
        submit(wrapper);

        iter = lSQFIFO.erase(iter);
      }
    }

    // Delete SQueue
    delete ppSQueue[sqid];
    ppSQueue[sqid] = NULL;

    // DPRINTF(NVMeQueue, "SQ %-5d| DELETE\n", sqid);
  }
  else {
    ret = 1;  // Invalid Queue ID
  }

  return ret;
}

int Controller::abort(uint16_t sqid, uint16_t cid) {
  int ret = 0;  // Not aborted
  uint16_t sqHead;
  uint16_t status;

  for (auto iter = lSQFIFO.begin(); iter != lSQFIFO.end(); iter++) {
    if (iter->sqID == sqid && iter->entry.dword0.commandID == cid) {
      CQEntry entry;

      // Create abort response
      sqHead = ppSQueue[sqid]->getHead();
      status = 0x8000 | (TYPE_GENERIC_COMMAND_STATUS << 9) |
                       (STATUS_ABORT_REQUESTED << 1);

      // Submit abort
      CQEntryWrapper wrapper(*iter);
      wrapper.entry.dword2.sqHead = sqHead;
      wrapper.entry.dword3.status = status;

      submit(wrapper);

      // Remove
      iter = lSQFIFO.erase(iter);
      ret = 1;  // Aborted

      break;
    }
  }

  return ret;
}

void Controller::identify(uint8_t *data) {}

void Controller::collectSQueue() {}

}  // namespace NVMe

}  // namespace SimpleSSD
