/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "simulation/ElementsCommon.h"

int BOYL_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	float limit = parts[i].temp / 100;
	if (sim->air->pv[y / CELL][x / CELL] < limit)
		sim->air->pv[y / CELL][x / CELL] += 0.001f*(limit - sim->air->pv[y / CELL][x / CELL]);
	if (sim->air->pv[y / CELL + 1][x / CELL] < limit)
		sim->air->pv[y / CELL + 1][x / CELL] += 0.001f*(limit - sim->air->pv[y / CELL + 1][x / CELL]);
	if (sim->air->pv[y / CELL - 1][x / CELL] < limit)
		sim->air->pv[y / CELL - 1][x / CELL] += 0.001f*(limit - sim->air->pv[y / CELL - 1][x / CELL]);

	sim->air->pv[y / CELL][x / CELL + 1] += 0.001f*(limit - sim->air->pv[y / CELL][x / CELL + 1]);
	sim->air->pv[y / CELL + 1][x / CELL + 1] += 0.001f*(limit - sim->air->pv[y / CELL + 1][x / CELL + 1]);
	sim->air->pv[y / CELL][x / CELL - 1] += 0.001f*(limit - sim->air->pv[y / CELL][x / CELL - 1]);
	sim->air->pv[y / CELL - 1][x / CELL - 1] += 0.001f*(limit - sim->air->pv[y / CELL - 1][x / CELL - 1]);

	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((r&0xFF)==PT_WATR)
				{
					if (!(rand()%30))
						part_change_type(ID(r),x+rx,y+ry,PT_FOG);
				}
				else if ((r&0xFF)==PT_O2)
				{
					if (!(rand()%9))
					{
						kill_part(ID(r));
						part_change_type(i, x, y, PT_WATR);
						sim->air->pv[y/CELL][x/CELL] += 4.0;
					}
				}
			}
	return 0;
}

void BOYL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BOYL";
	elem->Name = "BOYL";
	elem->Colour = COLPACK(0x0A3200);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.18f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP+2.0f	+273.15f;
	elem->HeatConduct = 42;
	elem->Latent = 0;
	elem->Description = "Boyle, variable pressure gas. Expands when heated.";

	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &BOYL_update;
	elem->Graphics = NULL;
	elem->Init = &BOYL_init_element;
}
