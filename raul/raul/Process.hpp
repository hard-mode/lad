/*
  This file is part of Raul.
  Copyright 2007-2012 David Robillard <http://drobilla.net>

  Raul is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Raul is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Raul.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RAUL_PROCESS_HPP
#define RAUL_PROCESS_HPP

#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include <string>

#include "raul/Noncopyable.hpp"

namespace Raul {

/** A child process.
 *
 * \ingroup raul
 */
class Process : Noncopyable
{
public:

	/** Launch a sub process.
	 *
	 * @param command can be a typical shell command with parameters, the PATH is searched etc.
	 */
	static bool launch(const std::string& command) {
		const std::string executable = (command.find(" ") != std::string::npos)
			? command.substr(0, command.find(" "))
			: command;

        const std::string arguments = command.substr((command.find(" ") + 1));

        // Use the same double fork() trick as JACK to prevent zombie children
        const int err = fork();

        if (err == 0) {
            // (child)

            // close all nonstandard file descriptors
            struct rlimit max_fds;
            getrlimit(RLIMIT_NOFILE, &max_fds);

            for (rlim_t fd = 3; fd < max_fds.rlim_cur; ++fd)
                close(fd);

            switch (fork()) {
                case 0:
                    // (grandchild)
                    setsid();
                    execlp(executable.c_str(), arguments.c_str(), NULL);
					_exit(-1);

				case -1:
					// (second) fork failed, there is no grandchild
					_exit(-1);

					/* exit the child process here */
				default:
					_exit(0);
			}
		}

		return (err > 0);
	}

private:
	Process() {}
};

} // namespace Raul

#endif // RAUL_PROCESS_HPP
