#pragma once

#include <windows.h>
#include <d2d1_3.h>
#include <d3d11_1.h>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <dwrite.h>
#include <dshow.h>
#include <wincodec.h>
#include <iostream>
#include <stdlib.h>
#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "strmiids")
#pragma comment(lib, "dxguid")

#ifndef WIN32
#error "Bob only works on Windows"
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

namespace bob {
	// Converts const char* to wchar_t*
	wchar_t* to_wchar(const char* title) {
		size_t s = strlen(title) + 1;
		wchar_t* n = new wchar_t[s];
		size_t o;
		mbstowcs_s(&o, n, s, title, s - 1);
		return n;
	}

	struct color {
		float r = 0.f;
		float g = 0.f;
		float b = 0.f;
		float a = 1.f;
	};

	// Converts hex color to bob::color with string hex
	color hex_to_color(std::string hex) {
		color c;
		if (hex[0] == '#') hex.erase(0, 1);
		if (hex.size() == 6) hex += "ff";
		if (hex.size() == 8) {
			c.r = (float)(std::stoi(hex.substr(0, 2), nullptr, 16));
			c.g = (float)(std::stoi(hex.substr(2, 2), nullptr, 16));
			c.b = (float)(std::stoi(hex.substr(4, 2), nullptr, 16));
			c.a = (float)(std::stoi(hex.substr(6, 2), nullptr, 16));
		}
		return c;
	}

	struct outline {
		float thickness = 0.f;
		color color;
	};

	// The base object class. All objects inherit this class
	class object {
	public:
		virtual void init() {};
		virtual void render() {};

		// Events
		void click(int x, int y) {
			for (auto& e : click_events) {
				e(x, y);
			}
		}

		// Unused
		void add_click(std::function<void(int x, int y)> e) {
			click_events.push_back(e);
		};
	public:
		std::string id;

		float x = 0;
		float y = 0;
		float w = 0;
		float h = 0;

		ID2D1DeviceContext* m_d2dContext = 0;
	private:
		std::vector<std::function<void(int x, int y)>> click_events;
	};

	// Audio class for playing MP3 files
	class audio : public object {
	public:
		void init() {
			if (!ready) {
				if (FAILED(CoInitialize(NULL))) {
					return;
				}

				if (FAILED(::CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
					IID_IGraphBuilder, (void**)&pGraph))) {
					return;
				}

				pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
				pGraph->QueryInterface(IID_IBasicAudio, (void**)&pAudio);
				pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);

				ready = true;
			}
		}

		void play() {
			if (!ready) init();

			pGraph->RenderFile(to_wchar(path.data()), NULL);
			pControl->Run();
		}

		void stop() {
			pControl->Stop();
		}

		void pause() {
			pControl->Pause();
		}

		void resume() {
			pControl->Run();
		}

		void volume(long volume) {
			long vol = static_cast<long>(volume * 100);
			pAudio->put_Volume(vol);
		}

		void speed(double spd) {
			pSeek->SetRate(spd);
		}
	public:
		std::string path;
	private:
		bool ready = false;
		IGraphBuilder* pGraph = 0;
		IMediaControl* pControl = 0;
		IMediaSeeking* pSeek = 0;
		IBasicAudio* pAudio = 0;
	};

	class rect : public object {
	public:
		void init() {
			// Create the brushes
			m_d2dContext->CreateSolidColorBrush(
				D2D1::ColorF(color.r / 255, color.g / 255, color.b / 255, color.a),
				&brush
			);

			m_d2dContext->CreateSolidColorBrush(
				D2D1::ColorF(outline.color.r / 255, outline.color.g / 255, outline.color.b / 255, outline.color.a),
				&outline_brush
			);

			started = true;
		};
		void render() {
			if (!started) init();

			rectangle = D2D1::RoundedRect(
				D2D1::RectF(x, y, w + x, h + y),
				radius,
				radius
			);

			if (aliased) {
				m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
			}
			else {
				m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			}

			// Change brush if color is different
			D2D1_COLOR_F brush_color = brush->GetColor();
			if (color.r != brush_color.r || color.g != brush_color.g || color.b != brush_color.b || color.a != brush_color.a) {
				m_d2dContext->CreateSolidColorBrush(
					D2D1::ColorF(color.r / 255, color.g / 255, color.b / 255, color.a),
					&brush
				);
			}

			// Change outline brush if color is different
			D2D1_COLOR_F outline_color = outline_brush->GetColor();
			if (outline.color.r != outline_color.r || outline.color.g != outline_color.g || outline.color.b != outline_color.b || outline.color.a != outline_color.a) {
				m_d2dContext->CreateSolidColorBrush(
					D2D1::ColorF(outline.color.r / 255, outline.color.g / 255, outline.color.b / 255, outline.color.a),
					&outline_brush
				);
			}

			if (brush && outline_brush) {
				// Check for rotations
				if (rotation) {
					if (rotate_center) m_d2dContext->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(x + (0.5f * w), y + (0.5f * h))));
					else m_d2dContext->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(x, y)));
				}

				m_d2dContext->DrawRoundedRectangle(
					rectangle,
					outline_brush,
					outline.thickness,
					NULL
				);

				// Fills in rectangle
				if (filled)
				{
					brush->SetOpacity(opacity);
					m_d2dContext->FillRoundedRectangle(
						rectangle,
						brush
					);
				}
			}
		};
	public:
		float radius = 0.f;
		float rotation = 0.f;
		float opacity = 1.f;
		color color;
		outline outline;
		bool aliased = true;
		bool filled = true;
		bool rotate_center = true;
	private:
		bool started = false;
		D2D1_ROUNDED_RECT rectangle = D2D1::RoundedRect(
			D2D1::RectF(x, y, w + x, h + y),
			radius,
			radius
		);
		ID2D1SolidColorBrush* brush = 0;
		ID2D1SolidColorBrush* outline_brush = 0;
	};

	class text : public object {
	public:
		void init() {
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(m_pDWriteFactory),
				reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
			);

			if (m_fontCollection == NULL) {
				m_pDWriteFactory->GetSystemFontCollection(&m_fontCollection);
			}

			m_pDWriteFactory->CreateTextFormat(
				to_wchar(font.data()),
				m_fontCollection,
				(DWRITE_FONT_WEIGHT)font_weight,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_SEMI_EXPANDED,
				px,
				L"",
				&m_pTextFormat
			);

			// Create the text's color brush
			m_d2dContext->CreateSolidColorBrush(
				D2D1::ColorF(color.r / 255, color.g / 255, color.b / 255, color.a),
				&brush
			);

			// Nobody wants aliased text...right?
			m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

			started = true;
		};
		virtual void render() {
			if (!started) init();

			if (centered) {
				m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			}
			else if (right_dir) {
				m_pTextFormat->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
			}

			// Create a new brush if the current brush doesn't match the current color
			D2D1_COLOR_F brush_color = brush->GetColor();
			if (color.r != brush_color.r || color.g != brush_color.g || color.b != brush_color.b || color.a != brush_color.a) {
				m_d2dContext->CreateSolidColorBrush(
					D2D1::ColorF(color.r / 255, color.g / 255, color.b / 255, color.a),
					&brush
				);
			}

			wchar_t* wtext = to_wchar(content.data());

			if (brush != NULL) {
				m_d2dContext->DrawText(
					wtext,
					static_cast<UINT32>(wcslen(wtext) + 1),
					m_pTextFormat,
					D2D1::RectF(x, y, w + x, h + y),
					brush
				);
			}
		};

		void add_font_file(const char* path) {
			AddFontResourceA(path);
			SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

			// Update the font collection
			if (m_pDWriteFactory != NULL) {
				m_pDWriteFactory->GetSystemFontCollection(&m_fontCollection, true);
			}
		}
	public:
		std::string font = "Gilroy";
		std::string content = "";
		float px = 32;
		int font_weight = 600;
		int font_spacing = 5;
		color color = { 255,255,255,1 };
		bool aliased = false;
		bool centered = false;
		bool right_dir = false;
	private:
		bool started = false;
		IDWriteTextFormat* m_pTextFormat = 0;
		IDWriteFactory* m_pDWriteFactory = NULL;
		IDWriteFontCollection* m_fontCollection = NULL;
		ID2D1SolidColorBrush* brush = 0;
	};

	class image : public object {
	public:
		void init() {
			if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))) {
				return;
			}

			if (FAILED(CoCreateInstance(
				CLSID_WICImagingFactory,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&pIWICFactory)
			))) {
				return;
			}

			if (path != "") {
				HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
					to_wchar(path),
					NULL,
					GENERIC_READ,
					WICDecodeMetadataCacheOnLoad,
					&pDecoder
				);

				if (SUCCEEDED(hr)) {
					pDecoder->GetFrame(0, &pSource);
					pIWICFactory->CreateFormatConverter(&pConverter);

					pConverter->Initialize(
						pSource,
						GUID_WICPixelFormat32bppPBGRA,
						WICBitmapDitherTypeNone,
						NULL,
						0.f,
						WICBitmapPaletteTypeMedianCut
					);

					m_d2dContext->CreateBitmapFromWicBitmap(
						pConverter,
						NULL,
						&pBitmap
					);

					pSource->Release();
					pConverter->Release();
				}
			}
		}

		virtual void render() {
			if (!pIWICFactory) {
				init(); // Initialize if not already done
			}

			if (pBitmap != NULL) {
				if (!w || !h) {
					D2D1_SIZE_F size = pBitmap->GetSize();
					w = size.width;
					h = size.height;
				}

				m_d2dContext->DrawBitmap(
					pBitmap,
					D2D1::RectF(x, y, x + w, y + h),
					opacity,
					D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC
				);
			}
		}

	public:
		const char* path = "";
		float opacity = 1.f;
		float blur = 0.f;
		color color;
		rect crop;

	private:
		ID2D1Bitmap* pBitmap = NULL;
		IWICImagingFactory* pIWICFactory = NULL;
		IWICBitmapDecoder* pDecoder = NULL;
		IWICBitmapFrameDecode* pSource = NULL;
		IWICFormatConverter* pConverter = NULL;
	};

	class app {
	public:
		app() :
			m_hwnd(NULL),
			m_pDirect2dFactory(NULL),
			m_d2dContext(NULL),
			m_pDirect2dDevice(NULL),
			m_pDirect3dContext(NULL),
			m_pDirect3dDevice(NULL),
			Direct2DBackBuffer(NULL),
			DXGISwapChain(NULL)
		{}

		app& init(const char* title, int width, int height, bool load_mode = 0)
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
			wcex.hIcon = LoadIcon(HINST_THISCOMPONENT, MAKEINTRESOURCE(101));

			RegisterClassEx(&wcex);

			wchar_t* n = to_wchar(title);

			int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
			int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

			auto flags = WS_POPUP | WS_SYSMENU;
			if (!load_mode) flags = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;

			RECT wr = { 0, 0, width, height };
			AdjustWindowRect(&wr, flags, FALSE);

			m_hwnd = CreateWindow(
				L"bob_application",
				n,
				flags,
				x,
				y,
				wr.right - wr.left,
				wr.bottom - wr.top,
				NULL,
				NULL,
				HINST_THISCOMPONENT,
				this);

			return *this;
		}

		void start()
		{
			if (!started) {
				started = true;

				ShowWindow(m_hwnd, SW_SHOWNORMAL);
				UpdateWindow(m_hwnd);

				// Initialize all objects before anything
				for (auto& o : objects) {
					o->m_d2dContext = m_d2dContext;
					o->init();
				}

				// Renders the first frame
				render();

				MSG msg = { 0 };

				while (GetMessage(&msg, NULL, 0, 0))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		bool stop() {
			return PostMessage(m_hwnd, WM_CLOSE, 0, 0);
		}

		app& add_obj(std::shared_ptr<object> obj) {
			objects.push_back(obj);
			return *this;
		}

		std::shared_ptr<object> get_obj(std::string id) {
			for (auto& o : objects) {
				if (o->id == id) return o;
			}
			return nullptr;
		}

		app& remove_obj(object& obj) {
			objects.erase(std::remove_if(objects.begin(), objects.end(), [&obj](const std::shared_ptr<object>& ptr) {
				return ptr.get() == &obj;
				}), objects.end());
			return *this;
		}

		app& set_bg(color c) {
			bg = c;
			return *this;
		}

		app& alert(const char* t, const char* m) {
			MessageBoxA(m_hwnd, m, t, NULL);
			return *this;
		}
	public:
		bool started = false;
		std::vector<std::shared_ptr<object>> objects;
	private:
		void create_drs_independent()
		{
			D2D1_FACTORY_OPTIONS options;
			ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

			D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory7), &options, reinterpret_cast<void**>(&m_pDirect2dFactory));
		}

		// Creates the main resources for Direct2D
		void create_drs()
		{
			if (!m_d2dContext)
			{
				RECT rc;
				GetClientRect(m_hwnd, &rc);

				D2D1_SIZE_U size = D2D1::SizeU(
					rc.right - rc.left,
					rc.bottom - rc.top
				);

				D3D_FEATURE_LEVEL featureLevels[] =
				{
					D3D_FEATURE_LEVEL_12_2,
					D3D_FEATURE_LEVEL_12_1,
					D3D_FEATURE_LEVEL_12_0,
					D3D_FEATURE_LEVEL_11_1,
					D3D_FEATURE_LEVEL_11_0,
					D3D_FEATURE_LEVEL_10_1,
					D3D_FEATURE_LEVEL_10_0,
					D3D_FEATURE_LEVEL_9_3,
					D3D_FEATURE_LEVEL_9_2,
					D3D_FEATURE_LEVEL_9_1
				};

				UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

				ID3D11Device* device;
				ID3D11DeviceContext* context;
				D3D_FEATURE_LEVEL returnedFeatureLevel;

				D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
					&device, &returnedFeatureLevel, &context);

				device->QueryInterface(__uuidof(ID3D11Device1), (void**)&m_pDirect3dDevice);
				context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&m_pDirect3dContext);

				IDXGIDevice* dxgiDevice;
				m_pDirect3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
				if (dxgiDevice) m_pDirect2dFactory->CreateDevice(dxgiDevice, &m_pDirect2dDevice);
				m_pDirect2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);

				IDXGIAdapter* dxgiAdapter;
				dxgiDevice->GetAdapter(&dxgiAdapter);

				IDXGIFactory2* dxgiFactory;
				dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

				DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

				swapChainDesc.Width = 0;
				swapChainDesc.Height = 0;
				swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				swapChainDesc.Stereo = false;
				swapChainDesc.SampleDesc.Count = 1;
				swapChainDesc.SampleDesc.Quality = 0;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 2;
				swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
				swapChainDesc.Flags = 0;

				if (m_pDirect3dDevice) dxgiFactory->CreateSwapChainForHwnd(m_pDirect3dDevice, m_hwnd, &swapChainDesc, nullptr, nullptr, &DXGISwapChain);

				IDXGISurface* dxgiBackBuffer;
				DXGISwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));

				D2D1_BITMAP_PROPERTIES1 bitmapProperties =
					D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
						D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96, 96);

				if (dxgiBackBuffer) m_d2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer, &bitmapProperties, &Direct2DBackBuffer);
				m_d2dContext->SetTarget(Direct2DBackBuffer);
			}
		}

		// Renders to the current window
		void render()
		{
			create_drs();

			RECT rc;
			GetClientRect(m_hwnd, &rc);

			m_d2dContext->BeginDraw();
			m_d2dContext->Clear(D2D1::ColorF(bg.r / 255, bg.g / 255, bg.b / 255, 1));

			// Render all objects in the list
			for (auto& o : objects) {
				o->m_d2dContext = m_d2dContext;
				o->render();
			}

			DXGI_PRESENT_PARAMETERS parameters = { 0 };
			parameters.DirtyRectsCount = 0;
			parameters.pDirtyRects = nullptr;
			parameters.pScrollRect = nullptr;
			parameters.pScrollOffset = nullptr;

			DXGISwapChain->Present1(1, 0, &parameters);

			m_d2dContext->EndDraw();
		}

		static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			LRESULT result = 0;

			if (message == WM_CREATE)
			{
				LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
				app* _app = (app*)pcs->lpCreateParams;

				::SetWindowLongPtrW(
					hwnd,
					GWLP_USERDATA,
					reinterpret_cast<LONG_PTR>(_app)
				);

				result = 1;
			}
			else
			{
				app* _app = reinterpret_cast<app*>(static_cast<LONG_PTR>(
					::GetWindowLongPtrW(
						hwnd,
						GWLP_USERDATA
					)));

				bool wasHandled = false;

				if (_app)
				{
					switch (message)
					{
					case WM_DISPLAYCHANGE:
					{
						InvalidateRect(hwnd, NULL, FALSE);
					}
					result = 0;
					wasHandled = true;
					break;

					case WM_PAINT:
					{
						_app->render();
						ValidateRect(hwnd, NULL);
					}
					result = 0;
					wasHandled = true;
					break;

					case WM_LBUTTONUP:
					{
						int x = ((int)(short)LOWORD(lParam));
						int y = ((int)(short)HIWORD(lParam));

						for (auto& i : _app->objects) {
							if ((x > i->x) && (x < i->x + i->w) && (y > i->y) && (y < i->y + i->h)) {
								i->click(x, y);

								// Update the window
								_app->render();
								SendMessageA(hwnd, WM_PAINT, 0, 0);
							}
						}
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
		color bg;
	private:
		HWND m_hwnd;

		ID3D11Device1* m_pDirect3dDevice;
		ID3D11DeviceContext1* m_pDirect3dContext;

		ID2D1Device6* m_pDirect2dDevice;
		ID2D1Factory7* m_pDirect2dFactory;
		ID2D1DeviceContext5* m_d2dContext;

		IDXGISwapChain1* DXGISwapChain;
		ID2D1Bitmap1* Direct2DBackBuffer;
	};
}
