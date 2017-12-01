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

#include "ftl/page_mapping.hh"

#include "ftl/old/ftl.hh"
#include "ftl/old/ftl_defs.hh"
#include "log/trace.hh"
#include "pal/old/Latency.h"
#include "pal/old/LatencyMLC.h"
#include "pal/old/LatencySLC.h"
#include "pal/old/LatencyTLC.h"
#include "pal/old/PAL2.h"
#include "pal/old/PALStatistics.h"
#include "util/algorithm.hh"

namespace SimpleSSD {

namespace FTL {

PageMapping::PageMapping(Parameter *p, PAL::PAL *l, ConfigReader *c)
    : AbstractFTL(p, l), pPAL(l), conf(c->ftlConfig), pFTLParam(p) {}

PageMapping::~PageMapping() {}

bool PageMapping::initialize() {
  uint64_t nPagesToWarmup;
  uint64_t nTotalPages;
  uint64_t tick = 0;
  Request req;

  nTotalPages = pFTLParam->totalLogicalBlocks * pFTLParam->pagesInBlock;
  nPagesToWarmup = nTotalPages * conf.readFloat(FTL_WARM_UP_RATIO);
  nPagesToWarmup = MIN(nPagesToWarmup, nTotalPages);

  for (uint64_t lpn = 0; lpn < pFTLParam->totalLogicalBlocks;
       lpn += nTotalPages) {
    for (uint32_t page = 0; page < nPagesToWarmup; page++) {
      req.lpn = lpn + page;

      writeInternal(req, tick, false);
    }
  }

  return true;
}

void PageMapping::read(Request &req, uint64_t &tick) {
  uint64_t begin = tick;

  readInternal(req, tick);

  Logger::debugprint(Logger::LOG_FTL_OLD,
                     "READ  | LPN %" PRIu64 " | %" PRIu64 " - %" PRIu64
                     " (%" PRIu64 ")",
                     req.lpn, begin, tick, tick - begin);
}

void PageMapping::write(Request &req, uint64_t &tick) {
  uint64_t begin = tick;

  writeInternal(req, tick);

  Logger::debugprint(Logger::LOG_FTL_OLD,
                     "WRITE | LPN %" PRIu64 " | %" PRIu64 " - %" PRIu64
                     " (%" PRIu64 ")",
                     req.lpn, begin, tick, tick - begin);
}

void PageMapping::trim(Request &req, uint64_t &tick) {
  uint64_t begin = tick;

  trimInternal(req, tick);

  Logger::debugprint(Logger::LOG_FTL_OLD,
                     "TRIM  | LPN %" PRIu64 " | %" PRIu64 " - %" PRIu64
                     " (%" PRIu64 ")",
                     req.lpn, begin, tick, tick - begin);
}

float PageMapping::freeBlockRatio() {
  return (float)blocks.size() / pFTLParam->totalPhysicalBlocks;
}

void PageMapping::selectVictimBlock(std::vector<uint32_t> &list) {}

void PageMapping::doGarbageCollection(uint64_t &tick) {}

void PageMapping::readInternal(Request &req, uint64_t &tick) {}

void PageMapping::writeInternal(Request &req, uint64_t &tick, bool sendToPAL) {}

void PageMapping::trimInternal(Request &req, uint64_t &tick) {}

void PageMapping::eraseInternal(uint32_t blockIndex, uint64_t &tick) {}

}  // namespace FTL

}  // namespace SimpleSSD
