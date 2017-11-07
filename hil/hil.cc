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

#include "hil/hil.hh"

#include "log/trace.hh"
#include "util/algorithm.hh"

namespace SimpleSSD {

namespace HIL {

HIL::HIL(ConfigReader *c) : pConf(c), reqCount(0) {
  pICL = new ICL::ICL(pConf);
}

HIL::~HIL() {
  delete pICL;
}

void HIL::read(Request &req, uint64_t &tick) {
  // TODO: stat

  Logger::debugprint(Logger::LOG_HIL, "READ  | LPN %" PRIu64 " + %" PRIu64,
                     req.range.slpn, req.range.nlp);

  req.reqID = reqCount++;
  pICL->read(req, tick);
}

void HIL::write(Request &req, uint64_t &tick) {
  // TODO: stat

  Logger::debugprint(Logger::LOG_HIL, "WRITE | LPN %" PRIu64 " + %" PRIu64,
                     req.range.slpn, req.range.nlp);

  req.reqID = reqCount++;
  pICL->read(req, tick);
}

void HIL::flush(Request &req, uint64_t &tick) {
  // TODO: stat

  Logger::debugprint(Logger::LOG_HIL, "FLUSH | LPN %" PRIu64 " + %" PRIu64,
                     req.range.slpn, req.range.nlp);

  req.reqID = reqCount++;
  pICL->read(req, tick);
}

void HIL::trim(Request &req, uint64_t &tick) {
  // TODO: stat

  Logger::debugprint(Logger::LOG_HIL, "TRIM  | LPN %" PRIu64 " + %" PRIu64,
                     req.range.slpn, req.range.nlp);

  req.reqID = reqCount++;
  pICL->read(req, tick);
}

void HIL::getLPNInfo(uint64_t &totalLogicalPages, uint32_t &logicalPageSize) {
  pICL->getLPNInfo(totalLogicalPages, logicalPageSize);
}

}  // namespace HIL

}  // namespace SimpleSSD
