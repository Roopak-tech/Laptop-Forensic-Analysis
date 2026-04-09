#define UNICODE
#define _UNICODE

#include "stdafx.h"
#include "WSTBitmapImage.h"
#include "SysCallsLog.h"
//#include "zlib.h"

WSTBitmapImage::WSTBitmapImage(void)
{
}

WSTBitmapImage::~WSTBitmapImage(void)
{
}

WSTImageStatus WSTBitmapImage::SetImage(const HWND hwnd , const HBITMAP hbmp, const HDC hdc)
{

	if (hwnd != NULL && hbmp != NULL) {
		m_hWnd = hwnd;
		m_hBitmap = hbmp;
		m_hDC = hdc;
		return WSTIMAGE_SUCCESS;
	}
	else {
		return WSTIMAGE_INVALID_PARAM;
	}

}


WSTImageStatus WSTBitmapImage::SaveImage(std::tstring filename)
{

	PBITMAPINFO pbmi = CreateBitmapInfoStruct(m_hWnd, m_hBitmap);

   
	if (WriteBMPFile(m_hWnd, (LPTSTR)filename.c_str(), pbmi, m_hBitmap, m_hDC))
    {
		return WSTIMAGE_SUCCESS;
    }
	else
    {
		return WSTIMAGE_SAVE_FAILED;
    }

}

WSTImageStatus WSTBitmapImage::SaveCompressedImage(std::tstring filename, int compressLevel)
{
#ifndef _ENABLE_COMPRESSION
	return WSTIMAGE_SUCCESS;
#else
	std::tstringstream compressed;
	LPBYTE pBuffer, pCompressed;
	size_t bufSize, compSize;
	FILE * output;

	PBITMAPINFO pbmi = CreateBitmapInfoStruct(m_hWnd, m_hBitmap);

	if (!WriteBuffer(m_hWnd, pbmi, m_hBitmap, m_hDC, pBuffer, bufSize))
		return WSTIMAGE_SAVE_FAILED;

	// compress the data
	compSize = compressBound(bufSize);
	pCompressed = (LPBYTE) malloc(compSize);
	if (compress(pCompressed, (uLongf*)(&compSize), pBuffer, (uLong)bufSize) != Z_OK)
		return WSTIMAGE_SAVE_FAILED;
	
	if (fopen_s(&output, filename.c_str(), "wb+") != 0) {
		free(pBuffer);
		return WSTIMAGE_SAVE_FAILED;
	}

	// compSize is updzated by compress to the actual length of the
	// compressed data
	fwrite(pCompressed, sizeof(BYTE), compSize, output);
	fclose(output);

	free(pBuffer);
	free(pCompressed);

	return WSTIMAGE_SUCCESS;
#endif
}

PBITMAPINFO WSTBitmapImage::CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp)
{ 
    BITMAP bmp; 
    PBITMAPINFO pbmi; 
    WORD    cClrBits; 

    // Retrieve the bitmap color format, width, and height. 
    if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) 
        return NULL;

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

	if (cClrBits != 24) {
		INC_SYS_CALL_COUNT(LocalAlloc); // Needed to be INC TKH
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER) + 
                    sizeof(RGBQUAD) * (1<< cClrBits)); 
	}
	else {
     // There is no RGBQUAD array for the 24-bit-per-pixel format. 
		 INC_SYS_CALL_COUNT(LocalAlloc); // Needed to be INC TKH
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER)); 
	}
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

bool WSTBitmapImage::WriteBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, 
                  HBITMAP hBMP, HDC hDC) 
{ 
    HANDLE hf;                  // file handle 
    BITMAPFILEHEADER hdr;       // bitmap file-header 
    PBITMAPINFOHEADER pbih;     // bitmap info-header 
    LPBYTE lpBits;              // memory pointer 
    DWORD dwTotal;              // total count of bytes 
    DWORD cb;                   // incremental count of bytes 
    BYTE *hp;                   // byte pointer 
    DWORD dwTmp; 

    pbih = (PBITMAPINFOHEADER) pbi; 
	INC_SYS_CALL_COUNT(GlobalAlloc); // Needed to be INC TKH
    lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

    if (!lpBits) 
         return false; 

    // Retrieve the color table (RGBQUAD array) and the bits 
    // (array of palette indices) from the DIB. 
    if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, 
        DIB_RGB_COLORS)) 
    {
        return false;
    }

    // Create the .BMP file.
	INC_SYS_CALL_COUNT(CreateFile); // Needed to be INC TKH
    hf = CreateFile(pszFile, 
                   GENERIC_READ | GENERIC_WRITE, 
                   (DWORD) 0, 
                   NULL, 
                   CREATE_ALWAYS, 
                   FILE_ATTRIBUTE_NORMAL, 
                   (HANDLE) NULL); 
    if (hf == INVALID_HANDLE_VALUE) 
        return false; 
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
       return false; 
    }

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
    if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
                  + pbih->biClrUsed * sizeof (RGBQUAD), 
                  (LPDWORD) &dwTmp, ( NULL)) )
        return false;

    // Copy the array of color indices into the .BMP file. 
    dwTotal = cb = pbih->biSizeImage; 
    hp = lpBits; 
    if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)) 
           return false; 

    // Close the .BMP file. 
	 INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
     if (!CloseHandle(hf)) 
           return false; 

    // Free memory. 
	INC_SYS_CALL_COUNT(GlobalFree); // Needed to be INC TKH
    GlobalFree((HGLOBAL)lpBits);

	return true;
}

bool WSTBitmapImage::WriteBuffer(HWND hwnd, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC, LPBYTE& pBuffer, size_t& bufSize)
{

    BITMAPFILEHEADER hdr;       // bitmap file-header 
    PBITMAPINFOHEADER pbih;     // bitmap info-header 
    LPBYTE lpBits;              // memory pointer 
    DWORD dwTotal;              // total count of bytes 
    DWORD cb;                   // incremental count of bytes 
    BYTE *hp;                   // byte pointer 

	LPBYTE pBuf, pos;

    pbih = (PBITMAPINFOHEADER) pbi; 
	INC_SYS_CALL_COUNT(GlobalAlloc); // Needed to be INC TKH
    lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
    if (!lpBits) 
         return false; 

    // Retrieve the color table (RGBQUAD array) and the bits 
    // (array of palette indices) from the DIB. 
    if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, 
        DIB_RGB_COLORS)) 
    {
        return false;
    }

    hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
    // Compute the size of the entire file. 
    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
                 pbih->biSize + pbih->biClrUsed 
                 * sizeof(RGBQUAD) + pbih->biSizeImage); 
    hdr.bfReserved1 = 0; 
    hdr.bfReserved2 = 0; 

	bufSize = hdr.bfSize;
	pBuf = new BYTE[hdr.bfSize];
	pos = pBuf;

    // Compute the offset to the array of color indices. 
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
                    pbih->biSize + pbih->biClrUsed 
                    * sizeof (RGBQUAD); 

    // Copy the BITMAPFILEHEADER into the buffer. 
    CopyMemory( pos, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER) );
	pos += sizeof(BITMAPFILEHEADER);
    
    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
    CopyMemory( pos, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
                  + pbih->biClrUsed * sizeof(RGBQUAD) );
	pos += sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD);
                  
    // Copy the array of color indices into the .BMP file. 
    dwTotal = cb = pbih->biSizeImage; 
    hp = lpBits; 
    CopyMemory( pos, (LPVOID)hp, (int)cb );

    // Free memory. 
	INC_SYS_CALL_COUNT(GlobalFree); // Needed to be INC TKH
    GlobalFree((HGLOBAL)lpBits);

	pBuffer = (LPBYTE)pBuf;

	return true;

}

#undef _UNICODE
#undef UNICODE