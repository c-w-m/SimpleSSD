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

#include "icl/abstract_cache.hh"

namespace SimpleSSD {

namespace ICL {

Line::_Line(uint32_t count)
    : tag(0),
      lastAccessed(0),
      insertedAt(0),
      dirtyBits(count),
      validBits(count) {}

Line::_Line(uint32_t count, uint64_t t, bool d)
    : tag(t),
      lastAccessed(0),
      insertedAt(0),
      dirtyBits(count),
      validBits(count) {
  if (d) {
    dirtyBits.set();
  }
  else {
    dirtyBits.reset();
  }

  validBits.set();
}

AbstractCache::AbstractCache(ConfigReader *c, FTL::FTL *f) : conf(c), pFTL(f) {}

AbstractCache::~AbstractCache() {}

}  // namespace ICL

}  // namespace SimpleSSD
