#pragma once
#include "../obj.h"
class bob_outline {
public:
	float thickness = 0.f;
	bob_color color;
};
class bob_rect : public Object {
public:
	void init() {};
	virtual void render() {
		if (aliased) {
			m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		}
		else {
			m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		}

		D2D1_ROUNDED_RECT rect = D2D1::RoundedRect(
			D2D1::RectF(x, y, w+x, h+y),
			radius,
			radius
		);

		m_pRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(color.r / 255, color.g / 255, color.b / 255, color.a),
			&brush
		);
		m_pRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(outline.color.r, outline.color.g, outline.color.b, outline.color.a),
			&outline_brush
		);

		if (brush) {
			m_pRenderTarget->DrawRoundedRectangle(
				rect,
				brush,
				0,
				NULL
			);

			m_pRenderTarget->FillRoundedRectangle(rect, brush);
			
		}
	};
public:
	float radius = 0;
	bob_color color;
	bob_outline outline;
	bool aliased = true;
private:
	ID2D1SolidColorBrush* brush;
	ID2D1SolidColorBrush* outline_brush;
};