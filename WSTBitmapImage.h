#pragma once
#include "wstimage.h"
//
// WSTBitmapImage: Derived class for handling bitmap images specifically.
//
class WSTBitmapImage
{

public:

	WSTBitmapImage(void);
	~WSTBitmapImage(void);

	// sets data for usage of a Windows bitmap image
	virtual WSTImageStatus SetImage(const HWND hwnd , const HBITMAP hbmp, const HDC hdc);

	// saves image to the specified filename
	WSTImageStatus SaveImage(std::tstring filename);

	// saves an image file in ZLIB compressed form
	WSTImageStatus SaveCompressedImage(std::tstring filename, int compressLevel);

private:

	// handles for necessary Windows bitmap-related structures
	HWND m_hWnd;
	HBITMAP m_hBitmap;
	HDC m_hDC;

	// functions for dealing with Windows bitmaps
	// (yanked from http://msdn2.microsoft.com/en-us/library/ms532340.aspx)
	PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp);
	bool WriteBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC); 

	// get a buffer with the bitmap inside it instead of writing to a file
	bool WriteBuffer(HWND hwnd, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC, LPBYTE& pBuffer, size_t& bufSize);

};
