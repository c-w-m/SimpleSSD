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

#ifndef __FTL_FTL__
#define __FTL_FTL__

#include "ftl/common/latency.hh"
#include "pal/pal.hh"
#include "util/config.hh"
#include "util/def.hh"

namespace SimpleSSD {

namespace FTL {

class AbstractFTL;

typedef struct {
  uint64_t totalPhysicalBlocks;  //!< (PAL::Parameter::superBlock)
  uint64_t totalLogicalBlocks;
  uint64_t pagesInBlock;  //!< (PAL::Parameter::page)
  uint32_t pageSize;      //!< Mapping unit (PAL::Parameter::superPageSize)
  uint32_t ioUnitInPage;  //!< # smallest I/O unit in one page
  uint32_t pageCountToMaxPerf;  //!< # pages to fully utilize internal parallism
} Parameter;

class FTL {
 private:
  Parameter param;
  PAL::PAL *pPAL;

  ConfigReader *pConf;
  AbstractFTL *pFTL;

  Latency *pLatency;

 public:
  FTL(ConfigReader *);
  ~FTL();

  void read(Request &, uint64_t &);
  void write(Request &, uint64_t &);
  void trim(Request &, uint64_t &);

  void format(LPNRange &, uint64_t &);

  Parameter *getInfo();
  uint64_t getUsedPageCount();
};

}  // namespace FTL

}  // namespace SimpleSSD

#endif
