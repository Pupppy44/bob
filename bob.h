#pragma once
#include <windows.h>
#include <d2d1.h>
#include <vector>
#pragma comment(lib, "d2d1")

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

// Other classes/structures
struct bob_color {
	float r;
	float g;
	float b;
	float a = 1.f;
};

struct bob_gradient_stop {
	bob_color color;
	float position;
};

class bob_gradient {
public:
	void add_stop(bob_gradient_stop s) {
		gradient_stops.push_back(s);
	}
private:
	std::vector<bob_gradient_stop> gradient_stops;
};
// End of other classes/structures

// External functions
wchar_t* ex_bob_to_wchar(const char* title) {
	size_t s = strlen(title) + 1;
	wchar_t* n = new wchar_t[s];
	size_t o;
	mbstowcs_s(&o, n, s, title, s - 1);
	return n;
}

// Objects
#include "./bob/objs/rect.h"
#include "./bob/objs/text.h"
#include "./bob/objs/img.h"

// Adds compatibility with the Exporter program
#ifdef EXPORTER
bob_color SDL_Color(int r, int g, int b) { bob_color c; c.r = r; c.g = g; c.b = b; c.a = 1; return c; };
#define TkRect bob_rect
#define TkText bob_text
#define TkImage bob_image
#endif

class bob {
public:
	bob() :
		m_hwnd(NULL),
		m_pDirect2dFactory(NULL),
		m_pRenderTarget(NULL)
	{}

	bob& init(const char* title, int width, int height, bool load_mode = 0)
	{
		create_drs_independent();

		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = wnd_proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = HINST_THISCOMPONENT;
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
		wcex.lpszClassName = L"bob_application";

		RegisterClassEx(&wcex);

		wchar_t* n = ex_bob_to_wchar(title);

		int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
		int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
		
		auto flags = WS_POPUP | WS_SYSMENU;
		if (!load_mode) flags = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;

		m_hwnd = CreateWindow(
			L"bob_application",
			n,
			flags,
			x,
			y,
			0,
			0,
			NULL,
			NULL,
			HINST_THISCOMPONENT,
			this);

		SetWindowPos(
			m_hwnd,
			HWND_TOP,
			NULL,
			NULL,
			width,
			height,
			SWP_NOMOVE);

		return *this;
	}

	void start()
	{
		started = 1;

		for (auto& o : objects) {
			o->init();
		}

		ShowWindow(m_hwnd, SW_SHOWNORMAL);
		UpdateWindow(m_hwnd);

		MSG msg;

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	bob& add_obj(Object& obj, ...) {
		objects.push_back(&obj);
		return *this;
	}

	bob& set_bg(bob_color c) {
		bg = c;
		return *this;
	}

	bob& alert(const char* t, const char* m) {
		MessageBoxA(m_hwnd, m, t, NULL);
		return *this;
	}
public:
	int started = 0;
	std::vector<Object*> objects;
private:
	void create_drs_independent()
	{
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
	}

	void create_drs()
	{
		if (!m_pRenderTarget)
		{
			RECT rc;
			GetClientRect(m_hwnd, &rc);

			D2D1_SIZE_U size = D2D1::SizeU(
				rc.right - rc.left,
				rc.bottom - rc.top
			);

			m_pDirect2dFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(m_hwnd, size),
				&m_pRenderTarget
			);

		}
	}

	void discard_drs();

	void on_render()
	{
		create_drs();
		m_pRenderTarget->BeginDraw();
		m_pRenderTarget->Clear(D2D1::ColorF(bg.r / 255, bg.g / 255, bg.b / 255, 1));
		for (auto& o : objects) {
			o->m_pRenderTarget = m_pRenderTarget;
			o->render();
		}
		m_pRenderTarget->EndDraw();
	}

	void on_resize(int width, int height)
	{
		if (m_pRenderTarget)
		{
			m_pRenderTarget->Resize(D2D1::SizeU(width, height));
		}
	}

	static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = 0;

		if (message == WM_CREATE)
		{
			LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
			bob* app = (bob*)pcs->lpCreateParams;

			::SetWindowLongPtrW(
				hwnd,
				GWLP_USERDATA,
				reinterpret_cast<LONG_PTR>(app)
			);

			result = 1;
		}
		else
		{
			bob* app = reinterpret_cast<bob*>(static_cast<LONG_PTR>(
				::GetWindowLongPtrW(
					hwnd,
					GWLP_USERDATA
				)));

			bool wasHandled = false;

			if (app)
			{
				switch (message)
				{
				case WM_SIZE:
				{
					UINT width = LOWORD(lParam);
					UINT height = HIWORD(lParam);
					app->on_resize(width, height);
				}
				result = 0;
				wasHandled = true;
				break;

				case WM_DISPLAYCHANGE:
				{
					InvalidateRect(hwnd, NULL, FALSE);
				}
				result = 0;
				wasHandled = true;
				break;

				case WM_PAINT:
				{
					app->on_render();
					ValidateRect(hwnd, NULL);
				}
				result = 0;
				wasHandled = true;
				break;

				case WM_LBUTTONUP:
				{
					int x = ((int)(short)LOWORD(lParam));
					int y = ((int)(short)HIWORD(lParam));

					for (auto& i : app->objects) {
						if ((x > i->x) && (x < i->x + i->w) && (y > i->y) && (y < i->y + i->h)) {
							i->click(x, y);
						}
					}
					app->on_render();
				}
				result = 0;
				wasHandled = true;
				break;

				case WM_DESTROY:
				{
					PostQuitMessage(0);
				}
				result = 1;
				wasHandled = true;
				break;
				}
			}

			if (!wasHandled)
			{
				result = DefWindowProc(hwnd, message, wParam, lParam);
			}
		}

		return result;
	}
private:
	bob_color bg;
private:
	HWND m_hwnd;
	ID2D1Factory* m_pDirect2dFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
};