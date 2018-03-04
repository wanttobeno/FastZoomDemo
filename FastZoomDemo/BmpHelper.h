#ifndef CBMPHELPER_H__
#define CBMPHELPER_H__
#include <Windows.h>

#ifndef uint8
/// signed 8 bit integer value
typedef signed char int8;
/// unsigned 8 bit integer value
typedef unsigned char uint8;
/// signed 16 bit integer value
typedef signed short int16;
/// unsigned 16 bit integer value
typedef unsigned short uint16;
/// signed 32 bit integer value
typedef signed int int32;
/// unsigned 32 bit integer value
typedef unsigned int uint32;
#endif //!uint8


#ifdef _DEBUG

#define  ZY_TIME_COUNT_START \
	DWORD dwTime = GetTickCount();

#define ZY_TIME_COUNT_END \
	char buf[128]={0};\
	dwTime = GetTickCount() - dwTime;\
	sprintf(buf,"### Time %d ms\n",dwTime);\
	OutputDebugStringA(buf);\

#else

#define		ZY_TIME_COUNT_START
#define		ZY_TIME_COUNT_END

#endif // _DEBUG

inline int DoPadding(int value, int padvalue) {
	return (value + padvalue - 1) & ~(padvalue - 1);
}

HBITMAP MakeBitmap(HDC hDc, LPBYTE lpBits, long lWidth, long lHeight, WORD wBitCount);

// pImageData即是已知的图像裸数据。
HBITMAP CreateBitmap(int nWidth,INT nHeight,int nBpp,void * pImageData);

void FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin);

//////////////////////////////// 转换 start ////////////////////////////////
// 不支持
void* Convert1To4Channels(int nWidth, int nHeight, void* pInDIB);
void* Convert16To4Channels(int nWidth, int nHeight, void* pInDIB);
void* Convert8bppTo32bppDIB(int nWidth, int nHeight, void* pInDIB);
// 支持
void* Convert24DibTo32Dib(int nWidth, int nHeight, void* p24DIB);

// 在外面申请内存，内存大小4字节对其
// 24位的数据内存大小申请：
//uint32 nSizeLinePadded = DoPadding(nWidth*3, 4);
//uint32 nSizeBytes = nSizeLinePadded * nHeight;
void Convert32bppTo24bppDIB(int nWidth, int nHeight, void* pDIBTarget,
							const void* pDIBSource, bool bFlip);
//////////////////////////////// 转换 end ////////////////////////////////

void* CutBmpDbb(int nWidth, int nHeight, int nFirstX, int nLastX, int nFirstY, int nLastY, 
				const void* pDIB, int nChannels, int padding);

//////////////////////////////// 显示 start ////////////////////////////////

void DrawDIB32bppPart(HDC& dc,void* pDIBData,
					  const RECT& targetArea, SIZE dibSize,
					  int XSrc,                // x-coord of source lower-left corner
					  int YSrc,                // y-coord of source lower-left corner
					  UINT uStartScan,         // first scan line in array
					  UINT cScanLines,         // number of scan lines
					  POINT offset);

// 参数3：显示区域，为客户区大小，则居中显示
void DrawDIB32bpp(HDC& dc,void* pDIBData,
				  const RECT& targetArea, SIZE dibSize, POINT offset);

void DrawDIB32bppWithBlackBorders(HDC& dc, BITMAPINFO& bmInfo, void* pDIBData, HBRUSH backBrush,
								  const RECT& targetArea, SIZE dibSize, POINT offset);
//////////////////////////////// 显示 end ////////////////////////////////

//////////////////////////////// 保存 start ////////////////////////////////
enum GDIP_SAVE_FORMAT
{
	BMP,
	PNG,
	TIFF
};
enum GDIP_RotateFlipType
{
	GDIP_RotateNoneFlipNone = 0,
	GDIP_Rotate90FlipNone   = 1,
	GDIP_Rotate180FlipNone  = 2,
	GDIP_Rotate270FlipNone  = 3,

	GDIP_RotateNoneFlipX    = 4,
	GDIP_Rotate90FlipX      = 5,
	GDIP_Rotate180FlipX     = 6,
	GDIP_Rotate270FlipX     = 7,
};

// 推荐
bool SaveUseGDIPlus(LPCTSTR sFileName, GDIP_SAVE_FORMAT nSaveFormat, int nWidth, int nHeight, void* pData,int nBpp = 32,GDIP_RotateFlipType nFlip=GDIP_RotateNoneFlipNone) ;
// 某些大图保存有问题
bool SaveBmpFile(const char* pSaveFile,int nWidth,int nHeight,void *pDibData,int nBpp,bool bFlip= false);
// 保存dc为图片，截屏
BOOL SaveHBitMap(HBITMAP hBitmap, TCHAR *pFileName);

//////////////////////////////// 保存 end ////////////////////////////////


#endif //!CBMPHELPER_H__