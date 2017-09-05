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

#ifndef __UTIL_HASH_TABLE__
#define __UTIL_HASH_TABLE__

#include <functional>

#include "util/list.hh"

namespace SimpleSSD {

template <typename T1, typename T2>
class HashTable {
 private:
  List<T2> *hashTable;
  std::function<uint64_t(T1 &, bool)> hashFunction;
  std::function<bool(T1 &, T1 &)> compareFunction;

 public:
  HashTable(std::function<uint64_t(T1, bool)> &,
            std::function<bool(T1 &, T1 &)> &);

  bool set(T1 &, T2 &);
  bool remove(T1 &, T2 &);

  T2 &get(T1 &);
  const T2 &get(T1 &) const;
};

}  // namespace SimpleSSD

#endif
