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

#include "hil/nvme/queue.hh"

namespace SimpleSSD {

namespace NVMe {

SQEntry::_SQEntry() {
  memset(data, 0, 64);
}

CQEntry::_CQEntry() {
  memset(data, 0, 16);
}

SQEntryWrapper::_SQEntryWrapper(SQEntry &sqdata, uint16_t sqid, uint16_t cqid,
                               uint16_t sqhead)
    : entry(sqdata), sqID(sqid), cqID(cqid), sqHead(sqhead) {}

CQEntryWrapper::_CQEntryWrapper(SQEntryWrapper &sqew) {
  cqID = sqew.cqID;
  entry.dword2.sqHead = sqew.sqHead;
  entry.dword2.sqID = sqew.sqID;
  entry.dword3.commandID = sqew.entry.dword0.commandID;
}

Queue::Queue(uint16_t qid, uint16_t length)
    : id(qid), head(0), tail(0), size(length), stride(0), base(nullptr) {}

Queue::~Queue() {
  if (base) {
    delete base;
  }
}

uint16_t Queue::getID() {
  return id;
}

uint16_t Queue::getItemCount() {
  if (tail >= head) {
    return tail - head;
  }
  else {
    return (size - head) + tail;
  }
}

uint16_t Queue::getHead() {
  return head;
}

uint16_t Queue::getTail() {
  return tail;
}

uint16_t Queue::getSize() {
  return size;
}

CQueue::CQueue(uint16_t iv, bool en, uint16_t qid, uint16_t size)
    : Queue(qid, size), ien(en), phase(true), interruptVector(iv) {}

uint64_t CQueue::setData(CQEntry *entry, uint64_t tick) {
  if (entry) {
    // Set phase
    entry->dword3.status &= 0xFFFE;
    entry->dword3.status |= (phase ? 0x0001 : 0x0000);

    // Write entry
    tick = base->write(tail * stride, 0x10, entry->data, tick);

    // Increase tail
    tail++;

    if (tail == size) {
      tail = 0;
      phase = !phase;
    }

    if (head == tail) {
      // TODO: logging system
      // panic("nvme_ctrl: Completion Queue Overflow!! CQID: %d, head: %d\n",
      // qid, head);
    }

    /*
    TODO: Logging system
    DPRINTF(NVMeQueue,
            "CQ %-5d| WRITE | Item count in queue %d | head %d | tail %d\n",
            qid, getItemCount(), head, tail);
    DPRINTF(NVMeBreakdown, "C%d|3|H%d|T%d|S%d|I%d\n", qid, head, tail,
            entry->DW2.SQID, entry->DW3.CID); */
  }

  return tick;
}

uint16_t CQueue::incHead() {
  head++;

  if (head == size) {
    head = 0;
  }

  return head;
}

void CQueue::setHead(uint16_t newHead) {
  head = newHead;
}

bool CQueue::interruptEnabled() {
  return ien;
}

uint16_t CQueue::getInterruptVector() {
  return interruptVector;
}

SQueue::SQueue(uint16_t cqid, uint8_t pri, uint16_t qid, uint16_t size)
    : Queue(qid, size), cqID(cqid), priority(pri) {}

uint16_t SQueue::getCQID() {
  return cqID;
}

void SQueue::setTail(uint16_t newTail) {
  tail = newTail;
}

uint64_t SQueue::getData(SQEntry *entry, uint64_t tick) {
  if (entry && head != tail) {
    // Read entry
    tick = base->read(head * stride, 0x40, entry->data, tick);

    // Increase head
    head++;

    if (head == size) {
      head = 0;
    }

    /*
    TODO: logging system
        DPRINTF(NVMeQueue,
                "SQ %-5d| READ  | Item count in queue %d | head %d | tail %d\n",
                qid, getItemCount(), head, tail);
        DPRINTF(NVMeBreakdown, "S%d|1|H%d|T%d|I%d|N%u\n", qid, head, tail,
                entry->CDW0.CID, entry->NSID); */
  }

  return tick;
}

}  // namespace NVMe

}  // namespace SimpleSSD
