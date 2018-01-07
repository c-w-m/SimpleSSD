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

void printBits(std::vector<bool> &bits) {
  uint32_t size = bits.size();

  for (uint32_t i = 0; i < size; i += 16) {
    std::string str;

    str.reserve(64);

    for (uint32_t j = 0; j < 16; j++) {
      if (i + j < size) {
        str += (bits.at(i + j) ? "1 " : "0 ");
      }
    }

    Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE, str.c_str());
  }
}

GenericCache::GenericCache(ConfigReader *c, FTL::FTL *f)
    : AbstractCache(c, f), gen(rd()) {
  uint64_t cacheSize = c->iclConfig.readUint(ICL_CACHE_SIZE);
  waySize = c->iclConfig.readUint(ICL_WAY_SIZE);
  lineSize = f->getInfo()->pageSize;

  setSize = MAX(cacheSize / lineSize / waySize, 1);

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                     "CREATE  | Set size %u | Way size %u | Capacity %" PRIu64,
                     setSize, waySize, (uint64_t)setSize * waySize * lineSize);

  dist = std::uniform_int_distribution<>(0, waySize - 1);

  useReadCaching = c->iclConfig.readBoolean(ICL_USE_READ_CACHE);
  useWriteCaching = c->iclConfig.readBoolean(ICL_USE_WRITE_CACHE);
  useReadPrefetch = c->iclConfig.readBoolean(ICL_USE_READ_PREFETCH);
  prefetchIOCount = c->iclConfig.readUint(ICL_PREFETCH_COUNT);
  usePartialIO = c->tweakConfig.readBoolean(TWEAK_PARTIAL_IO);

  policy = (EVICT_POLICY)c->iclConfig.readInt(ICL_EVICT_POLICY);

  partialIOUnitCount = f->getInfo()->ioUnitInPage;
  partialIOUnitSize = lineSize / partialIOUnitCount;

  // TODO: replace this with DRAM model
  pTiming = c->iclConfig.getDRAMTiming();
  pStructure = c->iclConfig.getDRAMStructure();

  ppCache = (Line **)calloc(setSize, sizeof(Line *));

  for (uint32_t i = 0; i < setSize; i++) {
    ppCache[i] = new Line[waySize]();

    for (uint32_t j = 0; j < waySize; j++) {
      ppCache[i][j].validBits.resize(partialIOUnitCount);
      ppCache[i][j].dirtyBits.resize(partialIOUnitCount);
    }
  }

  lastRequest.reqID = 1;
  prefetchEnabled = false;
  hitCounter = 0;
}

GenericCache::~GenericCache() {
  for (uint32_t i = 0; i < setSize; i++) {
    delete[] ppCache[i];
  }

  free(ppCache);
}

uint32_t GenericCache::calcSet(uint64_t lpn) {
  return lpn % setSize;
}

uint32_t GenericCache::flushVictim(Request req, uint64_t &tick, bool *isCold) {
  uint32_t setIdx = calcSet(req.range.slpn);
  uint32_t wayIdx = waySize;
  uint64_t min = std::numeric_limits<uint64_t>::max();
  FTL::Request reqInternal;

  if (isCold) {
    *isCold = false;
  }

  // Check set has empty entry
  for (uint32_t i = 0; i < waySize; i++) {
    if (!merge(ppCache[setIdx][i].validBits)) {
      wayIdx = i;

      if (isCold) {
        *isCold = true;
      }

      break;
    }
  }

  // If no empty entry
  if (wayIdx == waySize) {
    switch (policy) {
      case POLICY_RANDOM:
        wayIdx = dist(gen);

        break;
      case POLICY_FIFO:
        for (uint32_t i = 0; i < waySize; i++) {
          if (ppCache[setIdx][i].insertedAt < min) {
            min = ppCache[setIdx][i].insertedAt;
            wayIdx = i;
          }
        }

        break;
      case POLICY_LEAST_RECENTLY_USED:
        for (uint32_t i = 0; i < waySize; i++) {
          if (ppCache[setIdx][i].lastAccessed < min) {
            min = ppCache[setIdx][i].lastAccessed;
            wayIdx = i;
          }
        }

        break;
    }

    // Let's flush 'em if dirty
    if (merge(ppCache[setIdx][wayIdx].dirtyBits)) {
      reqInternal.lpn = ppCache[setIdx][wayIdx].tag;
      reqInternal.ioFlag = ppCache[setIdx][wayIdx].dirtyBits;

      pFTL->write(reqInternal, tick);
    }
    else {
      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "----- | Line (%u, %u) is clean, no need to flush",
                         setIdx, wayIdx);
    }

    // Invalidate
    ppCache[setIdx][wayIdx].validBits =
        std::vector<bool>(partialIOUnitCount, false);
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

void GenericCache::convertIOFlag(std::vector<bool> &flags, uint64_t offset,
                                 uint64_t length) {
  if (usePartialIO && !prefetchEnabled) {
    flags.resize(partialIOUnitCount);
    setBits(flags, offset / partialIOUnitSize,
            (offset + length - 1) / partialIOUnitSize + 1, true);
  }
  else {
    flags = std::vector<bool>(partialIOUnitCount, true);
  }
}

void GenericCache::setBits(std::vector<bool> &bits, uint64_t begin,
                           uint64_t end, bool value) {
  end = MIN(end, bits.size());

  if (end < begin) {
    Logger::panic("Invalid parameter");
  }

  for (uint64_t i = begin; i < end; i++) {
    bits.at(i) = value;
  }
}

void GenericCache::checkPrefetch(Request &req) {
  printf("Last I/O: reqID %" PRIu64 ", LPN %" PRIu64 ", Byte %" PRIu64
         " + %" PRIu64 "\n",
         lastRequest.reqID, lastRequest.range.slpn, lastRequest.offset,
         lastRequest.length);
  printf("This I/O: reqID %" PRIu64 ", LPN %" PRIu64 ", Byte %" PRIu64
         " + %" PRIu64 "\n",
         req.reqID, req.range.slpn, req.offset, req.length);

  if (lastRequest.reqID == req.reqID) {
    lastRequest.range = req.range;

    return;
  }

  if (lastRequest.range.slpn * lineSize + lastRequest.offset +
          lastRequest.length ==
      req.range.slpn * lineSize + req.offset) {
    if (!prefetchEnabled) {
      hitCounter++;

      if (hitCounter >= prefetchIOCount) {
        prefetchEnabled = true;
      }
    }
  }
  else {
    hitCounter = 0;
    prefetchEnabled = false;
  }

  lastRequest = req;
}

bool GenericCache::merge(std::vector<bool> &bits) {
  bool ret = false;

  for (auto iter : bits) {
    if (iter) {
      ret = true;

      break;
    }
  }

  return ret;
}

void GenericCache::setIfTrue(std::vector<bool> &out, std::vector<bool> &cond,
                             bool value) {
  uint32_t size = out.size();

  if (size != cond.size()) {
    Logger::panic("Input size does not match");
  }

  for (uint32_t i = 0; i < size; i++) {
    if (cond.at(i)) {
      out.at(i) = value;
    }
  }
}

void GenericCache::calcAND(std::vector<bool> &out, std::vector<bool> &lhs,
                           std::vector<bool> &rhs) {
  uint32_t size = lhs.size();

  if (size != rhs.size()) {
    Logger::panic("Input size does not match");
  }

  out.resize(size);

  for (uint32_t i = 0; i < size; i++) {
    out.at(i) = lhs.at(i) & rhs.at(i);
  }
}

bool GenericCache::same(std::vector<bool> &lhs, std::vector<bool> &rhs) {
  bool ret = true;
  uint32_t size = lhs.size();

  if (size != rhs.size()) {
    Logger::panic("Input size does not match");
  }

  for (uint32_t i = 0; i < size; i++) {
    if (lhs.at(i) != rhs.at(i)) {
      ret = false;

      break;
    }
  }

  return ret;
}

// True when hit
bool GenericCache::read(Request &req, uint64_t &tick) {
  bool ret = false;
  FTL::Request reqInternal(req);

  convertIOFlag(reqInternal.ioFlag, req.offset, req.length);

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE, "I/O map");
  printBits(reqInternal.ioFlag);

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                     "READ  | LPN %" PRIu64 " | SIZE %" PRIu64, req.range.slpn,
                     req.length);

  if (useReadCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;
    uint64_t lat = calculateDelay(req.length);
    std::vector<bool> tmp;

    // Check prefetch status
    if (useReadPrefetch) {
      checkPrefetch(req);
    }

    for (wayIdx = 0; wayIdx < waySize; wayIdx++) {
      Line &line = ppCache[setIdx][wayIdx];

      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE, "Line %u map", wayIdx);
      printBits(line.validBits);

      calcAND(tmp, line.validBits, reqInternal.ioFlag);

      if (same(tmp, reqInternal.ioFlag) && line.tag == req.range.slpn) {
        line.lastAccessed = tick;
        ret = true;

        break;
      }
    }

    if (ret) {
      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "READ  | Cache hit at (%u, %u) | %" PRIu64
                         " - %" PRIu64 " (%" PRIu64 ")",
                         setIdx, wayIdx, tick, tick + lat, lat);

      tick += lat;
    }
    else {
      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "READ  | Cache miss at %u", setIdx);

      bool cold;
      wayIdx = flushVictim(req, tick, &cold);

      if (cold) {
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "READ  | Cache cold-miss, no need to flush", setIdx);
      }
      else {
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "READ  | Flush (%u, %u), LPN %" PRIu64, setIdx,
                           wayIdx, ppCache[setIdx][wayIdx].tag);
      }

      ppCache[setIdx][wayIdx].tag = req.range.slpn;
      ppCache[setIdx][wayIdx].lastAccessed = tick;
      ppCache[setIdx][wayIdx].insertedAt = tick;

      setIfTrue(ppCache[setIdx][wayIdx].validBits, reqInternal.ioFlag, true);
      setIfTrue(ppCache[setIdx][wayIdx].dirtyBits, reqInternal.ioFlag, false);

      // Resize I/O size
      setIfTrue(reqInternal.ioFlag, tmp, false);

      // Request read and flush at same time
      pFTL->read(reqInternal, tick);

      // DRAM delay should be hidden by NAND I/O
    }
  }
  else {
    pFTL->read(reqInternal, tick);
  }

  return ret;
}

// True when cold-miss/hit
bool GenericCache::write(Request &req, uint64_t &tick) {
  bool ret = false;
  FTL::Request reqInternal(req);

  convertIOFlag(reqInternal.ioFlag, req.offset, req.length);

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                     "WRITE | LPN %" PRIu64 " | SIZE %" PRIu64, req.range.slpn,
                     req.length);

  if (useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t wayIdx;
    uint64_t lat = calculateDelay(req.length);

    for (wayIdx = 0; wayIdx < waySize; wayIdx++) {
      Line &line = ppCache[setIdx][wayIdx];

      if (merge(line.validBits) && line.tag == req.range.slpn) {
        line.lastAccessed = tick;

        setIfTrue(line.dirtyBits, reqInternal.ioFlag, true);

        ret = true;

        break;
      }
    }

    if (ret) {
      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "WRITE | Cache hit at (%u, %u) | %" PRIu64
                         " - %" PRIu64 " (%" PRIu64 ")",
                         setIdx, wayIdx, tick, tick + lat, lat);

      tick += lat;
    }
    else {
      Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                         "WRITE | Cache miss at %u", setIdx);

      bool cold;
      uint64_t insertAt = tick;
      wayIdx = flushVictim(req, tick, &cold);

      if (cold) {
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "WRITE | Cache cold-miss, no need to flush", setIdx);

        tick += lat;
      }
      else {
        Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE,
                           "WRITE | Flush (%u, %u), LPN %" PRIu64, setIdx,
                           wayIdx, ppCache[setIdx][wayIdx].tag);
      }

      ppCache[setIdx][wayIdx].tag = req.range.slpn;
      ppCache[setIdx][wayIdx].lastAccessed = insertAt;
      ppCache[setIdx][wayIdx].insertedAt = insertAt;
      setIfTrue(ppCache[setIdx][wayIdx].validBits, reqInternal.ioFlag, true);
      setIfTrue(ppCache[setIdx][wayIdx].dirtyBits, reqInternal.ioFlag, false);

      // DRAM delay should be hidden by NAND I/O
    }
  }
  else {
    pFTL->write(reqInternal, tick);
  }

  return ret;
}

// True when hit
bool GenericCache::flush(Request &req, uint64_t &tick) {
  bool ret = false;
  FTL::Request reqInternal(req);

  convertIOFlag(reqInternal.ioFlag, req.offset, req.length);

  if (useReadCaching || useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t i;
    std::vector<bool> tmp;

    for (i = 0; i < waySize; i++) {
      Line &line = ppCache[setIdx][i];

      calcAND(tmp, line.validBits, reqInternal.ioFlag);

      if (merge(tmp) && line.tag == req.range.slpn) {
        ret = true;

        break;
      }
    }

    if (ret) {
      calcAND(reqInternal.ioFlag, ppCache[setIdx][i].dirtyBits, tmp);

      if (merge(tmp)) {
        // we have to flush this
        pFTL->write(reqInternal, tick);
      }

      // invalidate
      setIfTrue(ppCache[setIdx][i].validBits, tmp, false);
    }
  }

  return ret;
}

// True when hit
bool GenericCache::trim(Request &req, uint64_t &tick) {
  bool ret = false;
  FTL::Request reqInternal(req);

  convertIOFlag(reqInternal.ioFlag, req.offset, req.length);

  Logger::debugprint(Logger::LOG_ICL_GENERIC_CACHE, "TRIM  | LPN %" PRIu64,
                     req.range.slpn);

  if (useReadCaching || useWriteCaching) {
    uint32_t setIdx = calcSet(req.range.slpn);
    uint32_t i;

    for (i = 0; i < waySize; i++) {
      Line &line = ppCache[setIdx][i];

      if (merge(line.validBits) && line.tag == req.range.slpn) {
        ret = true;

        break;
      }
    }

    if (ret) {
      // invalidate
      ppCache[setIdx][i].validBits =
          std::vector<bool>(partialIOUnitCount, false);
    }

    // we have to trim this
    pFTL->trim(reqInternal, tick);
  }
  else {
    pFTL->trim(reqInternal, tick);
  }

  return ret;
}

void GenericCache::format(LPNRange &range, uint64_t &tick) {
  bool ret = false;

  if (useReadCaching || useWriteCaching) {
    uint64_t lpn;
    uint32_t setIdx;
    uint32_t way;

    for (uint64_t i = 0; i < range.nlp; i++) {
      lpn = range.slpn + i;
      setIdx = calcSet(lpn);

      for (way = 0; way < waySize; way++) {
        Line &line = ppCache[setIdx][way];

        if (merge(line.validBits) && line.tag == lpn) {
          ret = true;

          break;
        }
      }

      if (ret) {
        // invalidate
        ppCache[setIdx][way].validBits =
            std::vector<bool>(partialIOUnitCount, false);
      }
    }

    // we have to trim this
    pFTL->format(range, tick);
  }
  else {
    pFTL->format(range, tick);
  }
}

}  // namespace ICL

}  // namespace SimpleSSD
