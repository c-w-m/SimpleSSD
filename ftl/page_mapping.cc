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

#include <limits>

#include "ftl/old/ftl.hh"
#include "ftl/old/ftl_defs.hh"
#include "log/trace.hh"
#include "util/algorithm.hh"

namespace SimpleSSD {

namespace FTL {

PageMapping::PageMapping(Parameter *p, PAL::PAL *l, ConfigReader *c)
    : AbstractFTL(p, l), pPAL(l), conf(c->ftlConfig), pFTLParam(p) {
  Block temp(pFTLParam->pagesInBlock);

  for (uint32_t i = 0; i < pFTLParam->totalPhysicalBlocks; i++) {
    freeBlocks.insert({i, temp});
  }

  lastFreeBlock.first = false;
}

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
  return (float)freeBlocks.size() / pFTLParam->totalPhysicalBlocks;
}

uint32_t PageMapping::getFreeBlock() {
  uint32_t blockIndex = 0;

  if (freeBlocks.size() > 0) {
    uint32_t eraseCount = std::numeric_limits<uint32_t>::max();
    auto found = freeBlocks.end();

    for (auto iter = freeBlocks.begin(); iter != freeBlocks.end(); iter++) {
      uint32_t current = iter->second.getEraseCount();
      if (current < eraseCount) {
        eraseCount = current;
        blockIndex = iter->first;
        found = iter;
      }
    }

    // Insert found block to block list
    if (blocks.find(blockIndex) != blocks.end()) {
      Logger::panic("Corrupted");
    }

    blocks.insert({blockIndex, found->second});

    // Remove found block from free block list
    freeBlocks.erase(found);
  }
  else {
    Logger::panic("No free block left");
  }

  return blockIndex;
}

void PageMapping::selectVictimBlock(std::vector<uint32_t> &list) {}

uint64_t PageMapping::doGarbageCollection(uint64_t tick) {
  return tick;
}

void PageMapping::readInternal(Request &req, uint64_t &tick) {
  PAL::Request palRequest(req);

  auto mapping = table.find(req.lpn);

  if (mapping != table.end()) {
    palRequest.blockIndex = mapping->second.first;
    palRequest.pageIndex = mapping->second.second;

    auto block = blocks.find(palRequest.blockIndex);

    if (block == blocks.end()) {
      Logger::panic("Block is not in use");
    }

    block->second.read(palRequest.pageIndex, tick);

    pPAL->read(palRequest, tick);
  }
}

void PageMapping::writeInternal(Request &req, uint64_t &tick, bool sendToPAL) {
  PAL::Request palRequest(req);

  // Allocate new free block
  if (!lastFreeBlock.first) {
    lastFreeBlock.first = true;
    lastFreeBlock.second = getFreeBlock();
  }

  auto mapping = table.find(req.lpn);

  if (mapping != table.end()) {
    // Invalidate current page
    auto block = blocks.find(mapping->second.first);

    if (block == blocks.end()) {
      Logger::panic("No such block");
    }

    block->second.invalidate(mapping->second.second);
  }

  // Write data to free block
  auto block = blocks.find(lastFreeBlock.second);

  if (block == blocks.end()) {
    Logger::panic("No such block");
  }

  uint32_t pageIndex = block->second.getNextWritePageIndex();

  block->second.write(pageIndex, tick);

  palRequest.blockIndex = block->first;
  palRequest.pageIndex = pageIndex;

  pPAL->write(palRequest, tick);

  // Add new mapping to table
  mapping->second.first = palRequest.blockIndex;
  mapping->second.second = palRequest.pageIndex;

  // If this block is full, invalidate lastFreeBlock
  pageIndex = block->second.getNextWritePageIndex();

  if (pageIndex == 0) {
    lastFreeBlock.first = false;
  }

  // GC if needed
  if (freeBlockRatio() < conf.readFloat(FTL_GC_THRESHOLD_RATIO)) {
    doGarbageCollection(tick);
  }
}

void PageMapping::trimInternal(Request &req, uint64_t &tick) {
  auto mapping = table.find(req.lpn);

  if (mapping != table.end()) {
    auto block = blocks.find(mapping->second.first);

    if (block == blocks.end()) {
      Logger::panic("Block is not in use");
    }

    block->second.invalidate(mapping->second.second);

    // If no valid pages in block, erase
    if (block->second.getValidPageCount() == 0) {
      PAL::Request palRequest(req);

      palRequest.blockIndex = mapping->second.first;

      eraseInternal(palRequest, tick);
    }
  }
}

void PageMapping::eraseInternal(PAL::Request &req, uint64_t &tick) {
  auto block = blocks.find(req.blockIndex);

  // Sanity checks
  if (block == blocks.end()) {
    Logger::panic("No such block");
  }

  if (freeBlocks.find(req.blockIndex) != freeBlocks.end()) {
    Logger::panic("Corrupted");
  }

  if (block->second.getValidPageCount() != 0) {
    Logger::panic("There are valid pages in victim block");
  }

  // Erase block
  block->second.erase();

  pPAL->erase(req, tick);

  // Insert block to free block list
  freeBlocks.insert({req.blockIndex, block->second});

  // Remove block from block list
  blocks.erase(block);
}

}  // namespace FTL

}  // namespace SimpleSSD
