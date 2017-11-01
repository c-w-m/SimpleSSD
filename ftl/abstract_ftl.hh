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

#ifndef __FTL_ABSTRACT_FTL__
#define __FTL_ABSTRACT_FTL__

#include <cinttypes>

#include "ftl/ftl.hh"

namespace SimpleSSD {

namespace FTL {

class AbstractFTL {
 protected:
  Parameter *pParam;
  PAL::PAL *pPAL;

 public:
  AbstractFTL(Parameter *p, PAL::PAL *l) : pParam(p), pPAL(l) {}
  virtual ~AbstractFTL() = 0;

  virtual bool initialize() = 0;

  virtual void read(uint64_t, uint64_t &) = 0;
  virtual void write(uint64_t, uint64_t &) = 0;
  virtual void trim(uint64_t, uint64_t &) = 0;
};

}  // namespace FTL

}  // namespace SimpleSSD

#endif
