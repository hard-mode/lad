/*
  Oscillator wave data loading.
  Copyright 2011 David Robillard
  Copyright 2002 Mike Rawes

  This is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "blop_config.h"
#include "wavedata.h"

#ifdef _WIN32
#    include <windows.h>
#    define dlopen(path, flags) LoadLibrary(path)
#    define dlclose(lib) FreeLibrary((HMODULE)lib)
#    define dlsym(lib, sym) GetProcAddress((HMODULE)lib, sym)
#    define snprintf _snprintf
#else
#    include <dlfcn.h>
#endif

int
wavedata_load(Wavedata*   w,
              const char* bundle_path,
              const char* lib_name,
              const char* wdat_descriptor_name,
              double      sample_rate)
{
	const size_t bundle_len   = strlen(bundle_path);
	const size_t lib_name_len = strlen(lib_name);
	const size_t ext_len      = strlen(BLOP_SHLIB_EXT);
	const size_t path_len     = bundle_len + lib_name_len + ext_len + 2;
	int          retval       = -1;

	char* lib_path = (char*)malloc(path_len);
	snprintf(lib_path, path_len, "%s%s%s",
	         bundle_path, lib_name, BLOP_SHLIB_EXT);

	void* handle = dlopen(lib_path, RTLD_NOW);
	free(lib_path);

	if (handle) {
		typedef int (*DescFunc)(Wavedata*, unsigned long);
		DescFunc desc_func = (DescFunc)dlsym(handle, wdat_descriptor_name);
		if (desc_func) {
			retval         = desc_func(w, sample_rate);
			w->data_handle = handle;
			return retval;
		}
	}

	return retval;
}

void
wavedata_unload(Wavedata* w)
{
	dlclose(w->data_handle);
}
