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

#include "ftl/common/latency.hh"

namespace SimpleSSD {

namespace FTL {

Latency::Latency(uint64_t l) : lastFTLRequestAt(0), latency(l) {}

Latency::~Latency() {}

void Latency::access(uint64_t &tick) {
  if (tick > 0) {
    if (lastFTLRequestAt <= tick) {
      lastFTLRequestAt = tick + latency;
    }
    else {
      lastFTLRequestAt += latency;
    }

    tick = lastFTLRequestAt;
  }
}

}  // namespace FTL

}  // namespace SimpleSSD
