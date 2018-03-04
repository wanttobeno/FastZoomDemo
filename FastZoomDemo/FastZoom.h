/*
	32位位图的快速缩放处理
	// 算法0~7的耗时
	//31 31 32 0 0 0 0 0
*/
#pragma once

#include <Windows.h>

#define  asm __asm 
typedef unsigned  char  TUInt8;  // [0..255] 

struct  TARGB32       //32 bit color 
{ 
	TUInt8  B,G,R,A;           // A is alpha 
}; 

struct  TPicRegion   //一块颜色数据区的描述，便于参数传递 
{ 
	TARGB32*    pdata;          //颜色数据首地址 
	long         byte_width;     //一行数据的物理宽度(字节宽度)； 
	//abs(byte_width)有可能大于等于width*sizeof(TARGB32); 
	long         width;          //像素宽度 
	long         height;         //像素高度 
}; 

//访问一个点的函数
inline TARGB32& Pixels(const TPicRegion& pic,const  long  x,const  long  y) 
{ 
	return  ( (TARGB32*)((TUInt8*)pic.pdata+pic.byte_width*y) )[x]; 
} 

class CFastZoom
{
public:
	CFastZoom(void);
	~CFastZoom(void);
public:
	void	PicZoom0(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom1(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom2(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom3(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom4(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom5(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom6(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom7(const TPicRegion& Dst,const TPicRegion& Src);
private:
	void	PicZoom_float(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom_Table(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom_SSE(const TPicRegion& Dst,const TPicRegion& Src);
	void	PicZoom_SSE_mmh(const TPicRegion& Dst,const TPicRegion& Src);
};
