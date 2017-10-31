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

#include "pal/pal.hh"
#include "util/config.hh"

namespace SimpleSSD {

namespace FTL {

class FTL {
 public:
  typedef struct {
    uint64_t totalPhysicalBlocks;
    uint64_t totalLogicalBlocks;
    uint64_t pagesInBlock;
    uint32_t pageSize;
  } Parameter;

 private:
  Parameter param;
  PAL::PAL *pPAL;

  ConfigReader *pConf;

 public:
  FTL(ConfigReader *);
  ~FTL();

  Parameter *getInfo();
};

}  // namespace FTL

}  // namespace SimpleSSD

#endif
