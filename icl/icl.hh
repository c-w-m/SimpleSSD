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

#ifndef __ICL_ICL__
#define __ICL_ICL__

#include "ftl/ftl.hh"
#include "icl/cache.hh"
#include "util/config.hh"

namespace SimpleSSD {

namespace ICL {

class ICL {
 private:
  FTL::FTL *pFTL;

  ConfigReader *pConf;
  Cache *pCache;

 public:
  ICL(ConfigReader *);
  ~ICL();

  void read(uint64_t, uint64_t, uint64_t &);
  void write(uint64_t, uint64_t, uint64_t &);
  void flush(uint64_t, uint64_t, uint64_t &);
  void trim(uint64_t, uint64_t, uint64_t &);

  void getLPNInfo(uint64_t &, uint32_t &);
};

}  // namespace ICL

}  // namespace SimpleSSD

#endif
