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

/**
 * @file "def.cc"
 *
 * Declaration for PCI or PCI Express structures.
 */

#include "src/hil/pci/def.hh"

#include <cstring>

namespace SimpleSSD {

namespace PCI {

Config::Config() {
  memset(data, 0, SIZE_CONFIG);
}

PCIPowerManagement::PCIPowerManagement() {
  memset(data, 0, SIZE_PMCAP);
}

MSI::MSI() {
  memset(data, 0, SIZE_MSI);
}

MSIX::MSIX() {
  memset(data, 0, SIZE_MSIX);
}

MSIXTableEntry::MSIXTableEntry() {
  memset(data, 0, SIZE_MSIX_TABLE);
  vectorControl = 0x00000001;
}

MSIXPBAEntry::MSIXPBAEntry() {
  memset(data, 0, SIZE_MSIX_PBA);
}

PCIExpress::PCIExpress() {
  memset(data, 0, SIZE_PXCAP);
}

}  // namespace PCI

}  // namespace SimpleSSD
