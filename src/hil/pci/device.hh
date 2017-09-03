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

#ifndef __HIL_PCI_DEVICE__
#define __HIL_PCI_DEVICE__

#include "base/types.hh"
#include "src/config.hh"
#include "src/hil/pci/def.hh"

namespace SimpleSSD {

namespace PCI {

class Host {
 public:
  /* Functions for handling DMA accesses from Device */
  virtual uint64_t readDMA(uint64_t, uint64_t, uint8_t *);
  virtual uint64_t writeDMA(uint64_t, uint64_t, uint8_t *);

  /* Functions for handling Pin-based Interrupt from Device */
  virtual void postInterrupt();
  virtual void clearInterrupt();
};

class Device {
 protected:
  Config configuration;

 public:
  Device();

  /* Functions for access PCI configuration region */
  virtual uint64_t readConfig(uint64_t, uint64_t, uint8_t *);
  virtual uint64_t writeConfig(uint64_t, uint64_t, uint8_t *);

  /* Functions for access BAR region */
  virtual uint64_t read(uint64_t, uint64_t, uint8_t *);
  virtual uint64_t write(uint64_t, uint64_t, uint8_t *);

  /* Functions for DMA */
  virtual uint64_t readDMA(uint64_t, uint64_t, uint8_t *);
  virtual uint64_t writeDMA(uint64_t, uint64_t, uint8_t *);

  /* Functions for Pin-based Interrupt */
  virtual void postInterrupt();
  virtual void clearInterrupt();
};

}  // namespace PCI

}  // namespace SimpleSSD

#endif
