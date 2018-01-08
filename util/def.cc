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

#include "util/def.hh"

#include <cstdlib>

#include "util/algorithm.hh"

namespace SimpleSSD {

LPNRange::_LPNRange() : slpn(0), nlp(0) {}

LPNRange::_LPNRange(uint64_t s, uint64_t n) : slpn(s), nlp(n) {}

DynamicBitset::DynamicBitset(uint32_t size) : dataSize(size) {
  if (dataSize == 0) {
    Logger::panic("Invalid size of DynamicBitset");
  }

  allocSize = (dataSize - 1) / 8 + 1;
  pData = (uint8_t *)calloc(allocSize, 1);

  if (pData == nullptr) {
    Logger::panic("No memory");
  }
}

DynamicBitset::DynamicBitset(const DynamicBitset &copy) noexcept
    : pData(nullptr), dataSize(copy.dataSize), allocSize(copy.allocSize) {
  if (allocSize > 0) {
    pData = (uint8_t *)calloc(copy.allocSize, 1);

    if (pData == nullptr) {
      Logger::panic("No memory");
    }
  }

  if (pData) {
    std::copy(copy.pData, copy.pData + allocSize, pData);
  }
}

DynamicBitset::~DynamicBitset() {
  free(pData);
}

void DynamicBitset::boundCheck(uint32_t idx) {
  if (idx >= dataSize) {
    Logger::panic("Index out of range");
  }
}

bool DynamicBitset::test(uint32_t idx) {
  boundCheck(idx);

  return pData[idx / 8] & (0x01 << (idx % 8));
}

bool DynamicBitset::all() {
  uint8_t ret = 0xFF;
  uint8_t mask = 0xFF << (dataSize + 8 - allocSize * 8);

  for (uint32_t i = 0; i < allocSize - 1; i++) {
    ret &= pData[i];
  }

  ret &= pData[allocSize - 1] | mask;

  return ret == 0xFF;
}

bool DynamicBitset::any() {
  return !none();
}

bool DynamicBitset::none() {
  uint8_t ret = 0x00;

  for (uint32_t i = 0; i < allocSize; i++) {
    ret |= pData[i];
  }

  return ret == 0x00;
}

uint32_t DynamicBitset::count() {
  uint32_t count = 0;

  for (uint32_t i = 0; i < allocSize; i++) {
    count += popcount(pData[i]);
  }

  return count;
}

uint32_t DynamicBitset::size() {
  return dataSize;
}

void DynamicBitset::set() {
  uint8_t mask = 0xFF >> (allocSize * 8 - dataSize + 8);

  for (uint32_t i = 0; i < allocSize - 1; i++) {
    pData[i] = 0xFF;
  }

  pData[allocSize - 1] = mask;
}

void DynamicBitset::set(uint32_t idx, bool value) {
  boundCheck(idx);

  pData[idx / 8] &= ~(0x01 << (idx % 8));

  if (value) {
    pData[idx / 8] = pData[idx / 8] | (0x01 << (idx % 8));
  }
}

void DynamicBitset::reset() {
  for (uint32_t i = 0; i < allocSize; i++) {
    pData[i] = 0x00;
  }
}

void DynamicBitset::reset(uint32_t idx) {
  boundCheck(idx);

  pData[idx / 8] &= ~(0x01 << (idx % 8));
}

void DynamicBitset::flip() {
  uint8_t mask = 0xFF >> (allocSize * 8 - dataSize + 8);

  for (uint32_t i = 0; i < allocSize; i++) {
    pData[i] = ~pData[i];
  }

  pData[allocSize - 1] &= mask;
}

void DynamicBitset::flip(uint32_t idx) {
  boundCheck(idx);

  pData[idx / 8] = (~pData[idx / 8] & (0x01 << (idx % 8))) |
                   (pData[idx / 8] & ~(0x01 << (idx % 8)));
}

bool DynamicBitset::operator[](uint32_t idx) {
  return test(idx);
}

DynamicBitset &DynamicBitset::operator&=(const DynamicBitset &rhs) {
  if (dataSize != rhs.dataSize) {
    Logger::panic("Size does not match");
  }

  for (uint32_t i = 0; i < allocSize; i++) {
    pData[i] &= rhs.pData[i];
  }

  return *this;
}

DynamicBitset &DynamicBitset::operator|=(const DynamicBitset &rhs) {
  if (dataSize != rhs.dataSize) {
    Logger::panic("Size does not match");
  }

  for (uint32_t i = 0; i < allocSize; i++) {
    pData[i] |= rhs.pData[i];
  }

  return *this;
}

DynamicBitset &DynamicBitset::operator^=(const DynamicBitset &rhs) {
  if (dataSize != rhs.dataSize) {
    Logger::panic("Size does not match");
  }

  for (uint32_t i = 0; i < allocSize; i++) {
    pData[i] ^= rhs.pData[i];
  }

  return *this;
}

DynamicBitset DynamicBitset::operator~() const {
  DynamicBitset ret(*this);

  ret.flip();

  return ret;
}

namespace ICL {

Request::_Request() : reqID(0), reqSubID(0), offset(0), length(0) {}

}  // namespace ICL

namespace FTL {

Request::_Request() : reqID(0), reqSubID(0), lpn(0) {}

Request::_Request(ICL::Request &r)
    : reqID(r.reqID), reqSubID(r.reqSubID), lpn(r.range.slpn) {}

}  // namespace FTL

namespace PAL {

Request::_Request() : reqID(0), reqSubID(0), blockIndex(0), pageIndex(0) {}

Request::_Request(FTL::Request &r)
    : reqID(r.reqID),
      reqSubID(r.reqSubID),
      blockIndex(0),
      pageIndex(0),
      ioFlag(r.ioFlag) {}

}  // namespace PAL

}  // namespace SimpleSSD
