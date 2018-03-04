#include "Win32App.h"
int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
	int nRet = -1;
	CWin32App app;
	if (app.Create(hPrevInstance))
		nRet = app.Run();
	return nRet;
}