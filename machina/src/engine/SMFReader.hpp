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

#ifndef MACHINA_SMF_READER_HPP
#define MACHINA_SMF_READER_HPP

#include <stdexcept>
#include <string>
#include <inttypes.h>
#include "raul/TimeStamp.hpp"

namespace machina {

/** Standard Midi File (Type 0) Reader
 *
 * Currently this only reads SMF files with tempo-based timing.
 * \ingroup raul
 */
class SMFReader
{
public:
	class PrematureEOF
		: public std::exception
	{
		const char* what() const throw() { return "Unexpected end of file"; }
	};
	class CorruptFile
		: public std::exception
	{
		const char* what() const throw() { return "Corrupted file"; }
	};
	class UnsupportedTime
		: public std::exception
	{
		const char* what() const throw() { return
			                                   "Unsupported time stamp type (SMPTE)"; }
	};

	explicit SMFReader(const std::string filename = "");
	~SMFReader();

	bool open(const std::string& filename) throw (std::logic_error,
	                                              UnsupportedTime);

	bool seek_to_track(unsigned track) throw (std::logic_error);

	uint16_t type() const { return _type; }
	uint16_t ppqn() const { return _ppqn; }
	size_t   num_tracks() { return _num_tracks; }

	int read_event(size_t    buf_len,
	               uint8_t*  buf,
	               uint32_t* ev_size,
	               uint32_t* ev_delta_time)
	throw (std::logic_error, PrematureEOF, CorruptFile);

	void close();

	static uint32_t read_var_len(FILE* fd) throw (PrematureEOF);

protected:
	/** size of SMF header, including MTrk chunk header */
	static const uint32_t HEADER_SIZE = 22;

	std::string _filename;
	FILE*       _fd;
	uint16_t    _type;
	uint16_t    _ppqn;
	uint16_t    _num_tracks;
	uint32_t    _track;
	uint32_t    _track_size;
};

} // namespace machina

#endif // MACHINA_SMF_READER_HPP
