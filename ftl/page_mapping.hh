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

#ifndef __FTL_PAGE_MAPPING__
#define __FTL_PAGE_MAPPING__

#include <cinttypes>
#include <unordered_map>

#include "ftl/abstract_ftl.hh"
#include "ftl/common/block.hh"
#include "ftl/ftl.hh"

class PAL2;
class PALStatistics;
class Latency;

namespace SimpleSSD {

namespace FTL {

class PageMapping : public AbstractFTL {
 private:
  ::PAL2 *pal;
  ::PALStatistics *stats;
  ::Latency *lat;

  Config &conf;
  Parameter *pFTLParam;

  std::unordered_map<uint64_t, std::pair<uint32_t, uint32_t>> table;
  std::unordered_map<uint32_t, uint32_t> eraseCount;
  std::unordered_map<uint32_t, Block> blocks;

  float freeBlockRatio();
  void selectVictimBlock(std::vector<uint32_t> &);
  void doGarbageCollection(uint64_t &);
  void readInternal(Request &, uint64_t &);
  void writeInternal(Request &, uint64_t &, bool = true);
  void trimInternal(Request &, uint64_t &);
  void eraseInternal(uint32_t, uint64_t &);

 public:
  PageMapping(Parameter *, PAL::PAL *, ConfigReader *);
  ~PageMapping() override;

  bool initialize() override;

  void read(Request &, uint64_t &) override;
  void write(Request &, uint64_t &) override;
  void trim(Request &, uint64_t &) override;
};

}  // namespace FTL

}  // namespace SimpleSSD

#endif