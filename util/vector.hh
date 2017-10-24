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

#ifndef __UTIL_VECTOR__
#define __UTIL_VECTOR__

#include <cinttypes>
#include <cstring>

namespace SimpleSSD {

#define ALLOC_UNIT 64

/**
 * This vector uses calloc/free to allocate memory
 * So if you pass object (not pointer), constructor will not called
 */
template <typename T>
class Vector {
 private:
  T *data;
  uint64_t length;
  uint64_t capacity;

 public:
  Vector();
  Vector(uint64_t);
  ~Vector();

  T &at(uint64_t);
  T &operator[](uint64_t);

  uint64_t size();
  void resize(uint64_t);

  void push_back(T &);
  T pop_back();

  void insert(uint64_t, T &);
  void erase(uint64_t);
};

}  // namespace SimpleSSD

#endif
