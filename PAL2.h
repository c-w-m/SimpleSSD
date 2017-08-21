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
 * Authors: Jie Zhang <jie@camelab.org>
 */

#ifndef __PAL2_h__
#define __PAL2_h__

#include "SimpleSSD_types.h"

#include "dev/storage/nvme/nvme_cfg.hh"
#include "Latency.h"
#include "PALStatistics.h"

#include "PAL2_TimeSlot.h"

#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
using namespace std;

class PALStatistics;

class PAL2 //let's not inherit PAL1
{
  public:
    PAL2(PALStatistics* statistics, NVMeConfig *c, Latency *l);
    ~PAL2();

    NVMeConfig *gconf;
    Latency *lat;

    TimeSlot** ChTimeSlots;
    TimeSlot** DieTimeSlots;
    TimeSlot** MergedTimeSlots; //for gathering busy time

    std::map<uint64, uint64> OpTimeStamp[3];

    std::map<uint64, std::map<uint64, uint64>* > * ChFreeSlots;
    uint64* ChStartPoint; //record the start point of rightmost free slot
    std::map<uint64, std::map<uint64, uint64>* > * DieFreeSlots;
    uint64* DieStartPoint;

    void submit(Command &cmd);
    void TimelineScheduling(Command& req);
    PALStatistics* stats; //statistics of PAL2, not created by itself
    void InquireBusyTime(uint64 currentTick);
    void FlushTimeSlots(uint64 currentTick);
    void FlushOpTimeStamp();
    TimeSlot* FlushATimeSlot(TimeSlot* tgtTimeSlot, uint64 currentTick);
    TimeSlot* FlushATimeSlotBusyTime(TimeSlot* tgtTimeSlot, uint64 currentTick, uint64* TimeSum);
    //Jie: merge time slotss
    void MergeATimeSlot(TimeSlot* tgtTimeSlot);
    void MergeATimeSlot(TimeSlot* startTimeSlot, TimeSlot* endTimeSlot);
    void MergeATimeSlotCH(TimeSlot* tgtTimeSlot);
    void MergeATimeSlotDIE(TimeSlot* tgtTimeSlot);
    TimeSlot* InsertAfter(TimeSlot* tgtTimeSlot, uint64 tickLen, uint64 tickFrom);

    TimeSlot* FindFreeTime(TimeSlot* tgtTimeSlot, uint64 tickLen, uint64 tickFrom); // you can insert a tickLen TimeSlot after Returned TimeSlot.

    //Jie: return: FreeSlot is found?
    bool FindFreeTime(std::map<uint64, std::map<uint64, uint64>* >& tgtFreeSlot, uint64 tickLen, uint64 & tickFrom, uint64 & startTick, bool & conflicts);
    void InsertFreeSlot(std::map<uint64, std::map<uint64, uint64>* >& tgtFreeSlot, uint64 tickLen, uint64 tickFrom, uint64 startTick, uint64 & startPoint, bool split);
    void AddFreeSlot(std::map<uint64, std::map<uint64, uint64>* >& tgtFreeSlot, uint64 tickLen, uint64 tickFrom);
    void FlushFreeSlots(uint64 currentTick);
    void FlushAFreeSlot(std::map<uint64, std::map<uint64, uint64>* >& tgtFreeSlot, uint64 currentTick);
    uint8 VerifyTimeLines(uint8 print_on);


    //PPN Conversion related //ToDo: Shifted-Mode is also required for better performance.
    uint32 RearrangedSizes[7];
    uint32 CPDPBPtoDieIdx(CPDPBP* pCPDPBP);
    void printCPDPBP(CPDPBP* pCPDPBP);
    void PPNdisassemble(uint64* pPPN, CPDPBP* pCPDPBP);
    void AssemblePPN(CPDPBP* pCPDPBP, uint64* pPPN);
};

#endif
