#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <tuple>
#include <queue>

#include "glad/glad.h"

using String = std::basic_string<TCHAR>;
using Size = std::pair<uint32_t, uint32_t>;

// simple events
struct WindowEvent {
	enum {
		unknown = 0,
		keyboardKey,
		textInput,
		mouseMotion,
		mouseButton
	} type;

	int keyCode;
	TCHAR keyChar;

	int button;
	enum {
		up = 0,
		down
	} buttonState;

	int screenX, screenY;
	int deltaX, deltaY;
};

struct WindowParams {
	uint32_t width{ 1280 }, height{ 720 };
	String className{ TEXT("GLWindow") }, title{ TEXT("GL Window")};
};

class Window {
public:
	Window() = default;
	virtual ~Window();

	bool create(const WindowParams& params);
	bool pollEvents(WindowEvent& e); // TODO: Handle events (mouse/keyboard)

	void swapBuffers();

	const String& title() const;
	void title(const String& title);

	Size size();

	void submitEvent(WindowEvent e);
	int mouseX() const { return m_mouseX; }
	int mouseY() const { return m_mouseY; }
	void updateMouse(int x, int y) { m_mouseX = x; m_mouseY = y; }

private:
	HWND m_handle{ nullptr };
	HDC m_dc{ nullptr };
	HGLRC m_glrc{ nullptr };

	void initializeGLEXT();
	HGLRC initializeGL();

	std::queue<WindowEvent> m_eventQueue;
	int m_mouseX, m_mouseY; // For calculating deltas
};