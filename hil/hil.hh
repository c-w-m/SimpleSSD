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

#ifndef __HIL_HIL__
#define __HIL_HIL__

#include "icl/cache.hh"
#include "util/config.hh"

namespace SimpleSSD {

namespace HIL {

class HIL {
 private:
  ConfigReader *conf;
  ICL::Cache *cache;

 public:
  HIL(ConfigReader *);

  uint64_t read(uint64_t, uint64_t, uint64_t);
  uint64_t write(uint64_t, uint64_t, uint64_t);
  uint64_t flush(uint64_t, uint64_t, uint64_t);
  uint64_t trim(uint64_t, uint64_t, uint64_t);
};

}  // namespace HIL

}  // namespace SimpleSSD

#endif
