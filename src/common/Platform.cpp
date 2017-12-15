#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#ifdef WIN
#include <shlobj.h>
#include <shlwapi.h>
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifdef MACOSX
#include <ApplicationServices/ApplicationServices.h>
#include <mach-o/dyld.h>
#endif

#ifdef LIN
#include "images.h"
#endif

#ifdef ANDROID
#include <SDL/SDL_screenkeyboard.h>
#include <SDL/SDL_android.h>
#endif

#include "Platform.h"
#include "defines.h"

namespace Platform
{

char *ExecutableName()
{
#ifdef ANDROID
	return SDL_ANDROID_GetAppName();
#else
#ifdef WIN
	char *name= (char *)malloc(64);
	DWORD max=64, res;
	while ((res = GetModuleFileName(NULL, name, max)) >= max)
	{
#elif MACOSX
	char *fn=(char*)malloc(64),*name=(char*)malloc(PATH_MAX);
	uint32_t max=64, res;
	if (_NSGetExecutablePath(fn, &max) != 0)
	{
		char *realloced_fn = (char*)realloc(fn, max);
		assert(realloced_fn != NULL);
		fn = realloced_fn;
	}
	if (realpath(fn, name) == NULL)
	{
		free(fn);
		free(name);
		return NULL;
	}
	res = 1;
#else
	char fn[64], *name=(char*)malloc(64);
	size_t max=64, res;
	sprintf(fn, "/proc/self/exe");
	memset(name, 0, max);
	while ((res = readlink(fn, name, max)) >= max-1)
	{
#endif
#ifndef MACOSX
		max *= 2;
		char* realloced_name = (char *)realloc(name, max);
		assert(realloced_name != NULL);
		name = realloced_name;
		memset(name, 0, max);
	}
#endif //not MACOSX
	if (res <= 0)
	{
		free(name);
		return NULL;
	}
	return name;
#endif //ANDROID
}

void DoRestart(bool saveTab)
{
	if (saveTab)
	{
		sys_pause = true;
		tab_save(tab_num);
	}
#ifdef ANDROID
	SDL_ANDROID_RestartMyself("");
	exit(-1);
#else
	char *exename = ExecutableName();
	if (exename)
	{
#ifdef WIN
		ShellExecute(NULL, "open", exename, NULL, NULL, SW_SHOWNORMAL);
#elif defined(LIN) || defined(MACOSX)
		execl(exename, "powder", NULL);
#endif
		free(exename);
	}
	exit(-1);
#endif
}

void OpenLink(std::string uri)
{
	int ret = 0;
#ifdef ANDROID
	SDL_ANDROID_OpenExternalWebBrowser(uri.c_str());
#elif WIN
	ShellExecute(0, "OPEN", uri.c_str(), NULL, NULL, 0);
#elif MACOSX
	std::string command = "open " + uri;
	ret = system(command.c_str());
#elif LIN
	std::string command = "xdg-open " + uri;
	ret = system(command.c_str());
#else
	ret = 1;
#endif
	if (ret)
		printf("Cannot open browser\n");
}

void Millisleep(long int t)
{
#ifdef WIN
	Sleep(t);
#else
	struct timespec s;
	s.tv_sec = t/1000;
	s.tv_nsec = (t%1000)*10000000;
	nanosleep(&s, NULL);
#endif
}

void LoadFileInResource(int name, int type, unsigned int& size, const char*& data)
{
#ifdef _MSC_VER
	HMODULE handle = ::GetModuleHandle(NULL);
	HRSRC rc = ::FindResource(handle, MAKEINTRESOURCE(name), MAKEINTRESOURCE(type));
	HGLOBAL rcData = ::LoadResource(handle, rc);
	size = ::SizeofResource(handle, rc);
	data = static_cast<const char*>(::LockResource(rcData));
#endif
}

bool RegisterExtension()
{
#ifdef WIN
	bool returnval;
	LONG rresult;
	HKEY newkey;
	char *currentfilename = Platform::ExecutableName();
	char *iconname = NULL;
	char *opencommand = NULL;
	char *protocolcommand = NULL;
	//char AppDataPath[MAX_PATH];
	char *AppDataPath = NULL;
	iconname = (char*)malloc(strlen(currentfilename)+6);
	sprintf(iconname, "%s,-102", currentfilename);

	//Create Roaming application data folder
	/*if(!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, AppDataPath)))
	{
		returnval = false;
		goto finalise;
	}*/

	AppDataPath = (char*)_getcwd(NULL, 0);

	//Move Game executable into application data folder
	//TODO: Implement

	opencommand = (char*)malloc(strlen(currentfilename)+53+strlen(AppDataPath));
	protocolcommand = (char*)malloc(strlen(currentfilename)+55+strlen(AppDataPath));
	/*if((strlen(AppDataPath)+strlen(APPDATA_SUBDIR "\\Powder Toy"))<MAX_PATH)
	{
		strappend(AppDataPath, APPDATA_SUBDIR);
		_mkdir(AppDataPath);
		strappend(AppDataPath, "\\Powder Toy");
		_mkdir(AppDataPath);
	} else {
		returnval = false;
		goto finalise;
	}*/
	sprintf(opencommand, "\"%s\" open \"%%1\" ddir \"%s\"", currentfilename, AppDataPath);
	sprintf(protocolcommand, "\"%s\" ddir \"%s\" ptsave \"%%1\"", currentfilename, AppDataPath);

	//Create protocol entry
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\ptsave", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)"Powder Toy Save", strlen("Powder Toy Save")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, (LPCSTR)"URL Protocol", 0, REG_SZ, (LPBYTE)"", strlen("")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);


	//Set Protocol DefaultIcon
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\ptsave\\DefaultIcon", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)iconname, strlen(iconname)+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Set Protocol Launch command
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\ptsave\\shell\\open\\command", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)protocolcommand, strlen(protocolcommand)+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Create extension entry
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\.cps", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)"PowderToySave", strlen("PowderToySave")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\.stm", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)"PowderToySave", strlen("PowderToySave")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Create program entry
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\PowderToySave", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)"Powder Toy Save", strlen("Powder Toy Save")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Set DefaultIcon
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\PowderToySave\\DefaultIcon", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)iconname, strlen(iconname)+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Set Launch command
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\PowderToySave\\shell\\open\\command", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)opencommand, strlen(opencommand)+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	returnval = true;
	finalise:

	if(iconname) free(iconname);
	if(opencommand) free(opencommand);
	if(currentfilename) free(currentfilename);
	if(protocolcommand) free(protocolcommand);

	return returnval;
#elif ANDROID
	return false;
#elif LIN
	int success = 1;
	std::string filename = Platform::ExecutableName(), pathname = filename.substr(0, filename.rfind('/'));
	for (size_t i = 0; i < filename.size(); i++)
	{
		if (filename[i] == '\'')
		{
			filename.insert(i, "'\\'");
			i += 3;
		}
	}
	filename.insert(filename.size(), "'");
	filename.insert(0, "'");
	FILE *f;
	const char *mimedata =
"<?xml version=\"1.0\"?>\n"
"	<mime-info xmlns='http://www.freedesktop.org/standards/shared-mime-info'>\n"
"	<mime-type type=\"application/vnd.powdertoy.save\">\n"
"		<comment>Powder Toy save</comment>\n"
"		<glob pattern=\"*.cps\"/>\n"
"		<glob pattern=\"*.stm\"/>\n"
"	</mime-type>\n"
"</mime-info>\n";
	f = fopen("powdertoy-save.xml", "wb");
	if (!f)
		return false;
	fwrite(mimedata, 1, strlen(mimedata), f);
	fclose(f);

	const char *protocolfiledata_tmp =
"[Desktop Entry]\n"
"Type=Application\n"
"Name=Jacob1's Mod\n"
"Comment=Physics sandbox game\n"
"MimeType=x-scheme-handler/ptsave;\n"
"NoDisplay=true\n"
"Categories=Game;Simulation\n"
"Icon=powdertoy.png\n";
	std::stringstream protocolfiledata;
	protocolfiledata << protocolfiledata_tmp << "Exec=" << filename << " ptsave %u\nPath=" << pathname << "\n";
	f = fopen("powdertoy-tpt-ptsave.desktop", "wb");
	if (!f)
		return false;
	fwrite(protocolfiledata.str().c_str(), 1, strlen(protocolfiledata.str().c_str()), f);
	fclose(f);
	success = system("xdg-desktop-menu install powdertoy-tpt-ptsave.desktop") && success;

	const char *desktopopenfiledata_tmp =
"[Desktop Entry]\n"
"Type=Application\n"
"Name=Jacob1's Mod\n"
"Comment=Physics sandbox game\n"
"MimeType=application/vnd.powdertoy.save;\n"
"NoDisplay=true\n"
"Categories=Game;Simulation\n"
"Icon=powdertoy.png\n";
	std::stringstream desktopopenfiledata;
	desktopopenfiledata << desktopopenfiledata_tmp << "Exec=" << filename << " open %f\nPath=" << pathname << "\n";
	f = fopen("powdertoy-tpt-open.desktop", "wb");
	if (!f)
		return false;
	fwrite(desktopopenfiledata.str().c_str(), 1, strlen(desktopopenfiledata.str().c_str()), f);
	fclose(f);
	success = system("xdg-mime install powdertoy-save.xml") && success;
	success = system("xdg-desktop-menu install powdertoy-tpt-open.desktop") && success;

	const char *desktopfiledata_tmp =
"[Desktop Entry]\n"
"Version=1.0\n"
"Encoding=UTF-8\n"
"Name=Jacob1's Mod\n"
"Type=Application\n"
"Comment=Physics sandbox game\n"
"Categories=Game;Simulation\n"
"Icon=powdertoy.png\n";
	std::stringstream desktopfiledata;
	desktopfiledata << desktopfiledata_tmp << "Exec=" << filename << "\nPath=" << pathname << "\n";
	f = fopen("powdertoy-jacobsmod.desktop", "wb");
	if (!f)
		return false;
	fwrite(desktopfiledata.str().c_str(), 1, strlen(desktopfiledata.str().c_str()), f);
	fclose(f);
	success = system("xdg-desktop-menu install powdertoy-jacobsmod.desktop") && success;

	f = fopen("powdertoy-save-32.png", "wb");
	if (!f)
		return false;
	fwrite(icon_doc_32_png, 1, icon_doc_32_png_size, f);
	fclose(f);
	f = fopen("powdertoy-save-16.png", "wb");
	if (!f)
		return false;
	fwrite(icon_doc_16_png, 1, icon_doc_16_png_size, f);
	fclose(f);
	f = fopen("powdertoy.png", "wb");
	if (!f)
		return false;
	fwrite(icon_desktop_48_png, 1, icon_desktop_48_png_size, f);
	fclose(f);
	success = system("xdg-icon-resource install --noupdate --context mimetypes --size 32 powdertoy-save-32.png application-vnd.powdertoy.save") && success;
	success = system("xdg-icon-resource install --noupdate --context mimetypes --size 16 powdertoy-save-16.png application-vnd.powdertoy.save") && success;
	success = system("xdg-icon-resource install --noupdate --novendor --size 48 powdertoy.png") && success;
	success = system("xdg-icon-resource forceupdate") && success;
	success = system("xdg-mime default powdertoy-tpt-open.desktop application/vnd.powdertoy.save") && success;
	success = system("xdg-mime default powdertoy-tpt-ptsave.desktop x-scheme-handler/ptsave") && success;
	unlink("powdertoy.png");
	unlink("powdertoy-save-32.png");
	unlink("powdertoy-save-16.png");
	unlink("powdertoy-save.xml");
	unlink("powdertoy-jacobsmod.desktop");
	unlink("powdertoy-tpt-open.desktop");
	unlink("powdertoy-tpt-ptsave.desktop");
	return !success;
#elif defined MACOSX
	return false;
#endif
}

// brings up an on screen keyboard and sends one key input for every key pressed
// the tiny keyboard designed to do this doesn't work, so this will bring up a blocking keyboard
// key presses are still sent one at a time when it is done (also seems to overflow every 90 characters, which seems to be the max)
bool ShowOnScreenKeyboard(const char *str, bool autoCorrect)
{
#ifdef ANDROID
	// keyboard without text input is a lot nicer
	// but for some reason none of the keys work, and mouse input is never sent through outside of the keyboard
	// unless you try to press a key (inside the keyboard) where for some reason it clicks the thing under that key
	//SDL_ANDROID_ToggleScreenKeyboardWithoutTextInput();

	// blocking fullscreen keyboard
	SDL_ANDROID_ToggleScreenKeyboardTextInput(str, autoCorrect);
	return true;
#endif
	return false;
}

// takes a buffer (which can have existing text), and the buffer size
// The user then types the text, which is placed in the buffer for use
void GetOnScreenKeyboardInput(char * buff, int buffSize, bool autoCorrect)
{
#ifdef ANDROID
	SDL_ANDROID_GetScreenKeyboardTextInput(buff, buffSize, autoCorrect);
#endif
}

bool IsOnScreenKeyboardShown()
{
#ifdef ANDROID
	return SDL_IsScreenKeyboardShown(NULL);
#endif
	return false;
}

void Vibrate(int milliseconds)
{
#ifdef ANDROID
	SDL_ANDROID_Vibrate(milliseconds);
#endif
}

int CheckLoadedPtsave()
{
#ifdef ANDROID
	return SDL_ANDROID_GetPtsaveOpen();
#endif
	return 0;
}

}
