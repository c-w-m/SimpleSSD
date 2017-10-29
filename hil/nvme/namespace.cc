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

#include "hil/nvme/namespace.hh"

namespace SimpleSSD {

namespace HIL {

namespace NVMe {

LBARange::_LBARange() : slba(0), nlblk(0) {}

LBARange::_LBARange(uint64_t s, uint64_t n) : slba(s), nlblk(n) {}

Namespace::Namespace(Subsystem *p, ConfigData *c)
    : pParent(p),
      pCfgdata(c),
      nsid(NSID_NONE),
      attached(false),
      allocated(false) {}

void Namespace::setData(uint32_t id, Information *data) {
  nsid = id;
  memcpy(&info, data, sizeof(Information));

  // TODO calculate lbaratio

  allocated = true;
}

void Namespace::attach(bool attach) {
  attached = attach;
}

uint32_t Namespace::getNSID() {
  return nsid;
}

}  // namespace NVMe

}  // namespace HIL

}  // namespace SimpleSSD
