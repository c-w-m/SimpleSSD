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

#ifndef __UTIL_DEF__
#define __UTIL_DEF__

#include <cinttypes>
#include <vector>

namespace SimpleSSD {

typedef struct _LPNRange {
  uint64_t slpn;
  uint64_t nlp;

  _LPNRange();
  _LPNRange(uint64_t, uint64_t);
} LPNRange;

class DynamicBitset {
 private:
  uint8_t *pData;
  uint32_t dataSize;
  uint32_t allocSize;

  void boundCheck(uint32_t);

 public:
  DynamicBitset(uint32_t);
  DynamicBitset(const DynamicBitset &) noexcept;
  ~DynamicBitset();

  bool test(uint32_t);
  bool all();
  bool any();
  bool none();
  uint32_t count();
  uint32_t size();
  void set();
  void set(uint32_t, bool = true);
  void reset();
  void reset(uint32_t);
  void flip();
  void flip(uint32_t);

  bool operator[](uint32_t);
  DynamicBitset &operator&=(const DynamicBitset &);
  DynamicBitset &operator|=(const DynamicBitset &);
  DynamicBitset &operator^=(const DynamicBitset &);
  DynamicBitset operator~() const;

  friend DynamicBitset operator&(DynamicBitset lhs, const DynamicBitset &rhs) {
    return lhs &= rhs;
  }

  friend DynamicBitset operator|(DynamicBitset lhs, const DynamicBitset &rhs) {
    return lhs |= rhs;
  }

  friend DynamicBitset operator^(DynamicBitset lhs, const DynamicBitset &rhs) {
    return lhs ^= rhs;
  }
};

namespace ICL {

typedef struct _Request {
  uint64_t reqID;
  uint64_t reqSubID;
  uint64_t offset;
  uint64_t length;
  LPNRange range;

  _Request();
} Request;

}  // namespace ICL

namespace FTL {

typedef struct _Request {
  uint64_t reqID;  // ID of ICL::Request
  uint64_t reqSubID;
  uint64_t lpn;
  std::vector<bool> ioFlag;

  _Request();
  _Request(ICL::Request &);
} Request;

}  // namespace FTL

namespace PAL {

typedef struct _Request {
  uint64_t reqID;  // ID of ICL::Request
  uint64_t reqSubID;
  uint32_t blockIndex;
  uint32_t pageIndex;
  std::vector<bool> ioFlag;

  _Request();
  _Request(FTL::Request &);
} Request;

}  // namespace PAL

}  // namespace SimpleSSD

#endif
