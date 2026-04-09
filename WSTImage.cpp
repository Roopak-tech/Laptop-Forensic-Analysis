#include "stdafx.h"
#include "WSTImage.h"

WSTImage::WSTImage(void) :
	isImageSet(false),
	width(0),
	height(0)
{
}

WSTImage::~WSTImage(void)
{
}

WSTImageStatus WSTImage::GetImageDims(int* const w, int* const h) const
{

	WSTImageStatus rv = WSTIMAGE_SUCCESS;

	if (isImageSet) {
		*w = width;
		*h = height;
	}
	else {
		rv = WSTIMAGE_NOT_SET;
	}

	return rv;

}