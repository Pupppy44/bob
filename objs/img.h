#pragma once
#include <wincodec.h>
#include <d2d1_1.h>
#include "../obj.h"
#pragma comment(lib, "dxguid")
class bob_image : public Object {
public:
	void init() {};
	virtual void render() {
		IWICBitmapDecoder* pDecoder = NULL;
		IWICBitmapFrameDecode* pSource = NULL;
		IWICStream* pStream = NULL;
		IWICFormatConverter* pConverter = NULL;
		IWICBitmapScaler* pScaler = NULL;

		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pIWICFactory)
		);

		HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
			ex_bob_to_wchar(path),
			NULL,
			GENERIC_READ,
			WICDecodeMetadataCacheOnLoad,
			&pDecoder
		);

		if (FAILED(hr)) return;

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

		m_pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			NULL,
			&pBitmap
		);

		D2D1_SIZE_F size = pBitmap->GetSize();
		if (!w || !h) {
			w = size.width;
			h = size.height;
		}

		if (!crop.w || !crop.h) {
			m_pRenderTarget->DrawBitmap(
				pBitmap,
				D2D1::RectF(x, y, x + w, y + h),
				opacity
			);
		}
		else {
			m_pRenderTarget->DrawBitmap(
				pBitmap,
				D2D1::RectF(x, y, x + w, y + h),
				opacity,
				D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
				D2D1::RectF(crop.x, crop.y, crop.w+x, crop.h+y)
			);
		}
	};
public:
	const char* path = "";
	float opacity = 1.f;
	bob_color color;
	bob_rect crop;
private:
	ID2D1Bitmap* pBitmap;
	IWICImagingFactory* pIWICFactory = NULL;
};