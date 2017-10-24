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

#ifndef __HIL_NVME_CONFIG__
#define __HIL_NVME_CONFIG__

#include "util/base_config.hh"

namespace SimpleSSD {

namespace NVMe {

typedef enum {
  NVME_DMA_DELAY,
  NVME_QUEUE_INTERVAL,
  NVME_MAX_IO_CQUEUE,
  NVME_MAX_IO_SQUEUE,
  NVME_WRR_HIGH,
  NVME_WRR_MID,
  NVME_ENABLE_DEFAULT_NAMESPACE,
  NVME_LBA_SIZE,
  NVME_ENABLE_DISK_IMAGE,
  NVME_STRICT_DISK_SIZE,
  NVME_DISK_IMAGE_PATH,
} NVME_CONFIG;

class Config : public BaseConfig {
 private:
  std::string dmaDelay;
  std::string queueInterval;
  std::string maxIOCQueue;
  std::string maxIOSQueue;
  std::string wrrHigh;
  std::string wrrMidium;
  std::string enableDefaultNamespace;
  std::string lbaSize;
  std::string enableDiskImage;
  std::string strictDiskSize;
  std::string diskImagePath;

 public:
  Config();

  bool setConfig(const char *, const char *);
  void update();

  int32_t readInt(uint32_t);
  float readFloat(uint32_t);
  std::string readString(uint32_t);
  bool readBoolean(uint32_t);
};

}  // namespace NVMe

}  // namespace SimpleSSD

#endif
