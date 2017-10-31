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

GenericCache::GenericCache(ConfigReader *c) : Cache(c) {
  setSize = c->iclConfig.readUint(ICL_SET_SIZE);
  entrySize = c->iclConfig.readUint(ICL_ENTRY_SIZE);
  dataSize = c->palConfig.readUint(PAL::NAND_PAGE_SIZE);

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

}  // namespace ICL

}  // namespace SimpleSSD
