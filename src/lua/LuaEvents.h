#ifndef LUAEVENTS_H
#define LUAEVENTS_H

#include <string>

struct lua_State;

class Event
{
protected:
	void PushInteger(lua_State * l, int num);
	void PushBoolean(lua_State * l, bool flag);
	void PushString(lua_State * l, std::string str);

public:
	virtual int PushToStack(lua_State * l) = 0;
};

class TextInputEvent : public Event
{
	std::string text;

public:
	TextInputEvent(std::string text);

	int PushToStack(lua_State * l) override;
};

class KeyEvent : public Event
{
	int key;
	int scan;
	bool repeat;
	bool shift;
	bool ctrl;
	bool alt;

public:
	KeyEvent(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt);

	int PushToStack(lua_State * l) override;
};

class MouseDownEvent : public Event
{
	int x;
	int y;
	int button;

public:
	MouseDownEvent(int x, int y, int button);

	int PushToStack(lua_State * l) override;
};

class MouseUpEvent : public Event
{
	int x;
	int y;
	int button;
	int reason;

public:
	MouseUpEvent(int x, int y, int button, int reason);

	int PushToStack(lua_State * l) override;
};

class MouseMoveEvent : public Event
{
	int x;
	int y;
	int dx;
	int dy;

public:
	MouseMoveEvent(int x, int y, int dx, int dy);

	int PushToStack(lua_State * l) override;
};

class MouseWheelEvent : public Event
{
	int x;
	int y;
	int d;

public:
	MouseWheelEvent(int x, int y, int d);

	int PushToStack(lua_State * l) override;
};

class TickEvent: public Event
{
public:
	int PushToStack(lua_State *l) override { return 0; }
};

class BlurEvent: public Event
{
public:
	int PushToStack(lua_State *l) override { return 0; }
};

class CloseEvent: public Event
{
public:
	int PushToStack(lua_State *l) override { return 0; }
};


class LuaEvents
{
public:
	static int RegisterEventHook(lua_State *l, std::string eventName);
	static int UnregisterEventHook(lua_State *l, std::string eventName);
	static bool HandleEvent(lua_State *l, Event * event, std::string eventName);

	enum EventTypes {
		keypress,
		keyrelease,
		textinput,
		mousedown,
		mouseup,
		mousemove,
		mousewheel,
		tick,
		blur,
		close
	};
};

#endif // LUAEVENTS_H

