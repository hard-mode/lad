/*
  Apply a C arithmetical operator to two sample buffers.
  Copyright 2012-2014 David Robillard

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

#ifndef BLOP_VECTOR_OP_H
#define BLOP_VECTOR_OP_H

#define VECTOR_OP(op, output, input1, input1_is_cv, input2, input2_is_cv) \
	switch ((input1_is_cv << 1) + input2_is_cv) { \
	case 0: /* 00 (control * control) */ \
		output[0] = input1[0] op input2[0]; \
		break; \
	case 1: /* 01 (control * cv) */ \
		for (uint32_t s = 0; s < sample_count; ++s) { \
			output[s] = input1[0] op input2[s]; \
		} \
		break; \
	case 2: /* 10 (cv * control) */ \
		for (uint32_t s = 0; s < sample_count; ++s) { \
			output[s] = input1[s] op input2[0]; \
		} \
		break; \
	case 3: /* 11 (cv * cv) */ \
		for (uint32_t s = 0; s < sample_count; ++s) { \
			output[s] = input1[s] op input2[s]; \
		} \
		break; \
	}

#endif /* BLOP_VECTOR_OP_H */
