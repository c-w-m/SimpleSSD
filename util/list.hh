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

#ifndef __UTIL_LIST__
#define __UTIL_LIST__

#include <cinttypes>
#include <cstring>

namespace SimpleSSD {

/**
 * This list uses calloc/free to allocate memory
 * So if you pass object (not pointer), constructor will not called
 */
template <typename T1>
class List {
 public:
  template <typename T2>
  class Item {
   private:
    Item<T2> *_next;
    Item<T2> *_before;
    T2 _value;

   public:
    Item();
    Item(Item<T2> *, Item<T2> *, T2 &);

    T2 &val();
    const T2 &val() const;

    const Item<T2> *next();
    const Item<T2> *before();
  };

 private:
  Item<T1> *head;
  Item<T1> *tail;
  uint64_t length;

 public:
  List();
  List(uint64_t);

  uint64_t size();
  void resize(uint64_t);

  void push_back(T1 &);
  T1 &pop_back();
  const T1 &pop_back() const;

  void push_front(T1 &);
  T1 &pop_front();
  const T1 &pop_front() const;

  T1 &front();
  const T1 &front() const;
  T1 &back();
  const T1 &back() const;

  void insert(Item<T1> *, T1 &);
  void erase(Item<T1> *);
};

}  // namespace SimpleSSD

#endif
