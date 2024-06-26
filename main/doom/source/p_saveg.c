// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Archiving: SaveGame I/O.
//
//-----------------------------------------------------------------------------

#include "doomtype.h"
#include "global_data.h"
#include "i_system.h"
#include "z_zone.h"
//#include "p_local.h"
#include "p_spec.h"
// State.
#include "doomstat.h"
#include "r_state.h"

byte*		save_p;



extern void wb(char b);
extern void ww(short w);
extern void wr(char * b, unsigned int sz);
extern void rb(char * b);
extern void rw(short * b);
extern void rr(char * b, unsigned int sz);
extern void rseek(unsigned int sz);
//
// P_ArchivePlayers
//
void P_ArchivePlayers (void)
{
    int		i;
    int		j;
    player_t*	dest;
/*		
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!playeringame[i])
	    continue;
	

*/
	dest = &_g->player;
	//memcpy (dest,&_g->player,sizeof(player_t));
	
	char need_restore[NUMSPRITES];
	memset(need_restore,0,NUMSPRITES);

	//save_p += sizeof(player_t);
	for (j=0 ; j<NUMPSPRITES ; j++)
	{

	    if (dest->psprites[j].state)
	    {
			dest->psprites[j].state 
				= (char *)dest->psprites[j].state-(char *)states;
			need_restore[j] = 1;
	    }
		
	}
	wr(&_g->player,sizeof(player_t));
	for (j=0 ; j<NUMPSPRITES ; j++)
	{

	    if (need_restore[j])
	    {
		dest->psprites[j].state 
		    = (state_t *)((char *)dest->psprites[j].state+(size_t)(char *)states);
	    }
		
	}

	/*
    }
	*/
}




//
// P_UnArchivePlayers
//
void P_UnArchivePlayers (void)
{
	printf("P_UnArchivePlayers()\n");
    int		i;
    int		j;
	/*
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!playeringame[i])
	    continue;
	

	*/
	rr(&_g->player, sizeof(player_t));
	

	for (j=0 ; j<NUMPSPRITES ; j++)
	{
	    if (_g->player. psprites[j].state)
	    {
			_g->player. psprites[j].state 
				= (state_t *)((char *)states + (size_t)_g->player.psprites[j].state);
	    }
	}
	/*
    }
	*/
}


//
// P_ArchiveWorld
//
void P_ArchiveWorld (void)
{
    int			i;
    int			j;
    sector_t*		sec;
    line_t*		li;
    side_t*		si;
    short*		put;
	
    //put = (short *)save_p;
    

    // do lines
    for (i=0, li = _g->lines ; i<_g->numlines ; i++,li++)
    {

	for (j=0 ; j<2 ; j++)
	{
	    if (li->sidenum[j] == -1)
		continue;
	    
	    si = &_g->sides[li->sidenum[j]];

	    ww(si->textureoffset >> FRACBITS);
	    ww(si->rowoffset >> FRACBITS);
	    ww(si->toptexture);
	    ww(si->bottomtexture);
	    ww(si->midtexture);	
	}
    }
	printf("Save: _g->numsectors = %d, _g->numlines = %d\n", _g->numsectors, _g->numlines);
    //save_p = (byte *)put;
}


void _trace(char const *function, 
            char const *file, 
            long line) 
{
    printf("%s[%s:%ld]\n", function, file, line);
}

#define trace() _trace(__FUNCTION__, __FILE__, __LINE__)
//
// P_UnArchiveWorld
//
void P_UnArchiveWorld (void)
{
	printf("P_UnArchiveWorld()\n");
    int			i;
    int			j;
    sector_t*		sec;
    line_t*		li;
    side_t*		si;
    short*		get;


    // do lines
    for (i=0, li = _g->lines ; i<_g->numlines ; i++,li++)
    {

	for (j=0 ; j<2 ; j++)
	{
	    if (li->sidenum[j] == -1)

	    si = &_g->sides[li->sidenum[j]];


	    rw(&si->textureoffset);


		si->textureoffset = si->textureoffset << FRACBITS;


	    rw(&si->rowoffset);


		si->rowoffset = si->rowoffset << FRACBITS;

		short v;

		
		rw(&v);

		si->toptexture = v;

	    
		rw(&v);

	    si->bottomtexture = v;



		rw(&v);


	    si->midtexture = v;


	}
    }

	printf("Save: _g->numsectors = %d, _g->numlines = %d\n", _g->numsectors, _g->numlines);

    //save_p = (byte *)get;	
}





//
// Thinkers
//
typedef enum
{
    tc_end,
    tc_mobj

} thinkerclass_t;



//
// P_ArchiveThinkers
//
void P_ArchiveThinkers (void)
{
    thinker_t*		th;
    mobj_t*		mobj;
	
    // save off the current thinkers
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
		if (th->function == P_MobjThinker)
		{
			char b = tc_mobj;
			wb(b);
			mobj = (mobj_t *)th;
			mobj->state = (state_t *)((char *)mobj->state - (char *)states);
			wr(mobj, sizeof(*mobj));	    
			mobj->state = (state_t *)((char *)mobj->state + (size_t)(char *)states);
			
			printf("P_ArchiveThinkers: save thinker %d\n", tc_mobj);
		}
    }

    // add a terminating marker
    wb(tc_end);	
}



//
// P_UnArchiveThinkers
//
void P_UnArchiveThinkers (void)
{
	printf("P_UnArchiveThinkers (void)\n");
    byte		tclass;
    thinker_t*		currentthinker;
    thinker_t*		next;
    mobj_t*		mobj;
    thinker_t *th;
    // remove all the current thinkers
  	for (th = thinkercap.next; th != &thinkercap; )
    {
      thinker_t *next = th->next;
      if (th->function == P_MobjThinker)
        P_RemoveMobj ((mobj_t *) th);
      else {
        //Z_Free (th);
	  }
      th = next;
    }
    P_InitThinkers ();
	
    // read in saved thinkers
    while (1)
    {
	rb(&tclass);
	printf("P_UnArchiveThinkers: tclass = %d\n", tclass);
	switch (tclass)
	{
	  case tc_end:
	    return; 	// end of list
			
	  case tc_mobj:


	    mobj = Z_Malloc (sizeof(*mobj), PU_LEVEL, NULL);
	    rr(mobj, sizeof(*mobj));
	    mobj->state = (state_t *)((char*)states + (size_t)mobj->state);


	    P_SetThingPosition (mobj);
	    //mobj->info = &mobjinfo[mobj->type];
	    mobj->floorz = mobj->subsector->sector->floorheight;
	    mobj->ceilingz = mobj->subsector->sector->ceilingheight;
	    mobj->thinker.function = P_MobjThinker;
	    P_AddThinker (&mobj->thinker);
	    break;
			
	  default:
	    printf("Unknown tclass %d in savegame",tclass);
		
	    I_Error ("Unknown tclass %d in savegame",tclass);
	}
	
    }

}


//
// P_ArchiveSpecials
//
enum
{
    tc_ceiling,
    tc_door,
    tc_floor,
    tc_plat,
    tc_flash,
    tc_strobe,
    tc_glow,
    tc_endspecials

} specials_e;	



//
// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
//
void P_ArchiveSpecials (void)
{
    thinker_t*		th;
    ceiling_t*		ceiling;
    vldoor_t*		door;
    floormove_t*	floor;
    plat_t*		plat;
    lightflash_t*	flash;
    strobe_t*		strobe;
    glow_t*		glow;
    int			i;
	
    // save off the current thinkers
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
	if (th->function == NULL)
	{
		ceilinglist_t * c = _g->activeceilings;
	    while (c) {
			if (c->ceiling == (ceiling_t *)th)
				break;
		}	    
	    if (c)
	    {
			wb(tc_ceiling);
			ceiling = (ceiling_t *)th;
			ceiling->sector = (sector_t *)((char*)ceiling->sector - (char*)_g->sectors);
			wr(ceiling, sizeof(*ceiling));
			ceiling->sector = (sector_t *)((char*)ceiling->sector + (size_t)(char*)_g->sectors);

	    }
	    continue;
	}
			
	if (th->function == T_MoveCeiling)
	{
	    wb(tc_ceiling);
	    ceiling = (ceiling_t *)th;
	    ceiling->sector = (sector_t *)((char*)ceiling->sector - (char*)_g->sectors);
		wr(ceiling, sizeof(*ceiling));
	    ceiling->sector = (sector_t *)((char*)ceiling->sector + (size_t)(char*)_g->sectors);

	    continue;
	}
			
	if (th->function == T_VerticalDoor)
	{
	    wb(tc_door);
	    door = (vldoor_t *)th;
	    door->sector = (sector_t *)((char*)door->sector - (char*)_g->sectors);
		wr(door, sizeof(*door));
	    door->sector = (sector_t *)((char*)door->sector + (size_t)(char*)_g->sectors);
	    continue;
	}
			
	if (th->function == T_MoveFloor)
	{
	    wb(tc_floor);
	    floor = (floormove_t *)th;
	    floor->sector = (sector_t *)((char*)floor->sector - (char*)_g->sectors);
	    wr(floor, sizeof(*floor));
	    floor->sector = (sector_t *)((char*)floor->sector + (size_t)(char*)_g->sectors);
	    continue;
	}
			
	if (th->function == T_PlatRaise)
	{
	    wb(tc_plat);
	    plat = (plat_t *)th;
	    plat->sector = (sector_t *)((char*)plat->sector - (char*)_g->sectors);
	    wr(plat, sizeof(*plat));
	    plat->sector = (sector_t *)((char*)plat->sector + (size_t)(char*)_g->sectors);
	    continue;
	}
			
	if (th->function == T_LightFlash)
	{
	    wb(tc_flash);
	    flash = (lightflash_t *)th;
	    flash->sector = (sector_t *)((char*)flash->sector - (char*)_g->sectors);
	    wr(flash, sizeof(*flash));
	    flash->sector = (sector_t *)((char*)flash->sector + (size_t)(char*)_g->sectors);
	    continue;
	}
			
	if (th->function == T_StrobeFlash)
	{
	    wb(tc_strobe);
	    strobe = (strobe_t *)th;
	    strobe->sector = (sector_t *)((char*)strobe->sector - (char*)_g->sectors);
	    wr(strobe, sizeof(*strobe));
	    strobe->sector = (sector_t *)((char*)strobe->sector + (size_t)(char*)_g->sectors);
	    continue;
	}
			
	if (th->function == T_Glow)
	{
	    wb(tc_glow);
	    glow = (glow_t *)th;
	    glow->sector = (sector_t *)((char*)glow->sector - (char*)_g->sectors);
	    wr(glow, sizeof(*glow));
	    glow->sector = (sector_t *)((char*)glow->sector + (size_t)(char*)_g->sectors);
	    continue;
	}
    }
	
    // add a terminating marker
    wb(tc_endspecials);	

}


//
// P_UnArchiveSpecials
//
void P_UnArchiveSpecials (void)
{
	printf("P_UnArchiveSpecials()\n");
    byte		tclass;
    ceiling_t*		ceiling;
    vldoor_t*		door;
    floormove_t*	floor;
    plat_t*		plat;
    lightflash_t*	flash;
    strobe_t*		strobe;
    glow_t*		glow;
	
	
    // read in saved thinkers
    while (1)
    {
	rb(&tclass);
	switch (tclass)
	{
	  case tc_endspecials:
	    return;	// end of list
			
	  case tc_ceiling:

	    ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVEL, NULL);
		rr(ceiling, sizeof(*ceiling));
	    ceiling->sector = (sector_t *)((char*)_g->sectors + (size_t)ceiling->sector);
	    //ceiling->sector->specialdata = ceiling;

	    if (ceiling->thinker.function)
		ceiling->thinker.function = T_MoveCeiling;

	    P_AddThinker (&ceiling->thinker);
	    P_AddActiveCeiling(ceiling);
	    break;
				
	  case tc_door:

	    door = Z_Malloc (sizeof(*door), PU_LEVEL, NULL);
		rr(door, sizeof(*door));
	    door->sector = (sector_t *)((char*)_g->sectors + (size_t)door->sector);
	    //door->sector->specialdata = door;
	    door->thinker.function = T_VerticalDoor;
	    P_AddThinker (&door->thinker);
	    break;
				
	  case tc_floor:

	    floor = Z_Malloc (sizeof(*floor), PU_LEVEL, NULL);
	    rr(floor, sizeof(*floor));
	    floor->sector = (sector_t *)((char*)_g->sectors + (size_t)floor->sector);
	    //floor->sector->specialdata = floor;
	    floor->thinker.function = T_MoveFloor;
	    P_AddThinker (&floor->thinker);
	    break;
				
	  case tc_plat:
	    plat = Z_Malloc (sizeof(*plat), PU_LEVEL, NULL);
	    rr(plat, sizeof(*plat));
	    plat->sector = (sector_t *)((char*)_g->sectors + (size_t)plat->sector);
	    //plat->sector->specialdata = plat;

	    if (plat->thinker.function)
		plat->thinker.function = T_PlatRaise;

	    P_AddThinker (&plat->thinker);
	    P_AddActivePlat(plat);
	    break;
				
	  case tc_flash:
	    flash = Z_Malloc (sizeof(*flash), PU_LEVEL, NULL);
	    rr(flash, sizeof(*flash));
	    flash->sector = (sector_t *)((char*)_g->sectors + (size_t)flash->sector);
	    flash->thinker.function = T_LightFlash;
	    P_AddThinker (&flash->thinker);
	    break;
				
	  case tc_strobe:
	    strobe = Z_Malloc (sizeof(*strobe), PU_LEVEL, NULL);
	    rr(strobe, sizeof(*strobe));
	    strobe->sector = (sector_t *)((char*)_g->sectors + (size_t)strobe->sector);
	    strobe->thinker.function = T_StrobeFlash;
	    P_AddThinker (&strobe->thinker);
	    break;
				
	  case tc_glow:
	    glow = Z_Malloc (sizeof(*glow), PU_LEVEL, NULL);
	    rr(glow, sizeof(*glow));
	    glow->sector = (sector_t *)((char*)_g->sectors + (size_t)glow->sector);
	    glow->thinker.function = T_Glow;
	    P_AddThinker (&glow->thinker);
	    break;
				
	  default:
	  	printf("P_UnarchiveSpecials:Unknown tclass %d "
		     "in savegame",tclass);
	    I_Error ("P_UnarchiveSpecials:Unknown tclass %d "
		     "in savegame",tclass);
	}
	
    }

}

char * find_thinker_by_prev_pointer(char * ptr) {
	if(!ptr) return NULL;

    thinker_t *th;
  	for (th = thinkercap.next; th != &thinkercap; )
    {
      thinker_t *next = th->next;
      if (th->self_before_load == ptr) {
		printf("Thinker %p found!\n", ptr);
		return th;		
	  }
      th = next;
    }
	printf("EEEE Thinker %p not found!\n", ptr);
	return NULL;

}

void P_UnArchiveThinkerPointers (void) {
	
	byte		tclass;
    thinker_t*		currentthinker;
    thinker_t*		next;
    mobj_t*		mobj;
    thinker_t *th;
  	for (th = thinkercap.next; th != &thinkercap; )
    {
      thinker_t *next = th->next;
      if (th->function == P_MobjThinker) {
        ((mobj_t *) th)->target = (mobj_t *)find_thinker_by_prev_pointer((char *)((mobj_t *) th)->target);
        ((mobj_t *) th)->tracer = (mobj_t *)find_thinker_by_prev_pointer((char *)((mobj_t *) th)->tracer);
        ((mobj_t *) th)->lastenemy = (mobj_t *)find_thinker_by_prev_pointer((char *)((mobj_t *) th)->lastenemy);
		
	  } else {
        //Z_Free (th);
	  }
      th = next;
    }

	_g->player.mo = (mobj_t *)find_thinker_by_prev_pointer((char *)_g->player.mo);
	_g->player.attacker = (mobj_t *)find_thinker_by_prev_pointer((char *)_g->player.attacker);
	
}