#include <cstddef> //needed for NULL apparently ...
#include "Component.h"
#include "Window.h"

Component::Component(Point position, Point size) :
	parent(NULL),
	position(position),
	size(size),
	isMouseInside(false),
	visible(true),
	enabled(true),
	color(COLRGB(255, 255, 255)),
	toDelete(false),
	toAdd(false)
{

}

bool Component::IsFocused()
{
	if (parent)
		return parent->IsFocused(this);
	return false;
}

bool Component::IsClicked()
{
	if (parent)
		return parent->IsClicked(this);
	return false;
}

bool Component::IsMouseDown()
{
	if (parent)
		return parent->IsMouseDown();
	return false;
}

void Component::SetVisible(bool visible)
{
	this->visible = visible;
	toAdd = false;
	if (!visible && parent)
		parent->DefocusComponent(this);
}
