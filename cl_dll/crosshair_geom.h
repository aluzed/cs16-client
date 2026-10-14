/*
crosshair_geom.h -- geometry of the customizable crosshair

Turns the crosshair parameters into the exact coverage of every screen
pixel, as rectangles of whole pixels with a constant alpha. Sizes can be
fractional: a pixel half covered by a bar gets half its alpha. Drawing
whole pixel rectangles needs nothing from the renderer state, and the HUD
and the options preview share this code, so they draw the same pixels.
*/

#pragma once
#ifndef CROSSHAIR_GEOM_H
#define CROSSHAIR_GEOM_H

enum
{
	XHAIR_STYLE_CROSS = 0,
	XHAIR_STYLE_CIRCLE = 1,
};

struct xhair_params_t
{
	int style;        // XHAIR_STYLE_*
	bool no_top;      // cross without its top bar (T)
	float gap;        // centre to the inner end of the bars, minus half the thickness (CS:GO convention)
	float length;     // bar length
	float thickness;  // bar or ring thickness
	bool dot;         // centre dot
	float dot_size;   // 0: the thickness
	float outline;    // outline width around every shape, 0: none
	bool antialias;   // fractional coverage; off: shapes snapped to whole pixels
};

// pixels x .. x + w - 1, y .. y + h - 1, covered by alpha (0-1)
struct xhair_rect_t
{
	int x, y, w, h;
	float alpha;
};

#define XHAIR_MAX_RECTS 4096

// Fills out with the pixel rectangles of the crosshair centred on cx, cy
// (screen pixels, pixel i spanning [i, i + 1]): the outline layer when
// outline is true, the crosshair itself otherwise. Rectangles of a layer
// don't overlap. Returns how many were written.
int XHair_Build( const xhair_params_t *p, float cx, float cy, bool outline, xhair_rect_t *out, int max_rects );

// Screen centre for a crosshair: the middle of a pixel, so that an odd
// thickness lands on whole pixels instead of straddling two.
float XHair_Centre( int size );

#endif // CROSSHAIR_GEOM_H
