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

#include "util/vector.hh"

#include <cerrno>
#include <cstdlib>

namespace SimpleSSD {

template <typename T>
Vector<T>::Vector() {
  data = (T *)calloc(ALLOC_UNIT, sizeof(T));

  if (data == NULL) {
    errno = -ENOMEM;
  }
  else {
    errno = 0;
    length = 0;
    capacity = ALLOC_UNIT;
  }
}

template <typename T>
Vector<T>::Vector(uint64_t count) {
  capacity = (count / ALLOC_UNIT + 1) * ALLOC_UNIT;
  length = count;

  data = (T *)calloc(capacity, sizeof(T));

  if (data == NULL) {
    errno = -ENOMEM;
  }
  else {
    errno = 0;
  }
}

template <typename T>
Vector<T>::~Vector() {
  errno = 0;
  free(data);
}

template <typename T>
T &Vector<T>::at(uint64_t idx) {
  if (idx < length) {
    errno = 0;

    return data[idx];
  }
  else {
    errno = -ERANGE;

    return NULL;
  }
}

template <typename T>
T &Vector<T>::operator[](uint64_t idx) {
  return this->at(idx);
}

template <typename T>
uint64_t Vector<T>::size() {
  return length;
}

template <typename T>
void Vector<T>::resize(uint64_t count) {
  if (count >= capacity) {
    capacity = (count / ALLOC_UNIT + 1) * ALLOC_UNIT;

    T *ptr = (T *)realloc(data, capacity * sizeof(T));

    if (ptr == NULL) {
      errno = -ENOMEM;
      capacity = (length / ALLOC_UNIT + 1) * ALLOC_UNIT;
    }
    else {
      errno = 0;
      data = ptr;
      memset(data + length, 0, capacity - length);
    }
  }

  length = count;
}

template <typename T>
void Vector<T>::push_back(T &val) {
  data[length++] = val;

  resize(length);
}

template <typename T>
T Vector<T>::pop_back() {
  if (length > 0) {
    T val = data[--length];
    data[length] = NULL;

    return val;
  }
  else {
    errno = -ERANGE;

    return NULL;
  }
}

template <typename T>
void Vector<T>::insert(uint64_t idx, T &val) {
  resize(length + 1);

  if (errno == 0) {
    memmove(data + idx, data + idx + 1, (length - idx - 1) * sizeof(T));
    data[idx] = val;
  }
}

template <typename T>
void Vector<T>::erase(uint64_t idx) {
  if (idx < length) {
    memmove(data + idx + 1, data + idx, (length-- - idx - 1) * sizeof(T));
    data[length] = NULL;
  }
  else {
    errno = -ERANGE;
  }
}

}  // namespace SimpleSSD
