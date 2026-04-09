#pragma once

#include "stdafx.h"

typedef enum {
	WSTIMAGE_SUCCESS = 0,
	WSTIMAGE_UNKNOWN_FAILURE,
	WSTIMAGE_SAVE_FAILED,
	WSTIMAGE_NOT_SET,
	WSTIMAGE_INVALID_PARAM
} WSTImageStatus;

//
// WSTImage: An in-house base class for handling images in situations where we aren't able to use
//			 .NET or the VCL...
//
class WSTImage
{

public:

	WSTImage(void);
	~WSTImage(void);

	virtual WSTImageStatus SaveImage(std::tstring filename) = 0;
	WSTImageStatus GetImageDims(int* const w, int* const h) const;

protected:

	bool isImageSet;
	int width, height;

};
