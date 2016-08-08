/**
 * Powder Toy - miscellaneous functions
 *
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

#include "common/tpt-minmax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sstream>
#include <math.h>
#include <SDLCompat.h>

#ifdef ANDROID
#include <SDL/SDL_screenkeyboard.h>
#endif

#include "misc.h"
#include "defines.h"
#include "interface.h"
#include "graphics.h"
#include "powdergraphics.h"
#include "powder.h"
#include "gravity.h"
#include "hud.h"
#include "cJSON.h"
#include "update.h"

#include "common/Platform.h"
#include "game/Favorite.h"
#include "game/Menus.h"
#include "simulation/Simulation.h"
#include "simulation/Tool.h"

#ifdef MACOSX
extern "C"
{
char * readClipboard();
void writeClipboard(const char * clipboardData);
}
#endif

char *clipboard_text = NULL;
static char hex[] = "0123456789ABCDEF";

//Signum function
int isign(float i)
{
	if (i<0)
		return -1;
	if (i>0)
		return 1;
	return 0;
}

unsigned clamp_flt(float f, float min, float max)
{
	if (f<min)
		return 0;
	if (f>max)
		return 255;
	return (int)(255.0f*(f-min)/(max-min));
}

float restrict_flt(float f, float min, float max)
{
	if (f<min)
		return min;
	if (f>max)
		return max;
	return f;
}

char *mystrdup(const char *s)
{
	char *x;
	if (s)
	{
		x = (char*)malloc(strlen(s)+1);
		strcpy(x, s);
		return x;
	}
	return NULL;
}

void strlist_add(struct strlist **list, char *str)
{
	struct strlist *item = (struct strlist *)malloc(sizeof(struct strlist));
	item->str = mystrdup(str);
	item->next = *list;
	*list = item;
}

int strlist_find(struct strlist **list, char *str)
{
	struct strlist *item;
	for (item=*list; item; item=item->next)
		if (!strcmp(item->str, str))
			return 1;
	return 0;
}

void strlist_free(struct strlist **list)
{
	struct strlist *item;
	while (*list)
	{
		item = *list;
		*list = (*list)->next;
		free(item);
	}
}

void clean_text(char *text, int vwidth)
{
	if (vwidth >= 0 && textwidth(text) > vwidth)
		text[textwidthx(text, vwidth)] = 0;	

	for (unsigned i = 0; i < strlen(text); i++)
		if (text[i] < ' ' || text[i] >= 127)
			text[i] = ' ';
}

void save_console_history(cJSON **historyArray, command_history *commandList)
{
	if (!commandList)
		return;
	save_console_history(historyArray, commandList->prev_command);
	cJSON_AddItemToArray(*historyArray, cJSON_CreateString(commandList->command.str));
}

void cJSON_AddString(cJSON** obj, const char *name, int number)
{
	std::stringstream str;
	str << number;
	cJSON_AddStringToObject(*obj, name, str.str().c_str());
}

bool doingUpdate = false;
void save_presets()
{
	char * outputdata;
	cJSON *root, *userobj, *versionobj, *recobj, *graphicsobj, *hudobj, *simulationobj, *consoleobj, *tmpobj;
	FILE *f = fopen("powder.pref", "wb");
	if(!f)
		return;
	root = cJSON_CreateObject();
	
	cJSON_AddStringToObject(root, "Powder Toy Preferences", "Don't modify this file unless you know what you're doing. P.S: editing the admin/mod fields in your user info doesn't give you magical powers");
	
	//Tpt++ User Info
	if (svf_login)
	{
		cJSON_AddItemToObject(root, "User", userobj=cJSON_CreateObject());
		cJSON_AddStringToObject(userobj, "Username", svf_user);
		cJSON_AddNumberToObject(userobj, "ID", atoi(svf_user_id));
		cJSON_AddStringToObject(userobj, "SessionID", svf_session_id);
		cJSON_AddStringToObject(userobj, "SessionKey", svf_session_key);
		if (svf_admin)
		{
			cJSON_AddStringToObject(userobj, "Elevation", "Admin");
		}
		else if (svf_mod)
		{
			cJSON_AddStringToObject(userobj, "Elevation", "Mod");
		}
		else {
			cJSON_AddStringToObject(userobj, "Elevation", "None");
		}
	}

	//Tpt++ Renderer settings
	cJSON_AddItemToObject(root, "Renderer", graphicsobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(graphicsobj, "ColourMode", colour_mode);
	if (DEBUG_MODE)
		cJSON_AddTrueToObject(graphicsobj, "DebugMode");
	else
		cJSON_AddFalseToObject(graphicsobj, "DebugMode");
	tmpobj = cJSON_CreateIntArray(NULL, 0);
	int i = 0;
	while (display_modes[i])
	{
		cJSON_AddItemToArray(tmpobj, cJSON_CreateNumber(display_modes[i]));
		i++;
	}
	cJSON_AddItemToObject(graphicsobj, "DisplayModes", tmpobj);
	tmpobj = cJSON_CreateIntArray(NULL, 0);
	i = 0;
	while (render_modes[i])
	{
		cJSON_AddItemToArray(tmpobj, cJSON_CreateNumber(render_modes[i]));
		i++;
	}
	cJSON_AddItemToObject(graphicsobj, "RenderModes", tmpobj);
	if (drawgrav_enable)
		cJSON_AddTrueToObject(graphicsobj, "GravityField");
	else
		cJSON_AddFalseToObject(graphicsobj, "GravityField");
	if (decorations_enable)
		cJSON_AddTrueToObject(graphicsobj, "Decorations");
	else
		cJSON_AddFalseToObject(graphicsobj, "Decorations");
	
	//Tpt++ Simulation setting(s)
	cJSON_AddItemToObject(root, "Simulation", simulationobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(simulationobj, "EdgeMode", globalSim->edgeMode);
	cJSON_AddNumberToObject(simulationobj, "NewtonianGravity", ngrav_enable);
	cJSON_AddNumberToObject(simulationobj, "AmbientHeat", aheat_enable);
	cJSON_AddNumberToObject(simulationobj, "PrettyPowder", pretty_powder);

	//Tpt++ install check, prevents annoyingness
	cJSON_AddTrueToObject(root, "InstallCheck");

	//Tpt++ console history
	cJSON_AddItemToObject(root, "Console", consoleobj=cJSON_CreateObject());
	tmpobj = cJSON_CreateStringArray(NULL, 0);
	save_console_history(&tmpobj, last_command);
	cJSON_AddItemToObject(consoleobj, "History", tmpobj);
	tmpobj = cJSON_CreateStringArray(NULL, 0);
	save_console_history(&tmpobj, last_command_result);
	cJSON_AddItemToObject(consoleobj, "HistoryResults", tmpobj);

	//Version Info
	cJSON_AddItemToObject(root, "version", versionobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(versionobj, "major", SAVE_VERSION);
	cJSON_AddNumberToObject(versionobj, "minor", MINOR_VERSION);
	cJSON_AddNumberToObject(versionobj, "build", BUILD_NUM);
#ifndef NOMOD
	cJSON_AddNumberToObject(versionobj, "modmajor", MOD_VERSION);
	cJSON_AddNumberToObject(versionobj, "modminor", MOD_MINOR_VERSION);
	cJSON_AddNumberToObject(versionobj, "modbuild", MOD_BUILD_VERSION);
#endif
#ifdef ANDROID
	cJSON_AddNumberToObject(versionobj, "modmajor", MOBILE_MAJOR);
	cJSON_AddNumberToObject(versionobj, "modminor", MOBILE_MINOR);
	cJSON_AddNumberToObject(versionobj, "modbuild", MOBILE_BUILD);
#endif
	if (doingUpdate)
		cJSON_AddTrueToObject(versionobj, "update");
	else
		cJSON_AddFalseToObject(versionobj, "update");
	if (doUpdates)
		cJSON_AddTrueToObject(versionobj, "updateChecks");
	else
		cJSON_AddFalseToObject(versionobj, "updateChecks");

	cJSON * favArr = cJSON_CreateArray();
	std::vector<std::string> favorites = Favorite::Ref().BuildFavoritesList(true);
	for (std::vector<std::string>::iterator iter = favorites.begin(); iter != favorites.end(); ++iter)
		cJSON_AddItemToArray(favArr, cJSON_CreateString((*iter).c_str()));
	cJSON_AddItemToObject(root, "Favorites", favArr);

	//Fav Menu/Records
	cJSON_AddItemToObject(root, "records", recobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(recobj, "Total Time Played", ((double)currentTime/1000)+((double)totaltime/1000)-((double)totalafktime/1000)-((double)afktime/1000));
	cJSON_AddNumberToObject(recobj, "Average FPS", totalfps/frames);
	cJSON_AddNumberToObject(recobj, "Number of frames", frames);
	cJSON_AddNumberToObject(recobj, "Total AFK Time", ((double)totalafktime/1000)+((double)afktime/1000)+((double)prevafktime/1000));
	cJSON_AddNumberToObject(recobj, "Times Played", timesplayed);

	//HUDs
	cJSON_AddItemToObject(root, "HUD", hudobj=cJSON_CreateObject());
	cJSON_AddItemToObject(hudobj, "normal", cJSON_CreateIntArray(normalHud, HUD_OPTIONS));
	cJSON_AddItemToObject(hudobj, "debug", cJSON_CreateIntArray(debugHud, HUD_OPTIONS));

	//General settings
	cJSON_AddStringToObject(root, "Proxy", http_proxy_string);
	cJSON_AddNumberToObject(root, "Scale", sdl_scale);
	if (kiosk_enable)
		cJSON_AddTrueToObject(root, "FullScreen");
	if (fastquit)
		cJSON_AddTrueToObject(root, "FastQuit");
	else
		cJSON_AddFalseToObject(root, "FastQuit");
	cJSON_AddNumberToObject(root, "WindowX", savedWindowX);
	cJSON_AddNumberToObject(root, "WindowY", savedWindowY);

	//additional settings from my mod
	cJSON_AddNumberToObject(root, "heatmode", heatmode);
	cJSON_AddNumberToObject(root, "autosave", autosave);
	cJSON_AddNumberToObject(root, "realistic", realistic);
	if (unlockedstuff & 0x01)
		cJSON_AddNumberToObject(root, "cracker_unlocked", 1);
	if (unlockedstuff & 0x08)
		cJSON_AddNumberToObject(root, "show_votes", 1);
	if (unlockedstuff & 0x10)
		cJSON_AddNumberToObject(root, "EXPL_unlocked", 1);
	if (old_menu)
		cJSON_AddNumberToObject(root, "old_menu", 1);
	if (finding & 0x8)
		cJSON_AddNumberToObject(root, "alt_find", 1);
	cJSON_AddNumberToObject(root, "dateformat", dateformat);
	cJSON_AddNumberToObject(root, "decobox_hidden", decobox_hidden);

	cJSON_AddItemToObject(root, "SavePreview", tmpobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(tmpobj, "scrollSpeed", scrollSpeed);
	cJSON_AddNumberToObject(tmpobj, "scrollDeceleration", scrollDeceleration);
	cJSON_AddNumberToObject(tmpobj, "ShowIDs", show_ids);
	
	outputdata = cJSON_Print(root);
	cJSON_Delete(root);

	fwrite(outputdata, 1, strlen(outputdata), f);
	fclose(f);
	free(outputdata);
}

void load_console_history(cJSON *tmpobj, command_history **last_command, int count)
{
	command_history *currentcommand = *last_command;
	for (int i = 0; i < count; i++)
	{
		currentcommand = (command_history*)malloc(sizeof(command_history));
		memset(currentcommand, 0, sizeof(command_history));
		currentcommand->prev_command = *last_command;
		ui_label_init(&currentcommand->command, 15, 0, 0, 0);
		if (tmpobj)
			strncpy(currentcommand->command.str, cJSON_GetArrayItem(tmpobj, i)->valuestring, 1023);
		else
			strcpy(currentcommand->command.str, "");
		*last_command = currentcommand;
	}
}

int cJSON_GetInt(cJSON **tmpobj)
{
	const char* ret = (*tmpobj)->valuestring;
	if (ret)
		return atoi(ret);
	else
		return 0;
}

void load_presets(void)
{
	int prefdatasize = 0;
	char * prefdata = (char*)file_load("powder.pref", &prefdatasize);
	cJSON *root;
	if (prefdata && (root = cJSON_Parse(prefdata)))
	{
		cJSON *userobj, *versionobj, *recobj, *tmpobj, *graphicsobj, *hudobj, *simulationobj, *consoleobj, *itemobj;
		
		//Read user data
		userobj = cJSON_GetObjectItem(root, "User");
		if (userobj && (tmpobj = cJSON_GetObjectItem(userobj, "SessionKey")))
		{
			svf_login = 1;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "Username")) && tmpobj->type == cJSON_String)
				strncpy(svf_user, tmpobj->valuestring, 63);
			else
				svf_user[0] = 0;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "ID")) && tmpobj->type == cJSON_Number)
				sprintf(svf_user_id, "%i", tmpobj->valueint, 63);
			else
				svf_user_id[0] = 0;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "SessionID")) && tmpobj->type == cJSON_String)
				strncpy(svf_session_id, tmpobj->valuestring, 63);
			else
				svf_session_id[0] = 0;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "SessionKey")) && tmpobj->type == cJSON_String)
				strncpy(svf_session_key, tmpobj->valuestring, 63);
			else
				svf_session_key[0] = 0;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "Elevation")) && tmpobj->type == cJSON_String)
			{
				if (!strcmp(tmpobj->valuestring, "Admin"))
				{
					svf_admin = 1;
					svf_mod = 0;
				}
				else if (!strcmp(tmpobj->valuestring, "Mod"))
				{
					svf_mod = 1;
					svf_admin = 0;
				}
				else
				{
					svf_admin = 0;
					svf_mod = 0;
				}
			}
		}
		else 
		{
			svf_login = 0;
			svf_user[0] = 0;
			svf_user_id[0] = 0;
			svf_session_id[0] = 0;
			svf_admin = 0;
			svf_mod = 0;
		}
		
		//Read version data
		versionobj = cJSON_GetObjectItem(root, "version");
		if (versionobj)
		{
			if (tmpobj = cJSON_GetObjectItem(versionobj, "major"))
				last_major = (unsigned short)tmpobj->valueint;
			if (tmpobj = cJSON_GetObjectItem(versionobj, "minor"))
				last_minor = (unsigned short)tmpobj->valueint;
			if (tmpobj = cJSON_GetObjectItem(versionobj, "build"))
				last_build = (unsigned short)tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(versionobj, "update")) && tmpobj->type == cJSON_True)
				update_flag = 1;
			else
				update_flag = 0;
			tmpobj = cJSON_GetObjectItem(versionobj, "updateChecks");
			if (tmpobj)
			{
				if (tmpobj->type == cJSON_True)
					doUpdates = true;
				else
					doUpdates = false;
			}
		}
		else
		{
			last_major = 0;
			last_minor = 0;
			last_build = 0;
			update_flag = 0;
		}
		
		//Read FavMenu
		tmpobj = cJSON_GetObjectItem(root, "Favorites");
		if (tmpobj)
		{
			int favSize = cJSON_GetArraySize(tmpobj);
			for (int i = 0; i < favSize; i++)
			{
				std::string identifier = cJSON_GetArrayItem(tmpobj, i)->valuestring;
				Favorite::Ref().AddFavorite(identifier);
			}
			if (favSize)
				FillMenus();
		}
		
		//Read Records
		recobj = cJSON_GetObjectItem(root, "records");
		if (recobj)
		{
			if (tmpobj = cJSON_GetObjectItem(recobj, "Total Time Played"))
				totaltime = (int)((tmpobj->valuedouble)*1000);
			if (tmpobj = cJSON_GetObjectItem(recobj, "Average FPS"))
				totalfps = tmpobj->valuedouble;
			if (tmpobj = cJSON_GetObjectItem(recobj, "Number of frames"))
				frames = tmpobj->valueint; totalfps = totalfps * frames;
			if (tmpobj = cJSON_GetObjectItem(recobj, "Total AFK Time"))
				prevafktime = (int)((tmpobj->valuedouble)*1000);
			if (tmpobj = cJSON_GetObjectItem(recobj, "Times Played"))
				timesplayed = tmpobj->valueint;
		}

		//Read display settings
		graphicsobj = cJSON_GetObjectItem(root, "Renderer");
		if(graphicsobj)
		{
			if (tmpobj = cJSON_GetObjectItem(graphicsobj, "ColourMode"))
				colour_mode = tmpobj->valueint;
			if (tmpobj = cJSON_GetObjectItem(graphicsobj, "DisplayModes"))
			{
				int count = cJSON_GetArraySize(tmpobj);
				cJSON * tempRenderMode;
				free(display_modes);
				display_mode = 0;
				display_modes = (unsigned int*)calloc(count+1, sizeof(unsigned int));
				for (int i = 0; i < count; i++)
				{
					tempRenderMode = cJSON_GetArrayItem(tmpobj, i);
					unsigned int mode = tempRenderMode->valueint;
					if (mode == 0)
					{
						char * strMode = tempRenderMode->valuestring;
						if (strlen(strMode))
							mode = atoi(strMode);
					}
					display_mode |= mode;
					display_modes[i] = mode;
				}
			}
			if (tmpobj = cJSON_GetObjectItem(graphicsobj, "RenderModes"))
			{
				int count = cJSON_GetArraySize(tmpobj);
				cJSON * tempRenderMode;
				free(render_modes);
				render_mode = 0;
				render_modes = (unsigned int*)calloc(count+1, sizeof(unsigned int));
				for (int i = 0; i < count; i++)
				{
					tempRenderMode = cJSON_GetArrayItem(tmpobj, i);
					unsigned int mode = tempRenderMode->valueint;
					if (mode == 0)
					{
						char * strMode = tempRenderMode->valuestring;
						if (strlen(strMode))
							mode = atoi(strMode);
					}
					// temporary hack until I update the json library (first for loading current modes, second only needed for loading with valuestring)
					if (mode == 2147483648 || mode == 2147483647)
						mode = 4278252144;
					render_mode |= mode;
					render_modes[i] = mode;
				}
			}
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "Decorations")) && tmpobj->type == cJSON_True)
				decorations_enable = true;
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "GravityField")) && tmpobj->type == cJSON_True)
				drawgrav_enable = tmpobj->valueint;
			if((tmpobj = cJSON_GetObjectItem(graphicsobj, "DebugMode")) && tmpobj->type == cJSON_True)
				DEBUG_MODE = tmpobj->valueint;
		}

		//Read simulation settings
		simulationobj = cJSON_GetObjectItem(root, "Simulation");
		if (simulationobj)
		{
			if(tmpobj = cJSON_GetObjectItem(simulationobj, "EdgeMode"))
			{
				char edgeMode = (char)tmpobj->valueint;
				if (edgeMode > 3)
					edgeMode = 0;
				globalSim->edgeMode = edgeMode;
			}
			if((tmpobj = cJSON_GetObjectItem(simulationobj, "NewtonianGravity")) && tmpobj->valuestring && !strcmp(tmpobj->valuestring, "1"))
				start_grav_async();
			if(tmpobj = cJSON_GetObjectItem(simulationobj, "AmbientHeat"))
				aheat_enable = tmpobj->valueint;
			if(tmpobj = cJSON_GetObjectItem(simulationobj, "PrettyPowder"))
				pretty_powder = tmpobj->valueint;
		}

		//read console history
		consoleobj = cJSON_GetObjectItem(root, "Console");
		if (consoleobj)
		{
			if (tmpobj = cJSON_GetObjectItem(consoleobj, "History"))
			{
				int size = cJSON_GetArraySize(tmpobj);
				tmpobj = cJSON_GetObjectItem(consoleobj, "HistoryResults");
				// if results doesn't have the same number of items as history, don't load them. This might cause a crash, and wouldn't match anyway
				if (tmpobj && cJSON_GetArraySize(tmpobj) == size)
				{
					load_console_history(cJSON_GetObjectItem(consoleobj, "History"), &last_command, size);
					load_console_history(tmpobj, &last_command_result, size);
				}
				// well, tpt++ doesn't save HistoryResults and it is annoying to discard everything, so load History anyway with some hacks
				else if (!tmpobj)
				{
					load_console_history(cJSON_GetObjectItem(consoleobj, "History"), &last_command, size);
					load_console_history(NULL, &last_command_result, size);
				}
			}
		}

		//Read HUDs
		hudobj = cJSON_GetObjectItem(root, "HUD");
		if (hudobj)
		{
			if (tmpobj = cJSON_GetObjectItem(hudobj, "normal"))
			{
				int count = std::min(HUD_OPTIONS,cJSON_GetArraySize(tmpobj));
				for (int i = 0; i < count; i++)
				{
					normalHud[i] = cJSON_GetArrayItem(tmpobj, i)->valueint;
				}
			}
			if (tmpobj = cJSON_GetObjectItem(hudobj, "debug"))
			{
				int count = std::min(HUD_OPTIONS,cJSON_GetArraySize(tmpobj));
				for (int i = 0; i < count; i++)
				{
					debugHud[i] = cJSON_GetArrayItem(tmpobj, i)->valueint;
				}
			}
		}
		
		//Read general settings
		if ((tmpobj = cJSON_GetObjectItem(root, "Proxy")) && tmpobj->type == cJSON_String)
			strncpy(http_proxy_string, tmpobj->valuestring, 255);
		else
			http_proxy_string[0] = 0;
		if (tmpobj = cJSON_GetObjectItem(root, "Scale"))
		{
			sdl_scale = tmpobj->valueint;
			if (sdl_scale == 0)
				sdl_scale = cJSON_GetInt(&tmpobj);
			else if (sdl_scale < 0)
				sdl_scale = 1;
		}
		if (tmpobj = cJSON_GetObjectItem(root, "Fullscreen"))
		{
			kiosk_enable = tmpobj->valueint;
			if (kiosk_enable)
				set_scale(sdl_scale, kiosk_enable);
		}
		if (tmpobj = cJSON_GetObjectItem(root, "FastQuit"))
			fastquit = tmpobj->valueint;
		if (tmpobj = cJSON_GetObjectItem(root, "WindowX"))
			savedWindowX = tmpobj->valueint;
		if (tmpobj = cJSON_GetObjectItem(root, "WindowY"))
			savedWindowY = tmpobj->valueint;

		//Read some extra mod settings
		if (tmpobj = cJSON_GetObjectItem(root, "heatmode"))
			heatmode = tmpobj->valueint;
		if (tmpobj = cJSON_GetObjectItem(root, "autosave"))
			autosave = tmpobj->valueint;
		if (tmpobj = cJSON_GetObjectItem(root, "autosave"))
			autosave = tmpobj->valueint;
		/*if(tmpobj = cJSON_GetObjectItem(root, "realistic"))
		{
			realistic = tmpobj->valueint;
			if (realistic)
				ptypes[PT_FIRE].hconduct = 1;
		}*/
		if (tmpobj = cJSON_GetObjectItem(root, "cracker_unlocked"))
		{
			unlockedstuff |= 0x01;
			menuSections[SC_CRACKER]->enabled = true;
		}
		if (tmpobj = cJSON_GetObjectItem(root, "show_votes"))
			unlockedstuff |= 0x08;
#ifndef NOMOD
		if (tmpobj = cJSON_GetObjectItem(root, "EXPL_unlocked"))
		{
			unlockedstuff |= 0x10;
			ptypes[PT_EXPL].enabled = 1;
			globalSim->elements[PT_EXPL].MenuVisible = 1;
			globalSim->elements[PT_EXPL].Enabled = 1;
			FillMenus();
		}
#endif
/*#ifndef TOUCHUI
		if (tmpobj = cJSON_GetObjectItem(root, "old_menu"))
			old_menu = 1;
#endif*/
		if (tmpobj = cJSON_GetObjectItem(root, "alt_find"))
			finding |= 0x8;
		if (tmpobj = cJSON_GetObjectItem(root, "dateformat"))
			dateformat = tmpobj->valueint;
		if (tmpobj = cJSON_GetObjectItem(root, "decobox_hidden"))
			decobox_hidden = tmpobj->valueint;

		itemobj = cJSON_GetObjectItem(root, "SavePreview");
		if (itemobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(itemobj, "scrollSpeed")) && tmpobj->valueint > 0)
				scrollSpeed = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(itemobj, "scrollDeceleration")) && tmpobj->valuedouble >= 0 && tmpobj->valuedouble <= 1)
				scrollDeceleration = tmpobj->valuedouble;
			if (tmpobj = cJSON_GetObjectItem(itemobj, "ShowIDs"))
				show_ids = tmpobj->valueint;
		}

		cJSON_Delete(root);
		free(prefdata);

		// settings that need to be changed on specific updates can go here
		if (update_flag)
		{
#ifdef WIN
			if (last_build == 322)
			{
				scrollSpeed = 15;
				scrollDeceleration = 0.99f;
			}
#endif
		}
	}
	else
		firstRun = true;
}

int sregexp(const char *str, char *pattern)
{
	int result;
	regex_t patternc;
	if (regcomp(&patternc, pattern,  0)!=0)
		return 1;
	result = regexec(&patternc, str, 0, NULL, 0);
	regfree(&patternc);
	return result;
}

int load_string(FILE *f, char *str, int max)
{
	int li;
	unsigned char lb[2];
	fread(lb, 2, 1, f);
	li = lb[0] | (lb[1] << 8);
	if (li > max)
	{
		str[0] = 0;
		return 1;
	}
	fread(str, li, 1, f);
	str[li] = 0;
	return 0;
}

void strcaturl(char *dst, char *src)
{
	char *d;
	unsigned char *s;

	for (d=dst; *d; d++) ;

	for (s=(unsigned char *)src; *s; s++)
	{
		if ((*s>='0' && *s<='9') ||
		        (*s>='a' && *s<='z') ||
		        (*s>='A' && *s<='Z'))
			*(d++) = *s;
		else
		{
			*(d++) = '%';
			*(d++) = hex[*s>>4];
			*(d++) = hex[*s&15];
		}
	}
	*d = 0;
}

void strappend(char *dst, const char *src)
{
	char *d;
	unsigned char *s;

	for (d=dst; *d; d++) ;

	for (s=(unsigned char *)src; *s; s++)
	{
		*(d++) = *s;
	}
	*d = 0;
}

// Strips stuff from a string. Can strip all non ascii characters (excluding color and newlines), strip all color, strip all newlines, or strip all non 0-9 characters
std::string CleanString(std::string dirtyString, bool ascii, bool color, bool newlines, bool numeric)
{
	for (size_t i = 0; i < dirtyString.size(); i++)
	{
		switch(dirtyString[i])
		{
		case '\b':
			if (color)
			{
				dirtyString.erase(i, 2);
				i--;
			}
			else
				i++;
			break;
		case '\x0E':
			if (color)
			{
				dirtyString.erase(i, 1);
				i--;
			}
			break;
		case '\x0F':
			if (color)
			{
				dirtyString.erase(i, 4);
				i--;
			}
			else
				i += 3;
			break;
		// erase these without question, first two are control characters used for the cursor
		// second is used internally to denote automatically inserted newline
		case '\x01':
		case '\x02':
		case '\r':
			dirtyString.erase(i, 1);
			i--;
			break;
		case '\n':
			if (newlines)
				dirtyString[i] = ' ';
			break;
		default:
			if (numeric && (dirtyString[i] < '0' || dirtyString[i] > '9'))
			{
				dirtyString.erase(i, 1);
				i--;
			}
			// if less than ascii 20 or greater than ascii 126, delete
			else if (ascii && (dirtyString[i] < ' ' || dirtyString[i] > '~'))
			{
				dirtyString.erase(i, 1);
				i--;
			}
			break;
		}
	}
	return dirtyString;
}

std::string CleanString(const char * dirtyData, bool ascii, bool color, bool newlines, bool numeric)
{
	return CleanString(std::string(dirtyData), ascii, color, newlines, numeric);
}

void *file_load(const char *fn, int *size)
{
	FILE *f = fopen(fn, "rb");
	void *s;

	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);
	s = malloc(*size);
	if (!s)
	{
		fclose(f);
		return NULL;
	}
	fread(s, *size, 1, f);
	fclose(f);
	return s;
}

int file_exists(const char *filename)
{
	int exists = 0;
#ifdef WIN
	struct _stat s;
	if(_stat(filename, &s) == 0)
#else
	struct stat s;
	if(stat(filename, &s) == 0)
#endif
	{
		if(s.st_mode & S_IFDIR)
		{
			exists = 1;
		}
		else if(s.st_mode & S_IFREG)
		{
			exists = 1;
		}
		else
		{
			exists = 1;
		}
	}
	else
	{
		exists = 0;
	}
	return exists;
}

matrix2d m2d_multiply_m2d(matrix2d m1, matrix2d m2)
{
	matrix2d result = {
		m1.a*m2.a+m1.b*m2.c, m1.a*m2.b+m1.b*m2.d,
		m1.c*m2.a+m1.d*m2.c, m1.c*m2.b+m1.d*m2.d
	};
	return result;
}
vector2d m2d_multiply_v2d(matrix2d m, vector2d v)
{
	vector2d result = {
		m.a*v.x+m.b*v.y,
		m.c*v.x+m.d*v.y
	};
	return result;
}
matrix2d m2d_multiply_float(matrix2d m, float s)
{
	matrix2d result = {
		m.a*s, m.b*s,
		m.c*s, m.d*s,
	};
	return result;
}

vector2d v2d_multiply_float(vector2d v, float s)
{
	vector2d result = {
		v.x*s,
		v.y*s
	};
	return result;
}

vector2d v2d_add(vector2d v1, vector2d v2)
{
	vector2d result = {
		v1.x+v2.x,
		v1.y+v2.y
	};
	return result;
}
vector2d v2d_sub(vector2d v1, vector2d v2)
{
	vector2d result = {
		v1.x-v2.x,
		v1.y-v2.y
	};
	return result;
}

matrix2d m2d_new(float me0, float me1, float me2, float me3)
{
	matrix2d result = {me0,me1,me2,me3};
	return result;
}
vector2d v2d_new(float x, float y)
{
	vector2d result = {x, y};
	return result;
}

char * clipboardtext = NULL;
void clipboard_push_text(char * text)
{
	if (clipboardtext)
	{
		free(clipboardtext);
		clipboardtext = NULL;
	}
	clipboardtext = mystrdup(text);
#ifdef ANDROID
	SDL_SetClipboardText(text);
#elif MACOSX
	writeClipboard(text);
#elif WIN
	if (OpenClipboard(NULL))
	{
		HGLOBAL cbuffer;
		char * glbuffer;

		EmptyClipboard();

		cbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(text)+1);
		glbuffer = (char*)GlobalLock(cbuffer);

		strcpy(glbuffer, text);

		GlobalUnlock(cbuffer);
		SetClipboardData(CF_TEXT, cbuffer);
		CloseClipboard();
	}
#elif defined(LIN) && defined(SDL_VIDEO_DRIVER_X11)
	if (clipboard_text!=NULL) {
		free(clipboard_text);
		clipboard_text = NULL;
	}
	clipboard_text = mystrdup(text);
	sdl_wminfo.info.x11.lock_func();
	XSetSelectionOwner(sdl_wminfo.info.x11.display, XA_CLIPBOARD, sdl_wminfo.info.x11.window, CurrentTime);
	XFlush(sdl_wminfo.info.x11.display);
	sdl_wminfo.info.x11.unlock_func();
#else
	printf("Not implemented: put text on clipboard \"%s\"\n", text);
#endif
}

char * clipboard_pull_text()
{
#ifdef ANDROID
	if (!SDL_HasClipboardText())
		return mystrdup("");
	char *data = SDL_GetClipboardText();
	if (!data)
		return mystrdup("");
	char *ret = mystrdup(data);
	SDL_free(data);
	return ret;
#elif MACOSX
	char * data = readClipboard();
	if (!data)
		return mystrdup("");
	return mystrdup(data);
#elif WIN
	if (OpenClipboard(NULL))
	{
		HANDLE cbuffer;
		char * glbuffer;

		cbuffer = GetClipboardData(CF_TEXT);
		glbuffer = (char*)GlobalLock(cbuffer);
		GlobalUnlock(cbuffer);
		CloseClipboard();
		if(glbuffer!=NULL){
			return mystrdup(glbuffer);
		} //else {
		//	return "";
		//}
	}
#elif defined(LIN) && defined(SDL_VIDEO_DRIVER_X11)
	char *text = NULL;
	Window selectionOwner;
	sdl_wminfo.info.x11.lock_func();
	selectionOwner = XGetSelectionOwner(sdl_wminfo.info.x11.display, XA_CLIPBOARD);
	if (selectionOwner != None)
	{
		unsigned char *data = NULL;
		Atom type;
		int format, result;
		unsigned long len, bytesLeft;
		XConvertSelection(sdl_wminfo.info.x11.display, XA_CLIPBOARD, XA_UTF8_STRING, XA_CLIPBOARD, sdl_wminfo.info.x11.window, CurrentTime);
		XFlush(sdl_wminfo.info.x11.display);
		sdl_wminfo.info.x11.unlock_func();
		while (1)
		{
			SDL_Event event;
			SDL_WaitEvent(&event);
			if (event.type == SDL_SYSWMEVENT)
			{
				XEvent xevent = event.syswm.msg->event.xevent;
				if (xevent.type == SelectionNotify && xevent.xselection.requestor == sdl_wminfo.info.x11.window)
					break;
				else
					EventProcess(event);
			}
		}
		sdl_wminfo.info.x11.lock_func();
		XGetWindowProperty(sdl_wminfo.info.x11.display, sdl_wminfo.info.x11.window, XA_CLIPBOARD, 0, 0, 0, AnyPropertyType, &type, &format, &len, &bytesLeft, &data);
		if (data)
		{
			XFree(data);
			data = NULL;
		}
		if (bytesLeft)
		{
			result = XGetWindowProperty(sdl_wminfo.info.x11.display, sdl_wminfo.info.x11.window, XA_CLIPBOARD, 0, bytesLeft, 0, AnyPropertyType, &type, &format, &len, &bytesLeft, &data);
			if (result == Success)
			{
				text = strdup((const char*) data);
				XFree(data);
			}
			else
			{
				printf("Failed to pull from clipboard\n");
				return mystrdup("?");
			}
		}
		else
			return mystrdup("");
		XDeleteProperty(sdl_wminfo.info.x11.display, sdl_wminfo.info.x11.window, XA_CLIPBOARD);
	}
	sdl_wminfo.info.x11.unlock_func();
	return text;
#else
	printf("Not implemented: get text from clipboard\n");
#endif
	if (clipboardtext)
		return mystrdup(clipboardtext);
	return "";
}

void HSV_to_RGB(int h,int s,int v,int *r,int *g,int *b)//convert 0-255(0-360 for H) HSV values to 0-255 RGB
{
	float hh, ss, vv, c, x;
	int m;
	hh = h/60.0f;//normalize values
	ss = s/255.0f;
	vv = v/255.0f;
	c = vv * ss;
	x = c * ( 1 - fabs(fmod(hh,2.0f) -1) );
	if(hh<1){
		*r = (int)(c*255.0);
		*g = (int)(x*255.0);
		*b = 0;
	}
	else if(hh<2){
		*r = (int)(x*255.0);
		*g = (int)(c*255.0);
		*b = 0;
	}
	else if(hh<3){
		*r = 0;
		*g = (int)(c*255.0);
		*b = (int)(x*255.0);
	}
	else if(hh<4){
		*r = 0;
		*g = (int)(x*255.0);
		*b = (int)(c*255.0);
	}
	else if(hh<5){
		*r = (int)(x*255.0);
		*g = 0;
		*b = (int)(c*255.0);
	}
	else if(hh<6){
		*r = (int)(c*255.0);
		*g = 0;
		*b = (int)(x*255.0);
	}
	m = (int)((vv-c)*255.0);
	*r += m;
	*g += m;
	*b += m;
}

void RGB_to_HSV(int r,int g,int b,int *h,int *s,int *v)//convert 0-255 RGB values to 0-255(0-360 for H) HSV
{
	float rr, gg, bb, a,x,c,d;
	rr = r/255.0f;//normalize values
	gg = g/255.0f;
	bb = b/255.0f;
	a = std::min(rr,gg);
	a = std::min(a,bb);
	x = std::max(rr,gg);
	x = std::max(x,bb);
	if (r == g && g == b)//greyscale
	{
		*h = 0;
		*s = 0;
		*v = (int)(a*255.0);
	}
	else
	{
		int min = (r < g) ? ((r < b) ? 0 : 2) : ((g < b) ? 1 : 2);
		c = (min==0) ? gg-bb : ((min==2) ? rr-gg : bb-rr);
		d = (float)((min==0) ? 3 : ((min==2) ? 1 : 5));
		*h = (int)(60.0*(d - c/(x - a)));
		*s = (int)(255.0*((x - a)/x));
		*v = (int)(255.0*x);
	}
}

Tool* GetToolFromIdentifier(std::string identifier)
{
	for (int i = 0; i < SC_TOTAL; i++)
	{
		for (unsigned int j = 0; j < menuSections[i]->tools.size(); j++)
		{
			if  (identifier == menuSections[i]->tools[j]->GetIdentifier())
				return menuSections[i]->tools[j];
		}
	}
	return NULL;
}

std::string URLEncode(std::string source)
{
	char * src = (char *)source.c_str();
	char * dst = new char[(source.length()*3)+2];
	std::fill(dst, dst+(source.length()*3)+2, 0);

	char *d;
	unsigned char *s;

	for (d=dst; *d; d++) ;

	for (s=(unsigned char *)src; *s; s++)
	{
		if ((*s>='0' && *s<='9') ||
				(*s>='a' && *s<='z') ||
				(*s>='A' && *s<='Z'))
			*(d++) = *s;
		else
		{
			*(d++) = '%';
			*(d++) = hex[*s>>4];
			*(d++) = hex[*s&15];
		}
	}
	*d = 0;

	std::string finalString(dst);
	delete[] dst;
	return finalString;
}

void membwand(void * destv, void * srcv, size_t destsize, size_t srcsize)
{
	size_t i;
	unsigned char * dest = (unsigned char*)destv;
	unsigned char * src = (unsigned char*)srcv;
	if (srcsize==destsize)
	{
		for(i = 0; i < destsize; i++){
			dest[i] = dest[i] & src[i];
		}
	}
	else
	{
		for(i = 0; i < destsize; i++){
			dest[i] = dest[i] & src[i%srcsize];
		}
	}
}
vector2d v2d_zero = {0,0};
matrix2d m2d_identity = {1,0,0,1};
