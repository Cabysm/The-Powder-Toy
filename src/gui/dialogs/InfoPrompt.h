#ifndef INFOPROMPT_H
#define INFOPROMPT_H
#include <string>
#include "interface/Window.h"

class InfoPrompt : public ui::Window
{
public:
	InfoPrompt(std::string title, std::string message, std::string OK = "OK");

	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};

#endif
