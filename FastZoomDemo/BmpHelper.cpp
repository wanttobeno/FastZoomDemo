#include "BmpHelper.h"
#include <stdio.h>
#include <tchar.h>
#include <assert.h>

#define ALPHA_OPAQUE 0xFF000000

// 参数5:1（单色）,4（16色）,8（256色）,24,32
HBITMAP MakeBitmap(HDC hDc, LPBYTE lpBits, long lWidth, long lHeight, WORD wBitCount)
{
	BITMAPINFO bitinfo;
	memset(&bitinfo, 0, sizeof(BITMAPINFO));

	bitinfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	bitinfo.bmiHeader.biWidth         = lWidth;     
	bitinfo.bmiHeader.biHeight        = lHeight;   
	bitinfo.bmiHeader.biPlanes        = 1;  
	bitinfo.bmiHeader.biBitCount      = wBitCount;   
	bitinfo.bmiHeader.biCompression   = BI_RGB;   
	bitinfo.bmiHeader.biSizeImage     = lWidth*lHeight*(wBitCount/8);  
	bitinfo.bmiHeader.biXPelsPerMeter = 96;
	bitinfo.bmiHeader.biYPelsPerMeter = 96;   
	bitinfo.bmiHeader.biClrUsed       = 0;   
	bitinfo.bmiHeader.biClrImportant  = 0;

	return CreateDIBitmap(hDc, &bitinfo.bmiHeader, CBM_INIT, lpBits, &bitinfo, DIB_RGB_COLORS);
}

// pImageData即是已知的图像裸数据。
HBITMAP CreateBitmap(int nWidth,INT nHeight,int nBpp,void * pImageData)
{
	BITMAPINFO bmpInfo; //创建位图 
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = nWidth;//宽度
	bmpInfo.bmiHeader.biHeight = nHeight;//高度
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = nBpp;
	bmpInfo.bmiHeader.biCompression = BI_RGB;

	UINT uiTotalBytes = nWidth * nHeight * (nBpp/8);
	void *pArray = new BYTE(uiTotalBytes );
	HBITMAP hbmp = CreateDIBSection(NULL, &bmpInfo, DIB_RGB_COLORS, &pArray, NULL, 0);//创建DIB
	assert(hbmp != NULL);
	//! 将裸数据复制到bitmap关联的像素区域
	memcpy(pArray, pImageData, uiTotalBytes);
	return hbmp;
}


void FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin)
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


void* Convert1To4Channels(int nWidth, int nHeight, void* pPixels)
{
	if (pPixels == NULL) return NULL;
	int nPadSrc = DoPadding(nWidth, 4) - nWidth;
	uint32* pNewDIB = new uint32[nWidth * nHeight];
	memset(pNewDIB,0x00,nWidth*nHeight);

	if (pNewDIB == NULL) return NULL;
	uint32* pTarget = pNewDIB;
	const uint8* pSource = (uint8*)pPixels;
	for (int j = 0; j < nHeight; j++) {
		for (int i = 0; i < nWidth; i++) {
			//*pTarget++ = 0xFFFF0000;
			*pTarget++ = *pSource + *pSource * 256 + *pSource * 65536 + ALPHA_OPAQUE;
			pSource++;
		}
		pSource += nPadSrc;
	}
	return pNewDIB;
}

void* Convert16To4Channels(int nWidth, int nHeight, void* pPixels)
{
	if (pPixels == NULL) return NULL;

	uint32* pNewDIB = new uint32[nWidth * nHeight];
	if (pNewDIB == NULL) return NULL;
	memset(pNewDIB,0x00,nWidth*nHeight*sizeof(uint32));

	uint32* pTarget = pNewDIB;
	const int16* pSource = (int16*)pPixels;
	for (int j = 0; j < nHeight; j++) {
		for (int i = 0; i < nWidth; i++) {
			//UNDONE
			*pTarget++ = 0xFFFF0000;
		}
	}
	return pNewDIB;

}

void* Convert8bppTo32bppDIB(int nWidth, int nHeight, void* pPixels)
{
	if (pPixels == NULL) return NULL;
	int nPadSrc = DoPadding(nWidth, 4) - nWidth;
	uint32* pNewDIB = new uint32[nWidth * nHeight];
	if (pNewDIB == NULL) return NULL;
	memset(pNewDIB,0xff,nWidth*nHeight*sizeof(uint32));
	uint32* pTarget = pNewDIB;
	const uint8* pSource = (uint8*)pPixels;
	for (int j = 0; j < nHeight; j++) {
		for (int i = 0; i < nWidth; i++) {
			// undone 
			//
			*pTarget++ = 0xFFFF0000;
		}
		pSource += nPadSrc;
	}
	return pNewDIB;
}

void* Convert24DibTo32Dib(int nWidth, int nHeight, void* p24DIB)
{
	uint32 nSizeBytes = nWidth* nHeight * 4;
	uint8* pDIB32bpp = new uint8[nSizeBytes];
	if (pDIB32bpp)
	{
		memset(pDIB32bpp,0xff,nSizeBytes);

		BYTE* pTargetDIB = (BYTE*)pDIB32bpp;
		BYTE* pSourceDIB = (BYTE*)p24DIB;

		// 24转32需要padding下
		int nPadSrc = DoPadding(nWidth*3, 4) - nWidth*3;

		ZY_TIME_COUNT_START;

		int nPos24 = 0;
		int nPos32 = 0;
		for (int j = 0; j < nHeight; j++)  // 列
		{
			//pSourceDIB = (BYTE*)m_pBmpBuf + j*nWidth*3;// 行首

			for (int i = 0; i < nWidth; i++) // 行
			{
				nPos32 = j*nWidth*4 + i*4;
				pTargetDIB[nPos32] = pSourceDIB[0];			// B
				pTargetDIB[nPos32+1] = pSourceDIB[1];		// G
				pTargetDIB[nPos32+2] = pSourceDIB[2];		// R
				pTargetDIB[nPos32+3] = 0xFF;
				pSourceDIB +=3;
			}
			pSourceDIB += nPadSrc; // 加上宽度的偏移量
		}	
		// 算法二：
		//uint32* pTarget = (uint32*)pDIB32bpp;
		//const uint8* pSource = (uint8*)m_pBmpBuf;
		//for (int j = 0; j < nHeight; j++) {
		//	for (int i = 0; i < nWidth; i++) {
		//		*pTarget++ = pSource[0] + pSource[1] * 256 + pSource[2] * 65536 + ALPHA_OPAQUE;
		//		pSource += 3;
		//	}
		//	pSource += nPadSrc;
		//}

	}
	return pDIB32bpp;
}

void Convert32bppTo24bppDIB(int nWidth, int nHeight, void* pDIBTarget, 
							const void* pDIBSource, bool bFlip) 
{
	if (pDIBTarget == NULL || pDIBSource == NULL) {
		return;
	}
	int nPaddedWidth = DoPadding(nWidth*3, 4);
	int nPad = nPaddedWidth - nWidth*3;
	uint8* pTargetDIB = (uint8*)pDIBTarget;
	for (int j = 0; j < nHeight; j++) {
		const uint8* pSourceDIB;
		if (bFlip) {
			pSourceDIB = (uint8*)pDIBSource + (nHeight - 1 - j)*nWidth*4;
		} else {
			pSourceDIB = (uint8*)pDIBSource + j*nWidth*4;
		}
		for (int i = 0; i < nWidth; i++) {
			*pTargetDIB++ = pSourceDIB[0];
			*pTargetDIB++ = pSourceDIB[1];
			*pTargetDIB++ = pSourceDIB[2];
			pSourceDIB += 4;
		}
		pTargetDIB += nPad;
	}
}

#include <GdiPlus.h>
#pragma comment(lib,"gdiplus.lib")

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j) {
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 ) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}    
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

ULONG_PTR m_gdiplusToken;  
bool SaveGDIPlus24(LPCTSTR sFileName, GDIP_SAVE_FORMAT nSaveFormat,int nWidth, int nHeight, void* pData,GDIP_RotateFlipType nFlip) 
{
	using namespace Gdiplus;
	Gdiplus::GdiplusStartupInput StartupInput;  
	GdiplusStartup(&m_gdiplusToken,&StartupInput,NULL);  

	Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(nWidth, nHeight,DoPadding(nWidth*3, 4), PixelFormat24bppRGB, (BYTE*)pData);
	if (pBitmap->GetLastStatus() != Gdiplus::Ok) {
		delete pBitmap;
		return false;
	}

	Gdiplus::RotateFlipType rotaType= RotateNoneFlipNone;
	switch(nFlip)
	{
	case GDIP_RotateNoneFlipNone:
		rotaType = RotateNoneFlipNone;
		break;
	case GDIP_Rotate90FlipNone:
		rotaType = Rotate90FlipNone;
		break;
	case GDIP_Rotate180FlipNone:
		rotaType = Rotate180FlipNone;
		break;
	case GDIP_Rotate270FlipNone:
		rotaType = Rotate270FlipNone;
		break;
	case GDIP_RotateNoneFlipX:
		rotaType = RotateNoneFlipX;
		break;
	case GDIP_Rotate90FlipX:
		rotaType = Rotate90FlipX;
		break;
	case GDIP_Rotate180FlipX:
		rotaType = Rotate180FlipX;
		break;
	case GDIP_Rotate270FlipX:
		rotaType = Rotate270FlipX;
		break;
	default:
		break;
	}

	//pBitmap->RotateFlip(rotaType);

	const wchar_t* sMIMEType = NULL;
	switch (nSaveFormat) {
		case GDIP_SAVE_FORMAT::BMP:
			sMIMEType = L"image/bmp";
			break;
		case GDIP_SAVE_FORMAT::PNG:
			sMIMEType = L"image/png";
			break;
		case GDIP_SAVE_FORMAT::TIFF:
			sMIMEType = L"image/tiff";
			break;
		default:
			sMIMEType = L"image/png";
	}

	CLSID encoderClsid;
	int result = (sMIMEType == NULL) ? -1 : GetEncoderClsid(sMIMEType, &encoderClsid);
	if (result < 0) {
		delete pBitmap;
		return false;
	}

	const wchar_t* sFileNameUnicode;
	sFileNameUnicode = (const wchar_t*)sFileName;

	bool bOk = pBitmap->Save(sFileNameUnicode, &encoderClsid, NULL) == Gdiplus::Ok;

	delete pBitmap;
	Gdiplus::GdiplusShutdown(m_gdiplusToken);  
	return bOk;
}

bool SaveUseGDIPlus(LPCTSTR sFileName, GDIP_SAVE_FORMAT nSaveFormat, int nWidth, int nHeight, void* pData,int nBpp,GDIP_RotateFlipType nFlip) 
{
	bool bRet = false;
	if (pData == NULL)
		return bRet;
	if (nBpp == 32)
	{

		uint32 nSizeLinePadded = DoPadding(nWidth*3, 4);
		uint32 nSizeBytes = nSizeLinePadded*nHeight;
		uint8* pDIB24bpp = new uint8[nSizeBytes];
		if (pDIB24bpp)
		{
			Convert32bppTo24bppDIB(nWidth,nHeight,pDIB24bpp,pData,1);
			bRet = SaveGDIPlus24(sFileName,nSaveFormat,nWidth,nHeight,pDIB24bpp,nFlip);
			delete [] pDIB24bpp;
		}
	}
	else
	{
		bRet = SaveGDIPlus24(sFileName,nSaveFormat,nWidth,nHeight,pData,nFlip);
	}
	return bRet;
}


static bool SaveBmp24File(const char* pSaveFile,int nWidth,int nHeight,void *pDibData)
{

	FILE* fpFile =NULL;
	try
	{
		if (0 == pDibData)
		{
			return false;
		}
		int nBMPType = 19778; // BM
		int nOffsetHeader = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER);
		int nFileSize = (nHeight * nWidth) + nOffsetHeader;

		// Fill the Bitmap File header.
		BITMAPFILEHEADER stBitmapFileHeader;
		stBitmapFileHeader.bfType = nBMPType;
		stBitmapFileHeader.bfOffBits = nOffsetHeader;
		stBitmapFileHeader.bfReserved1 = 0;
		stBitmapFileHeader.bfSize = nFileSize;
		stBitmapFileHeader.bfReserved2 = 0;

		int nWrappedWidth = nWidth;
		// Fill the Bitmap Info header.
		BITMAPINFO stBitmapInfo;
		stBitmapInfo.bmiHeader.biClrImportant = 0;
		stBitmapInfo.bmiHeader.biClrUsed = 0;
		stBitmapInfo.bmiHeader.biCompression = 0;
		stBitmapInfo.bmiHeader.biPlanes = 1;
		stBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		stBitmapInfo.bmiHeader.biXPelsPerMeter = 0;
		stBitmapInfo.bmiHeader.biYPelsPerMeter = 0;
		stBitmapInfo.bmiHeader.biBitCount = 3 * 8; // 24 位的bmp
		stBitmapInfo.bmiHeader.biSizeImage = nHeight * nWidth;
		stBitmapInfo.bmiHeader.biHeight = nHeight;
		stBitmapInfo.bmiHeader.biWidth = nWidth;
		stBitmapInfo.bmiColors[0].rgbBlue = 0x0;
		stBitmapInfo.bmiColors[0].rgbGreen = 0x0;
		stBitmapInfo.bmiColors[0].rgbRed = 0x0;
		stBitmapInfo.bmiColors[0].rgbReserved = 0x0;

		//_wfopen_s(&fpFile, pFileName_i, L"wb");
		FILE*fpFile = fopen(pSaveFile,"wb");
		fwrite(&stBitmapFileHeader, 1, sizeof(BITMAPFILEHEADER), fpFile);
		fwrite(&stBitmapInfo, 1, sizeof(BITMAPINFOHEADER), fpFile);
		fwrite(pDibData, 1, (nHeight * nWidth * 3), fpFile);
		fclose(fpFile);
		return true;
	}
	catch (...)
	{
		if (fpFile)
			fclose(fpFile);
		return false;
	}

	/*bool bRet = false;
	if (!pDibData) return bRet;

	FILE*pFile = fopen(pSaveFile,"wb");
	if (pFile)
	{
	BITMAPFILEHEADER bitmapfileheader;
	bitmapfileheader.bfType = 0x4D42;
	bitmapfileheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 0 * sizeof(RGBQUAD);
	bitmapfileheader.bfReserved1 = 0;
	bitmapfileheader.bfReserved2 = 0;
	bitmapfileheader.bfSize = bitmapfileheader.bfOffBits + nWidth*nHeight * 3;


	BITMAPINFOHEADER bmiHeader={0};
	memset(&bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader.biWidth = nWidth;
	bmiHeader.biHeight = nHeight;
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 24;
	bmiHeader.biCompression = BI_RGB;
	bmiHeader.biSizeImage = ((nWidth * 24 +31) & ~31) /8
	* nHeight;
	bmiHeader.biXPelsPerMeter = 2834;
	bmiHeader.biYPelsPerMeter = 2834;


	fwrite(&bitmapfileheader,1,sizeof(BITMAPFILEHEADER),pFile);
	fwrite(&bmiHeader,1,sizeof(BITMAPINFOHEADER),pFile);
	int nSize = nWidth*nHeight*3;
	int nHaveWrite = 0;
	nHaveWrite =fwrite(pDibData,1,nSize,pFile);
	fclose(pFile);
	if (nSize == nHaveWrite)
	bRet = true;
	}
	return bRet;*/
}

bool SaveBmpFile(const char* pSaveFile,int nWidth,int nHeight,void *pDibData,int nBpp,bool bFlip)
{
	bool bRet = false;
	switch(nBpp)
	{
	case 24:
		bRet = SaveBmp24File(pSaveFile,nWidth,nHeight,pDibData);
		break;
	case 32:
		{
			uint32 nSizeLinePadded = DoPadding(nWidth*3, 4);
			uint32 nSizeBytes = nSizeLinePadded*nHeight;
			uint8* pDIB24bpp = new uint8[nSizeBytes];
			if(pDIB24bpp)
			{
				memset(pDIB24bpp,0,nSizeBytes);
				Convert32bppTo24bppDIB(nWidth,nHeight,pDIB24bpp,pDibData,false);
				if (pDIB24bpp)
					bRet = SaveBmp24File(pSaveFile,nWidth,nHeight,pDIB24bpp);
				delete[] pDIB24bpp;
			}
		}
		break;
	default:
		break;
	}
	return bRet;
}

#define errhandler(a,b)
//Form MSDN  Storing an Image. 
PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp)
{ 
	BITMAP bmp; 
	PBITMAPINFO pbmi; 
	WORD    cClrBits; 

	// Retrieve the bitmap color format, width, and height. 
	if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) 
		errhandler("GetObject", hwnd); 

	// Convert the color format to a count of bits. 
	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
	if (cClrBits == 1) 
		cClrBits = 1; 
	else if (cClrBits <= 4) 
		cClrBits = 4; 
	else if (cClrBits <= 8) 
		cClrBits = 8; 
	else if (cClrBits <= 16) 
		cClrBits = 16; 
	else if (cClrBits <= 24) 
		cClrBits = 24; 
	else cClrBits = 32; 

	// Allocate memory for the BITMAPINFO structure. (This structure 
	// contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
	// data structures.) 

	if (cClrBits != 24) 
		pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
		sizeof(BITMAPINFOHEADER) + 
		sizeof(RGBQUAD) * (1<< cClrBits)); 

	// There is no RGBQUAD array for the 24-bit-per-pixel format. 

	else 
		pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
		sizeof(BITMAPINFOHEADER)); 

	// Initialize the fields in the BITMAPINFO structure. 

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	pbmi->bmiHeader.biWidth = bmp.bmWidth; 
	pbmi->bmiHeader.biHeight = bmp.bmHeight; 
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
	if (cClrBits < 24) 
		pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

	// If the bitmap is not compressed, set the BI_RGB flag. 
	pbmi->bmiHeader.biCompression = BI_RGB; 

	// Compute the number of bytes in the array of color 
	// indices and store the result in biSizeImage. 
	// For Windows NT, the width must be DWORD aligned unless 
	// the bitmap is RLE compressed. This example shows this. 
	// For Windows 95/98/Me, the width must be WORD aligned unless the 
	// bitmap is RLE compressed.
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
		* pbmi->bmiHeader.biHeight; 
	// Set biClrImportant to 0, indicating that all of the 
	// device colors are important. 
	pbmi->bmiHeader.biClrImportant = 0; 
	return pbmi; 
} 

void CreateBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, 
				   HBITMAP hBMP, HDC hDC) 
{ 
	HANDLE hf;                 // file handle 
	BITMAPFILEHEADER hdr;       // bitmap file-header 
	PBITMAPINFOHEADER pbih;     // bitmap info-header 
	LPBYTE lpBits;              // memory pointer 
	DWORD dwTotal;              // total count of bytes 
	DWORD cb;                   // incremental count of bytes 
	BYTE *hp;                   // byte pointer 
	DWORD dwTmp; 

	pbih = (PBITMAPINFOHEADER) pbi; 
	lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

	if (!lpBits) 
		errhandler("GlobalAlloc", hwnd); 

	// Retrieve the color table (RGBQUAD array) and the bits 
	// (array of palette indices) from the DIB. 
	if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, 
		DIB_RGB_COLORS)) 
	{
		errhandler("GetDIBits", hwnd); 
	}

	// Create the .BMP file. 
	hf = CreateFile(pszFile, 
		GENERIC_READ | GENERIC_WRITE, 
		(DWORD) 0, 
		NULL, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		(HANDLE) NULL); 
	if (hf == INVALID_HANDLE_VALUE) 
		errhandler("CreateFile", hwnd); 
	hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
	// Compute the size of the entire file. 
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
		pbih->biSize + pbih->biClrUsed 
		* sizeof(RGBQUAD) + pbih->biSizeImage); 
	hdr.bfReserved1 = 0; 
	hdr.bfReserved2 = 0; 

	// Compute the offset to the array of color indices. 
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
		pbih->biSize + pbih->biClrUsed 
		* sizeof (RGBQUAD); 

	// Copy the BITMAPFILEHEADER into the .BMP file. 
	if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
		(LPDWORD) &dwTmp,  NULL)) 
	{
		errhandler("WriteFile", hwnd); 
	}

	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
	if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
		+ pbih->biClrUsed * sizeof (RGBQUAD), 
		(LPDWORD) &dwTmp, ( NULL)))
		errhandler("WriteFile", hwnd); 

	// Copy the array of color indices into the .BMP file. 
	dwTotal = cb = pbih->biSizeImage; 
	hp = lpBits; 
	if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)) 
		errhandler("WriteFile", hwnd); 

	// Close the .BMP file. 
	if (!CloseHandle(hf)) 
		errhandler("CloseHandle", hwnd); 

	// Free memory. 
	GlobalFree((HGLOBAL)lpBits);
}



BOOL SaveHBitMap(HBITMAP hBitmap, TCHAR *pFileName)
{
	if(hBitmap==NULL )
		return false;
	HDC hDC;
	//当前分辨率下每象素所占字节数
	int iBits;
	//位图中每象素所占字节数
	WORD wBitCount;
	//定义调色板大小， 位图中像素字节大小 ，位图文件大小 ， 写入文件字节数 
	DWORD dwPaletteSize=0, dwBmBitsSize=0, dwDIBSize=0, dwWritten=0; 
	//位图属性结构 
	BITMAP Bitmap;  
	//位图文件头结构
	BITMAPFILEHEADER bmfHdr;  
	//位图信息头结构 
	BITMAPINFOHEADER bi;  
	//指向位图信息头结构  
	LPBITMAPINFOHEADER lpbi;  
	//定义文件，分配内存句柄，调色板句柄 
	HANDLE fh, hDib, hPal,hOldPal=NULL; 
	//计算位图文件每个像素所占字节数 
	hDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES); 
	DeleteDC(hDC); 
	if (iBits <= 1)  wBitCount = 1; 
	else if (iBits <= 4)  wBitCount = 4; 
	else if (iBits <= 8)  wBitCount = 8; 
	else if (iBits <= 24) wBitCount = 24; 
	else wBitCount = 32;
	GetObject(hBitmap, sizeof(Bitmap), (LPSTR)&Bitmap);
	bi.biSize   = sizeof(BITMAPINFOHEADER);
	bi.biWidth   = Bitmap.bmWidth;
	bi.biHeight   = Bitmap.bmHeight;
	bi.biPlanes   = 1;
	bi.biBitCount  = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage  = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrImportant = 0;
	bi.biClrUsed  = 0;

	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	//为位图内容分配内存 
	hDib = GlobalAlloc(GHND,dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER)); 
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib); 
	*lpbi = bi;

	// 处理调色板  
	hPal = GetStockObject(DEFAULT_PALETTE); 
	if (hPal) 
	{ 
		hDC = ::GetDC(NULL); 
		hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE); 
		RealizePalette(hDC); 
	}

	// 获取该调色板下新的像素值 
	GetDIBits(hDC, hBitmap, 0, (UINT) Bitmap.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) 
		+dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS); 

	//恢复调色板  
	if (hOldPal) 
	{ 
		::SelectPalette(hDC, (HPALETTE)hOldPal, TRUE); 
		RealizePalette(hDC); 
		::ReleaseDC(NULL, hDC); 
	}

	//创建位图文件  
	fh = CreateFile(pFileName, GENERIC_WRITE,0, NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 

	if (fh == INVALID_HANDLE_VALUE)  return FALSE; 

	// 设置位图文件头 
	bmfHdr.bfType = 0x4D42; // "BM" 
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;  
	bmfHdr.bfSize = dwDIBSize; 
	bmfHdr.bfReserved1 = 0; 
	bmfHdr.bfReserved2 = 0; 
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize; 
	// 写入位图文件头 
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL); 
	// 写入位图文件其余内容 
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL); 
	//清除  
	GlobalUnlock(hDib); 
	GlobalFree(hDib); 
	CloseHandle(fh);
	return TRUE;
}


void* CutBmpDbb(int nWidth, int nHeight, int nFirstX, int nLastX, int nFirstY, int nLastY, 
 const void* pDIB, int nChannels, int padding) 
{
	 int nSectionWidth = nLastX - nFirstX + 1;
	 int nSectionHeight = nLastY - nFirstY + 1;

	int bPadHeight = false;
	int m_nPaddedWidth = DoPadding(nWidth, padding);
	int m_nPaddedHeight =0;
	 if (bPadHeight) {
		 m_nPaddedHeight = DoPadding(nHeight, padding);
	 } else {
		 m_nPaddedHeight = nHeight;
	 }
	uint8 * m_pMemory = new uint8[ m_nPaddedWidth*nChannels*m_nPaddedHeight];
		
	 if (m_pMemory != NULL) {
		 int nSrcLineWidthPadded = DoPadding(nWidth * nChannels, 4);
		 const uint8* pSrc = (uint8*)pDIB + nFirstY*nSrcLineWidthPadded + nFirstX*nChannels;
		 uint16* pDst = (unsigned short*) m_pMemory;
		 for (int j = 0; j < nSectionHeight; j++) {
			 if (nChannels == 4) {
				 for (int i = 0; i < nSectionWidth; i++) {
					 uint32 sourcePixel = ((uint32*)pSrc)[i];
					 int d = i;
					 uint32 nBlue = sourcePixel & 0xFF;
					 uint32 nGreen = (sourcePixel >> 8) & 0xFF;
					 uint32 nRed = (sourcePixel >> 16) & 0xFF;
					 // The commented out code actually is worse than the simple shift for precision of the fixed point arithmetic
					 pDst[d] = ((uint16)nBlue << 6); // ((((uint16)nBlue) << 8) + nBlue) >> 2;
					 d += m_nPaddedWidth;
					 pDst[d] = ((uint16)nGreen << 6); //((((uint16) nGreen) << 8) + nGreen) >> 2;
					 d += m_nPaddedWidth;
					 pDst[d] = ((uint16)nRed << 6); //((((uint16) nRed) << 8) + nRed) >> 2;
				 }
			 } else {
				 for (int i = 0; i < nSectionWidth; i++) {
					 int s = i*3;
					 int d = i;
					 pDst[d] = ((uint16)pSrc[s] << 6);//((((uint16) pSrc[s]) << 8) + pSrc[s]) >> 2;
					 d += m_nPaddedWidth;
					 pDst[d] = ((uint16)pSrc[s+1] << 6);//((((uint16) pSrc[s+1]) << 8) + pSrc[s+1]) >> 2;
					 d += m_nPaddedWidth;
					 pDst[d] = ((uint16)pSrc[s+2] << 6);//((((uint16) pSrc[s+2]) << 8) + pSrc[s+2]) >> 2;
				 }
			 }
			 pDst += 3*m_nPaddedWidth;
			 pSrc += nSrcLineWidthPadded;
		 }
	 }
	 return m_pMemory;
}

void DrawDIB32bppPart(HDC& dc,void* pDIBData,
					  const RECT& targetArea, SIZE dibSize,
					  int XSrc,                // x-coord of source lower-left corner
					  int YSrc,                // y-coord of source lower-left corner
					  UINT uStartScan,         // first scan line in array
					  UINT cScanLines,         // number of scan lines
					  POINT offset)
{
	BITMAPINFO bmInfo;
	memset(&bmInfo, 0, sizeof(BITMAPINFO));
	bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmInfo.bmiHeader.biWidth = dibSize.cx;
	bmInfo.bmiHeader.biHeight = dibSize.cy;
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = 32;
	bmInfo.bmiHeader.biCompression = BI_RGB;
	int nWidth = targetArea.right - targetArea.left;
	int nHeight = targetArea.bottom - targetArea.top;
	int xDest = (nWidth - dibSize.cx) / 2 + offset.x;
	int yDest = (nHeight - dibSize.cy) / 2 + offset.y;
	int nLineCount = uStartScan+cScanLines;
	if (nLineCount > dibSize.cy)
		nLineCount = dibSize.cy;
	SetDIBitsToDevice(dc,xDest, yDest, dibSize.cx, dibSize.cy,
		XSrc, YSrc, uStartScan,nLineCount, pDIBData,
		&bmInfo, DIB_RGB_COLORS);
}

void DrawDIB32bpp(HDC& dc, void* pDIBData,
				  const RECT& targetArea, SIZE dibSize, POINT offset)
{
	BITMAPINFO bmInfo;
	memset(&bmInfo, 0, sizeof(BITMAPINFO));
	bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmInfo.bmiHeader.biWidth = dibSize.cx;
	bmInfo.bmiHeader.biHeight = dibSize.cy;
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = 32;
	bmInfo.bmiHeader.biCompression = BI_RGB;
	int nWidth = targetArea.right - targetArea.left;
	int nHeight = targetArea.bottom - targetArea.top;
	int xDest = (nWidth - dibSize.cx) / 2 + offset.x;
	int yDest = (nHeight - dibSize.cy) / 2 + offset.y;
	SetDIBitsToDevice(dc,xDest, yDest, dibSize.cx, dibSize.cy, 0, 0, 0, dibSize.cy, pDIBData,
		&bmInfo, DIB_RGB_COLORS);
}

// 参数2：未初始化的BITMAPINFO
// 参数3：32位位图的数据指针
// 参数4：初始化完毕的画刷
// 参数5：绘制的矩形区域大小
// 参数6：显示的图片大小
// 参数7：偏移显示量
//
void DrawDIB32bppWithBlackBorders(HDC& dc, BITMAPINFO& bmInfo, void* pDIBData, HBRUSH backBrush,
								  const RECT& targetArea, SIZE dibSize, POINT offset)
{
	memset(&bmInfo, 0, sizeof(BITMAPINFO));
	bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmInfo.bmiHeader.biWidth = dibSize.cx;
	bmInfo.bmiHeader.biHeight = -dibSize.cy;
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = 32;
	bmInfo.bmiHeader.biCompression = BI_RGB;
	int nWidth = targetArea.right - targetArea.left;
	int nHeight = targetArea.bottom - targetArea.top;
	int xDest = (nWidth - dibSize.cx) / 2 + offset.x;
	int yDest = (nHeight - dibSize.cy) / 2 + offset.y;

	// remaining client area is painted black
	if (xDest > 0) {
		RECT r = { 0, 0, xDest, nHeight };
		FillRect(dc,&r, backBrush);
	}
	if (xDest + dibSize.cx < nWidth) {
		RECT r = { xDest + dibSize.cx, 0,nWidth, nHeight };
		FillRect(dc, &r, backBrush);
	}
	if (yDest > 0) {
		RECT r = { xDest, 0, xDest + dibSize.cx, yDest };
		FillRect(dc, &r, backBrush);
	}
	if (yDest + dibSize.cy < nHeight) {
		RECT r = { xDest, yDest + dibSize.cy, xDest + dibSize.cx, nHeight };
		FillRect(dc, &r, backBrush);
	}
	SetDIBitsToDevice(dc,xDest, yDest, dibSize.cx, dibSize.cy, 0, 0, 0, dibSize.cy, pDIBData,
		&bmInfo, DIB_RGB_COLORS);
}