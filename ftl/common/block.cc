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

namespace SimpleSSD {

namespace FTL {

Block::Block(uint32_t count) : pageCount(count), lastAccessed(0) {
  validBits.resize(pageCount);
}

Block::~Block() {}

uint64_t Block::getLastAccessedTime() {
  return lastAccessed;
}

void Block::setLastAccessedTime(uint64_t tick) {
  lastAccessed = tick;
}

bool Block::getValid(uint32_t pageIndex) {
  return validBits.at(pageIndex);
}

void Block::setValid(uint32_t pageIndex, bool value) {
  validBits.at(pageIndex) = value;
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

}  // namespace FTL

}  // namespace SimpleSSD
