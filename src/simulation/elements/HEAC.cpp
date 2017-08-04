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

#include <algorithm>
#include <functional>
#include "simulation/ElementsCommon.h"

struct IsInsulator : public std::binary_function<Simulation*,int,bool> {
	bool operator() (Simulation* a, int b)
	{
		return b && (a->elements[b&0xFF].HeatConduct == 0 || ((b&0xFF) == PT_HSWC && a->parts[b>>8].life != 10));
	}
};
IsInsulator isInsulator = IsInsulator();

// If this is used elsewhere (GOLD), it should be moved into Simulation.h
template<class BinaryPredicate>
bool CheckLine(Simulation* sim, int x1, int y1, int x2, int y2, BinaryPredicate func)
{
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	int x, y, dx, dy, sy;
	float e, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	e = 0.0f;
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
		{
			if (func(sim, pmap[x][y])) return true;
		}
		else
		{
			if (func(sim, pmap[y][x])) return true;
		}
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if ((y1<y2) ? (y<=y2) : (y>=y2))
			{
				if (reverseXY)
				{
					if (func(sim, pmap[x][y])) return true;
				}
				else
				{
					if (func(sim, pmap[y][x])) return true;
				}
			}
			e -= 1.0f;
		}
	}
	return false;
}

int HEAC_update(UPDATE_FUNC_ARGS)
{
	const int rad = 4;
	int rry, rrx, r, count = 0;
	float tempAgg = 0;
	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			rry = ry * rad;
			rrx = rx * rad;
			if (x+rrx >= 0 && x+rrx < XRES && y+rry >= 0 && y+rry < YRES && !CheckLine<IsInsulator>(sim, x, y, x+rrx, y+rry, isInsulator))
			{
				r = pmap[y+rry][x+rrx];
				if (r && sim->elements[r&0xFF].HeatConduct > 0 && ((r&0xFF) != PT_HSWC || parts[r>>8].life == 10))
				{
					count++;
					tempAgg += parts[r>>8].temp;
				}
				r = photons[y+rry][x+rrx];
				if (r && sim->elements[r&0xFF].HeatConduct > 0 && ((r&0xFF) != PT_HSWC || parts[r>>8].life == 10))
				{
					count++;
					tempAgg += parts[r>>8].temp;
				}
			}
		}
	}

	if (count > 0)
	{
		parts[i].temp = tempAgg/count;

		for (int rx = -1; rx <= 1; rx++)
		{
			for (int ry = -1; ry <= 1; ry++)
			{
				rry = ry * rad;
				rrx = rx * rad;
				if (x+rrx >= 0 && x+rrx < XRES && y+rry >= 0 && y+rry < YRES && !CheckLine<IsInsulator>(sim, x, y, x+rrx, y+rry, isInsulator))
				{
					r = pmap[y+rry][x+rrx];
					if (r && sim->elements[r&0xFF].HeatConduct > 0 && ((r&0xFF) != PT_HSWC || parts[r>>8].life == 10))
					{
						parts[r>>8].temp = parts[i].temp;
					}
					r = photons[y+rry][x+rrx];
					if (r && sim->elements[r&0xFF].HeatConduct > 0 && ((r&0xFF) != PT_HSWC || parts[r>>8].life == 10))
					{
						parts[r>>8].temp = parts[i].temp;
					}
				}
			}
		}
	}

	return 0;
}

void HEAC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_HEAC";
	elem->Name = "HEAC";
	elem->Colour = PIXPACK(0xCB6351);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 1;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+273.15f;
	elem->HeatConduct = 251;
	elem->Description = "Rapid heat conductor.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	// can't melt by normal heat conduction, this is used by other elements for special melting behavior
	elem->HighTemperatureTransitionThreshold = 1887.15f;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &HEAC_update;
}
