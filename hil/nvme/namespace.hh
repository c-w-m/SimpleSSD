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

#ifndef __HIL_NVME_NAMESPACE__
#define __HIL_NVME_NAMESPACE__

#include "hil/nvme/subsystem.hh"

namespace SimpleSSD {

namespace HIL {

namespace NVMe {

typedef struct _LBARange {
  uint64_t slba;
  uint64_t nlblk;

  _LBARange();
  _LBARange(uint64_t, uint64_t);
} LBARange;

class Namespace {
 public:
  typedef struct _Information {
    uint64_t size;                         //!< NSZE
    uint64_t capacity;                     //!< NCAP
    uint64_t usage;                        //<! NUSE
    uint64_t sizeInByteL;                  //<! NVMCAPL
    uint64_t sizeInByteH;                  //<! NVMCAPH
    uint8_t feature;                       //!< NSFEAT
    uint8_t lbaFormatIndex;                //!< FLBAS
    uint8_t dataProtectionSettings;        //!< DPS
    uint8_t namespaceSharingCapabilities;  //!< NMIC

    uint32_t lbasize;
  } Information;

 private:
  Subsystem *pParent;
  ConfigData *pCfgdata;

  Information info;
  uint32_t nsid;
  bool attached;
  bool allocated;

  uint32_t lbaratio;  //!<NAND page size / LBA size

 public:
  Namespace(Subsystem *, ConfigData *);

  void setData(uint32_t, Information *);
  void attach(bool);
  uint32_t getNSID();
  void getIdentifyData(uint8_t *);
};

}  // namespace NVMe

}  // namespace HIL

}  // namespace SimpleSSD

#endif
