#pragma once
#include "../obj.h"
#include <dwrite.h>
#pragma comment(lib, "Dwrite")

class bob_text : public Object {
public:
	void init() {
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(m_pDWriteFactory),
			reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
		);

		m_pDWriteFactory->CreateTextFormat(
			ex_bob_to_wchar(font),
			NULL,
			(DWRITE_FONT_WEIGHT)font_weight,
			DWRITE_FONT_STYLE_NORMAL,
			(DWRITE_FONT_STRETCH)font_spacing,
			px,
			L"",
			&m_pTextFormat
		);
	};
	virtual void render() {
		if (aliased) {
			m_pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
		}
		else {
			m_pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
		}

		if (centered) {
			m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		}


		m_pRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(color.r, color.g, color.b, color.a),
			&brush
		);

		wchar_t* wtext = ex_bob_to_wchar(text);

		D2D1_SIZE_F size = m_pRenderTarget->GetSize();

		m_pRenderTarget->DrawText(
			wtext,
			wcslen(wtext) + 1,
			m_pTextFormat,
			D2D1::RectF(x, y, w+x, h+y),
			brush
		);
	};

	void add_font_file(const char* path) {

	}
public:
	const char* font = "Calibri";
	const char* text = "";
	float px = 32;
	int font_weight = 400;
	int font_spacing = 5;
	bob_color color;
	bool aliased = false;
	bool centered = false;
private:
	IDWriteTextFormat* m_pTextFormat;
	IDWriteFactory* m_pDWriteFactory;
	ID2D1SolidColorBrush* brush;
};