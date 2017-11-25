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

namespace HIL {

namespace NVMe {

const char NAME_QUEUE_INTERVAL[] = "QueueInterval";
const char NAME_WORK_INTERVAL[] = "WorkInterval";
const char NAME_MAX_IO_CQUEUE[] = "MaxIOCQueue";
const char NAME_MAX_IO_SQUEUE[] = "MaxIOSQueue";
const char NAME_WRR_HIGH[] = "WRRHigh";
const char NAME_WRR_MEDIUM[] = "WRRMedium";
const char NAME_ENABLE_DEFAULT_NAMESPACE[] = "DefaultNamespace";
const char NAME_LBA_SIZE[] = "LBASize";
const char NAME_ENABLE_DISK_IMAGE[] = "EnableDiskImage";
const char NAME_STRICT_DISK_SIZE[] = "StrickSizeCheck";
const char NAME_DISK_IMAGE_PATH[] = "DiskImageFile";
const char NAME_USE_COW_DISK[] = "UseCopyOnWriteDisk";

Config::Config() {
  queueInterval = 1000000;
  workInterval = 50000;
  maxIOCQueue = 16;
  maxIOSQueue = 16;
  wrrHigh = 2;
  wrrMedium = 2;
  lbaSize = 512;
  enableDefaultNamespace = true;
  enableDiskImage = false;
  strictDiskSize = false;
  diskImagePath = "";
  useCopyOnWriteDisk = false;
}

bool Config::setConfig(const char *name, const char *value) {
  bool ret = true;

  if (MATCH_NAME(NAME_QUEUE_INTERVAL)) {
    queueInterval = strtoul(value, nullptr, 10);
  }
  else if (MATCH_NAME(NAME_WORK_INTERVAL)) {
    workInterval = strtoul(value, nullptr, 10);
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
  else if (MATCH_NAME(NAME_WRR_MEDIUM)) {
    wrrMedium = (uint16_t)strtoul(value, nullptr, 10);
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
  else if (MATCH_NAME(NAME_USE_COW_DISK)) {
    useCopyOnWriteDisk = convertBool(value);
  }
  else {
    ret = false;
  }

  return ret;
}

void Config::update() {
  if (popcount(lbaSize) != 1) {
    // TODO: invalid lba size
  }
}

int64_t Config::readInt(uint32_t idx) {
  int64_t ret = 0;

  switch (idx) {
    case NVME_MAX_IO_CQUEUE:
      ret = maxIOCQueue;
      break;
    case NVME_MAX_IO_SQUEUE:
      ret = maxIOSQueue;
      break;
    case NVME_WRR_HIGH:
      ret = wrrHigh;
      break;
    case NVME_WRR_MEDIUM:
      ret = wrrMedium;
      break;
  }

  return ret;
}

uint64_t Config::readUint(uint32_t idx) {
  uint64_t ret = 0;

  switch (idx) {
    case NVME_QUEUE_INTERVAL:
      ret = queueInterval;
      break;
    case NVME_WORK_INTERVAL:
      ret = workInterval;
      break;
    case NVME_MAX_IO_CQUEUE:
      ret = maxIOCQueue;
      break;
    case NVME_MAX_IO_SQUEUE:
      ret = maxIOSQueue;
      break;
    case NVME_WRR_HIGH:
      ret = wrrHigh;
      break;
    case NVME_WRR_MEDIUM:
      ret = wrrMedium;
      break;
    case NVME_LBA_SIZE:
      ret = lbaSize;
      break;
  }

  return ret;
}

float Config::readFloat(uint32_t idx) {
  float ret = 0.f;

  switch (idx) {
    case NVME_DMA_DELAY:
      ret = dmaDelay;
      break;
  }

  return ret;
}

std::string Config::readString(uint32_t idx) {
  std::string ret("");

  switch (idx) {
    case NVME_DISK_IMAGE_PATH:
      ret = diskImagePath;
      break;
  }

  return ret;
}

bool Config::readBoolean(uint32_t idx) {
  bool ret = false;

  switch (idx) {
    case NVME_ENABLE_DEFAULT_NAMESPACE:
      ret = enableDefaultNamespace;
      break;
    case NVME_ENABLE_DISK_IMAGE:
      ret = enableDiskImage;
      break;
    case NVME_STRICT_DISK_SIZE:
      ret = strictDiskSize;
      break;
    case NVME_USE_COW_DISK:
      ret = useCopyOnWriteDisk;
      break;
  }

  return ret;
}

}  // namespace NVMe

}  // namespace HIL

}  // namespace SimpleSSD
