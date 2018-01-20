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

SequentialIO::SequentialIO()
    : sequentialIOEnabled(false), hitCounter(0), accessCounter(0) {}

GenericCache::GenericCache(ConfigReader *c, FTL::FTL *f)
    : AbstractCache(c, f),
      waySize(c->iclConfig.readUint(ICL_WAY_SIZE)),
      gen(rd()) {
  uint64_t cacheSize = c->iclConfig.readUint(ICL_CACHE_SIZE);
  superPageSize = f->getInfo()->pageSize;

  dist = std::uniform_int_distribution<>(0, waySize - 1);

  useReadCaching = c->iclConfig.readBoolean(ICL_USE_READ_CACHE);
  useWriteCaching = c->iclConfig.readBoolean(ICL_USE_WRITE_CACHE);
  useSequentialIODetection = c->iclConfig.readBoolean(ICL_USE_SEQ_IO_DETECTION);
  sequentialIOCount = c->iclConfig.readUint(ICL_MIN_SEQ_IO_COUNT);
  sequentialIORatio = c->iclConfig.readFloat(ICL_MIN_SEQ_IO_RATIO);

  lineCountInSuperPage = superPageSize / MIN_LBA_SIZE;
  lineSize = MIN_LBA_SIZE;

  setSize = MAX(cacheSize / lineSize / waySize, 1);

  lineCountInIOUnit = f->getInfo()->minIOSize / MIN_LBA_SIZE;

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

  readIOData.lastRequest.reqID = 1;
  writeIOData.lastRequest.reqID = 1;

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

void GenericCache::flushVictim(std::vector<FlushData> &list, uint64_t &tick,
                               bool write) {
  static uint64_t lat = calculateDelay(sizeof(Line) + lineSize);
  std::vector<FTL::Request> reqList;

  // Collect lines to write
  for (auto &iter : list) {
    auto &line = ppCache[iter.setIdx][iter.wayIdx];

    if (line.valid && line.dirty && line.tag != iter.tag) {
      FTL::Request req(lineCountInSuperPage);

      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "----- | Flush (%u, %u) | LBA %" PRIu64, iter.setIdx,
                         iter.wayIdx, line.tag);

      // We need to flush this
      req.lpn = line.tag / lineCountInSuperPage;
      req.ioFlag.set(line.tag % lineCountInSuperPage);

      reqList.push_back(req);
    }

    // Update line
    if (write) {
      // On write, make this cacheline empty
      line.valid = false;
    }
    else {
      // On read, make this cacheline valid and clean
      line.valid = true;
    }

    line.dirty = false;
    line.tag = iter.tag;
    line.insertedAt = tick;
    line.lastAccessed = tick;
  }

  if (reqList.size() == 0) {
    Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                       "----- | Cache line is clean, no need to flush");

    tick += lat;
  }
  else {
    uint64_t size = reqList.size();

    // Merge same lpn for performance
    for (uint64_t i = 0; i < size; i++) {
      auto &reqi = reqList.at(i);

      if (reqi.reqID) {  // reqID should zero (internal I/O)
        continue;
      }

      for (uint64_t j = i + 1; j < size; j++) {
        auto &reqj = reqList.at(j);

        if (reqi.lpn == reqj.lpn && reqj.reqID == 0) {
          reqj.reqID = 1;
          reqi.ioFlag |= reqj.ioFlag;
        }
      }
    }

    // Do write
    uint64_t beginAt;
    uint64_t finishedAt = tick;

    for (auto &iter : reqList) {
      if (iter.reqID == 0) {
        beginAt = tick;

        pFTL->write(iter, beginAt);

        finishedAt = MAX(finishedAt, beginAt);
      }
    }

    tick = finishedAt;
  }
}

uint64_t GenericCache::calculateDelay(uint64_t bytesize) {
  uint64_t pageCount =
      (bytesize > 0) ? (bytesize - 1) / pStructure->pageSize + 1 : 0;
  uint64_t pageFetch = pTiming->tRP + pTiming->tRCD + pTiming->tCL;
  double bandwidth =
      2.0 * pStructure->busWidth * pStructure->channel / 8.0 / pTiming->tCK;

  return (uint64_t)(pageFetch + pageCount * pStructure->pageSize / bandwidth);
}

void GenericCache::checkSequential(Request &req, SequentialIO &data) {
  if (data.lastRequest.reqID == req.reqID) {
    data.lastRequest.range = req.range;

    return;
  }

  if (data.lastRequest.range.slpn + 1 == req.range.slpn) {
    if (!data.sequentialIOEnabled) {
      data.hitCounter++;
      data.accessCounter += lineSize;

      if (data.hitCounter >= sequentialIOCount &&
          (float)data.accessCounter / superPageSize > sequentialIORatio) {
        data.sequentialIOEnabled = true;
      }
    }
  }
  else {
    data.sequentialIOEnabled = false;
    data.hitCounter = 0;
    data.accessCounter = 0;
  }

  data.lastRequest = req;
}

// True when hit
bool GenericCache::read(Request &req, uint64_t &tick) {
  bool ret = false;

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE, "READ  | LBA %" PRIu64,
                     req.range.slpn);

  if (useReadCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;
    static uint64_t lat = calculateDelay(sizeof(Line) + lineSize);

    // Check sequential IO status
    if (useSequentialIODetection) {
      checkSequential(req, readIOData);
    }

    // Check cache that we have data for corresponding LBA
    wayIdx = getValidWay(req.range.slpn);

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
      FTL::Request reqInternal(lineCountInSuperPage);

      // Read page
      reqInternal.reqID = req.reqID;
      reqInternal.reqSubID = req.reqSubID;
      reqInternal.lpn = req.range.slpn / lineCountInSuperPage;

      if (readIOData.sequentialIOEnabled) {
        reqInternal.ioFlag.set();
      }
      else {
        reqInternal.ioFlag.set(req.range.slpn % lineCountInSuperPage);
      }

      pFTL->read(reqInternal, tick);

      // Collect cachelines to flush from returned I/O map
      FlushData data;
      std::vector<FlushData> list;

      for (uint64_t i = 0; i < lineCountInSuperPage; i++) {
        data.tag = reqInternal.lpn * lineCountInSuperPage + i;
        data.setIdx = calcSet(data.tag);
        data.wayIdx = getVictimWay(data.tag);

        list.push_back(data);
      }

      if (list.size() == 0) {
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "READ  | Cache cold-miss, no need to flush", setIdx);
      }
      else {
        // Flush collected lines
        flushVictim(list, tick, false);
      }
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

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE, "WRITE | LBA %" PRIu64,
                     req.range.slpn);

  if (useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;
    static uint64_t lat = calculateDelay(sizeof(Line) + lineSize);

    // Check sequential IO status
    if (useSequentialIODetection) {
      checkSequential(req, writeIOData);
    }

    // Check cache that we have data for corresponding LBA
    wayIdx = getValidWay(req.range.slpn);

    // We have space for corresponding LBA
    if (wayIdx != waySize) {
      ppCache[setIdx][wayIdx].lastAccessed = tick;
      ppCache[setIdx][wayIdx].dirty = true;

      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "WRITE | Cache hit at (%u, %u) | %" PRIu64
                         " - %" PRIu64 " (%" PRIu64 ")",
                         setIdx, wayIdx, tick, tick + lat, lat);

      tick += lat;

      ret = true;
    }
    else {
      wayIdx = getEmptyWay(setIdx);

      // We have to flush
      if (wayIdx != waySize) {
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "WRITE | Cache miss at (%u, %u) | %" PRIu64
                           " - %" PRIu64 " (%" PRIu64 ")",
                           setIdx, wayIdx, tick, tick + lat, lat);

        tick += lat;

        ret = true;
      }
      else {
        // Calculate range of set to collect cachelines to flush
        uint32_t beginSet;
        uint32_t endSet;

        // if (writeIOData.sequentialIOEnabled) {
        uint32_t left = req.range.slpn % lineCountInSuperPage;

        beginSet = setIdx - left;
        endSet = beginSet + lineCountInSuperPage;
        // }
        // else {
        //   uint32_t left = req.range.slpn % lineCountInIOUnit;
        //
        //   beginSet = setIdx - left;
        //   endSet = beginSet + lineCountInIOUnit;
        // }

        // Collect cachelines
        FlushData data;
        std::vector<FlushData> list;

        for (uint32_t curSet = setIdx; curSet < endSet; curSet++) {
          data.tag = req.range.slpn + curSet - setIdx;
          data.setIdx = curSet;
          data.wayIdx = getVictimWay(data.tag);

          list.push_back(data);
        }

        if (list.size() == 0) {
          Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                             "WRITE | Cache cold-miss, no need to flush",
                             setIdx);
        }
        else {
          // Flush collected lines
          flushVictim(list, tick, true);
        }

        // Current data
        wayIdx = list.front().wayIdx;
      }

      // Update line
      ppCache[setIdx][wayIdx].valid = true;
      ppCache[setIdx][wayIdx].dirty = true;
      ppCache[setIdx][wayIdx].tag = req.range.slpn;
      ppCache[setIdx][wayIdx].insertedAt = tick;
      ppCache[setIdx][wayIdx].lastAccessed = tick;
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

      // Invalidate
      ppCache[setIdx][wayIdx] = Line();
    }
  }

  return ret;
}

// True when hit
bool GenericCache::trim(Request &req, uint64_t &tick) {
  bool ret = false;
  FTL::Request reqInternal(lineCountInSuperPage);

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE, "TRIM  | LBA %" PRIu64,
                     req.range.slpn);

  if (useReadCaching || useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;

    // Check cache that we have data for corresponding LBA
    wayIdx = getValidWay(req.range.slpn);

    if (wayIdx != waySize) {
      // Invalidate
      ppCache[setIdx][wayIdx] = Line();
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
        // Invalidate
        ppCache[setIdx][wayIdx] = Line();
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
