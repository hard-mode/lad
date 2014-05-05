/* This file is an audio plugin.
 * Copyright 2005 Loki Davison.
 *
 * Based on code by Brook Eaton and a couple of papers...
 *
 * Implements a Waveguide Mesh drum. FIXME to be extended, to have rimguides, power normalisation and all
 * manner of other goodies.
 *
 * Tension is well, drum tension
 * power is how hard you hit it.
 *
 *
 * This plugin is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This plugin is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#define _XOPEN_SOURCE 500 /* strdup */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#define MESH_BASE_ID 2670

#define MESH_NUM_PORTS 6

/* Port Numbers */
#define MESH_INPUT1   0
#define MESH_OUTPUT  1
#define MESH_TENSION 2
#define MESH_POWER 3
#define MESH_EX_X  4
#define MESH_EX_Y 5

#define LENGTH 8 // must be divisible by 4!!
#define WIDTH 8
#define SIZE LENGTH*WIDTH  // Size of mesh
#define INITIAL 0 // initial values stored at junctions
#define LOSS 0.2 // loss in wave propagation
#define INIT_DELTA 6 // initial values
#define INIT_T 0.1	// for the impedances
#define INIT_GAMMA 8 //
#define PORTS 8 //for the initialization of junction velocities from excitation

// 2D array of junctions.
// The important function of the junction is to store
// the velocities of travelling waves so that adjacent junction's
// velocities can be calculated.
typedef struct _junction
{
	float v_junction; // junction velocity
	float n_junction; // velocity heading north into junction
	float s_junction; // velocity heading south into junction
	float e_junction; // velocity heading east into junction
	float w_junction; // velocity heading west into junction
	float c_junction; // velocity heading back into the junction (delayed)
	float s_temp; // these two 'temp' values are used because calculation
	float e_temp; // of the mesh updates s/e values before they are used
} _junction;		// to calculate north and south velocities!

/* All state information for plugin */

typedef struct
{
    /* Ports */
    float* input1;
    float* output;
    float* tension;
    float* power;
    float* ex_x;
    float* ex_y;

    /* vars */
    _junction mesh[LENGTH][WIDTH];
    float last_trigger;

} WgMesh;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
    WgMesh * plugin = (WgMesh *) malloc (sizeof (WgMesh));

    return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	WgMesh * plugin = (WgMesh *) instance;
	for (int i=0; i<LENGTH; i++) {
		for (int j=0; j<WIDTH; j++) {
			plugin->mesh[i][j].v_junction = INITIAL;
			plugin->mesh[i][j].n_junction = INITIAL;
			plugin->mesh[i][j].s_junction = INITIAL;
			plugin->mesh[i][j].e_junction = INITIAL;
			plugin->mesh[i][j].w_junction = INITIAL;
			plugin->mesh[i][j].c_junction = INITIAL;
			plugin->mesh[i][j].s_temp = INITIAL;
			plugin->mesh[i][j].e_temp = INITIAL;
		}
	}
	plugin->last_trigger = 0.0;
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void*  location)
{
	WgMesh* plugin;

	plugin = (WgMesh*)instance;
	switch (port) {
	case MESH_INPUT1:
		plugin->input1 = location;
		break;
	case MESH_OUTPUT:
		plugin->output = location;
		break;
	case MESH_TENSION:
		plugin->tension = location;
		break;
	case MESH_POWER:
		plugin->power = location;
		break;
	case MESH_EX_X:
		plugin->ex_x = location;
		break;
	case MESH_EX_Y:
		plugin->ex_y = location;
		break;
	}
}

inline static void excite_mesh(WgMesh* plugin, float power, float ex_x, float ex_y)
{
	int i=ex_x,j=ex_y;
	float temp;
	float Yj;

	Yj = 2*(INIT_DELTA*INIT_DELTA/((INIT_T*INIT_T)*(INIT_GAMMA*INIT_GAMMA))); // junction admittance
	temp = power*2/(LENGTH+WIDTH);
	plugin->mesh[i][j].v_junction = plugin->mesh[i][j].v_junction +  temp;
	plugin->mesh[i][j].n_junction = plugin->mesh[i][j].n_junction + Yj*temp/PORTS;
	// All velocities leaving the junction are equal to
	plugin->mesh[i][j].s_junction = plugin->mesh[i][j].s_junction + Yj*temp/PORTS;
			// the total velocity in the junction * the admittance
			plugin->mesh[i][j].e_junction = plugin->mesh[i][j].e_junction + Yj*temp/PORTS;
			// divided by the number of outgoing ports.
			plugin->mesh[i][j].w_junction = plugin->mesh[i][j].w_junction + Yj*temp/PORTS;
			//mesh[i][j].c_junction = 0;
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	WgMesh*  plugin = (WgMesh*)instance;
	float tension = *(plugin->tension);
	float ex_x = *(plugin->ex_x);
	float ex_y = *(plugin->ex_y);
	float* input = plugin->input1;
	float* out = plugin->output;
	float* power = plugin->power;
	float last_trigger = plugin->last_trigger;
	size_t i,j,k;
	float filt, trg, oldfilt;
	float Yc,Yj,tempN,tempS,tempE,tempW;

	// Set input variables //
	oldfilt = plugin->mesh[LENGTH-LENGTH/4][WIDTH-WIDTH/4].v_junction;

	for (k=0; k<nframes; k++) {
		if (tension==0)
			tension=0.0001;
		trg = input[k];

		if (trg > 0.0f && !(last_trigger > 0.0f))
		{
		    //printf("got trigger, exciting mesh, %f \n", tension);
		    excite_mesh(plugin, power[k], ex_x, ex_y);
		}

		//junction admitance
		Yj = 2*INIT_DELTA*INIT_DELTA/(( (tension)*((tension) )*(INIT_GAMMA*INIT_GAMMA)));
		Yc = Yj-4; // junction admittance (left shift is for multiply by 2!)
		//plugin->v_power = power[k];

		for (i=1; i<LENGTH-1; i++) {
			// INNER MESH //
			for (j=1; j<WIDTH-1; j++) { // to multiply by 2 - simply shift to the left by 1!
				plugin->mesh[i][j].v_junction = 2.0*(plugin->mesh[i][j].n_junction + plugin->mesh[i][j].s_junction
					+ plugin->mesh[i][j].e_junction + plugin->mesh[i][j].w_junction + Yc*plugin->mesh[i][j].c_junction)/Yj;

				plugin->mesh[i][j+1].s_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].n_junction;
				plugin->mesh[i][j-1].n_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].s_temp;

				plugin->mesh[i+1][j].e_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].w_junction;
				plugin->mesh[i-1][j].w_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].e_temp;

				plugin->mesh[i][j].c_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].c_junction;

				plugin->mesh[i][j].s_temp = plugin->mesh[i][j].s_junction; //
				plugin->mesh[i][j].e_temp = plugin->mesh[i][j].e_junction; // update current values in the temp slots!
			}
			// BOUNDARY //
			tempS = plugin->mesh[i][0].s_junction;
			plugin->mesh[i][0].s_junction = -plugin->mesh[i][0].n_junction;
			plugin->mesh[i][1].s_junction = plugin->mesh[i][1].s_temp = tempS;
			tempN = plugin->mesh[i][WIDTH-1].n_junction;
			plugin->mesh[i][WIDTH-1].n_junction = -plugin->mesh[i][WIDTH-1].s_junction;
			plugin->mesh[i][WIDTH-2].n_junction = tempN;
			// 'i's in the neplugint few lines are really 'j's!
			tempE = plugin->mesh[0][i].e_junction;
			plugin->mesh[0][i].e_junction = -plugin->mesh[0][i].w_junction;
			plugin->mesh[1][i].e_junction = plugin->mesh[1][i].e_temp = tempE;
			tempW = plugin->mesh[WIDTH-1][i].w_junction;
			plugin->mesh[WIDTH-1][i].w_junction = -plugin->mesh[WIDTH-1][i].e_junction;
			plugin->mesh[WIDTH-2][i].w_junction = tempW;
		}

		filt = LOSS*(plugin->mesh[LENGTH-LENGTH/4][WIDTH-WIDTH/4].v_junction + oldfilt);
		oldfilt = plugin->mesh[LENGTH-LENGTH/4][WIDTH-WIDTH/4].v_junction;
		plugin->mesh[LENGTH-LENGTH/4][WIDTH-WIDTH/4].v_junction = filt;

		out[k] = plugin->mesh[WIDTH/4][WIDTH/4-1].v_junction;
	       	last_trigger = trg;
	}
	plugin->last_trigger = last_trigger;

}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/waveguide_mesh",
	instantiate,
	connect_port,
	activate,
	run,
	NULL,
	cleanup,
	NULL,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
