/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// tr_surf.c

#include "../server/exe_headers.h"

#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

//============================================================================

/*
==============
RB_CheckOverflow
==============
*/
void RB_CheckOverflow(const int verts, const int indexes)
{
	if ((tess.num_vertexes + verts) < SHADER_MAX_VERTEXES && (tess.num_indexes + indexes) < SHADER_MAX_INDEXES)
	{
		return;
	}

	RB_EndSurface();

	if (verts >= SHADER_MAX_VERTEXES) {
		Com_Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES);
	}
	if (indexes >= SHADER_MAX_INDEXES) {
		Com_Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES);
	}

	RB_BeginSurface(tess.shader, tess.fogNum);
}

/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, byte* color, const float s1, const float t1, const float s2, const float t2)
{
	vec3_t		normal;

	RB_CHECKOVERFLOW(4, 6);

	const int ndx = tess.num_vertexes;

	// triangle indexes for a simple quad
	tess.indexes[tess.num_indexes] = ndx;
	tess.indexes[tess.num_indexes + 1] = ndx + 1;
	tess.indexes[tess.num_indexes + 2] = ndx + 3;

	tess.indexes[tess.num_indexes + 3] = ndx + 3;
	tess.indexes[tess.num_indexes + 4] = ndx + 1;
	tess.indexes[tess.num_indexes + 5] = ndx + 2;

	tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

	tess.xyz[ndx + 1][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx + 1][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx + 1][2] = origin[2] - left[2] + up[2];

	tess.xyz[ndx + 2][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx + 2][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx + 2][2] = origin[2] - left[2] - up[2];

	tess.xyz[ndx + 3][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx + 3][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx + 3][2] = origin[2] + left[2] - up[2];

	// constant normal all the way around
	VectorSubtract(vec3_origin, backEnd.viewParms.ori.axis[0], normal);

	tess.normal[ndx][0] = tess.normal[ndx + 1][0] = tess.normal[ndx + 2][0] = tess.normal[ndx + 3][0] = normal[0];
	tess.normal[ndx][1] = tess.normal[ndx + 1][1] = tess.normal[ndx + 2][1] = tess.normal[ndx + 3][1] = normal[1];
	tess.normal[ndx][2] = tess.normal[ndx + 1][2] = tess.normal[ndx + 2][2] = tess.normal[ndx + 3][2] = normal[2];

	// standard square texture coordinates
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s1;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t1;

	tess.texCoords[ndx + 1][0][0] = tess.texCoords[ndx + 1][1][0] = s2;
	tess.texCoords[ndx + 1][0][1] = tess.texCoords[ndx + 1][1][1] = t1;

	tess.texCoords[ndx + 2][0][0] = tess.texCoords[ndx + 2][1][0] = s2;
	tess.texCoords[ndx + 2][0][1] = tess.texCoords[ndx + 2][1][1] = t2;

	tess.texCoords[ndx + 3][0][0] = tess.texCoords[ndx + 3][1][0] = s1;
	tess.texCoords[ndx + 3][0][1] = tess.texCoords[ndx + 3][1][1] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	const byteAlias_t* ba_source = reinterpret_cast<byteAlias_t*>(color);
	auto ba_dest = reinterpret_cast<byteAlias_t*>(&tess.vertexColors[ndx + 0]);
	ba_dest->ui = ba_source->ui;
	ba_dest = reinterpret_cast<byteAlias_t*>(&tess.vertexColors[ndx + 1]);
	ba_dest->ui = ba_source->ui;
	ba_dest = reinterpret_cast<byteAlias_t*>(&tess.vertexColors[ndx + 2]);
	ba_dest->ui = ba_source->ui;
	ba_dest = reinterpret_cast<byteAlias_t*>(&tess.vertexColors[ndx + 3]);
	ba_dest->ui = ba_source->ui;

	tess.num_vertexes += 4;
	tess.num_indexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up, byte* color) {
	RB_AddQuadStampExt(origin, left, up, color, 0, 0, 1, 1);
}

/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite() {
	vec3_t		left, up;

	// calculate the xyz locations for the four corners
	const float radius = backEnd.currentEntity->e.radius;
	if (backEnd.currentEntity->e.rotation == 0) {
		VectorScale(backEnd.viewParms.ori.axis[1], radius, left);
		VectorScale(backEnd.viewParms.ori.axis[2], radius, up);
	}
	else {
		const float ang = M_PI * backEnd.currentEntity->e.rotation / 180;
		const float s = sin(ang);
		const float c = cos(ang);

		VectorScale(backEnd.viewParms.ori.axis[1], c * radius, left);
		VectorMA(left, -s * radius, backEnd.viewParms.ori.axis[2], left);

		VectorScale(backEnd.viewParms.ori.axis[2], c * radius, up);
		VectorMA(up, s * radius, backEnd.viewParms.ori.axis[1], up);
	}
	if (backEnd.viewParms.isMirror) {
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA);
}

/*
=======================
RB_SurfaceOrientedQuad
=======================
*/
static void RB_SurfaceOrientedQuad()
{
	vec3_t	left, up;

	// calculate the xyz locations for the four corners
	const float radius = backEnd.currentEntity->e.radius;
	MakeNormalVectors(backEnd.currentEntity->e.axis[0], left, up);

	if (backEnd.currentEntity->e.rotation == 0)
	{
		VectorScale(left, radius, left);
		VectorScale(up, radius, up);
	}
	else
	{
		vec3_t	temp_left, temp_up;

		const float ang = M_PI * backEnd.currentEntity->e.rotation / 180;
		const float s = sin(ang);
		const float c = cos(ang);

		// Use a temp so we don't trash the values we'll need later
		VectorScale(left, c * radius, temp_left);
		VectorMA(temp_left, -s * radius, up, temp_left);

		VectorScale(up, c * radius, temp_up);
		VectorMA(temp_up, s * radius, left, up); // no need to use the temp anymore, so copy into the dest vector ( up )

		// This was copied for safekeeping, we're done, so we can move it back to left
		VectorCopy(temp_left, left);
	}

	if (backEnd.viewParms.isMirror)
	{
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA);
}

/*
==============
RB_SurfaceLine
==============
*/
//
//	Values for a proper line render primitive...
//		Width
//		STScale (how many times to loop a texture)
//		alpha
//		RGB
//
//  Values for proper line object...
//		lifetime
//		dscale
//		startalpha, endalpha
//		startRGB, endRGB
//

static void DoLine(const vec3_t start, const vec3_t end, const vec3_t up, const float spanWidth)
{
	RB_CHECKOVERFLOW(4, 6);

	const int vbase = tess.num_vertexes;

	const float span_width2 = -spanWidth;

	VectorMA(start, spanWidth, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = 0;
	tess.texCoords[tess.num_vertexes][0][1] = 0;
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];// * 0.25;//wtf??not sure why the code would be doing this
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];// * 0.25;
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];// * 0.25;
	tess.vertexColors[tess.num_vertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];// * 0.25;
	tess.num_vertexes++;

	VectorMA(start, span_width2, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.num_vertexes][0][1] = 0;
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.num_vertexes++;

	VectorMA(end, spanWidth, up, tess.xyz[tess.num_vertexes]);

	tess.texCoords[tess.num_vertexes][0][0] = 0;
	tess.texCoords[tess.num_vertexes][0][1] = 1;//backEnd.currentEntity->e.shaderTexCoord[1];
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.num_vertexes++;

	VectorMA(end, span_width2, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.num_vertexes][0][1] = 1;//backEnd.currentEntity->e.shaderTexCoord[1];
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.num_vertexes++;

	tess.indexes[tess.num_indexes++] = vbase;
	tess.indexes[tess.num_indexes++] = vbase + 1;
	tess.indexes[tess.num_indexes++] = vbase + 2;

	tess.indexes[tess.num_indexes++] = vbase + 2;
	tess.indexes[tess.num_indexes++] = vbase + 1;
	tess.indexes[tess.num_indexes++] = vbase + 3;
}

static void DoLine2(const vec3_t start, const vec3_t end, const vec3_t up, const float span_width, const float span_width2, const float tc_start, const float tc_end)
{
	RB_CHECKOVERFLOW(4, 6);

	const int vbase = tess.num_vertexes;

	VectorMA(start, span_width, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = 0;
	tess.texCoords[tess.num_vertexes][0][1] = tc_start;
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];// * 0.25;//wtf??not sure why the code would be doing this
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];// * 0.25;
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];// * 0.25;
	tess.vertexColors[tess.num_vertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];// * 0.25;
	tess.num_vertexes++;

	VectorMA(start, -span_width, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.num_vertexes][0][1] = tc_start;
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.num_vertexes++;

	VectorMA(end, span_width2, up, tess.xyz[tess.num_vertexes]);

	tess.texCoords[tess.num_vertexes][0][0] = 0;
	tess.texCoords[tess.num_vertexes][0][1] = tc_end;//backEnd.currentEntity->e.shaderTexCoord[1];
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.num_vertexes++;

	VectorMA(end, -span_width2, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.num_vertexes][0][1] = tc_end;//backEnd.currentEntity->e.shaderTexCoord[1];
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.num_vertexes++;

	tess.indexes[tess.num_indexes++] = vbase;
	tess.indexes[tess.num_indexes++] = vbase + 1;
	tess.indexes[tess.num_indexes++] = vbase + 2;

	tess.indexes[tess.num_indexes++] = vbase + 2;
	tess.indexes[tess.num_indexes++] = vbase + 1;
	tess.indexes[tess.num_indexes++] = vbase + 3;
}

//-----------------
// RB_SurfaceLine
//-----------------
static void RB_SurfaceLine()
{
	vec3_t		right;
	vec3_t		start, end;
	vec3_t		v1, v2;

	const refEntity_t* e = &backEnd.currentEntity->e;

	VectorCopy(e->oldorigin, end);
	VectorCopy(e->origin, start);

	// compute side vector
	VectorSubtract(start, backEnd.viewParms.ori.origin, v1);
	VectorSubtract(end, backEnd.viewParms.ori.origin, v2);
	CrossProduct(v1, v2, right);
	VectorNormalize(right);

	DoLine(start, end, right, e->radius);
}

/*
==============
RB_SurfaceCylinder
==============
*/

#define NUM_CYLINDER_SEGMENTS 40

// e->origin holds the bottom point
// e->oldorigin holds the top point
// e->radius holds the radius

// If a cylinder has a tapered end that has a very small radius, the engine converts it to a cone.  Not a huge savings, but the texture mapping is slightly better
//	and it uses half as many indicies as the cylinder version
//-------------------------------------
static void RB_SurfaceCone()
//-------------------------------------
{
	static vec3_t points[NUM_CYLINDER_SEGMENTS];
	vec3_t		vr, vu, midpoint;
	vec3_t		tapered, base;
	int			i;

	const refEntity_t* e = &backEnd.currentEntity->e;

	//Work out the detail level of this cylinder
	VectorAdd(e->origin, e->oldorigin, midpoint);
	VectorScale(midpoint, 0.5, midpoint);		// Average start and end

	VectorSubtract(midpoint, backEnd.viewParms.ori.origin, midpoint);
	float length = VectorNormalize(midpoint);

	// this doesn't need to be perfect....just a rough compensation for zoom level is enough
	length *= backEnd.viewParms.fovX / 90.0f;

	float detail = 1 - length / 2048;
	int segments = NUM_CYLINDER_SEGMENTS * detail;

	// 3 is the absolute minimum, but the pop between 3-8 is too noticeable
	if (segments < 8)
	{
		segments = 8;
	}

	if (segments > NUM_CYLINDER_SEGMENTS)
	{
		segments = NUM_CYLINDER_SEGMENTS;
	}

	// Get the direction vector
	MakeNormalVectors(e->axis[0], vr, vu);

	// we only need to rotate around the larger radius, the smaller radius get's welded
	if (e->radius < e->backlerp)
	{
		VectorScale(vu, e->backlerp, vu);
		VectorCopy(e->origin, base);
		VectorCopy(e->oldorigin, tapered);
	}
	else
	{
		VectorScale(vu, e->radius, vu);
		VectorCopy(e->origin, tapered);
		VectorCopy(e->oldorigin, base);
	}

	// Calculate the step around the cylinder
	detail = 360.0f / static_cast<float>(segments);

	for (i = 0; i < segments; i++)
	{
		// ring
		RotatePointAroundVector(points[i], e->axis[0], vu, detail * i);
		VectorAdd(points[i], base, points[i]);
	}

	// Calculate the texture coords so the texture can wrap around the whole cylinder
	detail = 1.0f / static_cast<float>(segments);

	RB_CHECKOVERFLOW(2 * (segments + 1), 3 * segments); // this isn't 100% accurate

	int vbase = tess.num_vertexes;

	for (i = 0; i < segments; i++)
	{
		VectorCopy(points[i], tess.xyz[tess.num_vertexes]);
		tess.texCoords[tess.num_vertexes][0][0] = detail * i;
		tess.texCoords[tess.num_vertexes][0][1] = 1.0f;
		tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
		tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
		tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
		tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
		tess.num_vertexes++;

		// We could add this vert once, but using the given texture mapping method, we need to generate different texture coordinates
		VectorCopy(tapered, tess.xyz[tess.num_vertexes]);
		tess.texCoords[tess.num_vertexes][0][0] = detail * i + detail * 0.5f; // set the texture coordinates to the point half-way between the untapered ends....but on the other end of the texture
		tess.texCoords[tess.num_vertexes][0][1] = 0.0f;
		tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
		tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
		tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
		tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
		tess.num_vertexes++;
	}

	// last point has the same verts as the first, but does not share the same tex coords, so we have to duplicate it
	VectorCopy(points[0], tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = detail * i;
	tess.texCoords[tess.num_vertexes][0][1] = 1.0f;
	tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
	tess.num_vertexes++;

	VectorCopy(tapered, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = detail * i + detail * 0.5f;
	tess.texCoords[tess.num_vertexes][0][1] = 0.0f;
	tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
	tess.num_vertexes++;

	// do the welding
	for (i = 0; i < segments; i++)
	{
		tess.indexes[tess.num_indexes++] = vbase;
		tess.indexes[tess.num_indexes++] = vbase + 1;
		tess.indexes[tess.num_indexes++] = vbase + 2;

		vbase += 2;
	}
}

//-------------------------------------
static void RB_SurfaceCylinder()
//-------------------------------------
{
	static vec3_t lower_points[NUM_CYLINDER_SEGMENTS], upper_points[NUM_CYLINDER_SEGMENTS];
	vec3_t		vr, vu, midpoint, v1;
	int			i;

	const refEntity_t* e = &backEnd.currentEntity->e;

	// check for tapering
	if (!(e->radius < 0.3f && e->backlerp < 0.3f) && (e->radius < 0.3f || e->backlerp < 0.3f))
	{
		// One end is sufficiently tapered to consider changing it to a cone
		RB_SurfaceCone();
		return;
	}

	//Work out the detail level of this cylinder
	VectorAdd(e->origin, e->oldorigin, midpoint);
	VectorScale(midpoint, 0.5, midpoint);		// Average start and end

	VectorSubtract(midpoint, backEnd.viewParms.ori.origin, midpoint);
	float length = VectorNormalize(midpoint);

	// this doesn't need to be perfect....just a rough compensation for zoom level is enough
	length *= backEnd.viewParms.fovX / 90.0f;

	float detail = 1 - length / 2048;
	int segments = NUM_CYLINDER_SEGMENTS * detail;

	// 3 is the absolute minimum, but the pop between 3-8 is too noticeable
	if (segments < 8)
	{
		segments = 8;
	}

	if (segments > NUM_CYLINDER_SEGMENTS)
	{
		segments = NUM_CYLINDER_SEGMENTS;
	}

	//Get the direction vector
	MakeNormalVectors(e->axis[0], vr, vu);

	VectorScale(vu, e->radius, v1);	// size1
	VectorScale(vu, e->backlerp, vu);	// size2

	// Calculate the step around the cylinder
	detail = 360.0f / static_cast<float>(segments);

	for (i = 0; i < segments; i++)
	{
		//Upper ring
		RotatePointAroundVector(upper_points[i], e->axis[0], vu, detail * i);
		VectorAdd(upper_points[i], e->origin, upper_points[i]);

		//Lower ring
		RotatePointAroundVector(lower_points[i], e->axis[0], v1, detail * i);
		VectorAdd(lower_points[i], e->oldorigin, lower_points[i]);
	}

	// Calculate the texture coords so the texture can wrap around the whole cylinder
	detail = 1.0f / static_cast<float>(segments);

	RB_CHECKOVERFLOW(2 * (segments + 1), 6 * segments); // this isn't 100% accurate

	int vbase = tess.num_vertexes;

	for (i = 0; i < segments; i++)
	{
		VectorCopy(upper_points[i], tess.xyz[tess.num_vertexes]);
		tess.texCoords[tess.num_vertexes][0][0] = detail * i;
		tess.texCoords[tess.num_vertexes][0][1] = 1.0f;
		tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
		tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
		tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
		tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
		tess.num_vertexes++;

		VectorCopy(lower_points[i], tess.xyz[tess.num_vertexes]);
		tess.texCoords[tess.num_vertexes][0][0] = detail * i;
		tess.texCoords[tess.num_vertexes][0][1] = 0.0f;
		tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
		tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
		tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
		tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
		tess.num_vertexes++;
	}

	// last point has the same verts as the first, but does not share the same tex coords, so we have to duplicate it
	VectorCopy(upper_points[0], tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = detail * i;
	tess.texCoords[tess.num_vertexes][0][1] = 1.0f;
	tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
	tess.num_vertexes++;

	VectorCopy(lower_points[0], tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = detail * i;
	tess.texCoords[tess.num_vertexes][0][1] = 0.0f;
	tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
	tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
	tess.num_vertexes++;

	// glue the verts
	for (i = 0; i < segments; i++)
	{
		tess.indexes[tess.num_indexes++] = vbase;
		tess.indexes[tess.num_indexes++] = vbase + 1;
		tess.indexes[tess.num_indexes++] = vbase + 2;

		tess.indexes[tess.num_indexes++] = vbase + 2;
		tess.indexes[tess.num_indexes++] = vbase + 1;
		tess.indexes[tess.num_indexes++] = vbase + 3;

		vbase += 2;
	}
}

static vec3_t sh1, sh2;
static int f_count;

// Up front, we create a random "shape", then apply that to each line segment...and then again to each of those segments...kind of like a fractal
//----------------------------------------------------------------------------
static void CreateShape()
//----------------------------------------------------------------------------
{
	VectorSet(sh1, 0.66f,// + Q_flrand(-1.0f, 1.0f) * 0.1f,	// fwd
		0.08f + Q_flrand(-1.0f, 1.0f) * 0.02f,
		0.08f + Q_flrand(-1.0f, 1.0f) * 0.02f);

	// it seems to look best to have a point on one side of the ideal line, then the other point on the other side.
	VectorSet(sh2, 0.33f,// + Q_flrand(-1.0f, 1.0f) * 0.1f,	// fwd
		-sh1[1] + Q_flrand(-1.0f, 1.0f) * 0.02f,	// forcing point to be on the opposite side of the line -- right
		-sh1[2] + Q_flrand(-1.0f, 1.0f) * 0.02f);// up
}

//----------------------------------------------------------------------------
static void ApplyShape(vec3_t start, vec3_t end, vec3_t right, const float sradius, const float eradius, const int count, const float start_perc, const float end_perc)
//----------------------------------------------------------------------------
{
	vec3_t	point1, point2, fwd;
	vec3_t	rt, up;

	if (count < 1)
	{
		// done recursing
		DoLine2(start, end, right, sradius, eradius, start_perc, end_perc);
		return;
	}

	CreateShape();

	VectorSubtract(end, start, fwd);
	const float dis = VectorNormalize(fwd) * 0.7f;
	MakeNormalVectors(fwd, rt, up);

	float perc = sh1[0];

	VectorScale(start, perc, point1);
	VectorMA(point1, 1.0f - perc, end, point1);
	VectorMA(point1, dis * sh1[1], rt, point1);
	VectorMA(point1, dis * sh1[2], up, point1);

	const float rads1 = sradius * 0.666f + eradius * 0.333f;
	const float rads2 = sradius * 0.333f + eradius * 0.666f;

	// recursion
	ApplyShape(start, point1, right, sradius, rads1, count - 1, start_perc, start_perc * 0.666f + end_perc * 0.333f);

	perc = sh2[0];

	VectorScale(start, perc, point2);
	VectorMA(point2, 1.0f - perc, end, point2);
	VectorMA(point2, dis * sh2[1], rt, point2);
	VectorMA(point2, dis * sh2[2], up, point2);

	// recursion
	ApplyShape(point2, point1, right, rads1, rads2, count - 1, start_perc * 0.333f + end_perc * 0.666f, start_perc * 0.666f + end_perc * 0.333f);
	ApplyShape(point2, end, right, rads2, eradius, count - 1, start_perc * 0.333f + end_perc * 0.666f, end_perc);
}

//----------------------------------------------------------------------------
static void DoBoltSeg(vec3_t start, vec3_t end, vec3_t right, const float radius)
//----------------------------------------------------------------------------
{
	vec3_t fwd, old;
	vec3_t off = { 10,10,10 };
	vec3_t rt, up;
	float old_perc = 0.0f, perc, old_radius;

	refEntity_t* e = &backEnd.currentEntity->e;

	VectorSubtract(end, start, fwd);
	float dis = VectorNormalize(fwd);

	if (dis > 2000)	//freaky long
	{
		//		ri.Printf( PRINT_WARNING, "DoBoltSeg: insane distance.\n" );
		dis = 2000;
	}
	MakeNormalVectors(fwd, rt, up);

	VectorCopy(start, old);

	float new_radius = old_radius = radius;

	for (int i = 16; i <= dis; i += 16)
	{
		vec3_t temp;
		vec3_t cur;
		// because of our large step size, we may not actually draw to the end.  In this case, fudge our percent so that we are basically complete
		if (i + 16 > dis)
		{
			perc = 1.0f;
		}
		else
		{
			// percentage of the amount of line completed
			perc = static_cast<float>(i) / dis;
		}

		// create our level of deviation for this point
		VectorScale(fwd, Q_crandom(&e->frame) * 3.0f, temp);				// move less in fwd direction, chaos also does not affect this
		VectorMA(temp, Q_crandom(&e->frame) * 7.0f * e->angles[0], rt, temp);	// move more in direction perpendicular to line, angles is really the chaos
		VectorMA(temp, Q_crandom(&e->frame) * 7.0f * e->angles[0], up, temp);	// move more in direction perpendicular to line

		// track our total level of offset from the ideal line
		VectorAdd(off, temp, off);

		// Move from start to end, always adding our current level of offset from the ideal line
		//	Even though we are adding a random offset.....by nature, we always move from exactly start....to end
		VectorAdd(start, off, cur);
		VectorScale(cur, 1.0f - perc, cur);
		VectorMA(cur, perc, end, cur);

		if (e->renderfx & RF_TAPERED)
		{
			// This does pretty close to perfect tapering since apply shape interpolates the old and new as it goes along.
			//	by using one minus the square, the radius stays fairly constant, then drops off quickly at the very point of the bolt
			old_radius = radius * (1.0f - old_perc * old_perc);
			new_radius = radius * (1.0f - perc * perc);
		}

		// Apply the random shape to our line seg to give it some micro-detail-jaggy-coolness.
		ApplyShape(cur, old, right, new_radius, old_radius, 2 - r_lodbias->integer, 0, 1);

		// randomly split off to create little tendrils, but don't do it too close to the end and especially if we are not even of the forked variety
		if (e->renderfx & RF_FORKED && f_count > 0 && Q_random(&e->frame) > 0.93f && 1.0f - perc > 0.8f)
		{
			vec3_t new_dest;

			f_count--;

			// Pick a point somewhere between the current point and the final endpoint
			VectorAdd(cur, e->oldorigin, new_dest);
			VectorScale(new_dest, 0.5f, new_dest);

			// And then add some crazy offset
			for (float& t : new_dest)
			{
				t += Q_crandom(&e->frame) * 80;
			}

			// we could branch off using OLD and NEWDEST, but that would allow multiple forks...whereas, we just want simpler brancing
			DoBoltSeg(cur, new_dest, right, new_radius);
		}

		// Current point along the line becomes our new old attach point
		VectorCopy(cur, old);
		old_perc = perc;
	}
}

static void DoRailCore(const vec3_t start, const vec3_t end, const vec3_t up, const float len, const float span_width)
{
	const float		t = len / 256.0f;

	const int vbase = tess.num_vertexes;

	const float spanWidth2 = -span_width;

	// FIXME: use quad stamp?
	VectorMA(start, span_width, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = 0;
	tess.texCoords[tess.num_vertexes][0][1] = 0;
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 0.25;
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 0.25;
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 0.25;
	tess.num_vertexes++;

	VectorMA(start, spanWidth2, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = 0;
	tess.texCoords[tess.num_vertexes][0][1] = 1;
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.num_vertexes++;

	VectorMA(end, span_width, up, tess.xyz[tess.num_vertexes]);

	tess.texCoords[tess.num_vertexes][0][0] = t;
	tess.texCoords[tess.num_vertexes][0][1] = 0;
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.num_vertexes++;

	VectorMA(end, spanWidth2, up, tess.xyz[tess.num_vertexes]);
	tess.texCoords[tess.num_vertexes][0][0] = t;
	tess.texCoords[tess.num_vertexes][0][1] = 1;
	tess.vertexColors[tess.num_vertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.num_vertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.num_vertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.num_vertexes++;

	tess.indexes[tess.num_indexes++] = vbase;
	tess.indexes[tess.num_indexes++] = vbase + 1;
	tess.indexes[tess.num_indexes++] = vbase + 2;

	tess.indexes[tess.num_indexes++] = vbase + 2;
	tess.indexes[tess.num_indexes++] = vbase + 1;
	tess.indexes[tess.num_indexes++] = vbase + 3;
}

//------------------------------------------
static void RB_SurfaceElectricity()
//------------------------------------------
{
	vec3_t		right, fwd;
	vec3_t		start, end;
	vec3_t		v1, v2;
	float perc = 1.0f;

	refEntity_t* e = &backEnd.currentEntity->e;
	const float radius = e->radius;

	VectorCopy(e->origin, start);

	VectorSubtract(e->oldorigin, start, fwd);
	const float dis = VectorNormalize(fwd);

	// see if we should grow from start to end
	if (e->renderfx & RF_GROW)
	{
		perc = 1.0f - (e->endTime - tr.refdef.time) / e->angles[1]/*duration*/;

		if (perc > 1.0f)
		{
			perc = 1.0f;
		}
		else if (perc < 0.0f)
		{
			perc = 0.0f;
		}
	}

	VectorMA(start, perc * dis, fwd, e->oldorigin);
	VectorCopy(e->oldorigin, end);

	// compute side vector
	VectorSubtract(start, backEnd.viewParms.ori.origin, v1);
	VectorSubtract(end, backEnd.viewParms.ori.origin, v2);
	CrossProduct(v1, v2, right);
	VectorNormalize(right);

	// allow now more than three branches on branch type electricity
	f_count = 3;
	DoBoltSeg(start, end, right, radius);
}

void RB_SurfaceLightningBolt()
{
	vec3_t		right;
	vec3_t		vec;
	vec3_t		start, end;
	vec3_t		v1, v2;

	const refEntity_t* e = &backEnd.currentEntity->e;

	VectorCopy(e->oldorigin, end);
	VectorCopy(e->origin, start);

	// compute variables
	VectorSubtract(end, start, vec);
	const int len = VectorNormalize(vec);

	// compute side vector
	VectorSubtract(start, backEnd.viewParms.ori.origin, v1);
	VectorNormalize(v1);
	VectorSubtract(end, backEnd.viewParms.ori.origin, v2);
	VectorNormalize(v2);
	CrossProduct(v1, v2, right);
	VectorNormalize(right);

	for (int i = 0; i < 4; i++) {
		vec3_t	temp;

		DoRailCore(start, end, right, len, 8);
		RotatePointAroundVector(temp, vec, right, 45);
		VectorCopy(temp, right);
	}
}

/*
=============
RB_SurfacePolychain
=============
*/
/* // we could try to do something similar to this to get better normals into the tess for these types of surfs.  As it stands, any shader pass that
//	requires a normal ( env map ) will not work properly since the normals seem to essentially be random garbage.
void RB_SurfacePolychain( srfPoly_t *p ) {
	int		i;
	int		numv;
	vec3_t	a,b,normal={1,0,0};

	RB_CHECKOVERFLOW( p->num_verts, 3*(p->num_verts - 2) );

	if ( p->num_verts >= 3 )
	{
		VectorSubtract( p->verts[0].xyz, p->verts[1].xyz, a );
		VectorSubtract( p->verts[2].xyz, p->verts[1].xyz, b );
		CrossProduct( a,b, normal );
		VectorNormalize( normal );
	}

	// fan triangles into the tess array
	numv = tess.num_vertexes;
	for ( i = 0; i < p->num_verts; i++ ) {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		VectorCopy( normal, tess.normal[numv] );
		*(int *)&tess.vertexColors[numv] = *(int *)p->verts[ i ].modulate;

		numv++;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->num_verts-2; i++ ) {
		tess.indexes[tess.num_indexes + 0] = tess.num_vertexes;
		tess.indexes[tess.num_indexes + 1] = tess.num_vertexes + i + 1;
		tess.indexes[tess.num_indexes + 2] = tess.num_vertexes + i + 2;
		tess.num_indexes += 3;
	}

	tess.num_vertexes = numv;
}
*/
void RB_SurfacePolychain(const srfPoly_t* p) {
	int		i;

	RB_CHECKOVERFLOW(p->num_verts, 3 * (p->num_verts - 2));

	// fan triangles into the tess array
	int numv = tess.num_vertexes;
	for (i = 0; i < p->num_verts; i++) {
		VectorCopy(p->verts[i].xyz, tess.xyz[numv]);
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		const auto ba_dest = reinterpret_cast<byteAlias_t*>(&tess.vertexColors[numv++]);
		const byteAlias_t* ba_source = reinterpret_cast<byteAlias_t*>(&p->verts[i].modulate);
		ba_dest->i = ba_source->i;
	}

	// generate fan indexes into the tess array
	for (i = 0; i < p->num_verts - 2; i++) {
		tess.indexes[tess.num_indexes + 0] = tess.num_vertexes;
		tess.indexes[tess.num_indexes + 1] = tess.num_vertexes + i + 1;
		tess.indexes[tess.num_indexes + 2] = tess.num_vertexes + i + 2;
		tess.num_indexes += 3;
	}

	tess.num_vertexes = numv;
}

static uint32_t ComputeFinalVertexColor(const byte* colors) {
	int k;
	byteAlias_t result{};
	uint32_t g, b;

	for (k = 0; k < 4; k++)
		result.b[k] = colors[k];

	if (tess.shader->lightmapIndex[0] != LIGHTMAP_BY_VERTEX)
		// ReSharper disable once CppObjectMemberMightNotBeInitialized
		return result.ui;

	if (r_fullbright->integer) {
		result.b[0] = 255;
		result.b[1] = 255;
		result.b[2] = 255;
		// ReSharper disable once CppObjectMemberMightNotBeInitialized
		return result.ui;
	}
	// an optimization could be added here to compute the style[0] (which is always the world normal light)
	uint32_t r = g = b = 0;
	for (k = 0; k < MAXLIGHTMAPS; k++) {
		if (tess.shader->styles[k] < LS_UNUSED) {
			byte* style_color = styleColors[tess.shader->styles[k]];

			r += static_cast<uint32_t>(*colors++) * static_cast<uint32_t>(*style_color++);
			g += static_cast<uint32_t>(*colors++) * static_cast<uint32_t>(*style_color++);
			b += static_cast<uint32_t>(*colors++) * static_cast<uint32_t>(*style_color);
			colors++;
		}
		else
			break;
	}
	result.b[0] = Com_Clamp(0, 255, r >> 8);
	result.b[1] = Com_Clamp(0, 255, g >> 8);
	result.b[2] = Com_Clamp(0, 255, b >> 8);

	// ReSharper disable once CppObjectMemberMightNotBeInitialized
	return result.ui;
}

/*
=============
RB_SurfaceTriangles
=============
*/
void RB_SurfaceTriangles(const srfTriangles_t* srf)
{
	int			i;

	const int dlight_bits = srf->dlightBits;
	tess.dlightBits |= dlight_bits;

	RB_CHECKOVERFLOW(srf->num_verts, srf->num_indexes);

	for (i = 0; i < srf->num_indexes; i += 3) {
		tess.indexes[tess.num_indexes + i + 0] = tess.num_vertexes + srf->indexes[i + 0];
		tess.indexes[tess.num_indexes + i + 1] = tess.num_vertexes + srf->indexes[i + 1];
		tess.indexes[tess.num_indexes + i + 2] = tess.num_vertexes + srf->indexes[i + 2];
	}
	tess.num_indexes += srf->num_indexes;

	drawVert_t* dv = srf->verts;
	float* xyz = tess.xyz[tess.num_vertexes];
	float* normal = tess.normal[tess.num_vertexes];
	float* tex_coords = tess.texCoords[tess.num_vertexes][0];
	byte* color = tess.vertexColors[tess.num_vertexes];

	for (i = 0; i < srf->num_verts; i++, dv++)
	{
		xyz[0] = dv->xyz[0];
		xyz[1] = dv->xyz[1];
		xyz[2] = dv->xyz[2];
		xyz += 4;

		//if ( needsNormal )
		{
			normal[0] = dv->normal[0];
			normal[1] = dv->normal[1];
			normal[2] = dv->normal[2];
		}
		normal += 4;

		tex_coords[0] = dv->st[0];
		tex_coords[1] = dv->st[1];

		for (int k = 0; k < MAXLIGHTMAPS; k++)
		{
			if (tess.shader->lightmapIndex[k] >= 0)
			{
				tex_coords[2 + k * 2] = dv->lightmap[k][0];
				tex_coords[2 + k * 2 + 1] = dv->lightmap[k][1];
			}
			else
			{	// can't have an empty slot in the middle, so we are done
				break;
			}
		}
		tex_coords += NUM_TEX_COORDS * 2;

		*reinterpret_cast<unsigned*>(color) = ComputeFinalVertexColor(reinterpret_cast<byte*>(dv->color));
		color += 4;
	}

	for (i = 0; i < srf->num_verts; i++) {
		tess.vertexDlightBits[tess.num_vertexes + i] = dlight_bits;
	}

	tess.num_vertexes += srf->num_verts;
}

/*
==============
RB_SurfaceBeam
==============
*/
static void RB_SurfaceBeam()
{
#define NUM_BEAM_SEGS 6
	int	i;
	vec3_t perpvec;
	vec3_t direction{}, normalized_direction{};
	vec3_t	start_points[NUM_BEAM_SEGS]{}, end_points[NUM_BEAM_SEGS]{};
	vec3_t oldorigin{}, origin{};

	const refEntity_t* e = &backEnd.currentEntity->e;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize(normalized_direction) == 0)
		return;

	PerpendicularVector(perpvec, normalized_direction);

	VectorScale(perpvec, 4, perpvec);

	for (i = 0; i < NUM_BEAM_SEGS; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec, 360.0 / NUM_BEAM_SEGS * i);
		//		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd(start_points[i], direction, end_points[i]);
	}

	GL_Bind(tr.whiteImage);

	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	switch (e->skinNum)
	{
	case 1://Green
		qglColor3f(0, 1, 0);
		break;
	case 2://Blue
		qglColor3f(0.5, 0.5, 1);
		break;
	case 0://red
	default:
		qglColor3f(1, 0, 0);
		break;
	}

	qglBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i <= NUM_BEAM_SEGS; i++) {
		qglVertex3fv(start_points[i % NUM_BEAM_SEGS]);
		qglVertex3fv(end_points[i % NUM_BEAM_SEGS]);
	}
	qglEnd();
}

//------------------
// DoSprite
//------------------
static void DoSprite(vec3_t origin, const float radius, const float rotation)
{
	vec3_t	left, up;

	const float ang = M_PI * rotation / 180.0f;
	const float s = sin(ang);
	const float c = cos(ang);

	VectorScale(backEnd.viewParms.ori.axis[1], c * radius, left);
	VectorMA(left, -s * radius, backEnd.viewParms.ori.axis[2], left);

	VectorScale(backEnd.viewParms.ori.axis[2], c * radius, up);
	VectorMA(up, s * radius, backEnd.viewParms.ori.axis[1], up);

	if (backEnd.viewParms.isMirror)
	{
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(origin, left, up, backEnd.currentEntity->e.shaderRGBA);
}

//------------------
// RB_SurfaceSaber
//------------------
static void RB_SurfaceSaberGlow()
{
	refEntity_t* e = &backEnd.currentEntity->e;

	// Render the glow part of the blade
	for (float i = e->saberLength; i > 0; i -= e->radius * 0.65f)
	{
		vec3_t end;
		VectorMA(e->origin, i, e->axis[0], end);

		DoSprite(end, e->radius, 0.0f);//Q_flrand(0.0f, 1.0f) * 360.0f );
		e->radius += 0.017f;
	}

	// Big hilt sprite
	// Please don't kill me Pat...I liked the hilt glow blob, but wanted a subtle pulse.:)  Feel free to ditch it if you don't like it.  --Jeff
	// Please don't kill me Jeff...  The pulse is good, but now I want the halo bigger if the saber is shorter...  --Pat
	DoSprite(e->origin, 2.5f + Q_flrand(0.0f, 0.5f) * 0.15f, 0.0f);
}

/*
** LerpMeshVertexes
*/
static void LerpMeshVertexes(md3Surface_t* surf, const float backlerp)
{
	int		vert_num;
	unsigned lat, lng;

	float* out_xyz = tess.xyz[tess.num_vertexes];
	float* out_normal = tess.normal[tess.num_vertexes];

	short* new_xyz = reinterpret_cast<short*>(reinterpret_cast<byte*>(surf) + surf->ofsXyzNormals)
		+ backEnd.currentEntity->e.frame * surf->num_verts * 4;
	short* new_normals = new_xyz + 3;

	const float new_xyz_scale = MD3_XYZ_SCALE * (1.0 - backlerp);
	const float new_normal_scale = 1.0 - backlerp;

	const int num_verts = surf->num_verts;

	if (backlerp == 0) {
		//
		// just copy the vertexes
		//
		for (vert_num = 0; vert_num < num_verts; vert_num++,
			new_xyz += 4, new_normals += 4,
			out_xyz += 4, out_normal += 4)
		{
			out_xyz[0] = new_xyz[0] * new_xyz_scale;
			out_xyz[1] = new_xyz[1] * new_xyz_scale;
			out_xyz[2] = new_xyz[2] * new_xyz_scale;

			lat = new_normals[0] >> 8 & 0xff;
			lng = new_normals[0] & 0xff;
			lat *= FUNCTABLE_SIZE / 256;
			lng *= FUNCTABLE_SIZE / 256;

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )
			out_normal[0] = tr.sinTable[lat + FUNCTABLE_SIZE / 4 & FUNCTABLE_MASK] * tr.sinTable[lng];
			out_normal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			out_normal[2] = tr.sinTable[lng + FUNCTABLE_SIZE / 4 & FUNCTABLE_MASK];
		}
	}
	else {
		//
		// interpolate and copy the vertex and normal
		//
		short* old_xyz = reinterpret_cast<short*>(reinterpret_cast<byte*>(surf) + surf->ofsXyzNormals)
			+ backEnd.currentEntity->e.oldframe * surf->num_verts * 4;
		short* old_normals = old_xyz + 3;

		const float old_xyz_scale = MD3_XYZ_SCALE * backlerp;
		const float old_normal_scale = backlerp;

		for (vert_num = 0; vert_num < num_verts; vert_num++,
			old_xyz += 4, new_xyz += 4, old_normals += 4, new_normals += 4,
			out_xyz += 4, out_normal += 4)
		{
			vec3_t uncompressed_old_normal{}, uncompressed_new_normal{};

			// interpolate the xyz
			out_xyz[0] = old_xyz[0] * old_xyz_scale + new_xyz[0] * new_xyz_scale;
			out_xyz[1] = old_xyz[1] * old_xyz_scale + new_xyz[1] * new_xyz_scale;
			out_xyz[2] = old_xyz[2] * old_xyz_scale + new_xyz[2] * new_xyz_scale;

			// FIXME: interpolate lat/long instead?
			lat = new_normals[0] >> 8 & 0xff;
			lng = new_normals[0] & 0xff;
			lat *= 4;
			lng *= 4;
			uncompressed_new_normal[0] = tr.sinTable[lat + FUNCTABLE_SIZE / 4 & FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressed_new_normal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressed_new_normal[2] = tr.sinTable[lng + FUNCTABLE_SIZE / 4 & FUNCTABLE_MASK];

			lat = old_normals[0] >> 8 & 0xff;
			lng = old_normals[0] & 0xff;
			lat *= 4;
			lng *= 4;

			uncompressed_old_normal[0] = tr.sinTable[lat + FUNCTABLE_SIZE / 4 & FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressed_old_normal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressed_old_normal[2] = tr.sinTable[lng + FUNCTABLE_SIZE / 4 & FUNCTABLE_MASK];

			out_normal[0] = uncompressed_old_normal[0] * old_normal_scale + uncompressed_new_normal[0] * new_normal_scale;
			out_normal[1] = uncompressed_old_normal[1] * old_normal_scale + uncompressed_new_normal[1] * new_normal_scale;
			out_normal[2] = uncompressed_old_normal[2] * old_normal_scale + uncompressed_new_normal[2] * new_normal_scale;

			VectorNormalize(out_normal);
		}
	}
}

/*
=============
RB_SurfaceMesh
=============
*/
void RB_SurfaceMesh(md3Surface_t* surface) {
	int				j;
	float			backlerp;

	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame) {
		backlerp = 0;
	}
	else {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW(surface->num_verts, surface->numTriangles * 3);

	LerpMeshVertexes(surface, backlerp);

	const int* triangles = reinterpret_cast<int*>(reinterpret_cast<byte*>(surface) + surface->ofsTriangles);
	const int indexes = surface->numTriangles * 3;
	const int bob = tess.num_indexes;
	const int doug = tess.num_vertexes;
	for (j = 0; j < indexes; j++) {
		tess.indexes[bob + j] = doug + triangles[j];
	}
	tess.num_indexes += indexes;

	const float* tex_coords = reinterpret_cast<float*>(reinterpret_cast<byte*>(surface) + surface->ofsSt);

	const int num_verts = surface->num_verts;
	for (j = 0; j < num_verts; j++) {
		tess.texCoords[doug + j][0][0] = tex_coords[j * 2 + 0];
		tess.texCoords[doug + j][0][1] = tex_coords[j * 2 + 1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.num_vertexes += surface->num_verts;
}

/*
==============
RB_SurfaceFace
==============
*/
void RB_SurfaceFace(srfSurfaceFace_t* surf) {
	int			i;

	RB_CHECKOVERFLOW(surf->num_points, surf->numIndices);

	const int dlight_bits = surf->dlightBits;
	tess.dlightBits |= dlight_bits;

	const unsigned int* indices = reinterpret_cast<unsigned*>(reinterpret_cast<char*>(surf) + surf->ofsIndices);

	const int bob = tess.num_vertexes;
	glIndex_t* tess_indexes = tess.indexes + tess.num_indexes;
	for (i = surf->numIndices - 1; i >= 0; i--) {
		tess_indexes[i] = indices[i] + bob;
	}

	tess.num_indexes += surf->numIndices;

	float* v;

	int ndx;

	const int num_points = surf->num_points;

	//if ( tess.shader->needsNormal )
	{
		const float* normal = surf->plane.normal;
		for (i = 0, ndx = tess.num_vertexes; i < num_points; i++, ndx++) {
			VectorCopy(normal, tess.normal[ndx]);
		}
	}

	for (i = 0, v = surf->points[0], ndx = tess.num_vertexes; i < num_points; i++, v += VERTEXSIZE, ndx++) {
		byteAlias_t ba{};
		VectorCopy(v, tess.xyz[ndx]);
		tess.texCoords[ndx][0][0] = v[3];
		tess.texCoords[ndx][0][1] = v[4];
		for (int k = 0; k < MAXLIGHTMAPS; k++)
		{
			if (tess.shader->lightmapIndex[k] >= 0)
			{
				tess.texCoords[ndx][k + 1][0] = v[VERTEX_LM + k * 2];
				tess.texCoords[ndx][k + 1][1] = v[VERTEX_LM + k * 2 + 1];
			}
			else
			{
				break;
			}
		}
		ba.ui = ComputeFinalVertexColor(reinterpret_cast<byte*>(&v[VERTEX_COLOR]));
		for (int j = 0; j < 4; j++)
			tess.vertexColors[ndx][j] = ba.b[j];
		tess.vertexDlightBits[ndx] = dlight_bits;
	}

	tess.num_vertexes += surf->num_points;
}

static float LodErrorForVolume(vec3_t local, const float radius) {
	vec3_t		world{};

	// never let it go negative
	if (r_lodCurveError->value < 0) {
		return 0;
	}

	world[0] = local[0] * backEnd.ori.axis[0][0] + local[1] * backEnd.ori.axis[1][0] +
		local[2] * backEnd.ori.axis[2][0] + backEnd.ori.origin[0];
	world[1] = local[0] * backEnd.ori.axis[0][1] + local[1] * backEnd.ori.axis[1][1] +
		local[2] * backEnd.ori.axis[2][1] + backEnd.ori.origin[1];
	world[2] = local[0] * backEnd.ori.axis[0][2] + local[1] * backEnd.ori.axis[1][2] +
		local[2] * backEnd.ori.axis[2][2] + backEnd.ori.origin[2];

	VectorSubtract(world, backEnd.viewParms.ori.origin, world);
	float d = DotProduct(world, backEnd.viewParms.ori.axis[0]);

	if (d < 0) {
		d = -d;
	}
	d -= radius;
	if (d < 1) {
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
void RB_SurfaceGrid(srfGridMesh_t* cv) {
	int		i, j;
	int irows, vrows;
	int		width_table[MAX_GRID_SIZE]{};
	int		height_table[MAX_GRID_SIZE]{};

	const int dlight_bits = cv->dlightBits;
	tess.dlightBits |= dlight_bits;

	// determine the allowable discrepance
	const float lod_error = LodErrorForVolume(cv->lodOrigin, cv->lodRadius);

	// determine which rows and columns of the subdivision
	// we are actually going to use
	width_table[0] = 0;
	int lod_width = 1;
	for (i = 1; i < cv->width - 1; i++) {
		if (cv->widthLodError[i] <= lod_error) {
			width_table[lod_width] = i;
			lod_width++;
		}
	}
	width_table[lod_width] = cv->width - 1;
	lod_width++;

	height_table[0] = 0;
	int lod_height = 1;
	for (i = 1; i < cv->height - 1; i++) {
		if (cv->heightLodError[i] <= lod_error) {
			height_table[lod_height] = i;
			lod_height++;
		}
	}
	height_table[lod_height] = cv->height - 1;
	lod_height++;

	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	int used = 0;
	while (used < lod_height - 1) {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = (SHADER_MAX_VERTEXES - tess.num_vertexes) / lod_width;
			irows = (SHADER_MAX_INDEXES - tess.num_indexes) / (lod_width * 6);

			// if we don't have enough space for at least one strip, flush the buffer
			if (vrows < 2 || irows < 1) {
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum);
			}
			else {
				break;
			}
		} while (true);

		int rows = irows;
		if (vrows < irows + 1) {
			rows = vrows - 1;
		}
		if (used + rows > lod_height) {
			rows = lod_height - used;
		}

		const int num_vertexes = tess.num_vertexes;

		float* xyz = tess.xyz[num_vertexes];
		float* normal = tess.normal[num_vertexes];
		float* tex_coords = tess.texCoords[num_vertexes][0];
		auto color = reinterpret_cast<unsigned char*>(&tess.vertexColors[num_vertexes]);
		int* v_dlight_bits = &tess.vertexDlightBits[num_vertexes];

		for (i = 0; i < rows; i++) {
			for (j = 0; j < lod_width; j++) {
				const drawVert_t* dv = cv->verts + height_table[used + i] * cv->width
					+ width_table[j];

				xyz[0] = dv->xyz[0];
				xyz[1] = dv->xyz[1];
				xyz[2] = dv->xyz[2];
				xyz += 4;

				tex_coords[0] = dv->st[0];
				tex_coords[1] = dv->st[1];

				for (int k = 0; k < MAXLIGHTMAPS; k++)
				{
					tex_coords[2 + k * 2] = dv->lightmap[k][0];
					tex_coords[2 + k * 2 + 1] = dv->lightmap[k][1];
				}
				tex_coords += NUM_TEX_COORDS * 2;

				//				if ( needsNormal )
				{
					normal[0] = dv->normal[0];
					normal[1] = dv->normal[1];
					normal[2] = dv->normal[2];
				}
				normal += 4;
				*reinterpret_cast<unsigned*>(color) = ComputeFinalVertexColor((byte*)dv->color);
				color += 4;
				*v_dlight_bits++ = dlight_bits;
			}
		}

		// add the indexes
		{
			const int h = rows - 1;
			const int w = lod_width - 1;
			int num_indexes = tess.num_indexes;
			for (i = 0; i < h; i++) {
				for (j = 0; j < w; j++) {
					// vertex order to be reckognized as tristrips
					const int v1 = num_vertexes + i * lod_width + j + 1;
					const int v2 = v1 - 1;
					const int v3 = v2 + lod_width;
					const int v4 = v3 + 1;

					tess.indexes[num_indexes] = v2;
					tess.indexes[num_indexes + 1] = v3;
					tess.indexes[num_indexes + 2] = v1;

					tess.indexes[num_indexes + 3] = v1;
					tess.indexes[num_indexes + 4] = v3;
					tess.indexes[num_indexes + 5] = v4;
					num_indexes += 6;
				}
			}

			tess.num_indexes = num_indexes;
		}

		tess.num_vertexes += rows * lod_width;

		used += rows - 1;
	}
}

#define LATHE_SEG_STEP	10
#define BEZIER_STEP		0.05f	// must be in the range of 0 to 1

// FIXME: This function is horribly expensive
static void RB_SurfaceLathe()
{
	vec2_t		pt, l_oldpt;
	vec2_t		pt2, l_oldpt2{};
	float d = 1.0f, pain = 0.0f;
	int			i;

	const refEntity_t* e = &backEnd.currentEntity->e;

	if (e->endTime && e->endTime > backEnd.refdef.time)
	{
		d = 1.0f - (e->endTime - backEnd.refdef.time) / 1000.0f;
	}

	if (e->frame && e->frame + 1000 > backEnd.refdef.time)
	{
		pain = (backEnd.refdef.time - e->frame) / 1000.0f;
		//		pain *= pain;
		pain = (1.0f - pain) * 0.08f;
	}

	VectorSet2(l_oldpt, e->axis[0][0], e->axis[0][1]);

	// do scalability stuff...r_lodbias 0-3
	int lod = r_lodbias->integer + 1;
	if (lod > 4)
	{
		lod = 4;
	}
	if (lod < 1)
	{
		lod = 1;
	}
	const float bezier_step = BEZIER_STEP * lod;
	const float lathe_step = LATHE_SEG_STEP * lod;

	// Do bezier profile strip, then lathe this around to make a 3d model
	for (float mu = 0.0f; mu <= 1.01f * d; mu += bezier_step)
	{
		vec2_t oldpt2;
		vec2_t oldpt;
		// Four point curve
		const float mum1 = 1 - mu;
		const float mum13 = mum1 * mum1 * mum1;
		const float mu3 = mu * mu * mu;
		const float group1 = 3 * mu * mum1 * mum1;
		const float group2 = 3 * mu * mu * mum1;

		// Calc the current point on the curve
		for (i = 0; i < 2; i++)
		{
			l_oldpt2[i] = mum13 * e->axis[0][i] + group1 * e->axis[1][i] + group2 * e->axis[2][i] + mu3 * e->oldorigin[i];
		}

		VectorSet2(oldpt, l_oldpt[0], 0);
		VectorSet2(oldpt2, l_oldpt2[0], 0);

		// lathe patch section around in a complete circle
		for (int t = lathe_step; t <= 360; t += lathe_step)
		{
			VectorSet2(pt, l_oldpt[0], 0);
			VectorSet2(pt2, l_oldpt2[0], 0);

			const float s = sin(DEG2RAD(t));
			const float c = cos(DEG2RAD(t));

			// rotate lathe points
//c -s 0
//s  c 0
//0  0 1
			float temp = c * pt[0] - s * pt[1];
			pt[1] = s * pt[0] + c * pt[1];
			pt[0] = temp;
			temp = c * pt2[0] - s * pt2[1];
			pt2[1] = s * pt2[0] + c * pt2[1];
			pt2[0] = temp;

			RB_CHECKOVERFLOW(4, 6);

			const int vbase = tess.num_vertexes;

			// Actually generate the necessary verts
			VectorSet(tess.normal[tess.num_vertexes], oldpt[0], oldpt[1], l_oldpt[1]);
			VectorAdd(e->origin, tess.normal[tess.num_vertexes], tess.xyz[tess.num_vertexes]);
			VectorNormalize(tess.normal[tess.num_vertexes]);
			i = oldpt[0] * 0.1f + oldpt[1] * 0.1f;
			tess.texCoords[tess.num_vertexes][0][0] = (t - lathe_step) / 360.0f;
			tess.texCoords[tess.num_vertexes][0][1] = mu - bezier_step + cos(i + backEnd.refdef.floatTime) * pain;
			tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
			tess.num_vertexes++;

			VectorSet(tess.normal[tess.num_vertexes], oldpt2[0], oldpt2[1], l_oldpt2[1]);
			VectorAdd(e->origin, tess.normal[tess.num_vertexes], tess.xyz[tess.num_vertexes]);
			VectorNormalize(tess.normal[tess.num_vertexes]);
			i = oldpt2[0] * 0.1f + oldpt2[1] * 0.1f;
			tess.texCoords[tess.num_vertexes][0][0] = (t - lathe_step) / 360.0f;
			tess.texCoords[tess.num_vertexes][0][1] = mu + cos(i + backEnd.refdef.floatTime) * pain;
			tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
			tess.num_vertexes++;

			VectorSet(tess.normal[tess.num_vertexes], pt[0], pt[1], l_oldpt[1]);
			VectorAdd(e->origin, tess.normal[tess.num_vertexes], tess.xyz[tess.num_vertexes]);
			VectorNormalize(tess.normal[tess.num_vertexes]);
			i = pt[0] * 0.1f + pt[1] * 0.1f;
			tess.texCoords[tess.num_vertexes][0][0] = t / 360.0f;
			tess.texCoords[tess.num_vertexes][0][1] = mu - bezier_step + cos(i + backEnd.refdef.floatTime) * pain;
			tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
			tess.num_vertexes++;

			VectorSet(tess.normal[tess.num_vertexes], pt2[0], pt2[1], l_oldpt2[1]);
			VectorAdd(e->origin, tess.normal[tess.num_vertexes], tess.xyz[tess.num_vertexes]);
			VectorNormalize(tess.normal[tess.num_vertexes]);
			i = pt2[0] * 0.1f + pt2[1] * 0.1f;
			tess.texCoords[tess.num_vertexes][0][0] = t / 360.0f;
			tess.texCoords[tess.num_vertexes][0][1] = mu + cos(i + backEnd.refdef.floatTime) * pain;
			tess.vertexColors[tess.num_vertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.num_vertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
			tess.num_vertexes++;

			tess.indexes[tess.num_indexes++] = vbase;
			tess.indexes[tess.num_indexes++] = vbase + 1;
			tess.indexes[tess.num_indexes++] = vbase + 3;

			tess.indexes[tess.num_indexes++] = vbase + 3;
			tess.indexes[tess.num_indexes++] = vbase + 2;
			tess.indexes[tess.num_indexes++] = vbase;

			// Shuffle new points to old
			VectorCopy2(pt, oldpt);
			VectorCopy2(pt2, oldpt2);
		}

		// shuffle lathe points
		VectorCopy2(l_oldpt2, l_oldpt);
	}
}

#define DISK_DEF	4
#define TUBE_DEF	6

static void RB_SurfaceClouds()
{
	// Disk definition
	float disk_strip_def[DISK_DEF] = {
				0.0f,
				0.4f,
				0.7f,
				1.0f };

	float disk_alpha_def[DISK_DEF] = {
				1.0f,
				1.0f,
				0.4f,
				0.0f };

	float diskCurveDef[DISK_DEF] = {
				0.0f,
				0.0f,
				0.008f,
				0.02f };

	// tube definition
	float tube_strip_def[TUBE_DEF] = {
				0.0f,
				0.05f,
				0.1f,
				0.5f,
				0.7f,
				1.0f };

	float tube_alpha_def[TUBE_DEF] = {
				0.0f,
				0.45f,
				1.0f,
				1.0f,
				0.45f,
				0.0f };

	float tube_curve_def[TUBE_DEF] = {
				0.0f,
				0.004f,
				0.006f,
				0.01f,
				0.006f,
				0.0f };

	vec3_t		pt;
	vec3_t		pt2;
	float* strip_def, * alpha_def, * curve_def, ct;

	refEntity_t* e = &backEnd.currentEntity->e;

	// select which type we shall be doing
	if (e->renderfx & RF_GROW) // doing tube type
	{
		ct = TUBE_DEF;
		strip_def = tube_strip_def;
		alpha_def = tube_alpha_def;
		curve_def = tube_curve_def;
		e->backlerp *= -1; // needs to be reversed
	}
	else
	{
		ct = DISK_DEF;
		strip_def = disk_strip_def;
		alpha_def = disk_alpha_def;
		curve_def = diskCurveDef;
	}

	// do the strip def, then lathe this around to make a 3d model
	for (int i = 0; i < ct - 1; i++)
	{
		constexpr float lathe_step = 30.0f;
		vec3_t oldpt2;
		vec3_t oldpt;
		VectorSet(oldpt, strip_def[i] * (e->radius - e->rotation) + e->rotation, 0, curve_def[i] * e->radius * e->backlerp);
		VectorSet(oldpt2, strip_def[i + 1] * (e->radius - e->rotation) + e->rotation, 0, curve_def[i + 1] * e->radius * e->backlerp);

		// lathe section around in a complete circle
		for (int t = lathe_step; t <= 360; t += lathe_step)
		{
			// rotate every time except last seg
			if (t < 360.0f)
			{
				VectorCopy(oldpt, pt);
				VectorCopy(oldpt2, pt2);

				const float s = sin(DEG2RAD(lathe_step));
				const float c = cos(DEG2RAD(lathe_step));

				// rotate lathe points
				float temp = c * pt[0] - s * pt[1];	// c -s 0
				pt[1] = s * pt[0] + c * pt[1];	// s  c 0
				pt[0] = temp;					// 0  0 1

				temp = c * pt2[0] - s * pt2[1];	 // c -s 0
				pt2[1] = s * pt2[0] + c * pt2[1];// s  c 0
				pt2[0] = temp;					 // 0  0 1
			}
			else
			{
				// just glue directly to the def points.
				VectorSet(pt, strip_def[i] * (e->radius - e->rotation) + e->rotation, 0, curve_def[i] * e->radius * e->backlerp);
				VectorSet(pt2, strip_def[i + 1] * (e->radius - e->rotation) + e->rotation, 0, curve_def[i + 1] * e->radius * e->backlerp);
			}

			RB_CHECKOVERFLOW(4, 6);

			const int vbase = tess.num_vertexes;

			// Actually generate the necessary verts
			VectorAdd(e->origin, oldpt, tess.xyz[tess.num_vertexes]);
			tess.texCoords[tess.num_vertexes][0][0] = tess.xyz[tess.num_vertexes][0] * 0.1f;
			tess.texCoords[tess.num_vertexes][0][1] = tess.xyz[tess.num_vertexes][1] * 0.1f;
			tess.vertexColors[tess.num_vertexes][0] =
				tess.vertexColors[tess.num_vertexes][1] =
				tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[0] * alpha_def[i];
			tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
			tess.num_vertexes++;

			VectorAdd(e->origin, oldpt2, tess.xyz[tess.num_vertexes]);
			tess.texCoords[tess.num_vertexes][0][0] = tess.xyz[tess.num_vertexes][0] * 0.1f;
			tess.texCoords[tess.num_vertexes][0][1] = tess.xyz[tess.num_vertexes][1] * 0.1f;
			tess.vertexColors[tess.num_vertexes][0] =
				tess.vertexColors[tess.num_vertexes][1] =
				tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[0] * alpha_def[i + 1];
			tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
			tess.num_vertexes++;

			VectorAdd(e->origin, pt, tess.xyz[tess.num_vertexes]);
			tess.texCoords[tess.num_vertexes][0][0] = tess.xyz[tess.num_vertexes][0] * 0.1f;
			tess.texCoords[tess.num_vertexes][0][1] = tess.xyz[tess.num_vertexes][1] * 0.1f;
			tess.vertexColors[tess.num_vertexes][0] =
				tess.vertexColors[tess.num_vertexes][1] =
				tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[0] * alpha_def[i];
			tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
			tess.num_vertexes++;

			VectorAdd(e->origin, pt2, tess.xyz[tess.num_vertexes]);
			tess.texCoords[tess.num_vertexes][0][0] = tess.xyz[tess.num_vertexes][0] * 0.1f;
			tess.texCoords[tess.num_vertexes][0][1] = tess.xyz[tess.num_vertexes][1] * 0.1f;
			tess.vertexColors[tess.num_vertexes][0] =
				tess.vertexColors[tess.num_vertexes][1] =
				tess.vertexColors[tess.num_vertexes][2] = e->shaderRGBA[0] * alpha_def[i + 1];
			tess.vertexColors[tess.num_vertexes][3] = e->shaderRGBA[3];
			tess.num_vertexes++;

			tess.indexes[tess.num_indexes++] = vbase;
			tess.indexes[tess.num_indexes++] = vbase + 1;
			tess.indexes[tess.num_indexes++] = vbase + 3;

			tess.indexes[tess.num_indexes++] = vbase + 3;
			tess.indexes[tess.num_indexes++] = vbase + 2;
			tess.indexes[tess.num_indexes++] = vbase;

			// Shuffle new points to old
			VectorCopy2(pt, oldpt);
			VectorCopy2(pt2, oldpt2);
		}
	}
}

/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
static void RB_SurfaceAxis() {
	GL_Bind(tr.whiteImage);
	GL_State(GLS_DEFAULT);
	qglLineWidth(3);
	qglBegin(GL_LINES);
	qglColor3f(1, 0, 0);
	qglVertex3f(0, 0, 0);
	qglVertex3f(16, 0, 0);
	qglColor3f(0, 1, 0);
	qglVertex3f(0, 0, 0);
	qglVertex3f(0, 16, 0);
	qglColor3f(0, 0, 1);
	qglVertex3f(0, 0, 0);
	qglVertex3f(0, 0, 16);
	qglEnd();
	qglLineWidth(1);
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
void RB_SurfaceEntity(surfaceType_t* surf_type)
{
	switch (backEnd.currentEntity->e.reType)
	{
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;
	case RT_ORIENTED_QUAD:
		RB_SurfaceOrientedQuad();
		break;
	case RT_LINE:
		RB_SurfaceLine();
		break;
	case RT_ELECTRICITY:
		RB_SurfaceElectricity();
		break;
	case RT_BEAM:
		RB_SurfaceBeam();
		break;
	case RT_SABER_GLOW:
		RB_SurfaceSaberGlow();
		break;
	case RT_CYLINDER:
		RB_SurfaceCylinder();
		break;
	case RT_LATHE:
		RB_SurfaceLathe();
		break;
	case RT_CLOUDS:
		RB_SurfaceClouds();
		break;
	case RT_LIGHTNING:
		RB_SurfaceLightningBolt();
		break;
	default:
		RB_SurfaceAxis();
		break;
	}
}

void RB_SurfaceBad(surfaceType_t* surf_type) {
	ri.Printf(PRINT_ALL, "Bad surface tesselated.\n");
}

/*
==================
RB_TestZFlare

This is called at surface tesselation time
==================
*/
static bool RB_TestZFlare(vec3_t point) {
	vec4_t			eye, clip, normalized, window;

	// if the point is off the screen, don't bother adding it
	// calculate screen coordinates and depth
	R_TransformModelToClip(point, backEnd.ori.model_matrix,
		backEnd.viewParms.projectionMatrix, eye, clip);

	// check to see if the point is completely off screen
	for (int i = 0; i < 3; i++) {
		if (clip[i] >= clip[3] || clip[i] <= -clip[3]) {
			return qfalse;
		}
	}

	R_TransformClipToWindow(clip, &backEnd.viewParms, normalized, window);

	if (window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth
		|| window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight) {
		return qfalse;	// shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	//do test
	float			depth = 0.0f;

	// read back the z buffer contents
	if (r_flares->integer != 1) {	//skipping the the z-test
		return true;
	}
	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse;
	qglReadPixels(backEnd.viewParms.viewportX + window[0], backEnd.viewParms.viewportY + window[1], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	const float screen_z = backEnd.viewParms.projectionMatrix[14] /
		((2 * depth - 1) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10]);

	const bool visible = -eye[2] - -screen_z < 24;
	return visible;
}

void RB_SurfaceFlare(srfFlare_t* surf) {
	vec3_t		left, up;
	byte		color[4]{};
	vec3_t		dir;
	vec3_t		origin;

	if (!r_flares->integer) {
		return;
	}

	if (!RB_TestZFlare(surf->origin)) {
		return;
	}

	// calculate the xyz locations for the four corners
	VectorMA(surf->origin, 3, surf->normal, origin);
	const float* snormal = surf->normal;

	VectorSubtract(origin, backEnd.viewParms.ori.origin, dir);
	const float dist = VectorNormalize(dir);

	float d = -DotProduct(dir, snormal);
	if (d < 0) {
		d = -d;
	}

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	color[0] = d * 255;
	color[1] = d * 255;
	color[2] = d * 255;
	color[3] = 255;	//only gets used if the shader has cgen exact_vertex!

	float radius = tess.shader->portalRange ? tess.shader->portalRange : 30;
	if (dist < 512.0f)
	{
		radius = radius * dist / 512.0f;
	}
	if (radius < 5.0f)
	{
		radius = 5.0f;
	}
	VectorScale(backEnd.viewParms.ori.axis[1], radius, left);
	VectorScale(backEnd.viewParms.ori.axis[2], radius, up);
	if (backEnd.viewParms.isMirror) {
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(origin, left, up, color);
}

void RB_SurfaceDisplayList(const srfDisplayList_t* surf) {
	// all appropriate state must be set in RB_BeginSurface
	// this isn't implemented yet...
	qglCallList(surf->listNum);
}

void RB_SurfaceSkip(void* surf) {
}

void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void*) = {
	reinterpret_cast<void(*)(void*)>(RB_SurfaceBad),			// SF_BAD,
	static_cast<void(*)(void*)>(RB_SurfaceSkip),			// SF_SKIP,
	reinterpret_cast<void(*)(void*)>(RB_SurfaceFace),			// SF_FACE,
	reinterpret_cast<void(*)(void*)>(RB_SurfaceGrid),			// SF_GRID,
	reinterpret_cast<void(*)(void*)>(RB_SurfaceTriangles),	// SF_TRIANGLES,
	reinterpret_cast<void(*)(void*)>(RB_SurfacePolychain),	// SF_POLY,
	reinterpret_cast<void(*)(void*)>(RB_SurfaceMesh),			// SF_MD3,
	reinterpret_cast<void(*)(void*)>(RB_SurfaceGhoul),		// SF_MDX,
	reinterpret_cast<void(*)(void*)>(RB_SurfaceFlare),		// SF_FLARE,
	reinterpret_cast<void(*)(void*)>(RB_SurfaceEntity),		// SF_ENTITY
	reinterpret_cast<void(*)(void*)>(RB_SurfaceDisplayList)	// SF_DISPLAY_LIST
};