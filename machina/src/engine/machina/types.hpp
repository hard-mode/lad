/*
  This file is part of Machina.
  Copyright 2007-2013 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MACHINA_TYPES_HPP
#define MACHINA_TYPES_HPP

#include <memory>

#include "raul/RingBuffer.hpp"

namespace machina {

typedef unsigned char byte;

typedef uint32_t URIInt;

#if __cplusplus >= 201103L
template <class T>
using SPtr = std::shared_ptr<T>;

template <class T>
using WPtr = std::weak_ptr<T>;
#else
#define SPtr std::shared_ptr
#define WPtr std::weak_ptr
#endif

template <class T>
void NullDeleter(T* ptr) {}

template<class T, class U>
SPtr<T> static_ptr_cast(const SPtr<U>& r) {
	return std::static_pointer_cast<T>(r);
}

template<class T, class U>
SPtr<T> dynamic_ptr_cast(const SPtr<U>& r) {
	return std::dynamic_pointer_cast<T>(r);
}

template<class T, class U>
SPtr<T> const_ptr_cast(const SPtr<U>& r) {
	return std::const_pointer_cast<T>(r);
}

} // namespace machina

#endif // MACHINA_TYPES_HPP
