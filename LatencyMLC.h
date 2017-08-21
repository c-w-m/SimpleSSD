/**
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
 * Authors: Gieseo Park <gieseo@camelab.org>
 *          Jie Zhang <jie@camelab.org>
 */

#ifndef __LatencyMLC_h__
#define __LatencyMLC_h__

#include "Latency.h"

class LatencyMLC : public Latency
{
    public:
        LatencyMLC(uint32 mhz, uint32 pagesize);

        uint64 GetLatency(uint32 AddrPage, uint8 Oper, uint8 Busy);
        inline uint8  GetPageType(uint32 AddrPage);
};

#endif //__LatencyMLC_h__
