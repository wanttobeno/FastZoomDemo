#pragma once
#include <Windows.h>

class CBmpReader;
class CFastZoom;

class CWin32App
{
public:
	CWin32App(void);
	~CWin32App(void);
public:
	BOOL		Create(HINSTANCE hinst);
	BOOL		Run();
	BOOL		InitializeObjects();
	void		RenderScene();
	void		Shutdown();
protected:
	void		OnPaint(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void		OpaintZoom(HDC dc, RECT &rt);
	void		OnPaintOri(HDC dc, RECT &rt);
private:	
	void		RBtnBrowseFile(HWND hWnd);
	void		SendDropFilePath(WPARAM wParam, HWND hWnd);
	LRESULT		OpenDropFile(HWND hWnd);
public:
	LRESULT WINAPI WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void SaveDcToFile(HWND hWnd);

private:
	CBmpReader*		m_pBmpRead;
	CFastZoom*		m_pFastZoom;
private:
	HWND		m_hWnd;
	bool		m_blMouseDown;
	LPTSTR		m_pFileList;
	LPTSTR		m_pCurFile;
private:
	// 避免DC和位图重复创建
	RECT		m_rtClient;
	HDC			m_MemDc;
	HBITMAP		m_hbitmap;
	HGDIOBJ		m_Old;
	bool		m_bNeedFresh;
};
