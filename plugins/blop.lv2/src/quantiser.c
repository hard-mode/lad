/*
  An LV2 plugin to quantise an input to a set of fixed values.
  Copyright 2011 David Robillard
  Copyright 2003 Mike Rawes

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

#include <stdio.h>
#include <stdlib.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "math_func.h"
#include "common.h"

#define QUANTISER_RANGE_MIN      0
#define QUANTISER_RANGE_MAX      1
#define QUANTISER_MATCH_RANGE    2
#define QUANTISER_MODE           3
#define QUANTISER_COUNT          4
#define QUANTISER_VALUE_START    5
#define QUANTISER_INPUT          (QUANTISER_MAX_INPUTS + 5)
#define QUANTISER_OUTPUT         (QUANTISER_MAX_INPUTS + 6)
#define QUANTISER_OUTPUT_CHANGED (QUANTISER_MAX_INPUTS + 7)

typedef struct {
	const float* min;
	const float* max;
	const float* match_range;
	const float* mode;
	const float* count;
	const float* values[QUANTISER_MAX_INPUTS];
	const float* input;
	float*       output_changed;
	float*       output;
	float        svalues[QUANTISER_MAX_INPUTS + 2];
	float        temp[QUANTISER_MAX_INPUTS + 2];
	float        last_found;
} Quantiser;

/*
 * f <= m <= l
*/
static inline void
merge(float* v,
      int    f,
      int    m,
      int    l,
      float* temp)
{
	int f1 = f;
	int l1 = m;
	int f2 = m + 1;
	int l2 = l;
	int i  = f1;

	while ((f1 <= l1) && (f2 <= l2)) {
		if (v[f1] < v[f2]) {
			temp[i] = v[f1];
			f1++;
		} else {
			temp[i] = v[f2];
			f2++;
		}
		i++;
	}
	while (f1 <= l1) {
		temp[i] = v[f1];
		f1++;
		i++;
	}
	while (f2 <= l2) {
		temp[i] = v[f2];
		f2++;
		i++;
	}
	for (i = f; i <= l; i++) {
		v[i] = temp[i];
	}
}

/*
 * Recursive Merge Sort
 * Sort elements in unsorted vector v
*/
static inline void
msort(float* v,
      int    f,
      int    l,
      float* temp)
{
	int m;

	if (f < l) {
		m = (f + l) / 2;
		msort(v, f, m, temp);
		msort(v, m + 1, l, temp);
		merge(v, f, m, l, temp);
	}
}

/*
 * Binary search for nearest match to sought value in
 * ordered vector v of given size
*/
static inline int
fuzzy_bsearch(float* v,
              int    size,
              float  sought)
{
	int f = 0;
	int l = size - 1;
	int m = ((l - f) / 2);

	while ((l - f) > 1) {
		if (v[m] < sought) {
			f = (l - f) / 2 + f;
		} else {
			l = (l - f) / 2 + f;
		}

		m = ((l - f) / 2 + f);
	}

	if (sought < v[m]) {
		if (m > 0) {
			if (FABSF(v[m] - sought) > FABSF(v[m - 1] - sought)) {
				m--;
			}
		}
	} else if (m < size - 1) {
		if (FABSF(v[m] - sought) > FABSF(v[m + 1] - sought)) {
			m++;
		}
	}

	return m;
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
activate(LV2_Handle instance)
{
	Quantiser* plugin = (Quantiser*)instance;

	plugin->last_found = 0.0f;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Quantiser* plugin = (Quantiser*)instance;

	switch (port) {
	case QUANTISER_RANGE_MIN:
		plugin->min = (const float*)data;
		break;
	case QUANTISER_RANGE_MAX:
		plugin->max = (const float*)data;
		break;
	case QUANTISER_MATCH_RANGE:
		plugin->match_range = (const float*)data;
		break;
	case QUANTISER_MODE:
		plugin->mode = (const float*)data;
		break;
	case QUANTISER_COUNT:
		plugin->count = (const float*)data;
		break;
	case QUANTISER_INPUT:
		plugin->input = (const float*)data;
		break;
	case QUANTISER_OUTPUT:
		plugin->output = (float*)data;
		break;
	case QUANTISER_OUTPUT_CHANGED:
		plugin->output_changed = (float*)data;
		break;
	default:
		if (port >= QUANTISER_VALUE_START && port < QUANTISER_OUTPUT) {
			plugin->values[port - QUANTISER_VALUE_START] = (float*)data;
		}
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Quantiser* plugin = (Quantiser*)malloc(sizeof(Quantiser));

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Quantiser* plugin = (Quantiser*)instance;

	/* Range Min (float value) */
	float min = *(plugin->min);

	/* Range Max (float value) */
	float max = *(plugin->max);

	/* Match Range (float value) */
	const float match_range = FABSF(*(plugin->match_range));

	/* Mode (float value) */
	const float mode = *(plugin->mode);

	/* Count (float value) */
	const float count = *(plugin->count);

	/* Input (array of float of length sample_count) */
	const float* input = plugin->input;

	/* Output (array of float of length sample_count) */
	float* output = plugin->output;

	/* Output Changed (array of float of length sample_count) */
	float* output_changed = plugin->output_changed;

	/* Instance Data */
	float* temp       = plugin->temp;
	float* values     = plugin->svalues;
	float  last_found = plugin->last_found;

	int   md          = LRINTF(mode);
	int   n           = LRINTF(count);
	int   i;
	float in;
	float out_changed;
	float range;
	float offset;
	float found = last_found;
	int   found_index = 0;

	/* Clip count if out of range */
	n = n < 1 ? 1 : (n > QUANTISER_MAX_INPUTS ? QUANTISER_MAX_INPUTS : n);

	/* Swap min and max if wrong way around */
	if (min > max) {
		range = min;
		min   = max;
		max   = range;
	}
	range = max - min;

	/* Copy and sort required values */
	for (i = 0; i < n; i++) {
		values[i + 1] = *(plugin->values[i]);
	}

	msort(values, 1, n, temp);

	/* Add wrapped extremes */
	values[0]     = values[n] - range;
	values[n + 1] = values[1] + range;

	if (md < 1) {
		/* Extend mode */
		for (uint32_t s = 0; s < sample_count; ++s) {
			in = input[s];

			if (range > 0.0f) {
				if ((in < min) || (in > max)) {
					offset  = FLOORF((in - max) / range) + 1.0f;
					offset *= range;
					in     -= offset;

					/* Quantise */
					found_index = fuzzy_bsearch(values, n + 2, in);

					/* Wrapped */
					if (found_index == 0) {
						found_index = n;
						offset     -= range;
					} else if (found_index == n + 1) {
						found_index = 1;
						offset     += range;
					}

					found = values[found_index];

					/* Allow near misses */
					if (match_range > 0.0f) {
						if (in < (found - match_range)) {
							found -= match_range;
						} else if (in > (found + match_range)) {
							found += match_range;
						}
					}
					found += offset;
				} else {
					/* Quantise */
					found_index = fuzzy_bsearch(values, n + 2, in);
					if (found_index == 0) {
						found_index = n;
						found       = values[n] - range;
					} else if (found_index == n + 1) {
						found_index = 1;
						found       = values[1] + range;
					} else {
						found = values[found_index];
					}

					if (match_range > 0.0f) {
						if (in < (found - match_range)) {
							found -= match_range;
						} else if (in > (found + match_range)) {
							found += match_range;
						}
					}
				}
			} else {
				/* Min and max the same, so only one value to quantise to */
				found = min;
			}

			/* Has quantised output changed? */
			if (FABSF(found - last_found) > 2.0001f * match_range) {
				out_changed = 1.0f;
				last_found  = found;
			} else {
				out_changed = 0.0f;
			}

			output[s]         = found;
			output_changed[s] = out_changed;
		}
	} else if (md == 1) {
		/* Wrap mode */
		for (uint32_t s = 0; s < sample_count; ++s) {
			in = input[s];

			if (range > 0.0f) {
				if ((in < min) || (in > max)) {
					in -= (FLOORF((in - max) / range) + 1.0f) * range;
				}

				/* Quantise */
				found_index = fuzzy_bsearch(values, n + 2, in);
				if (found_index == 0) {
					found_index = n;
				} else if (found_index == n + 1) {
					found_index = 1;
				}

				found = values[found_index];

				/* Allow near misses */
				if (match_range > 0.0f) {
					if (in < (found - match_range)) {
						found -= match_range;
					} else if (in > (found + match_range)) {
						found += match_range;
					}
				}
			} else {
				/* Min and max the same, so only one value to quantise to */
				found = min;
			}

			/* Has quantised output changed? */
			if (FABSF(found - last_found) > match_range) {
				out_changed = 1.0f;
				last_found  = found;
			} else {
				out_changed = 0.0f;
			}

			output[s]         = found;
			output_changed[s] = out_changed;
		}
	} else {
		/* Clip mode */
		for (uint32_t s = 0; s < sample_count; ++s) {
			in = input[s];

			if (range > 0.0f) {
				/* Clip to range */
				if (in < min) {
					found_index = 1;
				} else if (in > max) {
					found_index = n;
				} else {
					/* Quantise */
					found_index = fuzzy_bsearch(values + 1, n, in) + 1;
				}

				found = values[found_index];

				/* Allow near misses */
				if (match_range > 0.0f) {
					if (in < (found - match_range)) {
						found -= match_range;
					} else if (in > (found + match_range)) {
						found += match_range;
					}
				}
			} else {
				/* Min and max the same, so only one value to quantise to */
				found = min;
			}

			/* Has quantised output changed? */
			if (FABSF(found - last_found) > match_range) {
				out_changed = 1.0f;
				last_found  = found;
			} else {
				out_changed = 0.0f;
			}

			output[s]         = found;
			output_changed[s] = out_changed;
		}
	}
	plugin->last_found = last_found;
}

static const LV2_Descriptor descriptor = {
	QUANTISER_URI,
	instantiate,
	connect_port,
	activate,
	run,
	NULL,
	cleanup,
	NULL
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
