#ifndef ENGINE_H
#define ENGINE_H

#include <stack>
#include "Window.h"
#include "common/Singleton.h"
#include "common/tpt-stdint.h"
#include "common/Point.h"

union SDL_Event;
class Engine : public Singleton<Engine>
{
public:
	Engine();
	~Engine();

	void MainLoop();
	void ShowWindow(Window_ *window);
	void CloseWindow(Window_ *window);

	int GetScale();
	void SetScale(int newScale);

	int Get3dDepth();
	void Set3dDepth(int depth);

	Point GetLastMousePosition() { return lastMousePosition; }

private:
	bool EventProcess(SDL_Event event);
	void ShowWindowDelayed();
	void CloseWindowDelayed();
	std::stack<Window_*> windows;
	Window_ *top, *nextTop;

	Point lastMousePosition;
	uint32_t lastTick;

	int depth3d;
};

#endif
