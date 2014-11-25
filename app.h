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

#pragma once

#include "fgview.h"

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

#include <list>


class Application
{
  public:
    Application();
    ~Application();

    HRESULT Initialize(HINSTANCE, int);
    void RunMessageLoop();
    BOOL SetImageFile(LPCWSTR);

  private:
    static const WCHAR* CLASSNAME;

    HWND m_hwnd;
    ID2D1Factory* m_pDirect2dFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1SolidColorBrush* m_pCornflowerBlueBrush;
    IWICImagingFactory* m_pImagingFactory;
    IWICBitmapFrameDecode* m_pFrame;
    ID2D1Bitmap* m_pBitmap;
    IDWriteFactory* m_pWriteFactory;
    IDWriteTextFormat* m_pTextFormat;
    LPWSTR m_pImageFile;
    std::list<LPCWSTR>* m_pImageFileList;
    bool m_stretch;
    float m_multiplier;
    bool m_capture;
    bool m_usePos;
    POINT m_start;
    POINT m_pos;
    SIZE m_size;

    HRESULT Application::LoadImage();
    HRESULT CreateDeviceIndependentResources();
    HRESULT Application::CreateImageView();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();
    void SetupImageFileList();
    void ReleaseImageFileList();
    LPCWSTR Application::GetNextImageFile();
    LPCWSTR Application::GetPrevImageFile();
    void Application::ScaleUpImageView();
    void Application::ScaleDownImageView();
    HRESULT Render();
    void OnResize(UINT, UINT);
    bool Application::OnKeyDown(UINT, LPARAM);
    bool Application::OnLButtonDown(INT, INT);
    bool Application::OnLButtonUp(INT, INT);
    bool Application::OnMouseMove(INT, INT);
    bool Application::OnLButtonDblClk();
    bool Application::OnMouseWheel(INT);

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};
