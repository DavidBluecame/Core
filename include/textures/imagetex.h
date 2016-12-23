/****************************************************************************
 *
 *		imagetex.h: Image specific functions
 *      This is part of the yafray package
 *      Copyright (C) 2006  Mathias Wein
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
 *
 */

#ifndef Y_IMAGETEX_H
#define Y_IMAGETEX_H

#include <core_api/texture.h>
#include <core_api/environment.h>
#include <core_api/imagehandler.h>
#include <utilities/interpolation.h>
#include <utilities/image_buffers.h>

__BEGIN_YAFRAY

enum TEX_CLIPMODE
{
	TCL_EXTEND,
	TCL_CLIP,
	TCL_CLIPCUBE,
	TCL_REPEAT,
	TCL_CHECKER
};

class textureImage_t : public texture_t
{
	public:
		textureImage_t(imageHandler_t *ih, int intp, float gamma, colorSpaces_t color_space = RAW_MANUAL_GAMMA);
		virtual ~textureImage_t();
		virtual bool discrete() const { return true; }
		virtual bool isThreeD() const { return false; }
		virtual bool isNormalmap() const { return normalmap; }
		virtual colorA_t getColor(const point3d_t &sp, float dSdx = 0.f, float dTdx = 0.f, float dSdy = 0.f, float dTdy = 0.f, bool from_postprocessed=false) const;
		virtual colorA_t getColor(int x, int y, int z, float dSdx = 0.f, float dTdx = 0.f, float dSdy = 0.f, float dTdy = 0.f, bool from_postprocessed=false) const;
		virtual colorA_t getRawColor(const point3d_t &p, float dSdx = 0.f, float dTdx = 0.f, float dSdy = 0.f, float dTdy = 0.f, bool from_postprocessed=false) const;
		virtual colorA_t getRawColor(int x, int y, int z, float dSdx = 0.f, float dTdx = 0.f, float dSdy = 0.f, float dTdy = 0.f, bool from_postprocessed=false) const;
		virtual void resolution(int &x, int &y, int &z) const;
		virtual void postProcessedCreate();
		virtual void postProcessedBlur(float blur_factor);
		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);

	protected:
		void setCrop(float minx, float miny, float maxx, float maxy);
		bool doMapping(point3d_t &texp) const;
		colorA_t interpolateImage(const point3d_t &p, float dSdx = 0.f, float dTdx = 0.f, float dSdy = 0.f, float dTdy = 0.f, bool from_postprocessed=false) const;
		
		bool use_alpha, calc_alpha, normalmap;
		bool grayscale = false;	//!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case.
		bool cropx, cropy, checker_odd, checker_even, rot90;
		float cropminx, cropmaxx, cropminy, cropmaxy;
		float checker_dist;
		int xrepeat, yrepeat;
		int tex_clipmode;
		imageHandler_t *image;
		colorSpaces_t colorSpace;
		float gamma;
		bool mirrorX;
		bool mirrorY;
		rgba2DImage_nw_t * postProcessedImage = nullptr; //!< rgba color buffer for post-processed image (not linear, still in the original image color space)
		float mipmapleveltest = 0.f;	//FIXME DAVID, JUST FOR TESTS
};

/*static inline colorA_t cubicInterpolate(const colorA_t &c1, const colorA_t &c2,
								 const colorA_t &c3, const colorA_t &c4, float x)
{
	colorA_t t2(c3-c2);
	colorA_t t1(t2 - (c2-c1));
	t2 = (c4-c3) - t2;
	float ix = 1.f-x;
	return x*c3 + ix*c2 + ((4.f*t2 - t1)*(x*x*x-x) + (4.f*t1 - t2)*(ix*ix*ix-ix))*0.06666667f;
}*/

__END_YAFRAY

#endif // Y_IMAGETEX_H
