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

// for both ISZS and ISOZ
int ISZ_update(UPDATE_FUNC_ARGS)
{
	float rr, rrr;
	if (RNG::Ref().chance(1, 200) && RNG::Ref().chance(-4.0f * sim->air->pv[y/CELL][x/CELL], 1000))
	{
		sim->part_create(i, x, y, PT_PHOT);
		rr = RNG::Ref().between(128, 355) / 127.0f;
		rrr = RNG::Ref().between(0, 360) * M_PI / 180.0f;
		parts[i].vx = rr*cosf(rrr);
		parts[i].vy = rr*sinf(rrr);
	}
	return 0;
}

void ISZS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ISZS";
	elem->Name = "ISZS";
	elem->Colour = COLPACK(0x662089);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = -0.0007f* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 1;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 140.00f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Solid form of ISOZ, slowly decays into PHOT.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 300.0f;
	elem->HighTemperatureTransitionElement = PT_ISOZ;

	elem->Update = &ISZ_update;
	elem->Graphics = NULL;
	elem->Init = &ISZS_init_element;
}
