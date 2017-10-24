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

#include "hil/nvme/def.hh"
#include "hil/nvme/dma.hh"
#include "hil/nvme/queue.hh"
#include "hil/nvme/interface.hh"
#include "util/list.hh"

namespace SimpleSSD {

namespace NVMe {

class Controller {
 private:
  Interface *parent;
  Subsystem *subsystem;

  DMAScheduler *dmaEngine;

  RegisterTable registers;
  uint64_t sqstride;
  uint64_t cqstride;
  bool adminQueueInited;
  uint16_t arbitration;

 public:
  Controller(Interface *);
  ~Controller();
};

}  // namespace NVMe

}  // namespace SimpleSSD

#endif
