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

#ifndef MACHINA_SMF_WRITER_HPP
#define MACHINA_SMF_WRITER_HPP

#include <stdexcept>
#include <string>

#include "raul/TimeStamp.hpp"

#include "MIDISink.hpp"

namespace machina {

/** Standard Midi File (Type 0) Writer
 * \ingroup raul
 */
class SMFWriter
	: public MIDISink
{
public:
	explicit SMFWriter(Raul::TimeUnit unit);
	~SMFWriter();

	bool start(const std::string& filename,
	           Raul::TimeStamp    start_time) throw (std::logic_error);

	Raul::TimeUnit unit() const { return _unit; }

	void write_event(Raul::TimeStamp      time,
	                 size_t               ev_size,
	                 const unsigned char* ev) throw (std::logic_error);

	void flush();

	void finish() throw (std::logic_error);

protected:
	static const uint32_t VAR_LEN_MAX = 0x0FFFFFFF;

	void write_header();
	void write_footer();

	void   write_chunk_header(const char id[4], uint32_t length);
	void   write_chunk(const char id[4], uint32_t length, void* data);
	size_t write_var_len(uint32_t val);

	std::string     _filename;
	FILE*           _fd;
	Raul::TimeUnit  _unit;
	Raul::TimeStamp _start_time;
	Raul::TimeStamp _last_ev_time; ///< Time last event was written relative to _start_time
	uint32_t        _track_size;
	uint32_t        _header_size; ///< size of SMF header, including MTrk chunk header
};

} // namespace machina

#endif // MACHINA_SMF_WRITER_HPP
