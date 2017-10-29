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

#ifndef __UTIL_CONFIG_READER__
#define __UTIL_CONFIG_READER__

#include <cinttypes>
#include <string>

#include "ftl/config.hh"
#include "hil/nvme/config.hh"
#include "lib/ini/ini.h"

namespace SimpleSSD {

class ConfigReader {
 private:
  static int parserHandler(void *, const char *, const char *, const char *);

 public:
  HIL::NVMe::Config nvmeConfig;
  FTL::Config ftlConfig;

  bool init(std::string);

  uint64_t getNVMCapacity();
};

}  // namespace SimpleSSD

#endif
