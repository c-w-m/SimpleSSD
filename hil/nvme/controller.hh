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

#ifndef __HIL_NVME_CONTROLLER__
#define __HIL_NVME_CONTROLLER__

#include "hil/nvme/config.hh"
#include "hil/nvme/def.hh"
#include "hil/nvme/dma.hh"
#include "hil/nvme/interface.hh"
#include "hil/nvme/queue.hh"
#include "util/list.hh"

namespace SimpleSSD {

namespace NVMe {

typedef union _RegisterTable {
  uint8_t data[64];
  struct {
    uint64_t capabilities;
    uint32_t version;
    uint32_t interruptMaskSet;
    uint32_t interruptMaskClear;
    uint32_t configuration;
    uint32_t reserved;
    uint32_t status;
    uint32_t subsystemReset;
    uint32_t adminQueueAttributes;
    uint64_t adminSQueueBaseAddress;
    uint64_t adminCQueueBaseAddress;
    uint32_t memoryBufferLocation;
    uint32_t memoryBufferSize;
  };

  _RegisterTable();
} RegisterTable;

class Controller {
 private:
  Interface *parent;     //!< NVMe::Interface passed from constructor
  Subsystem *subsystem;  //!< NVMe::Subsystem allocate in constructor

  DMAScheduler *dmaEngine;  //!< DMA engine allocate in constructor

  RegisterTable registers;  //!< Table for NVMe Controller Registers
  uint64_t sqstride;        //!< Calculated SQ Stride
  uint64_t cqstride;        //!< Calculated CQ stride
  bool adminQueueInited;    //!< Flag for initialization of Admin CQ/SQ
  uint16_t arbitration;     //!< Selected Arbitration Mechanism
  uint32_t interruptMask;   //!< Variable to store current interrupt mask

  CQueue *pCQueue;  //!< Completion Queue array
  SQueue *pSQueue;  //!< Submission Queue array

  List<SQEntryWrapper *> lSQFIFO;  //!< Internal FIFO queue for submission
  List<CQEntryWrapper *> lCQFIFO;  //!< Internal FIFO queue for completion

 public:
  Controller(Interface *, Config *);
  ~Controller();

  uint64_t readRegister(uint64_t, uint64_t, uint8_t *, uint64_t);
  uint64_t writeRegister(uint64_t, uint64_t, uint8_t *, uint64_t);
  uint64_t ringCQHeadDoorbell(uint16_t, uint16_t, uint64_t);
  uint64_t ringSQHeadDoorbell(uint16_t, uint16_t, uint64_t);

  void clearInterrupt(uint16_t);
  void updateInterrupt(uint16_t, bool);

  int createCQueue(uint16_t, uint16_t, uint16_t, bool, bool, uint64_t);
  int createSQueue(uint16_t, uint16_t, uint16_t, uint8_t, bool, uint64_t);
  int deleteCQueue(uint16_t);
  int deleteSQueue(uint16_t);
  int abort(uint16_t, uint16_t);
  void identify(uint8_t *);

  void collectSQueue();
};

}  // namespace NVMe

}  // namespace SimpleSSD

#endif
