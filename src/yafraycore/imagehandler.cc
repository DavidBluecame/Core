/****************************************************************************
 *      imagehandler.cc: common code for all imagehandlers
 *      This is part of the yafaray package
 *      Copyright (C) 2016  David Bluecame
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <core_api/imagehandler.h>


__BEGIN_YAFRAY


imageBuffer_t::imageBuffer_t(int width, int height, int num_channels, int optimization):m_width(width),m_height(height),m_num_channels(num_channels)
{
	switch(optimization)
	{
		case TEX_OPTIMIZATION_NONE:
			if(m_num_channels == 4) rgba128_FloatImg = new rgba2DImage_nw_t(width, height);
			else if(m_num_channels == 3) rgb96_FloatImg = new rgb2DImage_nw_t(width, height);
			else if(m_num_channels == 1) gray32_FloatImg = new gray2DImage_nw_t(width, height);
			break;
			
		case TEX_OPTIMIZATION_OPTIMIZED:
			if(m_num_channels == 4) rgba32_OptimizedImg = new rgbaOptimizedImage_nw_t(width, height);
			else if(m_num_channels == 3) rgb24_OptimizedImg = new rgbOptimizedImage_nw_t(width, height);
			else if(m_num_channels == 1) gray8_OptimizedImg = new grayOptimizedImage_nw_t(width, height);
			break;
			
		case TEX_OPTIMIZATION_COMPRESSED:
			if(m_num_channels == 4) rgba24_CompressedImg = new rgbaCompressedImage_nw_t(width, height);
			else if(m_num_channels == 3) rgb16_CompressedImg = new rgbCompressedImage_nw_t(width, height);
			else if(m_num_channels == 1) gray8_OptimizedImg = new grayOptimizedImage_nw_t(width, height);
			break;
			
#ifdef HAVE_OPENEXR
		case TEX_OPTIMIZATION_HALF_FLOAT:
			if(m_num_channels == 4) rgba64_HalfFloatImg = new rgbaHalfImage_nw_t(width, height);
			else if(m_num_channels == 3) rgb48_HalfFloatImg = new rgbHalfImage_nw_t(width, height);
			else if(m_num_channels == 1) gray16_HalfFloatImg = new grayHalfImage_nw_t(width, height);
			break;
#endif
		default: break;
	}
}

imageBuffer_t::~imageBuffer_t()
{
	if(rgba32_OptimizedImg) { delete rgba32_OptimizedImg; rgba32_OptimizedImg = nullptr; }
	if(rgba24_CompressedImg) { delete rgba24_CompressedImg; rgba24_CompressedImg = nullptr; }
	if(rgba128_FloatImg) { delete rgba128_FloatImg; rgba128_FloatImg = nullptr; }
	if(rgb24_OptimizedImg) { delete rgb24_OptimizedImg; rgb24_OptimizedImg = nullptr; }
	if(rgb16_CompressedImg) { delete rgb16_CompressedImg; rgb16_CompressedImg = nullptr; }
	if(rgb96_FloatImg) { delete rgb96_FloatImg; rgb96_FloatImg = nullptr; }
	if(gray32_FloatImg) { delete gray32_FloatImg; gray32_FloatImg = nullptr; }
	if(gray8_OptimizedImg) { delete gray8_OptimizedImg; gray8_OptimizedImg = nullptr; }
	
#ifdef HAVE_OPENEXR
	if(rgba64_HalfFloatImg) { delete rgba64_HalfFloatImg; rgba64_HalfFloatImg = nullptr; }
	if(rgb48_HalfFloatImg) { delete rgb48_HalfFloatImg; rgb48_HalfFloatImg = nullptr; }
	if(gray16_HalfFloatImg) { delete gray16_HalfFloatImg; gray16_HalfFloatImg = nullptr; }
#endif
}


std::string imageHandler_t::getDenoiseParams() const
{
#ifdef HAVE_OPENCV	//Denoise only works if YafaRay is built with OpenCV support
	if(!m_Denoise) return "";
	std::stringstream paramString;
	paramString << "| Image file denoise enabled [mix=" << m_DenoiseMix << ", h(Luminance)=" << m_DenoiseHLum << ", h(Chrominance)=" <<  m_DenoiseHCol << "]" << yendl;
	return paramString.str();
#else
	return "";
#endif
}


void imageHandler_t::generateMipMaps()
{
	if(imgBuffer.empty()) return;
	
	int imgIndex = 0;
	bool blur_seamless = true;
	int w = m_width, h = m_height;
	
	Y_VERBOSE << "ImageHandler: generating mipmaps for texture of resolution [" << w << " x " << h << "]" << yendl;
	
	int gaussBlurMatrix[5][5]={{1, 4, 6, 4, 1}, {4, 16, 24, 16, 4}, {6, 24, 36, 24, 6}, {4, 16, 24, 16, 4}, {1, 4, 6, 4, 1}};
	
	//Temporary full float buffer to reduce information loss and color space conversion repetition when mipmapping optimized or compressed buffers
	std::vector<imageBuffer_t *> tmpFloatImgLinearRGBABuffer;
	tmpFloatImgLinearRGBABuffer.push_back(new imageBuffer_t(w, h, 4, TEX_OPTIMIZATION_NONE));
	for(int i=0; i < w; ++i)
	{
		for(int j=0; j < h; ++j)
		{
			colorA_t origCol = imgBuffer.at(imgIndex)->getColor(i, j);
			origCol.linearRGB_from_ColorSpace(m_colorSpace, m_gamma);
			tmpFloatImgLinearRGBABuffer.at(imgIndex)->setColor(i, j, origCol);
		}
	}

	//Mipmap generation using the temporary full float buffer to reduce information loss
	while(w > 1 || h > 1)
	{
		int w2 = (w + 1) / 2;
		int h2 = (h + 1) / 2;
		++imgIndex;
		imgBuffer.push_back(new imageBuffer_t(w2, h2, imgBuffer.at(imgIndex-1)->getNumChannels(), getTextureOptimization()));
		tmpFloatImgLinearRGBABuffer.push_back(new imageBuffer_t(w2, h2, 4, TEX_OPTIMIZATION_NONE));

		for(int i2=0; i2 < w2; ++i2)
		{
			for(int j2=0; j2 < h2; ++j2)
			{
				int accum_filter_weight = 0;
				colorA_t tmpCol(0.f);
				
				//For each texel in the new mipmap, we use 5x5 texels from the previous level mipmap with a gaussian matrix to blur it
				for(int p = 0; p < 5; ++p)
				{
					for(int q = 0; q < 5; ++q)
					{
						int i = 2*i2 + (p-2);	//(p-2) to "center" the matrix around the texel
						int j = 2*j2 + (q-2);	//(q-2) to "center" the matrix around the texel

						if(blur_seamless)
						{
							//If blur_seamless == true then when outside the parent mipmap area, find coordinates to find the "repeat" texel. TODO: this does not take into account any MirrorX/MirrorY setting in the texture!
							if(i < 0 || i >= w) { i = i - w * (int) floor((float)i / (float)w); }
							if(j < 0 || j >= h) { j = j - h * (int) floor((float)j / (float)h); }
						}
						else
						{
							//If blur_seamless == false then when outside the parent mipmap area skip the color calculation excluding out-of-range texels from the result.
							if(i < 0 || i >= w) continue;
							if(j < 0 || j >= h) continue;
						}

						tmpCol += tmpFloatImgLinearRGBABuffer.at(imgIndex-1)->getColor(i, j) * gaussBlurMatrix[p][q];
						accum_filter_weight += gaussBlurMatrix[p][q];
					}
				}
				//Normalize final color depending on how many texels from original texture (taking filter applied to each into account) were used to avoid darkening at edges of texture
				tmpCol = tmpCol / (float)accum_filter_weight;
				
				//Write mipmapped image into the temporary float buffer for next loop and into the permanent buffer
				tmpFloatImgLinearRGBABuffer.at(imgIndex)->setColor(i2, j2, tmpCol);

				//Convert to original texture color space for storage in the mipmap
				tmpCol.ColorSpace_from_linearRGB(m_colorSpace, m_gamma);
				imgBuffer.at(imgIndex)->setColor(i2, j2, tmpCol);
			}
		}
		w = w2;
		h = h2;
		Y_DEBUG << "ImageHandler: generated mipmap " << imgIndex << " [" << w2 << " x " << h2 << "]" << yendl;
	}
	
	//Delete temporary full float buffers
	for(size_t idx = 0; idx < tmpFloatImgLinearRGBABuffer.size(); ++idx)
	{
		delete tmpFloatImgLinearRGBABuffer.at(idx);
		tmpFloatImgLinearRGBABuffer.at(idx) = nullptr;
	}
	
	Y_VERBOSE << "ImageHandler: mipmap generation done: " << imgIndex << " mipmaps generated." << yendl;
}


__END_YAFRAY
