/*
  Structures to represent a set of wavetables.
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

#ifndef blop_wavedata_h
#define blop_wavedata_h

#include "blop_config.h"
#include "math_func.h"
#include "interpolate.h"
#include "common.h"

/* Functions identifying wavedata dlls */
#define BLOP_DLSYM_SAWTOOTH "blop_get_sawtooth"
#define BLOP_DLSYM_SQUARE   "blop_get_square"
#define BLOP_DLSYM_PARABOLA "blop_get_parabola"

/*
 * Structure holding a single segment of sample data
 * along with information relating to playback.
 */
typedef struct {
	unsigned long sample_count;  /* Sample count */
	float*        samples_lf;    /* Sample data played back at amplitude
	                                inversely proportional to frequency */
	float* samples_hf;           /* Sample data played back at amplitude
	                                proportional to frequency */
	unsigned long harmonics;     /* Max harmonic content of sample data */

	float phase_scale_factor;    /* Phase scale factor for playback */
	float min_frequency;         /* Minimum playback frequency */
	float max_frequency;         /* Maximum playback frequency */
	float range_scale_factor;    /* Blend scale factor for cross fading */
} Wavetable;

/*
 * Structure holding the wavetable data and playback state
 */
typedef struct {
	void*         data_handle;  /* Data DLL handle */
	unsigned long table_count;  /* Number of wavetables in wavedata */
	Wavetable**   tables;       /* One or more wavetables, plus pair of
	                               extra tables for frequency extremes */
	unsigned long* lookup;      /* Wavetable lookup vector */
	unsigned long  lookup_max;  /* For clipping lookup indices */

	float sample_rate;          /* Sample rate */
	float nyquist;              /* Nyquist rate (sample_rate / 2) */

	/* Playback state */
	float      frequency;       /* Current playback frequency */
	float      abs_freq;        /* Absolute playback frequency */
	float      xfade;           /* Crossfade factor for fading */
	Wavetable* table;           /* Wavetable to playback */
} Wavedata;

#ifdef __cplusplus
extern "C" {
#endif

int wavedata_load(Wavedata*   w,
                  const char* bundle_path,
                  const char* lib_name,
                  const char* wdat_descriptor_name,
                  double      sample_rate);

void wavedata_unload(Wavedata* w);

/*****************************************************************************
 * Description: Get interpolated sample from current wavetable in wavedata
 *               at given phase offset
 *
 *   Arguments: w               Wavedata containing playback state and data
 *              phase           Phase offset [0.0, sample_rate]
 *
 *     Returns: Interpolated sample
 *
 *       Notes: Cubic (or quintic) interpolation requires four consecutive
 *               samples for operation:
 *
 *                              phase
 *                                :
 *                    p1      p0  :   n0      n1
 *                    |       |   x   |       |
 *                            :   :
 *                            <-o->
 *                              :
 *                           interval
 *
 *              Phase values less than one make p0 the first sample in
 *               the table - p1 will be the last sample, as a previous
 *               sample does not exist. To avoid checking for this case,
 *               a copy of the last sample is stored before the first
 *               sample in each table.
 *              Likewise, if the phase means p0 is the last sample, n0
 *               and n1 will be the first and second samples respectively.
 *               Copies of these samples are stored after the last sample
 *               in each table.
 *
 *****************************************************************************/
static inline float
wavedata_get_sample(Wavedata* w,
                    float     phase)
{
	float*   samples_hf = w->table->samples_hf;
	float*   samples_lf = w->table->samples_lf;
	float    p1, p0, n0, n1;
	float    phase_f;
	long int index;

	/* Scale phase to map to position in wavetable */
	phase *= w->table->phase_scale_factor;

	/* Get position of first contributing sample (p1) */
	index   = LRINTF((float)phase - 0.5f);
	phase_f = (float)index;

	index %= w->table->sample_count;

	/* Cross-fade table pairs */
	/* Previous two samples */
	p1 = w->xfade * (samples_lf[index] - samples_hf[index]) + samples_hf[index];
	index++;
	p0 = w->xfade * (samples_lf[index] - samples_hf[index]) + samples_hf[index];
	index++;
	/* Next two samples */
	n0 = w->xfade * (samples_lf[index] - samples_hf[index]) + samples_hf[index];
	index++;
	n1 = w->xfade * (samples_lf[index] - samples_hf[index]) + samples_hf[index];

	/* Return interpolated sample */
	return interpolate_cubic(phase - phase_f, p1, p0, n0, n1);
}

/*****************************************************************************
 * Description: Get wavetable to use for playback frequency.
 *
 *   Arguments: w               Wavedata object (contains all table info)
 *              frequency       Playback frequency
 *
 *       Notes: The lookup vector used to determine the wavetable
 *               is indexed by harmonic number.
 *
 *              The maximum playback frequency for a wavetable is
 *               determined by its harmonic content and the sample rate,
 *               and equals sample_rate / 2 / max_harmonic_in_table.
 *
 *****************************************************************************/
static inline void
wavedata_get_table(Wavedata* w,
                   float     frequency)
{
	unsigned long harmonic;

	w->frequency = frequency;
	w->abs_freq  = (float)FABSF((float)frequency);

	/* Get highest harmonic possible in frequency */
	harmonic = LRINTF(w->nyquist / w->abs_freq - 0.5f);

	/* Clip so lookup is within bounds */
	if (harmonic > w->lookup_max) {
		harmonic = w->lookup_max;
	}

	/* Set playback table */
	w->table = w->tables[w->lookup[harmonic]];

	/* Get cross fade factor */
	w->xfade = f_max(w->table->max_frequency - w->abs_freq, 0.0f) * w->table->range_scale_factor;
	w->xfade = f_min(w->xfade, 1.0f);
}

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* blop_wavedata_h */
