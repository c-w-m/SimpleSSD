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

#include "hil/nvme/config.hh"
#include "util/algorithm.hh"

namespace SimpleSSD {

namespace NVMe {

const char NAME_DMA_DELAY[] = "DMADelay";
const char NAME_QUEUE_INTERVAL[] = "QueueInterval";
const char NAME_MAX_IO_CQUEUE[] = "MaxIOCQueue";
const char NAME_MAX_IO_SQUEUE[] = "MaxIOSQueue";
const char NAME_WRR_HIGH[] = "WRRHigh";
const char NAME_WRR_MID[] = "WRRMedium";
const char NAME_ENABLE_DEFAULT_NAMESPACE[] = "DefaultNamespace";
const char NAME_LBA_SIZE[] = "LBASize";
const char NAME_ENABLE_DISK_IMAGE[] = "EnableDiskImage";
const char NAME_STRICT_DISK_SIZE[] = "StrickSizeCheck";
const char NAME_DISK_IMAGE_PATH[] = "DiskImageFile";

Config::Config() {
  queueInterval = 1000000;
  dmaDelay = 256.90625f;
  maxIOCQueue = 16;
  maxIOSQueue = 16;
  wrrHigh = 2;
  wrrMidium = 2;
  lbaSize = 512;
  enableDefaultNamespace = true;
  enableDiskImage = false;
  strictDiskSize = false;
  diskImagePath = "";
}

bool Config::setConfig(const char *name, const char *value) {
  bool ret = true;

  if (MATCH_NAME(NAME_DMA_DELAY)) {
    dmaDelay = strtof(value, nullptr);
  }
  else if (MATCH_NAME(NAME_QUEUE_INTERVAL)) {
    queueInterval = strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_MAX_IO_CQUEUE)) {
    maxIOCQueue = (uint16_t)strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_MAX_IO_SQUEUE)) {
    maxIOSQueue = (uint16_t)strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_WRR_HIGH)) {
    wrrHigh = (uint16_t)strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_WRR_MID)) {
    wrrMidium = (uint16_t)strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_ENABLE_DEFAULT_NAMESPACE)) {
    enableDefaultNamespace = convertBool(value);
  }
  else if (MATCH_NAME(NAME_LBA_SIZE)) {
    lbaSize = strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_ENABLE_DISK_IMAGE)) {
    enableDiskImage = convertBool(value);
  }
  else if (MATCH_NAME(NAME_STRICT_DISK_SIZE)) {
    strictDiskSize = convertBool(value);
  }
  else if (MATCH_NAME(NAME_DISK_IMAGE_PATH)) {
    diskImagePath = value;
  }
  else {
    ret = false;
  }

  return ret;
}

void Config::update() {
  if (popcount(lbaSize) != 1) {
    // TODO: invalud lba size
  }
}

}  // namespace NVMe

}  // namespace SimpleSSD
