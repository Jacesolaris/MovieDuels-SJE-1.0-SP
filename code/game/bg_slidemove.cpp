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

#include "common_headers.h"

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "g_vehicles.h"

extern qboolean PM_ClientImpact(const trace_t* trace, qboolean damage_self);
extern qboolean PM_ControlledByPlayer();
extern qboolean PM_InReboundHold(int anim);
extern cvar_t* g_stepSlideFix;

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
constexpr auto MAX_CLIP_PLANES = 5;
extern qboolean PM_GroundSlideOkay(float z_normal);
extern qboolean PM_InSpecialJump(int anim);

qboolean PM_SlideMove(const float grav_mod)
{
	int bumpcount;
	int num_planes;
	vec3_t normal, planes[MAX_CLIP_PLANES]{};
	vec3_t primal_velocity;
	int i;
	trace_t trace;
	vec3_t end_velocity;
	qboolean damage_self;
	int slide_move_contents = pm->tracemask;

	if (pm->ps->client_num >= MAX_CLIENTS
		&& !PM_ControlledByPlayer())
	{
		//a non-player client, not an NPC under player control
		if (pml.walking //walking on the ground
			|| pm->ps->groundEntityNum != ENTITYNUM_NONE //in air
			&& PM_InSpecialJump(pm->ps->legsAnim) //in a special jump
			&& !(pm->ps->eFlags & EF_FORCE_GRIPPED) //not being gripped
			&& !(pm->ps->eFlags & EF_FORCE_GRASPED) //not being gripped
			&& !(pm->ps->pm_flags & PMF_TIME_KNOCKBACK)
			&& pm->gent
			&& pm->gent->forcePushTime < level.time) //not being pushed
		{
			//
			// If we're a vehicle, ignore this if we're being driven
			if (!pm->gent //not an game ent
				|| !pm->gent->client //not a client
				|| pm->gent->client->NPC_class != CLASS_VEHICLE //not a vehicle
				|| !pm->gent->m_pVehicle //no vehicle
				|| !pm->gent->m_pVehicle->m_pPilot //no pilot
				|| pm->gent->m_pVehicle->m_pPilot->s.number >= MAX_CLIENTS) //pilot is not the player
			{
				//then treat do not enter brushes as SOLID
				slide_move_contents |= CONTENTS_BOTCLIP;
			}
		}
	}

	constexpr int numbumps = 4;

	VectorCopy(pm->ps->velocity, primal_velocity);
	VectorCopy(pm->ps->velocity, end_velocity);

	if (grav_mod)
	{
		if (!(pm->ps->eFlags & EF_FORCE_GRIPPED) && !(pm->ps->eFlags & EF_FORCE_DRAINED) && !(pm->ps->eFlags &
			EF_FORCE_GRASPED))
		{
			end_velocity[2] -= pm->ps->gravity * pml.frametime * grav_mod;
		}
		pm->ps->velocity[2] = (pm->ps->velocity[2] + end_velocity[2]) * 0.5;
		primal_velocity[2] = end_velocity[2];
		if (pml.groundPlane)
		{
			if (PM_GroundSlideOkay(pml.groundTrace.plane.normal[2]))
			{
				// slide along the ground plane
				PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
					pm->ps->velocity, OVERCLIP);
			}
		}
	}

	float time_left = pml.frametime;

	// never turn against the ground plane
	if (pml.groundPlane)
	{
		num_planes = 1;
		VectorCopy(pml.groundTrace.plane.normal, planes[0]);
		if (!PM_GroundSlideOkay(planes[0][2]))
		{
			planes[0][2] = 0;
			VectorNormalize(planes[0]);
		}
	}
	else
	{
		num_planes = 0;
	}

	// never turn against original velocity
	VectorNormalize2(pm->ps->velocity, planes[num_planes]);
	num_planes++;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		vec3_t end;
		// calculate position we are trying to move to
		VectorMA(pm->ps->origin, time_left, pm->ps->velocity, end);

		// see if we can make it there
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->client_num, slide_move_contents,
			static_cast<EG2_Collision>(0), 0);
		if (trace.contents & CONTENTS_BOTCLIP
			&& slide_move_contents & CONTENTS_BOTCLIP)
		{
			//hit a do not enter brush
			if (trace.allsolid || trace.startsolid) //inside the botclip
			{
				//crap, we're in a do not enter brush, take it out for the remainder of the traces and re-trace this one right now without it
				slide_move_contents &= ~CONTENTS_BOTCLIP;
				pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->client_num, slide_move_contents,
					static_cast<EG2_Collision>(0), 0);
			}
			else if (trace.plane.normal[2] > 0.0f)
			{
				//on top of a do not enter brush, it, just redo this one trace without it
				pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->client_num,
					slide_move_contents & ~CONTENTS_BOTCLIP, static_cast<EG2_Collision>(0), 0);
			}
		}

		if (trace.allsolid)
		{
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0; // don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, pm->ps->origin);
		}

		if (trace.fraction == 1)
		{
			break; // moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt(trace.entity_num);

		//Hit it
		if (trace.surfaceFlags & SURF_NODAMAGE)
		{
			damage_self = qfalse;
		}
		else if (trace.entity_num == ENTITYNUM_WORLD && trace.plane.normal[2] > 0.5f)
		{
			//if we land on the ground, let falling damage do it's thing itself, otherwise do impact damage
			damage_self = qfalse;
		}
		else
		{
			damage_self = qtrue;
		}

		if (PM_ClientImpact(&trace, damage_self))
		{
			continue;
		}

		if (pm->gent->client &&
			pm->gent->client->NPC_class == CLASS_VEHICLE &&
			trace.plane.normal[2] < pm->gent->m_pVehicle->m_pVehicleInfo->maxSlope
			)
		{
			pm->ps->pm_flags |= PMF_BUMPED;
		}

		time_left -= time_left * trace.fraction;

		if (num_planes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			VectorClear(pm->ps->velocity);
			return qtrue;
		}

		VectorCopy(trace.plane.normal, normal);

		if (!PM_GroundSlideOkay(normal[2]))
		{
			//wall-running
			//never push up off a sloped wall
			normal[2] = 0;
			VectorNormalize(normal);
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		if (!(pm->ps->pm_flags & PMF_STUCK_TO_WALL))
		{
			//no sliding if stuck to wall!
			for (i = 0; i < num_planes; i++)
			{
				if (DotProduct(normal, planes[i]) > 0.99)
				{
					VectorAdd(normal, pm->ps->velocity, pm->ps->velocity);
					break;
				}
			}
			if (i < num_planes)
			{
				continue;
			}
		}
		VectorCopy(normal, planes[num_planes]);
		num_planes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for (i = 0; i < num_planes; i++)
		{
			vec3_t end_clip_velocity;
			vec3_t clip_velocity;
			const float into = DotProduct(pm->ps->velocity, planes[i]);
			if (into >= 0.1)
			{
				continue; // move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if (-into > pml.impactSpeed)
			{
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity(pm->ps->velocity, planes[i], clip_velocity, OVERCLIP);

			// slide along the plane
			PM_ClipVelocity(end_velocity, planes[i], end_clip_velocity, OVERCLIP);

			// see if there is a second plane that the new move enters
			for (int j = 0; j < num_planes; j++)
			{
				vec3_t dir;
				if (j == i)
				{
					continue;
				}
				if (DotProduct(clip_velocity, planes[j]) >= 0.1)
				{
					continue; // move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity(clip_velocity, planes[j], clip_velocity, OVERCLIP);
				PM_ClipVelocity(end_clip_velocity, planes[j], end_clip_velocity, OVERCLIP);

				// see if it goes back into the first clip plane
				if (DotProduct(clip_velocity, planes[i]) >= 0)
				{
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				float d = DotProduct(dir, pm->ps->velocity);
				VectorScale(dir, d, clip_velocity);

				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, end_velocity);
				VectorScale(dir, d, end_clip_velocity);

				// see if there is a third plane the the new move enters
				for (int k = 0; k < num_planes; k++)
				{
					if (k == i || k == j)
					{
						continue;
					}
					if (DotProduct(clip_velocity, planes[k]) >= 0.1)
					{
						continue; // move doesn't interact with the plane
					}

					// stop dead at a triple plane interaction
					VectorClear(pm->ps->velocity);
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy(clip_velocity, pm->ps->velocity);
			VectorCopy(end_clip_velocity, end_velocity);
			break;
		}
	}

	if (grav_mod)
	{
		VectorCopy(end_velocity, pm->ps->velocity);
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if (pm->ps->pm_time)
	{
		VectorCopy(primal_velocity, pm->ps->velocity);
	}

	return static_cast<qboolean>(bumpcount != 0);
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove(float grav_mod)
{
	vec3_t start_o, start_v;
	vec3_t down_o, down_v;
	vec3_t slide_move, step_up_move;
	trace_t trace;
	vec3_t up, down;
	qboolean is_giant = qfalse;
	int step_size = STEPSIZE;

	VectorCopy(pm->ps->origin, start_o);
	VectorCopy(pm->ps->velocity, start_v);

	if (PM_InReboundHold(pm->ps->legsAnim))
	{
		grav_mod = 0.0f;
	}

	if (PM_SlideMove(grav_mod) == 0)
	{
		return; // we got exactly where we wanted to go first try
	} //else Bumped into something, see if we can step over it

	if (pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_VEHICLE && pm->gent->m_pVehicle->
		m_pVehicleInfo->hoverHeight > 0)
	{
		//Hovering vehicles don't do steps
		//FIXME: maybe make hovering vehicles go up steps, but not down them?
		return;
	}

	if (pm->gent
		&& pm->gent->client
		&& (pm->gent->client->NPC_class == CLASS_ATST || pm->gent->client->NPC_class == CLASS_RANCOR))
	{
		is_giant = qtrue;
		if (pm->gent->client->NPC_class == CLASS_RANCOR)
		{
			if (pm->gent->spawnflags & 1)
			{
				step_size = 64; //hack for Mutant Rancor stepping
			}
			else
			{
				step_size = 48; //hack for Rancor stepping
			}
		}
		else
		{
			step_size = 70; //hack for AT-ST stepping, slightly taller than a standing stormtrooper
		}
	}
	else if (pm->maxs[2] <= 0)
	{
		//short little guys can't go up steps... FIXME: just make this a flag for certain NPCs- especially ones that roll?
		step_size = 4;
	}

	//Q3Final addition...
	VectorCopy(start_o, down);
	down[2] -= step_size;
	pm->trace(&trace, start_o, pm->mins, pm->maxs, down, pm->ps->client_num, pm->tracemask,
		static_cast<EG2_Collision>(0), 0);
	VectorSet(up, 0, 0, 1);
	// never step up when you still have up velocity
	if (pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 ||
		DotProduct(trace.plane.normal, up) < 0.7))
	{
		return;
	}

	if (!pm->ps->velocity[0] && !pm->ps->velocity[1])
	{
		//All our velocity was cancelled sliding
		return;
	}

	VectorCopy(pm->ps->origin, down_o);
	VectorCopy(pm->ps->velocity, down_v);

	VectorCopy(start_o, up);
	up[2] += step_size;

	// test the player position if they were a stepheight higher

	pm->trace(&trace, start_o, pm->mins, pm->maxs, up, pm->ps->client_num, pm->tracemask, static_cast<EG2_Collision>(0),
		0);
	if (trace.allsolid || trace.startsolid || trace.fraction == 0)
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:bend can't step\n", c_pmove);
		}
		return; // can't step up
	}

	if (pm->debugLevel)
	{
		G_DebugLine(start_o, trace.endpos, 2000, 0xffffff);
	}

	//===Another slidemove forward================================================================================
	// try slidemove from this position
	VectorCopy(trace.endpos, pm->ps->origin);
	VectorCopy(start_v, pm->ps->velocity);
	/*cantStepUpFwd = */
	PM_SlideMove(grav_mod);
	//===Another slidemove forward================================================================================

	if (pm->debugLevel)
	{
		G_DebugLine(trace.endpos, pm->ps->origin, 2000, 0xffffff);
	}
	//compare the initial slidemove and this slidemove from a step up position
	VectorSubtract(down_o, start_o, slide_move);
	VectorSubtract(trace.endpos, pm->ps->origin, step_up_move);

	if (fabs(step_up_move[0]) < 0.1 && fabs(step_up_move[1]) < 0.1 && VectorLengthSquared(slide_move) >
		VectorLengthSquared(step_up_move))
	{
		//slideMove was better, use it
		VectorCopy(down_o, pm->ps->origin);
		VectorCopy(down_v, pm->ps->velocity);
	}
	else
	{
		qboolean skip_step = qfalse;
		// push down the final amount
		VectorCopy(pm->ps->origin, down);
		down[2] -= step_size;
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->client_num, pm->tracemask,
			static_cast<EG2_Collision>(0), 0);
		if (pm->debugLevel)
		{
			G_DebugLine(pm->ps->origin, trace.endpos, 2000, 0xffffff);
		}
		if (g_stepSlideFix->integer)
		{
			if (pm->ps->client_num < MAX_CLIENTS
				&& trace.plane.normal[2] < MIN_WALK_NORMAL)
			{
				//normal players cannot step up slopes that are too steep to walk on!
				vec3_t stepVec;
				//okay, the step up ends on a slope that it too steep to step up onto,
				//BUT:
				//If the step looks like this:
				//  (B)\__
				//        \_____(A)
				//Then it might still be okay, so we figure out the slope of the entire move
				//from (A) to (B) and if that slope is walk-upabble, then it's okay
				VectorSubtract(trace.endpos, down_o, stepVec);
				VectorNormalize(stepVec);
				if (stepVec[2] > 1.0f - MIN_WALK_NORMAL)
				{
					if (pm->debugLevel)
					{
						G_DebugLine(down_o, trace.endpos, 2000, 0x0000ff);
					}
					skip_step = qtrue;
				}
			}
		}

		if (!trace.allsolid
			&& !skip_step) //normal players cannot step up slopes that are too steep to walk on!
		{
			if (pm->ps->client_num
				&& is_giant
				&& g_entities[trace.entity_num].client
				&& pm->gent
				&& pm->gent->client
				&& pm->gent->client->NPC_class == CLASS_RANCOR)
			{
				//Rancor don't step on clients
				if (g_stepSlideFix->integer)
				{
					VectorCopy(down_o, pm->ps->origin);
					VectorCopy(down_v, pm->ps->velocity);
				}
				else
				{
					VectorCopy(start_o, pm->ps->origin);
					VectorCopy(start_v, pm->ps->velocity);
				}
			}
			else if (pm->ps->client_num
				&& is_giant
				&& g_entities[trace.entity_num].client
				&& g_entities[trace.entity_num].client->playerTeam == pm->gent->client->playerTeam)
			{
				//AT-ST's don't step up on allies
				if (g_stepSlideFix->integer)
				{
					VectorCopy(down_o, pm->ps->origin);
					VectorCopy(down_v, pm->ps->velocity);
				}
				else
				{
					VectorCopy(start_o, pm->ps->origin);
					VectorCopy(start_v, pm->ps->velocity);
				}
			}
			else
			{
				VectorCopy(trace.endpos, pm->ps->origin);
				if (g_stepSlideFix->integer)
				{
					if (trace.fraction < 1.0)
					{
						PM_ClipVelocity(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP);
					}
				}
			}
		}
		else
		{
			if (g_stepSlideFix->integer)
			{
				VectorCopy(down_o, pm->ps->origin);
				VectorCopy(down_v, pm->ps->velocity);
			}
		}
		if (!g_stepSlideFix->integer)
		{
			if (trace.fraction < 1.0)
			{
				PM_ClipVelocity(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP);
			}
		}
	}

	/*
	if(cantStepUpFwd && pm->ps->origin[2] < start_o[2] + stepSize && pm->ps->origin[2] >= start_o[2])
	{//We bumped into something we could not step up
		pm->ps->pm_flags |= PMF_BLOCKED;
	}
	else
	{//We did step up, clear the bumped flag
	}
	*/
#if 0
	// if the down trace can trace back to the original position directly, don't step
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, start_o, pm->ps->client_num, pm->tracemask);
	if (trace.fraction == 1.0) {
		// use the original move
		VectorCopy(down_o, pm->ps->origin);
		VectorCopy(down_v, pm->ps->velocity);
		if (pm->debugLevel) {
			Com_Printf("%i:bend\n", c_pmove);
		}
	}
	else
#endif
	{
		const float delta = pm->ps->origin[2] - start_o[2];
		if (delta > 2)
		{
			if (delta < 7)
			{
				PM_AddEvent(EV_STEP_4);
			}
			else if (delta < 11)
			{
				PM_AddEvent(EV_STEP_8);
			}
			else if (delta < 15)
			{
				PM_AddEvent(EV_STEP_12);
			}
			else
			{
				PM_AddEvent(EV_STEP_16);
			}
		}
		if (pm->debugLevel)
		{
			Com_Printf("%i:stepped\n", c_pmove);
		}
	}
}