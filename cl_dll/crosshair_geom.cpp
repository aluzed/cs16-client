/*
crosshair_geom.cpp -- geometry of the customizable crosshair

A pixel shows the fraction of its square that a shape covers. Along one
axis, that fraction for an edge-aligned band [a, b] seen through a one
pixel wide window is a trapezoid of the window centre: 0 at a - 0.5,
rising to min(b - a, 1), flat, back to 0 at b + 0.5. For a rectangle the
coverage is the product of the two axes, which the bilinear interpolation
of a 4x4 vertex grid reproduces exactly. Rings use the same profile
across their radius.
*/

#include <math.h>
#include "crosshair_geom.h"

#define XHAIR_MIN_SEGMENTS 24
#define XHAIR_MAX_SEGMENTS 256
#define XHAIR_PI 3.14159265358979323846f

struct xhair_builder_t
{
	xhair_vertex_t *out;
	int count;
	int max;
};

static void XHair_Vertex( xhair_builder_t *b, float x, float y, float alpha )
{
	if( b->count >= b->max )
		return;

	b->out[b->count].x = x;
	b->out[b->count].y = y;
	b->out[b->count].alpha = alpha;
	b->count++;
}

static void XHair_Quad( xhair_builder_t *b, const xhair_vertex_t v[4] )
{
	if( v[0].alpha <= 0.0f && v[1].alpha <= 0.0f && v[2].alpha <= 0.0f && v[3].alpha <= 0.0f )
		return;

	// v in order around the quad
	if( v[0].alpha == v[1].alpha && v[1].alpha == v[2].alpha && v[2].alpha == v[3].alpha )
	{
		XHair_Vertex( b, v[0].x, v[0].y, v[0].alpha );
		XHair_Vertex( b, v[1].x, v[1].y, v[1].alpha );
		XHair_Vertex( b, v[2].x, v[2].y, v[2].alpha );
		XHair_Vertex( b, v[0].x, v[0].y, v[0].alpha );
		XHair_Vertex( b, v[2].x, v[2].y, v[2].alpha );
		XHair_Vertex( b, v[3].x, v[3].y, v[3].alpha );
		return;
	}

	// two triangles would bend the bilinear coverage along one diagonal:
	// four around the centre, whose coverage is the mean of the corners,
	// stay symmetric and keep the integral exact
	const float cx = ( v[0].x + v[1].x + v[2].x + v[3].x ) * 0.25f;
	const float cy = ( v[0].y + v[1].y + v[2].y + v[3].y ) * 0.25f;
	const float ca = ( v[0].alpha + v[1].alpha + v[2].alpha + v[3].alpha ) * 0.25f;

	for( int i = 0; i < 4; i++ )
	{
		const xhair_vertex_t *p0 = &v[i], *p1 = &v[( i + 1 ) & 3];

		XHair_Vertex( b, cx, cy, ca );
		XHair_Vertex( b, p0->x, p0->y, p0->alpha );
		XHair_Vertex( b, p1->x, p1->y, p1->alpha );
	}
}

// the four stops of the coverage trapezoid of [a, b], and their coverage
static void XHair_Profile( float a, float b, float stops[4], float alpha[4] )
{
	const float plateau = fminf( b - a, 1.0f );

	stops[0] = a - 0.5f;
	stops[1] = fminf( a + 0.5f, b - 0.5f );
	stops[2] = fmaxf( a + 0.5f, b - 0.5f );
	stops[3] = b + 0.5f;
	alpha[0] = 0.0f;
	alpha[1] = plateau;
	alpha[2] = plateau;
	alpha[3] = 0.0f;
}

static void XHair_Rect( xhair_builder_t *b, float x0, float y0, float x1, float y1, bool antialias )
{
	if( x1 <= x0 || y1 <= y0 )
		return;

	if( !antialias )
	{
		// whole pixels, never less than one
		float sx0 = floorf( x0 + 0.5f ), sx1 = floorf( x1 + 0.5f );
		float sy0 = floorf( y0 + 0.5f ), sy1 = floorf( y1 + 0.5f );

		if( sx1 <= sx0 ) sx1 = sx0 + 1.0f;
		if( sy1 <= sy0 ) sy1 = sy0 + 1.0f;

		const xhair_vertex_t v[4] = { { sx0, sy0, 1.0f }, { sx0, sy1, 1.0f }, { sx1, sy1, 1.0f }, { sx1, sy0, 1.0f } };
		XHair_Quad( b, v );
		return;
	}

	float xs[4], xa[4], ys[4], ya[4];
	XHair_Profile( x0, x1, xs, xa );
	XHair_Profile( y0, y1, ys, ya );

	for( int j = 0; j < 3; j++ )
	{
		for( int i = 0; i < 3; i++ )
		{
			const xhair_vertex_t v[4] = {
				{ xs[i], ys[j], xa[i] * ya[j] },
				{ xs[i], ys[j + 1], xa[i] * ya[j + 1] },
				{ xs[i + 1], ys[j + 1], xa[i + 1] * ya[j + 1] },
				{ xs[i + 1], ys[j], xa[i + 1] * ya[j] },
			};
			XHair_Quad( b, v );
		}
	}
}

static void XHair_Ring( xhair_builder_t *b, float cx, float cy, float r0, float r1, bool antialias )
{
	float rs[4], ra[4];
	int bands;

	if( r1 <= r0 || r1 <= 0.0f )
		return;

	if( antialias )
	{
		XHair_Profile( r0, r1, rs, ra );
		rs[0] = fmaxf( rs[0], 0.0f );
		bands = 3;
	}
	else
	{
		rs[0] = fmaxf( r0, 0.0f );
		rs[1] = fmaxf( r1, rs[0] + 1.0f );
		ra[0] = ra[1] = 1.0f;
		bands = 1;
	}

	// segments short enough that the chord stays within a tenth of a pixel
	int segments = (int)ceilf( XHAIR_PI / acosf( fmaxf( 1.0f - 0.1f / fmaxf( rs[bands], 1.0f ), -1.0f )));
	if( segments < XHAIR_MIN_SEGMENTS ) segments = XHAIR_MIN_SEGMENTS;
	if( segments > XHAIR_MAX_SEGMENTS ) segments = XHAIR_MAX_SEGMENTS;

	// an inscribed polygon is smaller than its circle: push the vertices out
	// so that each segment covers the area of its circular sector
	const float step = 2.0f * XHAIR_PI / segments;
	const float k = sqrtf( step / sinf( step ));

	for( int i = 0; i <= bands; i++ )
		rs[i] *= k;

	for( int s = 0; s < segments; s++ )
	{
		const float a0 = 2.0f * XHAIR_PI * s / segments;
		const float a1 = 2.0f * XHAIR_PI * ( s + 1 ) / segments;
		const float c0 = cosf( a0 ), s0 = sinf( a0 );
		const float c1 = cosf( a1 ), s1 = sinf( a1 );

		for( int k = 0; k < bands; k++ )
		{
			const xhair_vertex_t v[4] = {
				{ cx + c0 * rs[k], cy + s0 * rs[k], ra[k] },
				{ cx + c0 * rs[k + 1], cy + s0 * rs[k + 1], ra[k + 1] },
				{ cx + c1 * rs[k + 1], cy + s1 * rs[k + 1], ra[k + 1] },
				{ cx + c1 * rs[k], cy + s1 * rs[k], ra[k] },
			};
			XHair_Quad( b, v );
		}
	}
}

int XHair_Build( const xhair_params_t *p, float cx, float cy, bool outline, xhair_vertex_t *out, int max_vertices )
{
	xhair_builder_t b = { out, 0, max_vertices };
	const float grow = outline ? p->outline : 0.0f;
	const float t = fmaxf( p->thickness, 0.0f );

	if( outline && p->outline <= 0.0f )
		return 0;

	if( p->style == XHAIR_STYLE_CIRCLE )
	{
		// the inner edge of the ring is at the gap, like the inner end of the bars
		const float r = p->gap + t * 0.5f;
		XHair_Ring( &b, cx, cy, r - t * 0.5f - grow, r + t * 0.5f + grow, p->antialias );
	}
	else if( p->length > 0.0f )
	{
		const float half = t * 0.5f;
		const float inner = p->gap + half;  // centre to the inner end of a bar
		const float outer = inner + p->length;

		// left, right, bottom, then top unless T
		XHair_Rect( &b, cx - outer - grow, cy - half - grow, cx - inner + grow, cy + half + grow, p->antialias );
		XHair_Rect( &b, cx + inner - grow, cy - half - grow, cx + outer + grow, cy + half + grow, p->antialias );
		XHair_Rect( &b, cx - half - grow, cy + inner - grow, cx + half + grow, cy + outer + grow, p->antialias );

		if( !p->no_top )
			XHair_Rect( &b, cx - half - grow, cy - outer - grow, cx + half + grow, cy - inner + grow, p->antialias );
	}

	if( p->dot )
	{
		const float half = ( p->dot_size > 0.0f ? p->dot_size : t ) * 0.5f;
		XHair_Rect( &b, cx - half - grow, cy - half - grow, cx + half + grow, cy + half + grow, p->antialias );
	}

	return b.count;
}

float XHair_Centre( int size )
{
	return floorf( size * 0.5f ) + 0.5f;
}
