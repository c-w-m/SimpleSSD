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

FlushData::FlushData(uint32_t size)
    : setIdx(0), wayIdx(0), tag(0), valid(true), bitset(size) {}

GenericCache::GenericCache(ConfigReader *c, FTL::FTL *f)
    : AbstractCache(c, f), gen(rd()) {
  uint64_t cacheSize = c->iclConfig.readUint(ICL_CACHE_SIZE);
  waySize = c->iclConfig.readUint(ICL_WAY_SIZE);
  superPageSize = f->getInfo()->pageSize;

  dist = std::uniform_int_distribution<>(0, waySize - 1);

  useReadCaching = c->iclConfig.readBoolean(ICL_USE_READ_CACHE);
  useWriteCaching = c->iclConfig.readBoolean(ICL_USE_WRITE_CACHE);
  useReadPrefetch = c->iclConfig.readBoolean(ICL_USE_READ_PREFETCH);
  prefetchIOCount = c->iclConfig.readUint(ICL_PREFETCH_COUNT);
  prefetchIORatio = c->iclConfig.readFloat(ICL_PREFETCH_RATIO);

  policy = (EVICT_POLICY)c->iclConfig.readInt(ICL_EVICT_POLICY);

  lineCountInSuperPage = f->getInfo()->ioUnitInPage;
  lineSize = superPageSize / lineCountInSuperPage;

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
}

GenericCache::~GenericCache() {}

uint32_t GenericCache::calcSet(uint64_t lpn) {
  return lpn % setSize;
}

uint32_t GenericCache::getValidWay(uint64_t lpn, uint64_t &tick) {
  uint32_t setIdx = calcSet(lpn);
  uint32_t wayIdx;
  uint32_t count = 0;
  static const uint64_t lat = calculateDelay(sizeof(uint64_t) + 1);

  for (wayIdx = 0; wayIdx < waySize; wayIdx++) {
    count++;

    Line &line = ppCache[setIdx][wayIdx];

    if (line.valid && line.tag == lpn) {
      break;
    }
  }

  tick += count * lat;

  return wayIdx;
}

uint32_t GenericCache::getEmptyWay(uint32_t setIdx, uint64_t &tick) {
  uint32_t wayIdx;
  uint32_t count = 0;
  static const uint64_t lat = calculateDelay(1);

  for (wayIdx = 0; wayIdx < waySize; wayIdx++) {
    Line &line = ppCache[setIdx][wayIdx];

    if (!line.valid) {
      break;
    }
  }

  tick += count * lat;

  return wayIdx;
}

uint32_t GenericCache::getVictim(uint32_t setIdx, uint64_t &tick) {
  uint32_t wayIdx = 0;
  uint32_t count = 0;
  uint64_t min = std::numeric_limits<uint64_t>::max();
  static const uint64_t lat = calculateDelay(sizeof(uint64_t));

  switch (policy) {
    case POLICY_RANDOM:
      wayIdx = dist(gen);

      break;
    case POLICY_FIFO:
      for (uint32_t i = 0; i < waySize; i++) {
        count++;

        if (ppCache[setIdx][i].insertedAt < min) {
          min = ppCache[setIdx][i].insertedAt;
          wayIdx = i;
        }
      }

      break;
    case POLICY_LEAST_RECENTLY_USED:
      for (uint32_t i = 0; i < waySize; i++) {
        count++;

        if (ppCache[setIdx][i].lastAccessed < min) {
          min = ppCache[setIdx][i].lastAccessed;
          wayIdx = i;
        }
      }

      break;
  }

  tick += count * lat;

  return wayIdx;
}

uint32_t GenericCache::flushVictim(uint32_t setIdx, uint64_t &tick,
                                   bool *isCold, bool strict) {
  uint32_t wayIdx = waySize;

  if (isCold) {
    *isCold = true;
  }

  // Check set has empty entry
  wayIdx = getEmptyWay(setIdx, tick);

  // If no empty entry
  if (wayIdx == waySize) {
    if (isCold) {
      *isCold = false;
    }

    wayIdx = getVictim(setIdx, tick);

    // Let's flush 'em if dirty
    DynamicBitset found(lineCountInSuperPage);
    std::vector<FlushData> list;

    if (ppCache[setIdx][wayIdx].dirty) {
      uint32_t flushedIdx;
      FlushData data(lineCountInSuperPage);

      flushedIdx = ppCache[setIdx][wayIdx].tag % lineCountInSuperPage;
      data.setIdx = setIdx;
      data.wayIdx = wayIdx;
      data.tag = ppCache[setIdx][wayIdx].tag;
      data.bitset.set(flushedIdx);

      found.set(flushedIdx);
      list.push_back(data);
    }

    // Find more entries to flush to maximize internal parallelism
    uint64_t beginSet = 0;
    uint64_t endSet = setSize;

    if (strict) {
      beginSet = setIdx - (setIdx % lineCountInSuperPage);
      endSet = beginSet + lineCountInSuperPage;
    }

    for (setIdx = beginSet; setIdx < endSet; setIdx++) {
      if (!found.test(setIdx % lineCountInSuperPage)) {
        // Find cacheline to flush
        uint32_t way = getEmptyWay(setIdx, tick);

        if (way == waySize) {
          FlushData data(lineCountInSuperPage);

          data.setIdx = setIdx;
          data.wayIdx = getVictim(setIdx, tick);
          data.tag = ppCache[data.setIdx][data.wayIdx].tag;

          if (ppCache[data.setIdx][data.wayIdx].dirty) {
            data.bitset.set(setIdx % lineCountInSuperPage);
          }

          list.push_back(data);
          found.set(setIdx % lineCountInSuperPage);
        }
      }

      if (found.all()) {
        break;
      }
    }

    // Check if there is same super page
    for (auto i = list.begin(); i != list.end(); i++) {
      if (!i->valid) {
        continue;
      }

      for (auto j = i; j != list.end(); j++) {
        if (i->tag != j->tag &&
            i->tag / lineCountInSuperPage == j->tag / lineCountInSuperPage &&
            j->valid) {
          j->valid = false;
          i->bitset |= j->bitset;
        }
      }
    }

    // Flush collected entries
    uint64_t beginAt;
    uint64_t firstFlush = 0;
    FTL::Request reqInternal(lineCountInSuperPage);

    for (auto &iter : list) {
      if (iter.valid && iter.bitset.any()) {
        // Log
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "----- | Flush (%u, %u) | LCA %" PRIu64, iter.setIdx,
                           iter.wayIdx, iter.tag);

        beginAt = tick;

        reqInternal.lpn = iter.tag / lineCountInSuperPage;
        reqInternal.ioFlag = iter.bitset;

        // Flush
        pFTL->write(reqInternal, beginAt);

        if (firstFlush == 0) {
          firstFlush = beginAt;
        }
      }

      // Invalidate
      ppCache[iter.setIdx][iter.wayIdx].dirty = false;
      ppCache[iter.setIdx][iter.wayIdx].valid = false;

      // Clear
      reqInternal.ioFlag.reset();
    }

    tick = firstFlush;
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
                     "READ  | LCA %" PRIu64 " | SIZE %" PRIu64, req.range.slpn,
                     req.length);

  if (useReadCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;
    uint64_t lat = calculateDelay(sizeof(Line) + MIN(req.length, lineSize));

    // Check prefetch status
    if (useReadPrefetch) {
      checkPrefetch(req);
    }

    // Check cache that we have data for corresponding LBA
    wayIdx = getValidWay(req.range.slpn, tick);

    if (wayIdx != waySize) {
      ppCache[setIdx][wayIdx].lastAccessed = tick;

      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "READ  | Cache hit at (%u, %u) | %" PRIu64
                         " - %" PRIu64 " (%" PRIu64 ")",
                         setIdx, wayIdx, tick, tick + lat, lat);

      tick += lat;

      ret = true;
    }
    else {
      uint64_t beginAt = tick;
      uint64_t finishedAt = tick;
      FTL::Request reqInternal(lineCountInSuperPage);

      // We have to flush to store data to cache
      bool cold;

      wayIdx = flushVictim(setIdx, beginAt, &cold, true);
      finishedAt = beginAt;

      if (cold) {
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "READ  | Cache cold-miss, no need to flush", setIdx);
      }

      // Read missing data to cache
      std::vector<FlushData> list;
      FlushData data(1);  // bitset not used

      list.reserve(lineCountInSuperPage);

      reqInternal.reqID = req.reqID;
      reqInternal.reqSubID = req.reqSubID;
      reqInternal.lpn = req.range.slpn / lineCountInSuperPage;

      // Push flushed way index
      data.tag = req.range.slpn;
      data.setIdx = setIdx;
      data.wayIdx = wayIdx;

      list.push_back(data);
      reqInternal.ioFlag.set(req.range.slpn % lineCountInSuperPage);

      // We have to load one superpage
      if (prefetchEnabled) {
        // Ensure cache have space to load superpage
        uint64_t beginLPN = req.range.slpn - (setIdx % lineCountInSuperPage);
        uint32_t beginSet = setIdx - (setIdx % lineCountInSuperPage);
        uint32_t endSet = beginSet + lineCountInSuperPage;

        for (uint32_t set = beginSet; set < endSet; set++) {
          if (set != setIdx) {
            data.tag = beginLPN + set - beginSet;
            data.setIdx = set;

            wayIdx = getValidWay(data.tag, tick);

            if (wayIdx == waySize) {
              wayIdx = getEmptyWay(set, tick);

              if (wayIdx == waySize) {
                Logger::panic("Bug on flushVictim strict");
              }
            }

            data.wayIdx = wayIdx;

            list.push_back(data);
          }
        }

        reqInternal.ioFlag.set();
      }

      // Request read after flush
      pFTL->read(reqInternal, finishedAt);

      // Set cache
      for (auto &iter : list) {
        ppCache[iter.setIdx][iter.wayIdx].dirty = false;
        ppCache[iter.setIdx][iter.wayIdx].valid = true;
        ppCache[iter.setIdx][iter.wayIdx].tag = iter.tag;
        ppCache[iter.setIdx][iter.wayIdx].lastAccessed = tick;
        ppCache[iter.setIdx][iter.wayIdx].insertedAt = tick;
      }

      // DRAM delay should be hidden by NAND I/O
      tick = finishedAt;
    }
  }
  else {
    FTL::Request reqInternal(lineCountInSuperPage);

    // Just read one page
    reqInternal.reqID = req.reqID;
    reqInternal.reqSubID = req.reqSubID;
    reqInternal.lpn = req.range.slpn / lineCountInSuperPage;
    reqInternal.ioFlag.set(req.range.slpn % lineCountInSuperPage);

    pFTL->read(reqInternal, tick);
  }

  return ret;
}

// True when cold-miss/hit
bool GenericCache::write(Request &req, uint64_t &tick) {
  bool ret = false;

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                     "WRITE | LCA %" PRIu64 " | SIZE %" PRIu64, req.range.slpn,
                     req.length);

  if (useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;
    uint64_t lat = calculateDelay(sizeof(Line) + MIN(req.length, lineSize));

    // Reset prefetch status
    prefetchEnabled = false;
    hitCounter = 0;
    accessCounter = 0;

    // Check cache that we have data for corresponding LBA
    wayIdx = getValidWay(req.range.slpn, tick);

    // We have space for corresponding LBA
    if (wayIdx != waySize) {
      ppCache[setIdx][wayIdx].lastAccessed = tick;
      ppCache[setIdx][wayIdx].valid = true;
      ppCache[setIdx][wayIdx].dirty = true;

      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "WRITE | Cache hit at (%u, %u) | %" PRIu64
                         " - %" PRIu64 " (%" PRIu64 ")",
                         setIdx, wayIdx, tick, tick + lat, lat);

      tick += lat;

      ret = true;
    }
    else {
      bool cold;
      uint64_t insertAt = tick;

      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "WRITE | Cache miss at %u", setIdx);

      wayIdx = flushVictim(setIdx, tick, &cold);

      if (cold) {
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "WRITE | Cache cold-miss, no need to flush", setIdx);
      }

      ppCache[setIdx][wayIdx].valid = true;
      ppCache[setIdx][wayIdx].dirty = true;
      ppCache[setIdx][wayIdx].tag = req.range.slpn;
      ppCache[setIdx][wayIdx].lastAccessed = insertAt;
      ppCache[setIdx][wayIdx].insertedAt = insertAt;
    }
  }
  else {
    FTL::Request reqInternal(lineCountInSuperPage);

    // Just write one page
    reqInternal.reqID = req.reqID;
    reqInternal.reqSubID = req.reqSubID;
    reqInternal.lpn = req.range.slpn / lineCountInSuperPage;
    reqInternal.ioFlag.set(req.range.slpn % lineCountInSuperPage);

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
    wayIdx = getValidWay(req.range.slpn, tick);

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

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE, "TRIM  | LCA %" PRIu64,
                     req.range.slpn);

  if (useReadCaching || useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;

    // Check cache that we have data for corresponding LBA
    wayIdx = getValidWay(req.range.slpn, tick);

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
      wayIdx = getValidWay(lpn, tick);

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
