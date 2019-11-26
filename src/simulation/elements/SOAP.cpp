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

void attach(int i1, int i2)
{
	if (!(parts[i2].ctype&4))
	{
		parts[i1].ctype |= 2;
		parts[i1].tmp = i2;

		parts[i2].ctype |= 4;
		parts[i2].tmp2 = i1;
	}
	else if (!(parts[i2].ctype&2))
	{
		parts[i1].ctype |= 4;
		parts[i1].tmp2= i2;

		parts[i2].ctype |= 2;
		parts[i2].tmp = i1;
	}
}

void detach(int i)
{
	if ((parts[i].ctype&2) == 2 && parts[i].tmp >= 0 && parts[i].tmp < NPART && parts[parts[i].tmp].type == PT_SOAP)
	{
		if ((parts[parts[i].tmp].ctype&4) == 4)
			parts[parts[i].tmp].ctype ^= 4;
	}

	if ((parts[i].ctype&4) == 4 && parts[i].tmp2 >= 0 && parts[i].tmp2 < NPART && parts[parts[i].tmp2].type == PT_SOAP)
	{
		if ((parts[parts[i].tmp2].ctype&2) == 2)
			parts[parts[i].tmp2].ctype ^= 2;
	}

	parts[i].ctype = 0;
}

#define SOAP_FREEZING 248.15f
#define BLEND 0.85f

int SOAP_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, nr, ng, nb, na;
	float tr, tg, tb, ta;
	
	//0x01 - bubble on/off
	//0x02 - first mate yes/no
	//0x04 - "back" mate yes/no

	if (parts[i].ctype&1)
	{
		if (parts[i].temp>SOAP_FREEZING)
		{
			if (parts[i].tmp < 0 || parts[i].tmp >= NPART || parts[i].tmp2 < 0 || parts[i].tmp2 >= NPART)
			{
				parts[i].tmp = parts[i].tmp2 = parts[i].ctype = 0;
				return 0;
			}
			if (parts[i].life<=0)
			{
				//if only connected on one side
				if ((parts[i].ctype&6) != 6 && (parts[i].ctype&6))
				{
					int target = i;
					//break entire bubble in a loop
					while((parts[target].ctype&6) != 6 && (parts[target].ctype&6) && parts[target].type == PT_SOAP)
					{
						if (parts[target].ctype&2)
						{
							target = parts[target].tmp;
							detach(target);
						}
						if (parts[target].ctype&4)
						{
							target = parts[target].tmp2;
							detach(target);
						}
					}
				}
				if ((parts[i].ctype&6) != 6)
					parts[i].ctype = 0;
				if ((parts[i].ctype&6) == 6 && (parts[parts[i].tmp].ctype&6) == 6 && parts[parts[i].tmp].tmp == i)
					detach(i);
			}
			parts[i].vy = (parts[i].vy-0.1f)*0.5f;
			parts[i].vx *= 0.5f;
		}

		if (!(parts[i].ctype&2))
		{
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (!r)
							continue;

						if ((parts[ID(r)].type == PT_SOAP) && (parts[ID(r)].ctype&1) && !(parts[ID(r)].ctype&4))
							attach(i, ID(r));
					}
		}
		else
		{
			if (parts[i].life<=0)
				for (rx=-2; rx<3; rx++)
					for (ry=-2; ry<3; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if (!r && !bmap[(y+ry)/CELL][(x+rx)/CELL])
								continue;

							if (parts[i].temp>SOAP_FREEZING)
							{
								if (bmap[(y+ry)/CELL][(x+rx)/CELL] ||
								    (r && !(sim->elements[TYP(r)].Properties&TYPE_GAS) && TYP(r) != PT_SOAP && TYP(r) != PT_GLAS))
								{
									detach(i);
									continue;
								}
							}

							if (TYP(r) == PT_SOAP)
							{
								if (parts[ID(r)].ctype == 1)
								{
									int buf = parts[i].tmp;

									parts[i].tmp = ID(r);
									if (parts[buf].type == PT_SOAP)
										parts[buf].tmp2 = ID(r);
									parts[ID(r)].tmp2 = i;
									parts[ID(r)].tmp = buf;
									parts[ID(r)].ctype = 7;
								}
								else if (parts[ID(r)].ctype == 7 && parts[i].tmp != ID(r) && parts[i].tmp2 != ID(r))
								{
									if (parts[parts[i].tmp].type == PT_SOAP)
										parts[parts[i].tmp].tmp2 = parts[ID(r)].tmp2;
									if (parts[parts[ID(r)].tmp2].type == PT_SOAP)
										parts[parts[ID(r)].tmp2].tmp = parts[i].tmp;
									parts[ID(r)].tmp2 = i;
									parts[i].tmp = ID(r);
								}
							}
						}
		}

		if(parts[i].ctype&2)
		{
			float dx = parts[i].x - parts[parts[i].tmp].x;
			float dy = parts[i].y - parts[parts[i].tmp].y;
			float d = 9/(pow(dx, 2)+pow(dy, 2)+9)-0.5f;

			parts[parts[i].tmp].vx -= dx*d;
			parts[parts[i].tmp].vy -= dy*d;
			parts[i].vx += dx*d;
			parts[i].vy += dy*d;

			if ((parts[parts[i].tmp].ctype&2) && (parts[parts[i].tmp].ctype&1) 
				&& (parts[parts[i].tmp].tmp >= 0 && parts[parts[i].tmp].tmp < NPART) 
				&& (parts[parts[parts[i].tmp].tmp].ctype&2) && (parts[parts[parts[i].tmp].tmp].ctype&1))
			{
				int ii = parts[parts[parts[i].tmp].tmp].tmp;
				if (ii >= 0 && ii < NPART)
				{
					dx = parts[ii].x - parts[parts[i].tmp].x;
					dy = parts[ii].y - parts[parts[i].tmp].y;
					d = 81/(pow(dx, 2)+pow(dy, 2)+81)-0.5f;

					parts[parts[i].tmp].vx -= dx*d*0.5f;
					parts[parts[i].tmp].vy -= dy*d*0.5f;
					parts[ii].vx += dx*d*0.5f;
					parts[ii].vy += dy*d*0.5f;
				}
			}
		}
	}
	else
	{
		if (sim->air->pv[y/CELL][x/CELL]>0.5f || sim->air->pv[y/CELL][x/CELL]<(-0.5f))
		{
			parts[i].ctype = 1;
			parts[i].life = 10;
		}
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;

					if (TYP(r) == PT_OIL)
					{
						parts[i].vx = parts[ID(r)].vx = (parts[i].vx*0.5f + parts[ID(r)].vx)/2;
						parts[i].vy = parts[ID(r)].vy = ((parts[i].vy-0.1f)*0.5f + parts[ID(r)].vy)/2;
					}
				}
	}
	
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)!=PT_SOAP)
				{
					ta = (float)COLA(parts[ID(r)].dcolour);
					tr = (float)COLR(parts[ID(r)].dcolour);
					tg = (float)COLG(parts[ID(r)].dcolour);
					tb = (float)COLB(parts[ID(r)].dcolour);
					
					na = (int)(ta*BLEND);
					nr = (int)(tr*BLEND);
					ng = (int)(tg*BLEND);
					nb = (int)(tb*BLEND);
					
					parts[ID(r)].dcolour = COLARGB(na, nr, ng, nb);
				}
			}

	return 0;
}

void SOAP_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (from==PT_SOAP && to!=PT_SOAP)
	{
		detach(i);
	}
}

void SOAP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SOAP";
	elem->Name = "SOAP";
	elem->Colour = COLPACK(0xF5F5DC);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 35;

	elem->DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "Soap. Creates bubbles, washes off deco color, and cures virus.";

	elem->Properties = TYPE_LIQUID|PROP_NEUTPENETRATE|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp = -1;
	elem->DefaultProperties.tmp2 = -1;

	elem->Update = &SOAP_update;
	elem->Graphics = NULL;
	elem->Func_ChangeType = &SOAP_ChangeType;
	elem->Init = &SOAP_init_element;
}
