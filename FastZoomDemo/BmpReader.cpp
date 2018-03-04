#include "BmpReader.h"
#include <stdio.h>
#include <assert.h>
#include "BmpHelper.h"


CBmpReader::CBmpReader(void)
{
	m_pBmpBuf = NULL;
	this->Destory();
}

CBmpReader::~CBmpReader(void)
{
	this->Destory();
}

void CBmpReader::Destory()
{
	if (m_pBmpBuf)
	{
		delete[] m_pBmpBuf;
		m_pBmpBuf = NULL;
	}
	m_nBitCount = 0;
	m_nLineByte = 0;
	m_nWidth = 0;
	m_nHeight = 0;
}

void CBmpReader::FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin)
{
	assert(bmi && width >= 0 && height >= 0 && (bpp == 8 || bpp == 24 || bpp == 32));

	BITMAPINFOHEADER* bmih = &(bmi->bmiHeader);

	memset(bmih, 0, sizeof(*bmih));
	bmih->biSize = sizeof(BITMAPINFOHEADER);
	bmih->biWidth = width;
	bmih->biHeight = origin ? abs(height) : -abs(height);
	bmih->biPlanes = 1;
	bmih->biBitCount = (unsigned short)bpp;
	bmih->biCompression = BI_RGB;

	if (bpp == 8)
	{
		RGBQUAD* palette = bmi->bmiColors;
		int i;
		for (i = 0; i < 256; i++)
		{
			palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
			palette[i].rgbReserved = 0;
		}
	}
}

int CBmpReader::LineByte()
{
	int nRet = 0;
	if (m_pBmpBuf)
		nRet = (m_nWidth * 32 + 7) / 8;
	return nRet;
}

int CBmpReader::Pitch()
{
	int nRet = 0;
	if (m_pBmpBuf)
		nRet = this->LineByte()+ 3 & ~3;
	return nRet;
}

int CBmpReader::BPP()
{
	return 32;//m_nBitCount;
}

int CBmpReader::Width()
{
	return m_nWidth;
}

int CBmpReader::Height()
{
	return m_nHeight;
}

void* CBmpReader::BmpBuf()
{
	return	m_pBmpBuf;
}

bool CBmpReader::Load(TCHAR* pFilePath)
{
	this->Destory();
	if (this->ReadBmpFile(pFilePath) == NULL)
		return false;
	return  true;
}

// 位图转换为32位
void* CBmpReader::ConvertToDIBRGBA()
{
	if (m_pBmpBuf == NULL) {
		return NULL;
	}
	int nWidth = m_nWidth;
	int nHeight = m_nHeight;
	
	switch(m_nBitCount)
	{
	case 1:
		return Convert1To4Channels(nWidth,nHeight,m_pBmpBuf);
	case 4:
		return Convert16To4Channels(nWidth,nHeight,m_pBmpBuf);
	case 8:
		return Convert8bppTo32bppDIB(nWidth,nHeight,m_pBmpBuf);
	case 24:
		return Convert24DibTo32Dib(nWidth,nHeight,m_pBmpBuf);
	default:
		break;
	}
	return NULL;
}

void* CBmpReader::ReadBmpFile(TCHAR* pFilePath)
{
	this->Destory();
	RGBQUAD *g_pColorTable;//颜色表指针

	FILE* pFile = _tfopen(pFilePath, _T("rb"));
	if (pFile == NULL)
		return NULL;

	fseek(pFile, 0, SEEK_END);
	int nLen = ftell(pFile) + 1024;
	fseek(pFile, 0, SEEK_SET);

	// 跳过位图头结构
	fseek(pFile,sizeof(BITMAPFILEHEADER),0);

	// 定义位图信息头结构变量，读取位图信息头进内存，存放到变量head中
	BITMAPINFOHEADER head;
	fread(&head,sizeof(BITMAPINFOHEADER),1,pFile);

	m_nBitCount = head.biBitCount; // 定义变量，计算图像每行像素所占的字节数，必须是4的倍数

	int lineByte = (m_nBitCount * m_nWidth + 7)/8;// 灰度图像有颜色表，且颜色表表项为256

	if(m_nBitCount == 8)
	{
		// 申请颜色表所需要的空间，读颜色表进内存
		g_pColorTable = new RGBQUAD[256];
		fread(g_pColorTable,sizeof(RGBQUAD),256,pFile);
	}


	// DIBs are normally stored flipped vertically (meaning they are stored bottom-up)
	bool bFlipped;
	if (head.biHeight < 0) {
		head.biHeight = -head.biHeight;
		bFlipped = true;
	} else {
		bFlipped = false;
	}

	// 初始化变量
	m_nHeight = head.biHeight;
	m_nWidth = head.biWidth;

	int bytesPerPixel = head.biBitCount/8;
	int paddedWidth = DoPadding(head.biWidth*bytesPerPixel, 4);
	int fileSizeBytes = head.biHeight*paddedWidth;
	// 申请位图数据所需要的控件，读位图数据进内存
	uint8 *pDest = new uint8[fileSizeBytes];

	//fread(pDest,1,lineByte*m_nHeight,pFile);
	//fclose(pFile); // 关闭文件

	uint8* pStart = bFlipped ? pDest + paddedWidth*(head.biHeight-1) : pDest;
	for (int nLine = 0; nLine < head.biHeight; nLine++) {
		if (paddedWidth != fread(pStart, 1, paddedWidth, pFile)) {
			delete[] pDest;
			fclose(pFile);
			return NULL;
		}
		pStart = pStart + (bFlipped ? -paddedWidth : paddedWidth);
	}
	fclose(pFile);


	m_pBmpBuf = pDest;

	if (m_nBitCount != 32)
	{
		void *p32Buf = ConvertToDIBRGBA();
		delete [] m_pBmpBuf;
		m_pBmpBuf = (byte*)p32Buf;
	}
	return m_pBmpBuf;
}