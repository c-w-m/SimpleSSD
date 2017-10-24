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

#ifndef __UTIL_BASE_CONFIG__
#define __UTIL_BASE_CONFIG__

#include <cinttypes>
#include <string>

namespace SimpleSSD {

class BaseConfig {
 public:
  BaseConfig();

  virtual bool setConfig(const char *, const char *) = 0;

  virtual int32_t readInt(uint32_t) = 0;
  virtual float readFloat(uint32_t) = 0;
  virtual std::string readString(uint32_t) = 0;
  virtual bool readBoolean(uint32_t) = 0;
};

}  // namespace SimpleSSD

#endif
