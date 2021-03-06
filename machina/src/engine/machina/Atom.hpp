/*
  This file is part of Machina.
  Copyright 2007-2013 David Robillard <http://drobilla.net>

  Raul is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Raul is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Raul.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MACHINA_ATOM_HPP
#define MACHINA_ATOM_HPP

#include <stdint.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

namespace machina {

class Forge;

/**
   A generic typed data container.
 
   An Atom holds a value with some type and size, both specified by a uint32_t.
   Values with size less than sizeof(void*) are stored inline: no dynamic
   allocation occurs so Atoms may be created in hard real-time threads.
   Otherwise, if the size is larger than sizeof(void*), the value will be
   dynamically allocated in a separate chunk of memory.
*/
class Atom {
public:
	Atom() : _size(0), _type(0) { _val._blob = NULL; }
	~Atom() { dealloc(); }

	typedef uint32_t TypeID;

	/** Contruct a raw atom.
	 *
	 * Typically this is not used directly, use Forge methods to make atoms.
	 */
	Atom(uint32_t size, TypeID type, const void* body)
		: _size(size)
		, _type(type)
	{
		if (is_reference()) {
			_val._blob = malloc(size);
		}
		if (body) {
			memcpy(get_body(), body, size);
		}
	}

	Atom(const Atom& copy)
		: _size(copy._size)
		, _type(copy._type)
	{
		if (is_reference()) {
			_val._blob = malloc(_size);
			memcpy(_val._blob, copy._val._blob, _size);
		} else {
			memcpy(&_val, &copy._val, _size);
		}
	}

	Atom& operator=(const Atom& other) {
		if (&other == this) {
			return *this;
		}
		dealloc();
		_size = other._size;
		_type = other._type;
		if (is_reference()) {
			_val._blob = malloc(_size);
			memcpy(_val._blob, other._val._blob, _size);
		} else {
			memcpy(&_val, &other._val, _size);
		}
		return *this;
	}

	inline bool operator==(const Atom& other) const {
		if (_type == other.type() && _size == other.size()) {
			if (is_reference()) {
				return !memcmp(_val._blob, other._val._blob, _size);
			} else {
				return !memcmp(&_val, &other._val, _size);
			}
		}
		return false;
	}

	inline bool operator!=(const Atom& other) const {
		return !operator==(other);
	}

	inline bool operator<(const Atom& other) const {
		if (_type == other.type()) {
			if (is_reference()) {
				return memcmp(_val._blob, other._val._blob, _size) < 0;
			} else {
				return memcmp(&_val, &other._val, _size) < 0;
			}
		}
		return _type < other.type();
	}

	inline uint32_t size()     const { return _size; }
	inline bool     is_valid() const { return _type; }
	inline TypeID   type()     const { return _type; }

	inline const void* get_body() const {
		return is_reference() ? _val._blob : &_val;
	}

	inline void* get_body() {
		return is_reference() ? _val._blob : &_val;
	}

	template <typename T> const T& get() const {
		assert(size() == sizeof(T));
		return *static_cast<const T*>(get_body());
	}

	template <typename T> const T* ptr() const {
		return static_cast<const T*>(get_body());
	}

private:
	friend class Forge;

	/** Free dynamically allocated value, if applicable. */
	inline void dealloc() {
		if (is_reference()) {
			free(_val._blob);
		}
	}

	/** Return true iff this value is dynamically allocated. */
	inline bool is_reference() const {
		return _size > sizeof(_val);
	}

	uint32_t _size;
	TypeID   _type;

	union {
		intptr_t _val;
		void*    _blob;
	} _val;
};

class Forge {
public:
	Forge()
		: Int(1)
		, Float(2)
		, Bool(3)
		, URI(4)
		, URID(5)
		, String(6)
	{}

	virtual ~Forge() {}

	Atom make()          { return Atom(); }
	Atom make(int32_t v) { return Atom(sizeof(v), Int, &v); }
	Atom make(float v)   { return Atom(sizeof(v), Float, &v); }
	Atom make(bool v) {
		const int32_t iv = v ? 1 : 0;
		return Atom(sizeof(int32_t), Bool, &iv);
	}

	Atom make_urid(int32_t v) { return Atom(sizeof(int32_t), URID, &v); }

	Atom alloc(uint32_t size, uint32_t type, const void* val) {
		return Atom(size, type, val);
	}

	Atom alloc(const char* v) {
		const size_t len = strlen(v);
		return Atom(len + 1, String, v);
	}

	Atom alloc(const std::string& v) {
		return Atom(v.length() + 1, String, v.c_str());
	}

	Atom alloc_uri(const char* v) {
		const size_t len = strlen(v);
		return Atom(len + 1, URI, v);
	}

	Atom alloc_uri(const std::string& v) {
		return Atom(v.length() + 1, URI, v.c_str());
	}

	Atom::TypeID Int;
	Atom::TypeID Float;
	Atom::TypeID Bool;
	Atom::TypeID URI;
	Atom::TypeID URID;
	Atom::TypeID String;
};

} // namespace machina

#endif // MACHINA_ATOM_HPP
