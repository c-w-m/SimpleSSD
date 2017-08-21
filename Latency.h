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

#ifndef __Latency_h__
#define __Latency_h__

#include "SimpleSSD_types.h"

#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
using namespace std;

/*==============================
    Latency
==============================*/
class Latency
{
    public:
        uint32 SPDIV; //50 to 100mhz
        uint32 PGDIV;

        //Get Latency for PageAddress(L/C/MSBpage), Operation(RWE), BusyFor(Ch.DMA/Mem.Work)
        virtual uint64 GetLatency(uint32 AddrPage, uint8 Oper, uint8 BusyFor){ return 0; };
        virtual inline uint8  GetPageType(uint32 AddrPage) { return PAGE_NUM; };

        //Setup DMA speed and pagesize
        Latency(uint32 mhz, uint32 pagesize);
};


#endif //__Latency_h__
