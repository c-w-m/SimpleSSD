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

#include "LatencySLC.h"

LatencySLC::LatencySLC(uint32 mhz, uint32 pagesize)
: Latency(mhz, pagesize)
{
    ;
}

inline uint8 LatencySLC::GetPageType(uint32 AddrPage)
{
    return PAGE_LSB;
}

uint64 LatencySLC::GetLatency(uint32 AddrPage, uint8 Oper, uint8 Busy)
{
    #if 1
    //ps
  uint64 lat_tbl[3][5] =  //uint32 lat_tbl[3][5] = //Gieseo,is this right?
    {                /*  LSB           DMA0,                  DMA1*/
        /* Read  */{   25000000,     100000/SPDIV,         185000000*2/(PGDIV*SPDIV) },
        /* Write */{  300000000, 185000000*2/(PGDIV*SPDIV), 100000/SPDIV },
        /* Erase */{ 2000000000,    1500000/SPDIV,         100000/SPDIV }
    };
    #else
    //ns
    uint64 lat_tbl[3][5] =  //uint32 lat_tbl[3][5] = //Gieseo,is this right?
    {                /*  LSB      CSB      MSB    DMA0,  DMA1*/
        /* Read  */{   58000,   78000,  107000,    100/SPDIV, 185000/(PGDIV*SPDIV) },
        /* Write */{  558000, 2201000, 5001000, 185000/(PGDIV*SPDIV),    100/SPDIV },
        /* Erase */{ 2274000, 2274000, 2274000,   1500/SPDIV,    100/SPDIV }
    };
    #endif

    switch(Busy)
    {
        case BUSY_DMA0:
            return lat_tbl[Oper][1];
        case BUSY_DMA1:
            return lat_tbl[Oper][2];
        case BUSY_MEM:
        {
            //uint8 ptype = (AddrPage<=5)?PAGE_LSB : ( (AddrPage<=7)?PAGE_CSB : (((AddrPage-8)>>1)%3) );
            //uint8 ptype = GetPageType(AddrPage);
            uint64 ret= lat_tbl[Oper][ 0 ]; //SLC only has one type of page

            #if DBG_PRINT_TICK
            //    printf("LAT %s page_%u(%s) = %llu\n", OPER_STRINFO[Oper], AddrPage, PAGE_STRINFO[ptype], ret);
            #endif
            return ret;
        }
        default:
            break;
    }
    return 10;
}
