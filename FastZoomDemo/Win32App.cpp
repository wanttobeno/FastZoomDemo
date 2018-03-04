#include "Win32App.h"
#include <tchar.h>
#include <windows.h>
#include <ShellAPI.h>
#include <shlwapi.h>
#include <WindowsX.h>
#include <stdio.h>
#include "BmpReader.h"
#include "FastZoom.h"
#include "BmpHelper.h"
#include "resource.h"



#define WINDOW_CLASS    _T("Win32")
#define WINDOW_NAME     _T("Win32App")
#define WINDOW_WIDTH    CW_USEDEFAULT//640
#define WINDOW_HEIGHT   CW_USEDEFAULT//480


#define  ZY_TRACE(x)
#define WM_ZY_LOAD_DROP_FILE WM_USER+1


unsigned  int   g_nZoomFlag = 0;

#define  ZOOM_METHOD_COUNT 8


CWin32App	*g_pApp=NULL;
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return g_pApp->WinProc(hWnd,msg,wParam,lParam);
}

CWin32App::CWin32App(void)
{
	m_pCurFile = NULL;
	m_hbitmap = NULL;
	m_hWnd = NULL;
	g_pApp= this;
	m_bNeedFresh = true;
	m_blMouseDown = false;
	m_pBmpRead = new CBmpReader;
	m_pFastZoom = new CFastZoom;
}

CWin32App::~CWin32App(void)
{
	if (m_pBmpRead)
	{
		delete m_pBmpRead;
		m_pBmpRead = NULL;
	}
	if (m_pFastZoom)
	{
		delete m_pFastZoom;
		m_pFastZoom = NULL;
	}
	DeleteObject(m_hbitmap);
	m_bNeedFresh = false;
}

BOOL CWin32App::Run()
{
	// Show the window
	ShowWindow(m_hWnd, SW_SHOWDEFAULT);
	UpdateWindow(m_hWnd);

	// Enter the message loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while(msg.message != WM_QUIT)
	{
		if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			RenderScene();
	}
	Shutdown();
	return 0;
}

BOOL CWin32App::InitializeObjects()
{	
	GetClientRect(m_hWnd,&m_rtClient);
	m_MemDc = CreateCompatibleDC(0);
	return true;
}

void CWin32App::Shutdown()
{

}

void CWin32App::RenderScene()
{

}

BOOL CWin32App::Create(HINSTANCE hinst)
{
	// Register the window class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		WINDOW_CLASS, NULL };
	RegisterClassEx(&wc);

	// Create the application's window
	m_hWnd = CreateWindow(WINDOW_CLASS, WINDOW_NAME, WS_OVERLAPPEDWINDOW,
		100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, GetDesktopWindow(),
		NULL, wc.hInstance, NULL);

	BOOL bRet = IsWindow(m_hWnd);

	InitializeObjects();

	return bRet;
}

void CWin32App::RBtnBrowseFile(HWND hWnd)
{
	OPENFILENAME ofn;       // common dialog box structure
	TCHAR szFile[260] = { 0 };       // buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = _T('\0');
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("Bmp Files\0*.bmp\0All\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
		if (m_pCurFile)
			GlobalFreePtr(m_pCurFile);
		m_pCurFile = (LPTSTR)GlobalAllocPtr(GMEM_ZEROINIT, MAX_PATH);
		memcpy(m_pCurFile, ofn.lpstrFile, _tcslen(ofn.lpstrFile)*sizeof(TCHAR));
		::SendMessage(hWnd, WM_ZY_LOAD_DROP_FILE, 0, 0);
	}
}

void CWin32App::SendDropFilePath(WPARAM wParam, HWND hWnd)
{
	HDROP hDrop = (HDROP)wParam;

	// Don't accept more dropped files
	::DragAcceptFiles(hWnd, FALSE);

	// Get number of files dropped
	const int iFiles = ::DragQueryFile(hDrop, ~0U, NULL, 0);

	// Free previous file list (if allocated)
	if (m_pFileList)
		GlobalFreePtr(m_pFileList);

	// Allocate buffer to hold list of files
	m_pFileList = (LPTSTR)GlobalAllocPtr(GMEM_ZEROINIT, iFiles * MAX_PATH);
	if (m_pFileList)
	{
		LPTSTR pFile = m_pFileList;
		for (int iIndex = 0; iIndex < iFiles; ++iIndex)
		{
			// 获取拖入的文件名
			if (::DragQueryFile(hDrop, iIndex, pFile, MAX_PATH))
			{
				// Add terminating '\n' between files
				int iLength = lstrlen(pFile);
				pFile += (iLength + 1);
			}
		}
		*pFile++ = '\0';
		m_pCurFile = m_pFileList;
	}
	DragFinish(hDrop);
	::SendMessage(hWnd, WM_ZY_LOAD_DROP_FILE, 0, 0);
}

LRESULT  CWin32App::OpenDropFile(HWND hWnd)
{
	HRESULT hr = S_FALSE;
	bool bRet = m_pBmpRead->Load(m_pCurFile);
	if (bRet)
	{
		g_nZoomFlag = 0;
		m_bNeedFresh = true;
		::InvalidateRect(m_hWnd,NULL,FALSE);
		hr = S_OK;
	}
	return hr;
}

LRESULT WINAPI CWin32App::WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
		OnPaint(hWnd, msg, wParam, lParam);
		break;
	case  WM_LBUTTONDOWN:
		{
			g_nZoomFlag = (g_nZoomFlag +1)%ZOOM_METHOD_COUNT;
			::InvalidateRect(m_hWnd,NULL,FALSE);
		}
		break;	
	case WM_RBUTTONDOWN:
		RBtnBrowseFile(hWnd);
		break;
	case WM_DESTROY:
	{
		if (m_pFileList)
		{
			ZY_TRACE(_T("Free pszFileList!\n"));
			GlobalFreePtr(m_pFileList);
			m_pFileList = NULL;
		}
		PostQuitMessage(0);
	}
	break;
	case WM_CREATE:
		::DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_DROPFILES:
		SendDropFilePath(wParam, hWnd);
		break;
	case  WM_ZY_LOAD_DROP_FILE:
		return OpenDropFile(hWnd);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////

void CWin32App::OnPaint(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC dc = GetDC(hWnd);
	RECT rt;
	GetClientRect(hWnd,&rt);
	if (m_pBmpRead->BmpBuf() != NULL)
	{		
		// 更新画布大小
		if (rt.right-rt.left != m_rtClient.right-m_rtClient.left || rt.bottom-rt.top == m_rtClient.right - m_rtClient.left || m_bNeedFresh == true)
		{
			m_bNeedFresh = false;
			CopyRect(&m_rtClient,&rt);
			m_hbitmap = CreateCompatibleBitmap(dc,rt.right,rt.bottom);
			DeleteObject(SelectObject(m_MemDc,m_hbitmap));
		}

		HBRUSH br = CreateSolidBrush(RGB(128,128,128));
		SelectObject(m_MemDc,br);
		FillRect(m_MemDc,&rt,br);

		if (g_nZoomFlag == 0)
		{
			OutputDebugStringA("原图显示 ,");
			ZY_TIME_COUNT_START
			OnPaintOri(dc, rt);
			ZY_TIME_COUNT_END
		}
		else
			OpaintZoom(dc, rt);
		DeleteObject(br);
	}
	else
	{
		SetBkMode(dc,TRANSPARENT);
		FillRect(dc,&rt,CreateSolidBrush(RGB(128,128,128)));
		TCHAR *pBuf=_T("右键选择位图文件,单击切换缩放算法");
		TextOut(dc,100,100,pBuf,_tcslen(pBuf));
	}
	ReleaseDC(hWnd,dc);
}

void CWin32App::OnPaintOri(HDC dc, RECT &rt)
{
	BITMAPINFO bmInfo;
	m_pBmpRead->FillBitmapInfo(&bmInfo,m_pBmpRead->Width(),m_pBmpRead->Height(),
		m_pBmpRead->BPP(),1);

	// 原图显示
	SetDIBitsToDevice(m_MemDc,0, 0, m_pBmpRead->Width(),m_pBmpRead->Height(), 0, 0, 0,
		m_pBmpRead->Height(), m_pBmpRead->BmpBuf(),
		&bmInfo, DIB_RGB_COLORS);

	BitBlt(dc,0,0,rt.right,rt.bottom,m_MemDc,0,0,SRCCOPY);
}

void CWin32App::OpaintZoom(HDC dc, RECT &rt)
{
	int nNewWeight = m_pBmpRead->Width()/1.5;
	int nNewHeight = m_pBmpRead->Height()/1.5;

	BITMAPINFO bmInfo;
	m_pBmpRead->FillBitmapInfo(&bmInfo,nNewWeight,nNewHeight,
		m_pBmpRead->BPP(),1);

	TPicRegion picData ={0};
	picData.byte_width = m_pBmpRead->LineByte();
	picData.height = m_pBmpRead->Height();
	picData.width = m_pBmpRead->Width();
	picData.pdata = (TARGB32*)m_pBmpRead->BmpBuf();


	TPicRegion picNewData ={0};
	picNewData.byte_width =((nNewWeight) * m_pBmpRead->BPP() + 7) / 8;;
	picNewData.height = nNewHeight;
	picNewData.width = nNewWeight;
	void *pData = malloc((picNewData.byte_width)*(nNewHeight));
	memset(pData,0,(picNewData.byte_width)*(nNewHeight));
	picNewData.pdata =(TARGB32*)pData;


	ZY_TIME_COUNT_START;
	// 算法0~7的耗时
	//31 31 32 0 0 0 0 0
	switch(g_nZoomFlag)
	{
	case 1:
		m_pFastZoom->PicZoom0(picNewData,picData);
		break;
	case 2:
		m_pFastZoom->PicZoom1(picNewData,picData);
		break;
	case 3:
		m_pFastZoom->PicZoom2(picNewData,picData);
		break;
	case 4:
		m_pFastZoom->PicZoom3(picNewData,picData);
		break;
	case 5:
		m_pFastZoom->PicZoom4(picNewData,picData);
		break;
	case 6:
		m_pFastZoom->PicZoom5(picNewData,picData);
		break;
	case 7:
		m_pFastZoom->PicZoom6(picNewData,picData);
		break;
	case 8:
		m_pFastZoom->PicZoom7(picNewData,picData);
		break;
	default:
		m_pFastZoom->PicZoom0(picNewData,picData);
	}

	char szbuf[MAX_PATH] = "";
	sprintf(szbuf,"使用缩放算法%d ,",g_nZoomFlag);
	OutputDebugStringA(szbuf);
	ZY_TIME_COUNT_END;

	FillBitmapInfo(&bmInfo,picNewData.width,picNewData.height,32,1);
	SetDIBitsToDevice(m_MemDc,0, 0, picNewData.width,picNewData.height, 0, 0, 0,
		picNewData.height, picNewData.pdata,
		&bmInfo, DIB_RGB_COLORS);
	BitBlt(dc,0,0,rt.right,rt.bottom,m_MemDc,0,0,SRCCOPY);

	free(picNewData.pdata);
}

void CWin32App::SaveDcToFile(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);

	BITMAP bmp; 
	PBITMAPINFO pbmi;
	HBITMAP hBitmap = (HBITMAP)GetCurrentObject(hdc,OBJ_BITMAP);
	// Retrieve the bitmap color format, width, and height. 
	if (GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bmp)) 
	{
		//SaveHBitMap(hBitmap,_T("D:\\DcToBmp.bmp"));
		int n = 0;
	}
	// 保存测试
	//bool bRet2 = SaveUseGDIPlus(L"D:\\gdiplus.bmp",GDIP_SAVE_FORMAT::BMP,m_pBmpRead->Width(),
	//	m_pBmpRead->Height(),m_pBmpRead->BmpBuf());

	//bRet2 = SaveUseGDIPlus(L"D:\\gdiplus.png",GDIP_SAVE_FORMAT::PNG,m_pBmpRead->Width(),
	//	m_pBmpRead->Height(),m_pBmpRead->BmpBuf());

	//bool bRet = SaveBmpFile("d:\\bmpSave.bmp",m_pBmpRead->Width(),
	//	m_pBmpRead->Height(),m_pBmpRead->BmpBuf(),32,1);

	ReleaseDC(hWnd,hdc);
}
