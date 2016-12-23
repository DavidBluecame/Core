/****************************************************************************
 * 		imagetex.cc: a texture class for images
 *      This is part of the yafray package
 *      Based on the original by: Mathias Wein; Copyright (C) 2006 Mathias Wein
 *		Copyright (C) 2010 Rodrigo Placencia Vazquez (DarkTide)
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

#include <cstring>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <textures/imagetex.h>
#include <utilities/stringUtils.h>

__BEGIN_YAFRAY

textureImage_t::textureImage_t(imageHandler_t *ih, int intp, float gamma, colorSpaces_t color_space):
				image(ih), colorSpace(color_space), gamma(gamma), mirrorX(false), mirrorY(false)
{
	intp_type = intp;
}

textureImage_t::~textureImage_t()
{
	if(postProcessedImage)
	{
		delete postProcessedImage;
		postProcessedImage = nullptr;
	}
	
	// Here we simply clear the pointer, yafaray's core will handle the memory cleanup
	image = nullptr;
}

void textureImage_t::resolution(int &x, int &y, int &z) const
{
	x=image->getWidth();
	y=image->getHeight();
	z=0;
}

colorA_t textureImage_t::interpolateImage(const point3d_t &p, float dSdx, float dTdx, float dSdy, float dTdy, bool from_postprocessed) const
{
	if (intp_type == INTP_MIPMAP_TRILINEAR)
	{
		float dS = std::max(fabsf(dSdx), fabsf(dSdy)) * image->getWidth();
		float dT = std::max(fabsf(dTdx), fabsf(dTdy)) * image->getHeight();
		
		float mipmaplevel = 0.5f * log2(dS*dS + dT*dT) - 1.f;
		
		mipmaplevel = std::min(std::max(0.f, mipmaplevel), (float) image->getHighestImgIndex());
		
		int mipmaplevelA = (int) floor(mipmaplevel);
		int mipmaplevelB = (int) ceil(mipmaplevel);

		int xA, yA, xA2, yA2;
		
		int resxA=image->getWidth(mipmaplevelA);
		int resyA=image->getHeight(mipmaplevelA);
		
		float xfA = ((float)resxA * (p.x - floor(p.x)));
		float yfA = ((float)resyA * (p.y - floor(p.y)));
		
		xfA += 0.5f;
		yfA += 0.5f;

		//FIXME DAVID: parameter for seamless textures???
		
		if(resxA >= 2)
		{
			xA = ((int)xfA)-1;
			if(xA < 0 || xA >= resxA) xA = xA - resxA * (int) floor((float)xA / (float)resxA); 
		}
		else xA = 0;
		
		if(resyA >= 2)
		{
			yA = ((int)yfA)-1 % resyA;
			if(yA < 0 || yA >= resxA) yA = yA - resyA * (int) floor((float)yA / (float)resyA); 
		}
		else yA = 0;
		
		xA2 = (xA+1) % resxA;
		yA2 = (yA+1) % resyA;
	
		colorA_t c1A = image->getPixel(xA, yA, mipmaplevelA);
		colorA_t c2A = image->getPixel(xA2, yA, mipmaplevelA);
		colorA_t c3A = image->getPixel(xA, yA2, mipmaplevelA);
		colorA_t c4A = image->getPixel(xA2, yA2, mipmaplevelA);

		float dxA = fabs(xfA - (int)(xfA));
		float dyA = fabs(yfA - (int)(yfA));
	
		float w0A = (1-dxA) * (1-dyA);
		float w1A = (1-dxA) * dyA;
		float w2A = dxA * (1-dyA);
		float w3A = dxA * dyA;
		
		//Y_DEBUG << std::setprecision(3)<< "resxA="<<resxA<<", resyA="<<resyA<<", xA="<<xA<<", yA="<<yA<<", xA2="<<xA2<<", yA2="<<yA2<<", dxA="<<dxA<<", dyA="<<dyA<<yendl;
		
		colorA_t color_ret = (w0A * c1A) + (w1A * c3A) + (w2A * c2A) + (w3A * c4A);
		
		if(mipmaplevelA == mipmaplevelB) return color_ret;
		
		int xB, yB, xB2, yB2;
		
		int resxB=image->getWidth(mipmaplevelB);
		int resyB=image->getHeight(mipmaplevelB);
		
		float xfB = ((float)resxB * (p.x - floor(p.x)));
		float yfB = ((float)resyB * (p.y - floor(p.y)));
		
		xfB += 0.5f;
		yfB += 0.5f;

		//FIXME DBVID: parameter for seamless textures???
		
		if(resxB >= 2)
		{
			xB = ((int)xfB)-1;
			if(xB < 0 || xB >= resxB) xB = xB - resxB * (int) floor((float)xB / (float)resxB); 
		}
		else xB = 0;
		
		if(resyB >= 2)
		{
			yB = ((int)yfB)-1 % resyB;
			if(yB < 0 || yB >= resxB) yB = yB - resyB * (int) floor((float)yB / (float)resyB); 
		}
		else yB = 0;
		
		xB2 = (xB+1) % resxB;
		yB2 = (yB+1) % resyB;
	
		colorA_t c1B = image->getPixel(xB, yB, mipmaplevelB);
		colorA_t c2B = image->getPixel(xB2, yB, mipmaplevelB);
		colorA_t c3B = image->getPixel(xB, yB2, mipmaplevelB);
		colorA_t c4B = image->getPixel(xB2, yB2, mipmaplevelB);

		float dxB = fabs(xfB - (int)(xfB));
		float dyB = fabs(yfB - (int)(yfB));
	
		float w0B = (1-dxB) * (1-dyB);
		float w1B = (1-dxB) * dyB;
		float w2B = dxB * (1-dyB);
		float w3B = dxB * dyB;
		
		colorA_t cB = (w0B * c1B) + (w1B * c3B) + (w2B * c2B) + (w3B * c4B);
		
		float mipmaplevelDelta = mipmaplevel - (float) mipmaplevelA;
		
		color_ret.blend(cB, mipmaplevelDelta);
		
		return color_ret;
	}

	int x, y, x0, x2, x3, y0, y2, y3;
	
	int resx=image->getWidth();
	int resy=image->getHeight();
	
	float xf = ((float)resx * (p.x - floor(p.x)));
	float yf = ((float)resy * (p.y - floor(p.y)));
	
	float dx = 0.f;
	float dy = 0.f;

	if (intp_type != INTP_NONE)
	{
		xf -= 0.5f;
		yf -= 0.5f;
	}
	
	if (tex_clipmode==TCL_REPEAT)
	{
		x = ((int)xf) % resx;
		y = ((int)yf) % resy;

		if(mirrorX)
		{

			if(xf < 0.f)
			{
				x0 = 1 % resx;
				x2 = x;
				x3 = x0;
				dx = -xf;
			}
			else if(xf >= resx - 1.f)
			{
				x0 = (resx+resx-1) % resx;
				x2 = x;
				x3 = x0;
				dx = xf - ((int)xf);
			}
			else
			{
				x0 = (resx+x-1) % resx;
				x2 = x+1;
				if(x2 >= resx) x2 = (resx + resx - x2) % resx;
				x3 = x+2;
				if(x3 >= resx) x3 = (resx + resx - x3) % resx;
				dx = xf - ((int)xf);
			}			
		}
		else
		{
			if(xf > 0.f)
			{
				x0 = (resx+x-1) % resx;
				x2 = (x+1) % resx;
				x3 = (x+2) % resx;
				dx = xf - ((int)xf);
			}
			else
			{
				x0 = 1 % resx;
				x2 = (resx-1) % resx;
				x3 = (resx-2) % resx;
				dx = -xf;
			}
		}
		
		if(mirrorY)
		{

			if(yf < 0.f)
			{
				y0 = 1 % resy;
				y2 = y;
				y3 = y0;
				dy = -yf;
			}
			else if(yf >= resy - 1.f)
			{
				y0 = (resy+resy-1) % resy;
				y2 = y;
				y3 = y0;
				dy = yf - ((int)yf);
			}
			else
			{
				y0 = (resy+y-1) % resy;
				y2 = y+1;
				if(y2 >= resy) y2 = (resy + resy - y2) % resy;
				y3 = y+2;
				if(y3 >= resy) y3 = (resy + resy - y3) % resy;
				dy = yf - ((int)yf);
			}			
		}
		else
		{
			if(yf > 0.f)
			{
				y0 = (resy+y-1) % resy;
				y2 = (y+1) % resy;
				y3 = (y+2) % resy;
				dy = yf - ((int)yf);
			}
			else
			{
				y0 = 1 % resy;
				y2 = (resy-1) % resy;
				y3 = (resy-2) % resy;
				dy = -yf;
			}
		}
	}
	else
	{
		x = std::max(0, std::min(resx-1, ((int)xf)));
		y = std::max(0, std::min(resy-1, ((int)yf)));

		if(xf > 0.f) x2 = std::min(resx-1, x+1);
		else x2 = 0;
		
		if(yf > 0.f) y2 = std::min(resy-1, y+1);
		else y2 = 0;

		x0 = std::max(0, x-1);
		x3 = std::min(resx-1, x2+1);
		y0 = std::max(0, y-1);
		y3 = std::min(resy-1, y2+1);
		
		dx = xf - floor(xf);
		dy = yf - floor(yf);
	}

	colorA_t c1;
	if(from_postprocessed && postProcessedImage) c1 = (*postProcessedImage)(x, y);
	else c1 = image->getPixel(x, y);
	
	if (intp_type == INTP_NONE) return c1;
	
	colorA_t c2, c3, c4;
	
	if(from_postprocessed && postProcessedImage)
	{
		c2 = (*postProcessedImage)(x2, y);
		c3 = (*postProcessedImage)(x, y2);
		c4 = (*postProcessedImage)(x2, y2);
	}
	else
	{
		c2 = image->getPixel(x2, y);
		c3 = image->getPixel(x, y2);
		c4 = image->getPixel(x2, y2);
	}

	if (intp_type == INTP_BILINEAR)
	{
		float w0 = (1-dx) * (1-dy);
		float w1 = (1-dx) * dy;
		float w2 = dx * (1-dy);
		float w3 = dx * dy;
		
		return (w0 * c1) + (w1 * c3) + (w2 * c2) + (w3 * c4);
	}

	colorA_t c0, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF, ret;

	if(from_postprocessed && postProcessedImage)
	{
		c0 = (*postProcessedImage)(x0, y0);
		c5 = (*postProcessedImage)(x,  y0);
		c6 = (*postProcessedImage)(x2, y0);
		c7 = (*postProcessedImage)(x3, y0);
		c8 = (*postProcessedImage)(x0, y);
		c9 = (*postProcessedImage)(x3, y);
		cA = (*postProcessedImage)(x0, y2);
		cB = (*postProcessedImage)(x3, y2);
		cC = (*postProcessedImage)(x0, y3);
		cD = (*postProcessedImage)(x,  y3);
		cE = (*postProcessedImage)(x2, y3);
		cF = (*postProcessedImage)(x3, y3);
	}
	else
	{
		c0 = image->getPixel(x0, y0);
		c5 = image->getPixel(x,  y0);
		c6 = image->getPixel(x2, y0);
		c7 = image->getPixel(x3, y0);
		c8 = image->getPixel(x0, y);
		c9 = image->getPixel(x3, y);
		cA = image->getPixel(x0, y2);
		cB = image->getPixel(x3, y2);
		cC = image->getPixel(x0, y3);
		cD = image->getPixel(x,  y3);
		cE = image->getPixel(x2, y3);
		cF = image->getPixel(x3, y3);
	}
	
	c0 = CubicInterpolate(c0, c5, c6, c7, dx);
	c8 = CubicInterpolate(c8, c1, c2, c9, dx);
	cA = CubicInterpolate(cA, c3, c4, cB, dx);
	cC = CubicInterpolate(cC, cD, cE, cF, dx);
	
	c0 = CubicInterpolate(c0, c8, cA, cC, dy);
	
	return c0;
}

colorA_t textureImage_t::getColor(const point3d_t &p, float dSdx, float dTdx, float dSdy, float dTdy, bool from_postprocessed) const
{
	colorA_t ret = getRawColor(p, dSdx, dTdx, dSdy, dTdy, from_postprocessed);
	ret.linearRGB_from_ColorSpace(colorSpace, gamma);
	
	return applyAdjustments(ret);
}

colorA_t textureImage_t::getRawColor(const point3d_t &p, float dSdx, float dTdx, float dSdy, float dTdy, bool from_postprocessed) const
{
	point3d_t p1 = point3d_t(p.x, -p.y, p.z);
	colorA_t ret(0.f);

	bool outside = doMapping(p1);

	if(outside) return ret;
	
	ret = interpolateImage(p1, dSdx, dTdx, dSdy, dTdy, from_postprocessed);
	
	return ret;
}

colorA_t textureImage_t::getColor(int x, int y, int z, float dSdx, float dTdx, float dSdy, float dTdy, bool from_postprocessed) const
{
	colorA_t ret = getRawColor(x, y, z, dSdx, dTdx, dSdy, dTdy, from_postprocessed);
	ret.linearRGB_from_ColorSpace(colorSpace, gamma);
	
	return applyAdjustments(ret);
}

colorA_t textureImage_t::getRawColor(int x, int y, int z, float dSdx, float dTdx, float dSdy, float dTdy, bool from_postprocessed) const
{
	return color_t(x, y, z);
	int resx=image->getWidth();
	int resy=image->getHeight();

	y = resy - y; //on occasion change image storage from bottom to top...

	x = std::max(0, std::min(resx-1, x));
	y = std::max(0, std::min(resy-1, y));

	colorA_t c1(0.f);
	
	if(from_postprocessed && postProcessedImage) return (*postProcessedImage)(x, y);
	else return image->getPixel(x, y, 0.f); //FIXME DAVID!!
}

bool textureImage_t::doMapping(point3d_t &texpt) const
{
	bool outside = false;
	
	texpt = 0.5f*texpt + 0.5f;
	// repeat, only valid for REPEAT clipmode
	if (tex_clipmode==TCL_REPEAT) {
		
		if(xrepeat > 1) texpt.x *= (float)xrepeat;
		if(yrepeat > 1) texpt.y *= (float)yrepeat;
		
		if (mirrorX && int(ceilf(texpt.x)) % 2 == 0) texpt.x = -texpt.x;
		if (mirrorY && int(ceilf(texpt.y)) % 2 == 0) texpt.y = -texpt.y;
		
		if (texpt.x>1.f) texpt.x -= int(texpt.x);
		else if (texpt.x<0.f) texpt.x += 1 - int(texpt.x);
		
		if (texpt.y>1.f) texpt.y -= int(texpt.y);
		else if (texpt.y<0.f) texpt.y += 1 - int(texpt.y);
	}
	
	// crop
	if (cropx) texpt.x = cropminx + texpt.x*(cropmaxx - cropminx);
	if (cropy) texpt.y = cropminy + texpt.y*(cropmaxy - cropminy);
	
	// rot90
	if(rot90) std::swap(texpt.x, texpt.y);
	
	// clipping
	switch (tex_clipmode)
	{
		case TCL_CLIPCUBE: {
			if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1) || (texpt.z<-1) || (texpt.z>1))
				outside = true;
			break;
		}
		case TCL_CHECKER: {
			int xs=(int)floor(texpt.x), ys=(int)floor(texpt.y);
			texpt.x -= xs;
			texpt.y -= ys;
			if ( !checker_odd && !((xs+ys) & 1) )
			{
				outside = true;
				break;
			}
			if ( !checker_even && ((xs+ys) & 1) )
			{
				outside = true;
				break;
			}
			// scale around center, (0.5, 0.5)
			if (checker_dist<1.0) {
				texpt.x = (texpt.x-0.5) / (1.0-checker_dist) + 0.5;
				texpt.y = (texpt.y-0.5) / (1.0-checker_dist) + 0.5;
			}
			// continue to TCL_CLIP
		}
		case TCL_CLIP: {
			if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1))
				outside = true;
			break;
		}
		case TCL_EXTEND: {
			if (texpt.x>0.99999f) texpt.x=0.99999f; else if (texpt.x<0) texpt.x=0;
			if (texpt.y>0.99999f) texpt.y=0.99999f; else if (texpt.y<0) texpt.y=0;
			// no break, fall thru to TEX_REPEAT
		}
		default:
		case TCL_REPEAT: outside = false;
	}
	return outside;
}

void textureImage_t::setCrop(float minx, float miny, float maxx, float maxy)
{
	cropminx=minx, cropmaxx=maxx, cropminy=miny, cropmaxy=maxy;
	cropx = ((cropminx!=0.0) || (cropmaxx!=1.0));
	cropy = ((cropminy!=0.0) || (cropmaxy!=1.0));
}

void textureImage_t::postProcessedCreate()
{
	int w=0, h=0, z=0;
	resolution(w, h, z);
	
	postProcessedImage = new rgba2DImage_nw_t(w, h);
}

void textureImage_t::postProcessedBlur(float blur_factor)
{
//The background blur functionality will only work if YafaRay is built with OpenCV support
#ifdef HAVE_OPENCV
	if(!postProcessedImage) return;
	
	int w=0, h=0, z=0;
	resolution(w, h, z);
	
	cv::Mat A(h, w, CV_32FC4);
	cv::Mat B(h, w, CV_32FC4);
	cv::Mat_<cv::Vec4f> _A = A;
	cv::Mat_<cv::Vec4f> _B = B;
	
	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			colorA_t color = image->getPixel(x,y);
			color.linearRGB_from_ColorSpace(colorSpace, gamma);

			_A(y, x)[0] = color.getR();
			_A(y, x)[1] = color.getG();
			_A(y, x)[2] = color.getB();
			_A(y, x)[3] = color.getA();
		}
	}

	int blurSize = (int) ceil(std::min(w,h) * blur_factor);
	if(blurSize % 2 == 0) blurSize += 1;
	
	cv::GaussianBlur(A, B, cv::Size(blurSize,blurSize),0.0); //Important, sizes must be odd!

	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			(*postProcessedImage)(x,y).R = _B(y, x)[0];
			(*postProcessedImage)(x,y).G = _B(y, x)[1];
			(*postProcessedImage)(x,y).B = _B(y, x)[2];
			(*postProcessedImage)(x,y).A = _B(y, x)[3];

			(*postProcessedImage)(x,y).ColorSpace_from_linearRGB(colorSpace, gamma);
		}
	}

	for(int i=0; i<w; ++i)
	{
		for(int j=0; j<h; ++j)
		{
//			(*postProcessedImage)(i,j) = image->getPixel(i,j) * colorA_t(1.f,0.f,0.f,1.f);
		}
	}
#endif	//If YafaRay is not built with OpenCV, skip the OpenCV image processing
}

int string2cliptype(const std::string *clipname)
{
	// default "repeat"
	int	tex_clipmode = TCL_REPEAT;
	if(!clipname) return tex_clipmode;
	if (*clipname=="extend")			tex_clipmode = TCL_EXTEND;
	else if (*clipname=="clip")		tex_clipmode = TCL_CLIP;
	else if (*clipname=="clipcube")	tex_clipmode = TCL_CLIPCUBE;
	else if (*clipname=="checker")	tex_clipmode = TCL_CHECKER;
	return tex_clipmode;
}

texture_t *textureImage_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	const std::string *name = nullptr;
	const std::string *intpstr = nullptr;
	double gamma = 1.0;
	double expadj = 0.0;
	bool normalmap = false;
	std::string color_space_string = "Raw_Manual_Gamma";
	colorSpaces_t color_space = RAW_MANUAL_GAMMA;
	std::string texture_optimization_string = "none";
	textureOptimization_t texture_optimization = TEX_OPTIMIZATION_NONE;
	bool img_grayscale = false;
	textureImage_t *tex = nullptr;
	imageHandler_t *ih = nullptr;
	params.getParam("interpolate", intpstr);
	params.getParam("color_space", color_space_string);
	params.getParam("gamma", gamma);
	params.getParam("exposure_adjust", expadj);
	params.getParam("normalmap", normalmap);
	params.getParam("filename", name);
	params.getParam("texture_optimization", texture_optimization_string);
	params.getParam("img_grayscale", img_grayscale);
	
	if(!name)
	{
		Y_ERROR << "ImageTexture: Required argument filename not found for image texture" << yendl;
		return nullptr;
	}
	
	// interpolation type, bilinear default
	int intp = INTP_BILINEAR;
	
	if(intpstr)
	{
		if (*intpstr == "none") intp = INTP_NONE;
		else if (*intpstr == "bicubic") intp = INTP_BICUBIC;
		else if (*intpstr == "mipmap_trilinear") intp = INTP_MIPMAP_TRILINEAR;
	}
	
	size_t lDot = name->rfind(".") + 1;
	size_t lSlash = name->rfind("/") + 1;
	
	std::string ext = toLower(name->substr(lDot));
	
	std::string fmt = render.getImageFormatFromExtension(ext);
	
	if(fmt == "")
	{
		Y_ERROR << "ImageTexture: Image extension not recognized, dropping texture." << yendl;
		return nullptr;
	}
	
	paraMap_t ihpm;
	ihpm["type"] = fmt;
	ihpm["for_output"] = false;
	std::string ihname = "ih";
	ihname.append(toLower(name->substr(lSlash, lDot - lSlash - 1)));
	
	ih = render.createImageHandler(ihname, ihpm);
	
	if(!ih)
	{
		Y_ERROR << "ImageTexture: Couldn't create image handler, dropping texture." << yendl;
		return nullptr;
	}
	

	if(ih->isHDR())
	{
		if(color_space_string != "LinearRGB") Y_VERBOSE << "ImageTexture: The image is a HDR/EXR file: forcing linear RGB and ignoring selected color space '" << color_space_string <<"' and the gamma setting." << yendl;
		color_space = LINEAR_RGB;
		if(texture_optimization_string != "none") Y_VERBOSE << "ImageTexture: The image is a HDR/EXR file: forcing texture optimization to 'none' and ignoring selected texture optimization '" << texture_optimization_string <<"'" << yendl;
		texture_optimization = TEX_OPTIMIZATION_NONE;
	}
	else
	{
		if(color_space_string == "sRGB") color_space = SRGB;
		else if(color_space_string == "XYZ") color_space = XYZ_D65;
		else if(color_space_string == "LinearRGB") color_space = LINEAR_RGB;
		else if(color_space_string == "Raw_Manual_Gamma") color_space = RAW_MANUAL_GAMMA;
		else color_space = SRGB;
		
		if(texture_optimization_string == "none") texture_optimization = TEX_OPTIMIZATION_NONE;
		else if(texture_optimization_string == "optimized") texture_optimization = TEX_OPTIMIZATION_OPTIMIZED;
		else if(texture_optimization_string == "compressed") texture_optimization = TEX_OPTIMIZATION_COMPRESSED;
		else texture_optimization = TEX_OPTIMIZATION_NONE;
	}
	
	ih->setColorSpace(color_space, gamma);
	ih->setTextureOptimization(texture_optimization);
	ih->setGrayScaleSetting(img_grayscale);

	if(!ih->loadFromFile(*name))
	{
		Y_ERROR << "ImageTexture: Couldn't load image file, dropping texture." << yendl;
		return nullptr;
	}

	if(intp == INTP_MIPMAP_TRILINEAR)
	{
		ih->generateMipMaps();
		//FIXME DAVID: TEST SAVING MIPMAPS
/*		for(int i=0; i<=ih->getHighestImgIndex(); ++i)
		{
			std::stringstream ss;
			ss << "//tmp//pepe" << i << ".png";
			ih->saveToFile(ss.str(), i);
		}*/
	}
	
	tex = new textureImage_t(ih, intp, gamma, color_space);

	if(!tex)
	{
		Y_ERROR << "ImageTexture: Couldn't create image texture." << yendl;
		return nullptr;
	}

	// setup image
	bool rot90 = false;
	bool even_tiles=false, odd_tiles=true;
	bool use_alpha=true, calc_alpha=false;
	int xrep=1, yrep=1;
	double minx=0.0, miny=0.0, maxx=1.0, maxy=1.0;
	double cdist=0.0;
	const std::string *clipmode=0;
	bool mirror_x = false;
	bool mirror_y = false;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	float mipmapleveltest = 0.f;	//FIXME DAVID, JUST FOR TESTS
	
	params.getParam("xrepeat", xrep);
	params.getParam("yrepeat", yrep);
	params.getParam("cropmin_x", minx);
	params.getParam("cropmin_y", miny);
	params.getParam("cropmax_x", maxx);
	params.getParam("cropmax_y", maxy);
	params.getParam("rot90", rot90);
	params.getParam("clipping", clipmode);
	params.getParam("even_tiles", even_tiles);
	params.getParam("odd_tiles", odd_tiles);
	params.getParam("checker_dist", cdist);
	params.getParam("use_alpha", use_alpha);
	params.getParam("calc_alpha", calc_alpha);
	params.getParam("mirror_x", mirror_x);
	params.getParam("mirror_y", mirror_y);
	
	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);
		
	tex->xrepeat = xrep;
	tex->yrepeat = yrep;
	tex->rot90 = rot90;
	tex->setCrop(minx, miny, maxx, maxy);
	tex->use_alpha = use_alpha;
	tex->calc_alpha = calc_alpha;
	tex->normalmap = normalmap;
	tex->tex_clipmode = string2cliptype(clipmode);
	tex->checker_even = even_tiles;
	tex->checker_odd = odd_tiles;
	tex->checker_dist = cdist;
	tex->mirrorX = mirror_x;
	tex->mirrorY = mirror_y;
	
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	
	tex->mipmapleveltest = mipmapleveltest;//FIXME DAVID, JUST FOR TESTS
	
	return tex;
}

__END_YAFRAY
