// Copyright (c) 2014  NAKAMURA Usaku <usa@garbagecollect.jp>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "app.h"

#include <windowsx.h>
#include <shlwapi.h>


const WCHAR* Application::CLASSNAME = L"FGView";


Application::Application() :
    m_hwnd(0),
    m_pDirect2dFactory(0),
    m_pRenderTarget(0),
    m_pCornflowerBlueBrush(0),
    m_pImagingFactory(0),
    m_pFrame(0),
    m_pBitmap(0),
    m_pWriteFactory(0),
    m_pTextFormat(0),
    m_pImageFile(0),
    m_pImageFileList(0),
    m_stretch(true),
    m_multiplier(1.0f),
    m_capture(false),
    m_usePos(false)
{
}

Application::~Application()
{
    SafeRelease(&m_pDirect2dFactory);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pCornflowerBlueBrush);
    SafeRelease(&m_pImagingFactory);
    SafeRelease(&m_pFrame);
    SafeRelease(&m_pBitmap);
    SafeRelease(&m_pWriteFactory);
    SafeRelease(&m_pTextFormat);
    if (m_pImageFile) {
        delete[] m_pImageFile;
        m_pImageFile = 0;
    }
    ReleaseImageFileList();
}

HRESULT
Application::Initialize(HINSTANCE hInst, int nCmdShow)
{
    HRESULT hr = CreateDeviceIndependentResources();
    if (SUCCEEDED(hr)) {
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
        wcex.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcex.lpfnWndProc   = Application::WndProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = sizeof(LONG_PTR);
        wcex.hInstance     = hInst;
        wcex.hbrBackground = 0;
        wcex.lpszMenuName  = 0;
        wcex.hCursor       = LoadCursor(0, IDI_APPLICATION);
        wcex.lpszClassName = Application::CLASSNAME;
        RegisterClassEx(&wcex);

        FLOAT dpiX, dpiY;
        m_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);

        m_hwnd = CreateWindow(
            Application::CLASSNAME,
            L"FGView",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            static_cast<UINT>(ceil(816.f * dpiX / 96.f)),
            static_cast<UINT>(ceil(612.f * dpiY / 96.f)),
            0,
            0,
            hInst,
            this);
        hr = m_hwnd ? S_OK : E_FAIL;
        if (SUCCEEDED(hr)) {
            ShowWindow(m_hwnd, nCmdShow);
            UpdateWindow(m_hwnd);
        }
    }

    return hr;
}

HRESULT
Application::LoadImage()
{
    HRESULT hr = S_OK;

    if (m_pImageFile) {
        IWICBitmapDecoder* decoder;
        hr = m_pImagingFactory->CreateDecoderFromFilename(m_pImageFile, 0, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
        if (SUCCEEDED(hr)) {
            SafeRelease(&m_pFrame);
            hr = decoder->GetFrame(0, &m_pFrame);
        }
        SafeRelease(&decoder);
    }

    m_usePos = false;

    return hr;
}

HRESULT
Application::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
    if (SUCCEEDED(hr)) {
        hr = ::CoCreateInstance(CLSID_WICImagingFactory, 0, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&m_pImagingFactory));
    }

    if (SUCCEEDED(hr)) {
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pWriteFactory));
    }
    if (SUCCEEDED(hr)) {
        hr = m_pWriteFactory->CreateTextFormat(L"ƒƒCƒŠƒI", 0, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 48.0f, L"ja-jp", &m_pTextFormat);
    }

    if (SUCCEEDED(hr)) {
        hr = LoadImage();
    }

    return hr;
}


HRESULT
Application::CreateImageView()
{
    IWICFormatConverter* converter;
    HRESULT hr = m_pImagingFactory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) {
        hr = converter->Initialize(m_pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, 0, 0.0f, WICBitmapPaletteTypeMedianCut);
    }
    if (SUCCEEDED(hr)) {
        SafeRelease(&m_pBitmap);
        m_pRenderTarget->CreateBitmapFromWicBitmap(converter, 0, &m_pBitmap);
    }
    SafeRelease(&converter);

    return hr;
}

HRESULT
Application::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top);
        hr = m_pDirect2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_pRenderTarget);

        if (SUCCEEDED(hr)) {
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
                &m_pCornflowerBlueBrush);
        }

        if (SUCCEEDED(hr) && m_pFrame) {
            hr = CreateImageView();
        }
    }

    return hr;
}

void
Application::DiscardDeviceResources()
{
    SafeRelease(&m_pBitmap);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pCornflowerBlueBrush);
}

void
Application::RunMessageLoop()
{
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

BOOL
Application::SetImageFile(LPCWSTR pImageFile)
{
    if (m_pImageFile) {
        delete[] m_pImageFile;
        m_pImageFile = 0;
    }

    int len = lstrlen(pImageFile) + 1;
    m_pImageFile = new WCHAR[len];
    CopyMemory(m_pImageFile, pImageFile, sizeof(WCHAR) * len);

    HRESULT hr = S_OK;
    if (m_pImagingFactory) {
        hr = LoadImage();
        if (SUCCEEDED(hr) && m_pRenderTarget) {
            hr = CreateImageView();
            InvalidateRect(m_hwnd, 0, FALSE);

            WCHAR* buf = new WCHAR[lstrlen(m_pImageFile) + 10];
            wsprintf(buf, L"%s - FGView", m_pImageFile);
            SetWindowText(m_hwnd, buf);
            delete[] buf;
        }
    }

    return SUCCEEDED(hr);
}

void
Application::SetupImageFileList()
{
    ReleaseImageFileList();

    if (!m_pImageFile) {
        return;
    }

    int len = lstrlen(m_pImageFile) + 1 + 2;
    WCHAR* path = new WCHAR[len];
    CopyMemory(path, m_pImageFile, sizeof(WCHAR) * (len - 2));
    PathRemoveFileSpec(path);
    PathAppend(path, L"*");
    WIN32_FIND_DATA wfd;
    HANDLE h = FindFirstFile(path, &wfd);
    if (h != INVALID_HANDLE_VALUE) {
        PathRemoveFileSpec(path);
        BOOL cont;
        do {
            WCHAR* file = new WCHAR[lstrlen(path) + lstrlen(wfd.cFileName) + 2];
            PathCombine(file, path, wfd.cFileName);
            WCHAR* p = PathFindExtension(file);
            if (PathIsDirectory(file) || !*p || (
                lstrcmpi(p, L".png") != 0 &&
                lstrcmpi(p, L".jpg") != 0 &&
                lstrcmpi(p, L".bmp") != 0)) {
                delete[] file;
            }
            else {
                if (!m_pImageFileList) {
                    m_pImageFileList = new std::list<LPCWSTR>;
                }
                m_pImageFileList->push_back(file);
            }
            cont = FindNextFile(h, &wfd);
        } while (cont);
        FindClose(h);
    }
    delete path;
}

void
Application::ReleaseImageFileList()
{
    if (m_pImageFileList) {
        for (std::list<LPCWSTR>::iterator it = m_pImageFileList->begin(); it != m_pImageFileList->end(); ++it) {
            delete[] *it;
        }
        delete m_pImageFileList;
        m_pImageFileList = 0;
    }
}

LPCWSTR
Application::GetNextImageFile()
{
    if (!m_pImageFile) {
        return 0;
    }

    SetupImageFileList();
    if (!m_pImageFileList) {
        return 0;
    }

    bool found = false;
    for (std::list<LPCWSTR>::const_iterator it = m_pImageFileList->begin(); it != m_pImageFileList->end(); ++it) {
        if (found) {
            return *it;
        }
        if (lstrcmpi(*it, m_pImageFile) == 0) {
            found = true;
        }
    }
    if (found) {
        return *(m_pImageFileList->begin());
    }

    return 0;
}

LPCWSTR
Application::GetPrevImageFile()
{
    if (!m_pImageFile) {
        return 0;
    }

    SetupImageFileList();
    if (!m_pImageFileList) {
        return 0;
    }

    bool found = false;
    for (std::list<LPCWSTR>::const_reverse_iterator it = m_pImageFileList->rbegin(); it != m_pImageFileList->rend(); ++it) {
        if (found) {
            return *it;
        }
        if (lstrcmpi(*it, m_pImageFile) == 0) {
            found = true;
        }
    }
    if (found) {
        return *(m_pImageFileList->rbegin());
    }

    return 0;
}

void
Application::ScaleUpImageView()
{
    if (m_stretch) {
        m_stretch = false;
    }
    float m = 1.0f;
    if (m_multiplier >= 1.0f) {
        for (; m <= m_multiplier; m *= 2.0f)
            ;
    }
    else {
        for (; m > m_multiplier; m /= 2.0f)
            ;
        m *= 2.0f;
    }
    if (m_usePos) {
        POINT pos;
        GetCursorPos(&pos);
        ScreenToClient(m_hwnd, &pos);

        m_pos.x -= ((pos.x - m_pos.x) * m / m_multiplier) / 2.0f;
        m_pos.y -= ((pos.y - m_pos.y) * m / m_multiplier) / 2.0f;
    }
    m_multiplier = m;
    Render();
}

void
Application::ScaleDownImageView()
{
    if (m_stretch) {
        m_stretch = false;
    }
    float m = 1.0f;
    if (m_multiplier >= 1.0f) {
        for (; m < m_multiplier; m *= 2.0f)
            ;
        m /= 2.0f;
    }
    else {
        for (; m >= m_multiplier && m > 0.0f; m /= 2.0f)
            ;
    }
    if (m_usePos) {
        POINT pos;
        GetCursorPos(&pos);
        ScreenToClient(m_hwnd, &pos);

        m_pos.x += (pos.x - m_pos.x) * m / m_multiplier;
        m_pos.y += (pos.y - m_pos.y) * m / m_multiplier;
    }
    m_multiplier = m;
    Render();
}

HRESULT
Application::Render()
{
    HRESULT hr = CreateDeviceResources();
    if (SUCCEEDED(hr)) {
        m_pRenderTarget->BeginDraw();

        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Gainsboro));
        D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();

        if (m_pBitmap) {
            D2D1_SIZE_F rtBmp = m_pBitmap->GetSize();
            float x, y, w, h;
            if (m_stretch) {
                float bitmapAspect = rtBmp.width / rtBmp.height;
                float clientAspect = rtSize.width / rtSize.height;
                if (clientAspect >= bitmapAspect) {
                    h = rtSize.height;
                    w = h * bitmapAspect;
                    x = (rtSize.width - w) / 2.0f;
                    y = 0.0f;
                }
                else {
                    w = rtSize.width;
                    h = w / bitmapAspect;
                    x = 0.0f;
                    y = (rtSize.height - h) / 2.0f;
                }
                m_multiplier = w / rtBmp.width;
            }
            else {
                w = rtBmp.width * m_multiplier;
                h = rtBmp.height * m_multiplier;
                x = (rtSize.width - w) / 2.0f;
                y = (rtSize.height - h) / 2.0f;
            }
            if (m_usePos) {
                x = m_pos.x;
                y = m_pos.y;
                if (x >= rtSize.width) {
                    x = rtSize.width - 1;
                }
                if (y >= rtSize.height) {
                    y = rtSize.height - 1;
                }
                if (x + w <= 0) {
                    x = -w + 1;
                }
                if (y + h <= 0) {
                    y = -h + 1;
                }
            }
            m_pRenderTarget->DrawBitmap(m_pBitmap, D2D1::Rect<float>(x, y, x + w, y + h));
            m_pos.x = x;
            m_pos.y = y;
            m_size.cx = w;
            m_size.cy = h;

            IDWriteTextLayout *layout;
            WCHAR buf[128];
            _swprintf(buf, L"%.2f%%", m_multiplier * 100.0f);
            hr = m_pWriteFactory->CreateTextLayout(buf, lstrlen(buf), m_pTextFormat, rtSize.width, rtSize.height - 60.0f, &layout);
            if (SUCCEEDED(hr)) {
                DWRITE_TEXT_METRICS metrics;
                layout->GetMetrics(&metrics);
                m_pRenderTarget->DrawTextLayout(D2D1::Point2F((rtSize.width - metrics.width) / 2.0f, rtSize.height - 60.0f), layout, m_pCornflowerBlueBrush);
            }
        }

        hr = m_pRenderTarget->EndDraw();
        ValidateRect(m_hwnd, 0);
    }

    if (hr == D2DERR_RECREATE_TARGET) {
        hr = S_OK;
        DiscardDeviceResources();
    }

    return hr;
}

void
Application::OnResize(UINT width, UINT height)
{
    if (m_pRenderTarget) {
        m_pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
}

bool
Application::OnKeyDown(UINT vkey, LPARAM lParam)
{
    switch (vkey) {
      case VK_LEFT:
      case VK_RIGHT:
        for (int i = 0; i < LOWORD(lParam); i++) {
            LPCWSTR file = vkey == VK_LEFT ? GetPrevImageFile() : GetNextImageFile();
            if (file) {
                SetImageFile(file);
            }
            else {
                break;
            }
        }
        return true;

      case VK_ADD:
      case VK_PRIOR:
        ScaleUpImageView();
        return true;

      case VK_SUBTRACT:
      case VK_NEXT:
        ScaleDownImageView();
        return true;
    }

    return false;
}

bool
Application::OnLButtonDown(INT x, INT y)
{
    if (!m_pBitmap) {
        return false;
    }
    if (x < m_pos.x || x >= m_pos.x + m_size.cx ||
        y < m_pos.y || y >= m_pos.y + m_size.cy) {
        return false;
    }

    SetCapture(m_hwnd);
    m_capture = true;
    m_start.x = x;
    m_start.y = y;
    SetCursor(LoadCursor(0, IDC_SIZEALL));

    return true;
}

bool
Application::OnLButtonUp(INT x, INT y)
{
    if (m_capture) {
        ReleaseCapture();
        m_capture = false;
        SetCursor(LoadCursor(0, IDC_HAND));
    }

    return true;
}

bool
Application::OnMouseMove(INT x, INT y)
{
    if (!m_capture) {
        if (m_pBitmap &&
            x >= m_pos.x && x < m_pos.x + m_size.cx &&
            y >= m_pos.y && y < m_pos.y + m_size.cy) {
            // expect that LoadCursor and SetCursor do nothing and return
            // immediately if the cursor is already loaded and set.
            // (this expectation is based on the spec of these APIs.)
            SetCursor(LoadCursor(0, IDC_HAND));
        }
        else {
            SetCursor(LoadCursor(0, IDC_ARROW));
        }
        return false;
    }

    m_usePos = true;
    m_pos.x += x - m_start.x;
    m_pos.y += y - m_start.y;
    m_start.x = x;
    m_start.y = y;
    Render();

    return true;
}

bool
Application::OnLButtonDblClk()
{
    m_stretch = !m_stretch;
    if (!m_stretch) {
        m_multiplier = 1.0f;
    }
    m_usePos = false;
    Render();

    return true;
}

bool
Application::OnMouseWheel(INT delta)
{
    if (delta > 0) {
        ScaleUpImageView();
    }
    else {
        ScaleDownImageView();
    }
    return true;
}

LRESULT CALLBACK
Application::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Application *pApplication;
    LRESULT result = 0;
    bool handled = false;
    if (message == WM_CREATE ||
        (pApplication = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))) != 0) {
        switch (message) {
          case WM_CREATE:
            {
                LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
                pApplication = reinterpret_cast<Application*>(pcs->lpCreateParams);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApplication));
            }
            result = 1;
            handled = true;
            break;

          case WM_SIZE:
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                pApplication->OnResize(width, height);
            }
            result = 0;
            handled = true;
            break;

          case WM_DISPLAYCHANGE:
            InvalidateRect(hwnd, 0, FALSE);
            result = 0;
            handled = true;
            break;

          case WM_PAINT:
            pApplication->Render();
            result = 0;
            handled = true;
            break;

          case WM_KEYDOWN:
            result = 0;
            handled = pApplication->OnKeyDown((UINT)wParam, lParam);
            break;

          case WM_LBUTTONDOWN:
            result = 0;
            handled = pApplication->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

          case WM_LBUTTONUP:
            result = 0;
            handled = pApplication->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

          case WM_MOUSEMOVE:
            result = 0;
            handled = pApplication->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

          case WM_LBUTTONDBLCLK:
            result = 0;
            handled = pApplication->OnLButtonDblClk();
            break;

          case WM_MOUSEWHEEL:
            result = 0;
            handled = pApplication->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            break;

          case WM_DESTROY:
            PostQuitMessage(0);
            result = 1;
            handled = true;
            break;
        }
    }

    if (!handled) {
        result = DefWindowProc(hwnd, message, wParam, lParam);
    }

    return result;
}
