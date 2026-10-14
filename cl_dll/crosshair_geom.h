/*
crosshair_geom.h -- geometry of the customizable crosshair

Builds the triangles of the crosshair from its parameters, with no
dependency, so that the HUD and the options preview draw exactly the
same thing. Sizes are in screen pixels and can be fractional: edges get
an exact one pixel box-filter coverage, carried as a per-vertex alpha.
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
	float gap;        // centre to the inner edge, minus half the thickness (CS:GO convention)
	float length;     // bar length
	float thickness;  // bar or ring thickness
	bool dot;         // centre dot
	float dot_size;   // 0: the thickness
	float outline;    // outline width around every shape, 0: none
	bool antialias;   // coverage edges; off: snapped to whole pixels
};

struct xhair_vertex_t
{
	float x, y;
	float alpha;      // coverage, multiplies the colour alpha
};

#define XHAIR_MAX_VERTICES 8192

// Fills out with triangles (three vertices each) for the crosshair centred
// on cx, cy: the outline layer when outline is true, the crosshair itself
// otherwise. Returns the number of vertices written.
int XHair_Build( const xhair_params_t *p, float cx, float cy, bool outline, xhair_vertex_t *out, int max_vertices );

// Screen centre for a crosshair: the middle of a pixel, so that an odd
// thickness lands on whole pixels instead of straddling two.
float XHair_Centre( int size );

#endif // CROSSHAIR_GEOM_H
