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

#include <element.h>

void transferProp(UPDATE_FUNC_ARGS, int propOffset)
{
	int r, rx, ry, trade, transfer;
	for (trade = 0; trade < 9; trade++)
	{
		int random = rand();
		rx = random%7-3;
		ry = (random>>3)%7-3;
		if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
		{
			r = pmap[y+ry][x+rx];
			if ((r&0xFF) != parts[i].type)
				continue;
			if (*((int*)(((char*)&parts[i])+propOffset)) > *((int*)(((char*)&parts[r>>8])+propOffset)))
			{
				transfer = *((int*)(((char*)&parts[i])+propOffset)) - *((int*)(((char*)&parts[r>>8])+propOffset));
				if (transfer == 1)
				{
					*((int*)(((char*)&parts[r>>8])+propOffset)) += 1;
					*((int*)(((char*)&parts[i])+propOffset)) -= 1;
					trade = 9;
				}
				else if (transfer > 0)
				{
					*((int*)(((char*)&parts[r>>8])+propOffset)) += transfer/2;
					*((int*)(((char*)&parts[i])+propOffset)) -= transfer/2;
					trade = 9;
				}
			}
		}
	}	
}

int update_VIBR(UPDATE_FUNC_ARGS) {
	int r, rx, ry;
	if (parts[i].ctype == 1)
	{
		if (pv[y/CELL][x/CELL] > -2.5 || parts[i].tmp) //newly created BVBR turns to VIBR
		{
			parts[i].ctype = 0;
			part_change_type(i, x, y, PT_VIBR);
		}
	}
	else if (!parts[i].life)
	{
		//Heat absorption code
		if (parts[i].temp > 274.65f)
		{
			parts[i].tmp++;
			parts[i].temp -= 3;
		}
		if (parts[i].temp < 271.65f)
		{
			parts[i].tmp--;
			parts[i].temp += 3;
		}
		//Pressure absorption code
		if (pv[y/CELL][x/CELL] > 2.5)
		{
			parts[i].tmp += 10;
			pv[y/CELL][x/CELL]--;
		}
		if (pv[y/CELL][x/CELL] < -2.5)
		{
			parts[i].tmp -= 2;
			pv[y/CELL][x/CELL]++;
		}
	}
	//Release sparks before explode
	if (parts[i].life && parts[i].life < 300)
	{
		rx = rand()%3-1;
		ry = rand()%3-1;
		r = pmap[y+ry][x+rx];
		if ((r&0xFF) && (r&0xFF) != PT_BREL && (ptypes[r&0xFF].properties&PROP_CONDUCTS) && !parts[r>>8].life)
		{
			parts[r>>8].life = 4;
			parts[r>>8].ctype = r>>8;
			part_change_type(r>>8,x+rx,y+ry,PT_SPRK);
		}
	}
	//initiate explosion counter
	if (!parts[i].life && parts[i].tmp > 1000)
		parts[i].life = 750;
	//Release all heat
	if (parts[i].life && parts[i].life < 500)
	{
		int random = rand();
		rx = random%7-3;
		ry = (random>>3)%7-3;
		if(x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES)
		{
			r = pmap[y+ry][x+rx];
			if ((r&0xFF) && (r&0xFF) != parts[i].type)
			{
				parts[r>>8].temp += parts[i].tmp*6;
				parts[i].tmp -= parts[i].tmp*2;
			}
		}
	}
	//Explosion code
	if (parts[i].life == 1)
	{
		int random = rand(), index;
		create_part(i, x, y, PT_EXOT);
		parts[i].tmp2 = rand()%1000;
		index = create_part(-3,x+((random>>4)&3)-1,y+((random>>6)&3)-1,PT_ELEC);
		if (index != -1)
			parts[index].temp = 7000;
		index = create_part(-3,x+((random>>8)&3)-1,y+((random>>10)&3)-1,PT_PHOT);
		if (index != -1)
			parts[index].temp = 7000;
		index = create_part(-1,x+((random>>12)&3)-1,y+rand()%3-1,PT_BREL);
		if (index != -1)
			parts[index].temp = 7000;
		parts[i].temp=9000;
		pv[y/CELL][x/CELL]=200;
	}
	//Neighbor check loop
	for (rx=-3; rx<4; rx++)
		for (ry=-3; ry<4; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r || (r & (abs(rx) == 3 || abs(ry) == 3)) )
					r = photons[y+ry][x+rx];
				if (!r)
					continue;
				//Melts into EXOT
				if ((r&0xFF) == PT_EXOT && !(rand()%250))
				{
					create_part(i, x, y, PT_EXOT);
				}
				else if ((r&0xFF) == PT_BOYL)
				{
					part_change_type(i,x,y,PT_BVBR);
				}
				else if (parts[i].life && (r&0xFF) == parts[i].type && !parts[r>>8].life)
				{
					parts[r>>8].tmp += 10;
				}
				//Absorbs energy particles
				if ((ptypes[r&0xFF].properties & TYPE_ENERGY))
				{
					parts[i].tmp += 10;
					kill_part(r>>8);
				}
			}
	transferProp(UPDATE_FUNC_SUBCALL_ARGS, offsetof(particle, tmp));
	return 0;
}

int graphics_VIBR(GRAPHICS_FUNC_ARGS)
{
	int gradient = cpart->tmp/10;
	if (gradient >= 100 || cpart->life)
	{
		*colr = (int)(fabs(sin(exp((750.0f-cpart->life)/170)))*200.0f);
		*colg = 255;
		*colb = (int)(fabs(sin(exp((750.0f-cpart->life)/170)))*200.0f);
		*firea = 90;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode = PMODE_NONE;
		*pixel_mode |= FIRE_BLEND;
	}
	else if (gradient < 100)
	{
		*colr += (int)restrict_flt(gradient*2.0f,0,255);
		*colg += (int)restrict_flt(gradient*2.0f,0,175);
		*colb += (int)restrict_flt(gradient*2.0f,0,255);
		*firea = (int)restrict_flt(gradient*.6f,0,60);
		*firer = *colr/2;
		*fireg = *colg/2;
		*fireb = *colb/2;
		*pixel_mode |= FIRE_BLEND;
	}
	return 0;
}
