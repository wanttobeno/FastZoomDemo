#pragma once
#include <tchar.h>
#include <Windows.h>

class CBmpReader
{
public:
	CBmpReader(void);
	~CBmpReader(void);
	bool Load(TCHAR* pFilePath);
	void* ReadBmpFile(TCHAR* pFilePath);
	void FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin);
	int LineByte();
	// pixels are aligned on a 16 bytes boundary
	int Pitch();
	// 位数
	int BPP();
	int Width();
	int Height();
	void* BmpBuf();
	void Destory();
private:
	void* ConvertToDIBRGBA();
private:
	byte*		m_pBmpBuf; //读入图像数据的指针
	int			m_nBitCount;
	int			m_nWidth;
	int			m_nHeight;
	int			m_nLineByte;
};
