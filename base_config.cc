/**
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
 *
 * Authors: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "base_config.hh"

uint64_t BaseConfig::GetTotalSizeSSD() {
  return GetTotalNumPage() * SizePage;
}

uint64_t BaseConfig::GetTotalNumPage() {
  return GetTotalNumBlock() * NumPage;
}

uint64_t BaseConfig::GetTotalNumBlock() {
  return GetTotalNumPlane() * NumBlock;
}

uint64_t BaseConfig::GetTotalNumPlane() {
  return GetTotalNumDie() * NumPlane;
}

uint64_t BaseConfig::GetTotalNumDie() {
  return (uint64_t)NumChannel * NumPackage * NumDie;
}
