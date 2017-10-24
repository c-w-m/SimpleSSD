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

#include "util/config.hh"

namespace SimpleSSD {

namespace Config {

const char SECTION_NVME[] = "nvme";
const char SECTION_FTL[] = "ftl";
const char SECTION_ICL[] = "cache";

}  // namespace Config

bool ConfigReader::init(std::string file) {
  if (ini_parse(file.c_str(), parserHandler, this) < 0) {
    return false;
  }

  // Update all
  nvmeConfig.update();

  return true;
}

int ConfigReader::parserHandler(void *context, const char *section,
                                const char *name, const char *value) {
  ConfigReader *pThis = (ConfigReader *)context;

  if (MATCH_SECTION(Config::SECTION_NVME)) {
    pThis->nvmeConfig.setConfig(name, value);
  }

  return 1;
}

}  // namespace SimpleSSD
