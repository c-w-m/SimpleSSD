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

#ifndef __LOG_TRACE__
#define __LOG_TRACE__

#include <cinttypes>

namespace SimpleSSD {

namespace Logger {

typedef enum {
  LOG_COMMON,
  LOG_HIL,
  LOG_ISL,
  LOG_FTL,
  LOG_PAL,
  LOG_NUM
} LOG_ID;

inline void debugprint(LOG_ID, const char *, ...);
inline void debugprint(LOG_ID, const char *, uint64_t);

}  // namespace Logger

}  // namespace SimpleSSD

#endif
