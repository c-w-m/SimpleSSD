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

namespace SimpleSSD {

namespace ICL {

GenericCache::GenericCache(ConfigReader *c, FTL::FTL *f) : Cache(c, f) {
  setSize = c->iclConfig.readUint(ICL_SET_SIZE);
  entrySize = c->iclConfig.readUint(ICL_ENTRY_SIZE);
  lineSize = f->getInfo()->pageSize;

  // TODO: replace this with DRAM model
  width = c->iclConfig.readUint(DRAM_CHIP_BUS_WIDTH);
  latency = c->iclConfig.readUint(DRAM_TIMING_RP);
  latency += c->iclConfig.readUint(DRAM_TIMING_RCD);
  latency += c->iclConfig.readUint(DRAM_TIMING_CL);

  latency *= lineSize / (width / 8);

  ppCache = (Line **)calloc(setSize, sizeof(Line *));

  for (uint32_t i = 0; i < setSize; i++) {
    ppCache[i] = new Line[entrySize]();
  }
}

GenericCache::~GenericCache() {
  for (uint32_t i = 0; i < setSize; i++) {
    delete[] ppCache[i];
  }

  free(ppCache);
}

uint32_t GenericCache::calcSet(uint64_t lpn) {
  return lpn & (setSize - 1);
}

// True when hit
bool GenericCache::read(uint64_t lpn, uint64_t &tick) {
  bool ret = false;
  uint32_t setIdx = calcSet(lpn);
  uint32_t emptyIdx = entrySize;

  for (uint32_t i = 0; i < entrySize; i++) {
    Line &line = ppCache[setIdx][i];

    if (line.valid && line.tag == lpn) {
      ret = true;

      break;
    }

    if (!line.valid || !line.dirty) {
      emptyIdx = i;
    }
  }

  if (!ret) {
    // miss
    if (emptyIdx < entrySize) {
      // we have place to write
      pFTL->read(lpn, tick);

      ppCache[setIdx][emptyIdx].valid = true;
      ppCache[setIdx][emptyIdx].dirty = false;
      ppCache[setIdx][emptyIdx].tag = lpn;
    }
    else {
      // we don't have place
      pFTL->read(lpn, tick);

      // flush one TODO: you may want to apply algorithm to select victim
      pFTL->write(ppCache[setIdx][0].tag, tick);

      ppCache[setIdx][0].valid = true;
      ppCache[setIdx][0].dirty = false;
      ppCache[setIdx][0].tag = lpn;
    }
  }

  tick += latency;

  return ret;
}

// True when cold-miss/hit
bool GenericCache::write(uint64_t lpn, uint64_t &tick) {
  bool ret = false;
  uint32_t setIdx = calcSet(lpn);
  uint32_t emptyIdx = entrySize;

  for (uint32_t i = 0; i < entrySize; i++) {
    Line &line = ppCache[setIdx][i];

    if (line.valid && line.tag == lpn) {
      ret = true;

      break;
    }

    if (!line.valid || !line.dirty) {
      emptyIdx = i;
    }
  }

  if (emptyIdx < entrySize) {
    // miss
    ret = true;

    ppCache[setIdx][emptyIdx].valid = true;
    ppCache[setIdx][emptyIdx].dirty = true;
    ppCache[setIdx][emptyIdx].tag = lpn;
  }

  if (!ret) {
    // we don't have place
    // flush one TODO: you may want to apply algorithm to select victim
    pFTL->write(ppCache[setIdx][0].tag, tick);

    ppCache[setIdx][0].valid = true;
    ppCache[setIdx][0].dirty = true;
    ppCache[setIdx][0].tag = lpn;
  }

  tick += latency;

  return ret;
}

// True when hit
bool GenericCache::flush(uint64_t lpn, uint64_t &tick) {
  bool ret = false;
  uint32_t setIdx = calcSet(lpn);
  uint32_t i;

  for (i = 0; i < entrySize; i++) {
    Line &line = ppCache[setIdx][i];

    if (line.valid && line.tag == lpn) {
      ret = true;

      break;
    }
  }

  if (ret) {
    if (ppCache[setIdx][i].dirty) {
      // we have to flush this
      pFTL->write(ppCache[setIdx][i].tag, tick);
    }

    // invalidate
    ppCache[setIdx][i].valid = false;
  }

  return ret;
}

// True when hit
bool GenericCache::trim(uint64_t lpn, uint64_t &tick) {
  bool ret = false;
  uint32_t setIdx = calcSet(lpn);
  uint32_t i;

  for (i = 0; i < entrySize; i++) {
    Line &line = ppCache[setIdx][i];

    if (line.valid && line.tag == lpn) {
      ret = true;

      break;
    }
  }

  if (ret) {
    // invalidate
    ppCache[setIdx][i].valid = false;
  }

  // we have to trim this
  pFTL->trim(ppCache[setIdx][i].tag, tick);

  return ret;
}

}  // namespace ICL

}  // namespace SimpleSSD
