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

Block::Block(uint32_t count)
    : pageCount(count), lastWrittenIndex(0), lastAccessed(0), eraseCount(0) {
  validBits.resize(pageCount);
}

Block::~Block() {}

uint32_t Block::decreasePageIndex(uint32_t pageIndex) {
  if (pageIndex == 0) {
    pageIndex = pageCount - 1;
  }
  else {
    pageIndex--;
  }

  return pageIndex;
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
    if (iter) {
      ret++;
    }
  }

  return ret;
}

uint32_t Block::getLastWrittenPageIndex() {
  return lastWrittenIndex;
}

bool Block::read(uint32_t pageIndex, uint64_t tick) {
  bool valid = validBits.at(pageIndex);

  if (valid) {
    lastAccessed = tick;
  }
  else {
    Logger::panic("Read invalid page");
  }

  return valid;
}

bool Block::write(uint32_t pageIndex, uint64_t tick) {
  bool valid = validBits.at(pageIndex);

  if (!valid) {
    if (decreasePageIndex(pageIndex) != lastWrittenIndex) {
      Logger::panic("Write to block should sequential");
    }

    lastAccessed = tick;
    validBits.at(pageIndex) = true;

    lastWrittenIndex = pageIndex;
  }
  else {
    Logger::panic("Write to valid page");
  }

  return !valid;
}

void Block::erase() {
  for (auto iter : validBits) {
    iter = false;
  }

  eraseCount++;
}

}  // namespace FTL

}  // namespace SimpleSSD
