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

#include "ftl/common/block.hh"

#include "log/trace.hh"

namespace SimpleSSD {

namespace FTL {

Block::Block(uint32_t count, uint32_t ioUnit)
    : pageCount(count),
      ioUnitInPage(ioUnit),
      nextWritePageIndex(0),
      lastAccessed(0),
      eraseCount(0) {
  validBits.resize(pageCount);
  erasedBits.resize(pageCount);

  for (auto &iter : validBits) {
    iter = new DynamicBitset(ioUnitInPage);
  }

  for (auto &iter : erasedBits) {
    iter = new DynamicBitset(ioUnitInPage);
  }

  lpns.resize(pageCount);
  nextWritePageIndex.resize(ioUnitInPage);

  erase();
  eraseCount = 0;
}

Block::~Block() {
  for (auto &iter : validBits) {
    delete iter;
  }

  for (auto &iter : erasedBits) {
    delete iter;
  }
}

uint64_t Block::getLastAccessedTime() {
  return lastAccessed;
}

uint32_t Block::getEraseCount() {
  return eraseCount;
}

uint32_t Block::getValidPageCount() {
  uint32_t ret = 0;

  for (auto iter : validBits) {
    if (iter->any()) {
      ret++;
    }
  }

  return ret;
}

void Block::getNextWritePageIndex(std::vector<uint32_t> &out) {
  out = nextWritePageIndex;
}

bool Block::read(uint32_t pageIndex, uint64_t *pLPN, DynamicBitset &iomap,
                 uint64_t tick) {
  auto valid = validBits.at(pageIndex);
  auto tmp = *valid & iomap;
  bool read = tmp == iomap;

  if (read) {
    lastAccessed = tick;

    if (pLPN) {
      *pLPN = lpns.at(pageIndex);
    }
  }

  return read;
}

bool Block::write(uint32_t pageIndex, uint64_t lpn, DynamicBitset &iomap,
                  uint64_t tick) {
  auto valid = erasedBits.at(pageIndex);
  auto tmp = *valid & iomap;
  bool write = tmp == iomap;

  if (write) {
    bool fail = false;

    for (uint32_t i = 0 ;i < ioUnitInPage; i++) {
      if (iomap[i]) {
        if (pageIndex < nextWritePageIndex[i]) {
          fail = true;

          break;
        }
      }
    }

    if (fail) {
      Logger::panic("Write to block should sequential");
    }

    lastAccessed = tick;
    *erasedBits.at(pageIndex) &= ~iomap;
    *validBits.at(pageIndex) |= iomap;
    lpns.at(pageIndex) = lpn;

    for (uint32_t i = 0 ;i < ioUnitInPage; i++) {
      if (iomap[i]) {
        nextWritePageIndex[i] = pageIndex + 1;
      }
    }
  }
  else {
    Logger::panic("Write to non erased page");
  }

  return write;
}

void Block::erase() {
  for (auto iter : validBits) {
    iter->reset();
  }
  for (auto iter : erasedBits) {
    iter->set();
  }
  for (auto &iter : nextWritePageIndex) {
    iter = 0;
  }

  eraseCount++;
}

void Block::invalidate(uint32_t pageIndex) {
  validBits.at(pageIndex)->reset();
}

}  // namespace FTL

}  // namespace SimpleSSD
