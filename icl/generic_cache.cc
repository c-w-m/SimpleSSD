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

  policy = (EVICT_POLICY)c->iclConfig.readInt(ICL_EVICT_POLICY);

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

  lastRequest.reqID = 1;
  prefetchEnabled = false;
  hitCounter = 0;
  accessCounter = 0;
}

GenericCache::~GenericCache() {}

uint32_t GenericCache::calcSet(uint64_t lpn) {
  return lpn % setSize;
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

  if (data.lastRequest.range.slpn * lineSize == req.range.slpn * lineSize) {
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
    uint64_t lat = calculateDelay(sizeof(Line) + lineSize);

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

      // Flush collected lines
      flushVictim(list, tick);
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
    uint64_t lat = calculateDelay(sizeof(Line) + lineSize);

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
      //
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
