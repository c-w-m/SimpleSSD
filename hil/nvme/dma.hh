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

#ifndef __HIL_NVME_DMA__
#define __HIL_NVME_DMA__

#include "base/types.hh"
#include "hil/nvme/config.hh"
#include "hil/nvme/controller.hh"
#include "util/vector.hh"

#ifndef SIMPLESSD_STANDALONE
#include "sim/core.hh"
#endif

namespace SimpleSSD {

namespace NVMe {

class DMAScheduler {
 private:
  Tick lastReadEndAt;
  Tick lastWriteEndAt;
  double psPerByte;

 public:
  DMAScheduler(Controller *, Config *);

  Tick read(uint64_t, uint64_t, uint8_t *, Tick = curTick());
  Tick write(uint64_t, uint64_t, uint8_t *, Tick = curTick());
};

struct PRP {
  Addr addr;
  uint64_t size;

  PRP(Addr, uint64_t);
};

class PRPList {
 private:
  DMAScheduler *dmaEngine;
  Vector<PRP *> prpList;
  uint64_t totalSize;

  void getPRPListFromPRP(Addr, uint64_t);

 public:
  PRPList();
  PRPList(DMAScheduler *, Addr, Addr, uint64_t);
  PRPList(DMAScheduler *, Addr, uint64_t, bool);

  Tick read(uint64_t, uint64_t, uint8_t *, Tick = curTick());
  Tick write(uint64_t, uint64_t, uint8_t *, Tick = curTick());
};

}  // namespace NVMe

}  // namespace SimpleSSD

#endif
