#include "ToolTip.h"
#include "graphics.h"

// functions in tooltip class
ToolTip::ToolTip(std::string tip_, Point location_, int ID_, int alpha_):
	tip(tip_),
	location(location_),
	ID(ID_)
{
	if (alpha_ == -1)
		alpha = 15;
	else
		alpha = alpha_;
}

void ToolTip::UpdateToolTip(std::string toolTip, Point location_, int alpha_)
{
	tip = toolTip;
	location = location_;
	//alpha_ == -1 is used for fading in tooltips for some reason (gets reduced by 5 every frame in draw still)
	if (alpha_ == -1 && alpha < 255)
	{
		alpha += 15;
		if (alpha > 255)
			alpha = 255;
	}
	//else, we want to directly set alpha
	else if (ID != INTROTIP || alpha > 255)
		alpha = alpha_;
}

bool ToolTip::DrawToolTip()
{
	if (alpha)
	{
		if (ID == INFOTIP || ID == ELEMENTTIP)
			drawtext_outline(vid_buf, location.X, location.Y, tip.c_str(), 255, 255, 255, std::min(alpha, 255), 0, 0, 0, std::min(alpha, 255));
		else
			drawtext(vid_buf, location.X, location.Y, tip.c_str(), 255, 255, 255, std::min(alpha, 255));
		alpha -= 5;
	}
	return alpha > 0;
}

// other functions related to tooltips
std::vector<ToolTip*> toolTips;
void UpdateToolTip(std::string toolTip, Point location, int ID, int alpha)
{
	for (unsigned int i = 0; i < toolTips.size(); i++)
		if (toolTips[i]->GetID() == ID)
		{
			toolTips[i]->UpdateToolTip(toolTip, location, alpha);
			return;
		}
	if (ID != INTROTIP || alpha > 255)
		toolTips.push_back(new ToolTip(toolTip, location, ID, alpha));
}

void DrawToolTips()
{
	for (int i = toolTips.size()-1; i >= 0; i--)
	{
		if (!toolTips[i]->DrawToolTip())
		{
			delete toolTips[i];
			toolTips.erase(toolTips.begin()+i);
		}
	}
}

int GetToolTipAlpha(int ID)
{
	for (unsigned int i = 0; i < toolTips.size(); i++)
		if (toolTips[i]->GetID() == ID)
			return toolTips[i]->GetAlpha();
	return 0;
}
