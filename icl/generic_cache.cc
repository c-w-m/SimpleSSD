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

#include "icl/generic_cache.hh"

#include <limits>

#include "log/trace.hh"
#include "util/algorithm.hh"

namespace SimpleSSD {

namespace ICL {

FlushData::FlushData() : setIdx(0), wayIdx(0), tag(0) {}

GenericCache::GenericCache(ConfigReader *c, FTL::FTL *f)
    : AbstractCache(c, f),
      lineCountInSuperPage(f->getInfo()->ioUnitInPage),
      superPageSize(f->getInfo()->pageSize),
      waySize(c->iclConfig.readUint(ICL_WAY_SIZE)),
      lineSize(superPageSize / lineCountInSuperPage),
      lineCountInMaxIO(f->getInfo()->pageCountToMaxPerf * lineCountInSuperPage),
      prefetchIOCount(c->iclConfig.readUint(ICL_PREFETCH_COUNT)),
      prefetchIORatio(c->iclConfig.readFloat(ICL_PREFETCH_RATIO)),
      useReadCaching(c->iclConfig.readBoolean(ICL_USE_READ_CACHE)),
      useWriteCaching(c->iclConfig.readBoolean(ICL_USE_WRITE_CACHE)),
      useReadPrefetch(c->iclConfig.readBoolean(ICL_USE_READ_PREFETCH)),
      gen(rd()),
      dist(std::uniform_int_distribution<>(0, waySize - 1)) {
  uint64_t cacheSize = c->iclConfig.readUint(ICL_CACHE_SIZE);

  setSize = MAX(cacheSize / lineSize / waySize, 1);

  // Set size should multiples of lineCountInSuperPage
  uint32_t left = setSize % lineCountInSuperPage;

  if (left) {
    if (left * 2 > lineCountInSuperPage) {
      setSize = (setSize / lineCountInSuperPage + 1) * lineCountInSuperPage;
    }
    else {
      setSize -= left;
    }

    if (setSize < 1) {
      setSize = lineCountInSuperPage;
    }
  }

  Logger::debugprint(
      Logger::LOG_ICL_GENERIC_CACHE,
      "CREATE  | Set size %u | Way size %u | Line size %u | Capacity %" PRIu64,
      setSize, waySize, lineSize, (uint64_t)setSize * waySize * lineSize);

  // TODO: replace this with DRAM model
  pTiming = c->iclConfig.getDRAMTiming();
  pStructure = c->iclConfig.getDRAMStructure();

  ppCache.resize(setSize);

  for (uint32_t i = 0; i < setSize; i++) {
    ppCache[i] = std::vector<Line>(waySize, Line());
  }

  lastRequest.reqID = 1;
  prefetchEnabled = false;
  hitCounter = 0;
  accessCounter = 0;

  // Set evict policy functional
  EVICT_POLICY policy = (EVICT_POLICY)c->iclConfig.readInt(ICL_EVICT_POLICY);

  switch (policy) {
    case POLICY_RANDOM:
      evictFunction = [this](uint32_t setIdx) -> uint32_t { return dist(gen); };

      break;
    case POLICY_FIFO:
      evictFunction = [this](uint32_t setIdx) -> uint32_t {
        uint32_t wayIdx = 0;
        uint64_t min = std::numeric_limits<uint64_t>::max();

        for (uint32_t i = 0; i < waySize; i++) {
          if (ppCache[setIdx][i].insertedAt < min) {
            min = ppCache[setIdx][i].insertedAt;
            wayIdx = i;
          }
        }

        return wayIdx;
      };

      break;
    case POLICY_LEAST_RECENTLY_USED:
      evictFunction = [this](uint32_t setIdx) -> uint32_t {
        uint32_t wayIdx = 0;
        uint64_t min = std::numeric_limits<uint64_t>::max();

        for (uint32_t i = 0; i < waySize; i++) {
          if (ppCache[setIdx][i].lastAccessed < min) {
            min = ppCache[setIdx][i].lastAccessed;
            wayIdx = i;
          }
        }

        return wayIdx;
      };

      break;
    default:
      evictFunction = [](uint32_t setIdx) -> uint32_t { return 0; };
      break;
  }
}

GenericCache::~GenericCache() {}

uint32_t GenericCache::calcSet(uint64_t lpn) {
  return lpn % setSize;
}

uint32_t GenericCache::getEmptyWay(uint32_t setIdx) {
  uint32_t wayIdx;

  for (wayIdx = 0; wayIdx < waySize; wayIdx++) {
    Line &line = ppCache[setIdx][wayIdx];

    if (!line.valid) {
      break;
    }
  }

  return wayIdx;
}

uint32_t GenericCache::getValidWay(uint64_t lpn) {
  uint32_t setIdx = calcSet(lpn);
  uint32_t wayIdx;

  for (wayIdx = 0; wayIdx < waySize; wayIdx++) {
    Line &line = ppCache[setIdx][wayIdx];

    if (line.valid && line.tag == lpn) {
      break;
    }
  }

  return wayIdx;
}

uint32_t GenericCache::getVictimWay(uint64_t lpn) {
  uint32_t setIdx = calcSet(lpn);
  uint32_t wayIdx;

  for (wayIdx = 0; wayIdx < waySize; wayIdx++) {
    Line &line = ppCache[setIdx][wayIdx];

    if ((line.valid && line.tag == lpn) || !line.valid) {
      break;
    }
  }

  if (wayIdx == waySize) {
    wayIdx = evictFunction(setIdx);
  }

  return wayIdx;
}

uint64_t GenericCache::calculateDelay(uint64_t bytesize) {
  uint64_t pageCount =
      (bytesize > 0) ? (bytesize - 1) / pStructure->pageSize + 1 : 0;
  uint64_t pageFetch = pTiming->tRP + pTiming->tRCD + pTiming->tCL;
  double bandwidth =
      2.0 * pStructure->busWidth * pStructure->channel / 8.0 / pTiming->tCK;

  return (uint64_t)(pageFetch + pageCount * pStructure->pageSize / bandwidth);
}

void GenericCache::checkPrefetch(Request &req) {
  if (lastRequest.reqID == req.reqID) {
    lastRequest.range = req.range;
    lastRequest.offset = req.offset;
    lastRequest.length = req.length;

    return;
  }

  if (lastRequest.range.slpn * lineSize + lastRequest.offset +
          lastRequest.length ==
      req.range.slpn * lineSize + req.offset) {
    if (!prefetchEnabled) {
      hitCounter++;
      accessCounter += req.length;

      if (hitCounter >= prefetchIOCount &&
          (float)accessCounter / superPageSize > prefetchIORatio) {
        prefetchEnabled = true;
      }
    }
  }
  else {
    prefetchEnabled = false;
    hitCounter = 0;
    accessCounter = 0;
  }

  lastRequest = req;
}

// True when hit
bool GenericCache::read(Request &req, uint64_t &tick) {
  bool ret = false;

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                     "READ  | REQ %7u-%-4u | LCA %" PRIu64 " | SIZE %" PRIu64,
                     req.reqID, req.reqSubID, req.range.slpn, req.length);

  if (useReadCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;
    static uint64_t lat = calculateDelay(sizeof(Line) + lineSize);

    // Check prefetch
    if (useReadPrefetch) {
      checkPrefetch(req);
    }

    // Check cache that we have data for corresponding LCA
    wayIdx = getValidWay(req.range.slpn);

    if (wayIdx != waySize) {
      uint64_t arrived = tick;

      // Wait for cache
      if (tick < ppCache[setIdx][wayIdx].insertedAt) {
        tick = ppCache[setIdx][wayIdx].insertedAt;
      }

      // Update cache line
      ppCache[setIdx][wayIdx].lastAccessed = tick;

      // Add tDRAM
      tick += lat;

      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "READ  | Cache hit at (%u, %u) | %" PRIu64
                         " - %" PRIu64 " (%" PRIu64 ")",
                         setIdx, wayIdx, arrived, tick, tick - arrived);

      // Data served from DRAM
      ret = true;
    }
    else {
      FTL::Request reqInternal(lineCountInSuperPage, req);
      std::vector<FlushData> list;
      FlushData data;

      if (prefetchEnabled) {
        // Read one superpage
        reqInternal.ioFlag.set();

        for (uint32_t i = 0; i < lineCountInSuperPage; i++) {
          data.tag = reqInternal.lpn * lineCountInSuperPage + i;
          data.setIdx = calcSet(data.tag);
          data.wayIdx = getVictimWay(data.tag);

          list.push_back(data);
        }
      }
      else {
        data.tag = req.range.slpn;
        data.setIdx = setIdx;
        data.wayIdx = getVictimWay(data.tag);

        list.push_back(data);
      }

      pFTL->read(reqInternal, tick);

      // Flush collected lines
      evictVictim(list, true, tick);
    }
  }
  else {
    FTL::Request reqInternal(lineCountInSuperPage, req);

    pFTL->read(reqInternal, tick);
  }

  return ret;
}

// True when cold-miss/hit
bool GenericCache::write(Request &req, uint64_t &tick) {
  bool ret = false;

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                     "WRITE | REQ %7u-%-4u | LCA %" PRIu64 " | SIZE %" PRIu64,
                     req.reqID, req.reqSubID, req.range.slpn, req.length);

  if (useWriteCaching) {
  }
  else {
    FTL::Request reqInternal(lineCountInSuperPage, req);

    pFTL->write(reqInternal, tick);
  }

  return ret;
}

// True when hit
bool GenericCache::flush(Request &req, uint64_t &tick) {
  bool ret = false;

  if (useReadCaching || useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;

    // Check cache that we have data for corresponding LBA
    wayIdx = getValidWay(req.range.slpn);

    // We have data to flush
    if (wayIdx != waySize) {
      FTL::Request reqInternal(lineCountInSuperPage);

      reqInternal.reqID = req.reqID;
      reqInternal.reqSubID = req.reqSubID;
      reqInternal.lpn = req.range.slpn / lineCountInSuperPage;
      reqInternal.ioFlag.set(req.range.slpn % lineCountInSuperPage);

      // We have data which is dirty
      if (ppCache[setIdx][wayIdx].dirty) {
        // we have to flush this
        pFTL->write(reqInternal, tick);
      }

      // invalidate
      ppCache[setIdx][wayIdx].valid = false;
    }
  }

  return ret;
}

// True when hit
bool GenericCache::trim(Request &req, uint64_t &tick) {
  bool ret = false;
  FTL::Request reqInternal(lineCountInSuperPage);

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                     "TRIM  | REQ %7u-%-4u | LCA %" PRIu64 " | SIZE %" PRIu64,
                     req.reqID, req.reqSubID, req.range.slpn, req.length);

  if (useReadCaching || useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;

    // Check cache that we have data for corresponding LBA
    wayIdx = getValidWay(req.range.slpn);

    if (wayIdx != waySize) {
      // invalidate
      ppCache[setIdx][wayIdx].valid = false;
    }
  }

  reqInternal.reqID = req.reqID;
  reqInternal.reqSubID = req.reqSubID;
  reqInternal.lpn = req.range.slpn / lineCountInSuperPage;
  reqInternal.ioFlag.set(req.range.slpn % lineCountInSuperPage);

  // we have to flush this
  pFTL->trim(reqInternal, tick);

  return ret;
}

void GenericCache::format(LPNRange &range, uint64_t &tick) {
  if (useReadCaching || useWriteCaching) {
    uint64_t lpn;
    uint32_t setIdx;
    uint32_t wayIdx;

    for (uint64_t i = 0; i < range.nlp; i++) {
      lpn = range.slpn + i;
      setIdx = calcSet(lpn);
      wayIdx = getValidWay(lpn);

      if (wayIdx != waySize) {
        // invalidate
        ppCache[setIdx][wayIdx].valid = false;
      }
    }
  }

  // Convert unit
  range.slpn /= lineCountInSuperPage;
  range.nlp = (range.nlp - 1) / lineCountInSuperPage + 1;

  pFTL->format(range, tick);
}

}  // namespace ICL

}  // namespace SimpleSSD
