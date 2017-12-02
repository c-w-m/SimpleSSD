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

#include "ftl/config.hh"

#include "log/trace.hh"

namespace SimpleSSD {

namespace FTL {

const char NAME_MAPPING_MODE[] = "MappingMode";
const char NAME_OVERPROVISION_RATIO[] = "OverProvisioningRatio";
const char NAME_GC_THRESHOLD[] = "GCThreshold";
const char NAME_BAD_BLOCK_THRESHOLD[] = "EraseThreshold";
const char NAME_WARM_UP_RATIO[] = "Warmup";
const char NAME_NKMAP_N[] = "MapN";
const char NAME_NKMAP_K[] = "MapK";
const char NAME_GC_MODE[] = "GCMode";
const char NAME_GC_RECLAIM_BLOCK[] = "GCReclaimBlocks";
const char NAME_GC_RECLAIM_THRESHOLD[] = "GCReclaimThreshold";

Config::Config() {
  mapping = FTL_NK_MAPPING;
  overProvision = 0.25f;
  gcThreshold = 0.05f;
  badBlockThreshold = 100000;
  warmup = 1.f;
  reclaimBlock = 1;
  reclaimThreshold = 0.1f;
  gcMode = FTL_GC_MODE_0;

  N = 32;
  K = 32;
}

bool Config::setConfig(const char *name, const char *value) {
  bool ret = true;

  if (MATCH_NAME(NAME_MAPPING_MODE)) {
    mapping = (FTL_MAPPING)strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_OVERPROVISION_RATIO)) {
    overProvision = strtof(value, nullptr);
  }
  else if (MATCH_NAME(NAME_GC_THRESHOLD)) {
    gcThreshold = strtof(value, nullptr);
  }
  else if (MATCH_NAME(NAME_BAD_BLOCK_THRESHOLD)) {
    badBlockThreshold = strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_WARM_UP_RATIO)) {
    warmup = strtof(value, nullptr);
  }
  else if (MATCH_NAME(NAME_GC_MODE)) {
    gcMode = (FTL_GC)strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_GC_RECLAIM_BLOCK)) {
    reclaimBlock = strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_GC_RECLAIM_THRESHOLD)) {
    reclaimThreshold = strtof(value, nullptr);
  }
  else if (MATCH_NAME(NAME_NKMAP_N)) {
    N = strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_NKMAP_K)) {
    K = strtoul(value, nullptr, 10);
  }
  else {
    ret = false;
  }

  return ret;
}

void Config::update() {
  if (reclaimBlock == 0) {
    Logger::panic("Invalid GCReclaimBlocks");
  }

  if (reclaimThreshold < gcThreshold) {
    Logger::panic("Invalid GCReclaimThreshold");
  }
}

int64_t Config::readInt(uint32_t idx) {
  int64_t ret = 0;

  switch (idx) {
    case FTL_MAPPING_MODE:
      ret = mapping;
      break;
    case FTL_GC_MODE:
      ret = gcMode;
      break;
  }

  return ret;
}

uint64_t Config::readUint(uint32_t idx) {
  uint64_t ret = 0;

  switch (idx) {
    case FTL_BAD_BLOCK_THRESHOLD:
      ret = badBlockThreshold;
      break;
    case FTL_GC_RECLAIM_BLOCK:
      ret = reclaimBlock;
      break;
    case FTL_NKMAP_N:
      ret = N;
      break;
    case FTL_NKMAP_K:
      ret = K;
      break;
  }

  return ret;
}

float Config::readFloat(uint32_t idx) {
  float ret = 0.f;

  switch (idx) {
    case FTL_OVERPROVISION_RATIO:
      ret = overProvision;
      break;
    case FTL_GC_THRESHOLD_RATIO:
      ret = gcThreshold;
      break;
    case FTL_WARM_UP_RATIO:
      ret = warmup;
      break;
    case FTL_GC_RECLAIM_THRESHOLD:
      ret = reclaimThreshold;
      break;
  }

  return ret;
}

std::string Config::readString(uint32_t idx) {
  std::string ret("");

  return ret;
}

bool Config::readBoolean(uint32_t idx) {
  bool ret = false;

  return ret;
}

}  // namespace FTL

}  // namespace SimpleSSD
