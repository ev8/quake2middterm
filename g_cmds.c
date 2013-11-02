#include "g_local.h"
#include "m_player.h"
#include "q_devels.h"

char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	

	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
   if (ent->client->showmenu)
   {
     Menu_Dn(ent);
     return;
   }
	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	

	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;
	if (ent->client->showmenu)
   {
     Menu_Up(ent);
     return;
   }
	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}
// tom proto type to avoid error, should be in header but oh well
//as pointed out already, yes, functions in C should have thier prototype in the header
//Emmanuel Velez ev8
//simple function to see if a randomly selected monster can spawn in a randomly selected spawn point;
//tried making a function to randomly search for a suitable spawn point, but it became computaionaly expensive 
// and i couldn't quite figure out how to check is a point was "in" the world
int findSafeSpawn(edict_t *monster){
	int i;
	trace_t tr;

       
                tr = gi.trace(monster->s.origin,monster->mins,monster->maxs,monster->s.origin,monster,MASK_SHOT);
                if (tr.fraction < 1)
                {
                        return 0;
				}
                else
                {
                        return 1;
                }
        
      
		
}
//Emmanuel velez ev8
/*scans all entities in game and kills all monsters, used when all the player have died*/
void killmonsters(){
	int i;
	edict_t		*cl_ent;

	for (i=0 ; i<game.maxentities ; i++)
			{
				cl_ent=g_edicts + 1 + i;
				if(cl_ent->svflags & SVF_MONSTER)
				{
					G_FreeEdict(cl_ent);
				}
			}
}
/*spawns monsters up until the wave limit, using a randomly selected hard coded spawnpoint and a random monster type and attempts to spawn it
every 10th wave theres a chance a boss might spawn,the first 6 out of every 10 waves spawns easy monsters, the rest spawn harder ones
evey wave monster get more health and do slightly more damage, also checks to see if all players have die during a wave, if they have, the game 
is reset to the first wave.
all monster files have had thier deathmatch checks dealt with
*/
void spawn_monsters(edict_t *wave){
		edict_t *timer;
		edict_t *monster;
		int i,n, rando;
		vec3_t temp,look;
		vec3_t	spawns[20];
		edict_t		*cl_ent;
		//gi.bprintf(PRINT_MEDIUM,"%s","variables spawn");
		/*
		gi.centerprintf(player entity, message) could have been used as well to display the
		message on the center of the player's vision on a large portion of the screen
		*/
		//gi.bprintf(PRINT_MEDIUM,"%s","spawning monsters");
		//nasty hardcoded vector array of possible spawn points
		/*
		Hard coded position are fine. In a more in depth project, a spawn position finder could
		be used for balance purposes so player's couldn't exploit the monsters by standing behind the 
		spawnpoint to get the bonus damage from "surprising" some monsters
		*/
		spawns[0][0]=1157;
		spawns[0][1]=666;
		spawns[0][2]=872;
		spawns[1][0]=179;
		spawns[1][1]=489;
		spawns[1][2]=792;
		spawns[2][0]=-20;
		spawns[2][1]=904;
		spawns[2][2]=492;
		spawns[3][0]=-15.375;
		spawns[3][1]=380.5;
		spawns[3][2]=684;
		spawns[4][0]=779;
		spawns[4][1]=563;
		spawns[4][2]=800;
		spawns[5][0]=1275;
		spawns[5][1]=301.25;
		spawns[5][2]=684;
		spawns[6][0]=1863;
		spawns[6][1]=671;
		spawns[6][2]=546;
		spawns[7][0]=1632.5;
		spawns[7][1]=35.125;
		spawns[7][2]=684;
		spawns[8][0]=1309;
		spawns[8][1]=1618;
		spawns[8][2]=866;
		spawns[9][0]=1229;
		spawns[9][1]=-228;
		spawns[9][2]=670;
		spawns[10][0]=1275;
		spawns[10][1]=301.25;
		spawns[10][2]=684;
		spawns[11][0]=519;
		spawns[11][1]=976;
		spawns[11][2]=800;
		
		spawns[12][0]=1097;
		spawns[12][1]=626;
		spawns[12][2]=372;
		spawns[13][0]=692;
		spawns[13][1]=814;
		spawns[13][2]=372;
		spawns[14][0]=1314;
		spawns[14][1]=998;
		spawns[14][2]=674;
		spawns[15][0]=1457;
		spawns[15][1]=1069;
		spawns[15][2]=940;
		spawns[16][0]=1181;
		spawns[16][1]=738;
		spawns[16][2]=940;
		spawns[17][0]=1890;
		spawns[17][1]=-56;
		spawns[17][2]=940;
		spawns[18][0]=1117;
		spawns[18][1]=-68;
		spawns[18][2]=940;
	    spawns[19][0]=478;
		spawns[19][1]=445;
		spawns[19][2]=636;
		spawns[20][0]=-18;
		spawns[20][1]=392;
		spawns[20][2]=684;
		spawns[21][0]=730;
		spawns[21][1]=1162;
		spawns[21][2]=876;
		spawns[22][0]=1196;
		spawns[22][1]=1200;
		spawns[22][2]=812;
		spawns[23][0]=1457;
		spawns[23][1]=1724;
		spawns[23][2]=812;
		spawns[24][0]=1348;
		spawns[24][1]=1242;
		spawns[24][2]=940;
		spawns[25][0]=1695;
		spawns[25][1]=290;
		spawns[25][2]=556;
		spawns[26][0]=1910;
		spawns[26][1]=-187;
		spawns[26][2]=674;
		spawns[27][0]=1442;
		spawns[27][1]=-193;
		spawns[27][2]=674;
		
		/*
			Great use of variables. No suggested improvements here
		*/
		//gi.bprintf(PRINT_MEDIUM,"%s","spawning monsters");
		if(level.monsters_killed>=level.monsters_remaining)
		{
			gi.bprintf(PRINT_MEDIUM,"%s %d %s","wave ",level.wave_number, " completed");
			level.wave_number++;
			level.monsters_spawned=0;
			level.monsters_killed=0;
			level.boss_spawned = 0;
			timer=G_Spawn();
			timer->takedamage=DAMAGE_NO;
			timer->movetype=MOVETYPE_NONE;
			timer->solid = SOLID_NOT;
			VectorClear(timer->s.origin);
			VectorClear(timer->mins);
			VectorClear(timer->maxs);
			timer->think=start_Wave;
			timer->nextthink=level.time + 20;
			gi.linkentity(timer);
			G_FreeEdict(wave);
			
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl_ent=g_edicts + 1 + i;
				if((cl_ent->inuse) && (cl_ent->deadflag))
				{
					respawn(cl_ent);
				}
			}
		}else{
			//Cool idea. Good implementation. Especially the AI check and health multiplier
			 
			 //otherwise spawn chaff
		
			if((level.wave_number%10 == 0)&&(level.boss_spawned==0))// spawn boss
			{
			
			
				monster=G_Spawn();
				rando = (random()*2)-1 ;
				VectorCopy(spawns[rando],monster->s.origin);
				rando=random()*3;
				switch (rando)
				{
					case 1:
						SP_monster_supertank(monster);
						monster->mtype=50;
						break;
					case 2:
						SP_monster_jorg(monster);
						monster->mtype=75;
						break;
					case 3:
						SP_monster_boss2(monster);
						monster->mtype=30;
						break;
					default:
						SP_monster_boss2(monster);
						monster->mtype=30;
						break;
				}
				

				monster->health+= level.wave_number*100;
				 monster->monsterinfo.aiflags |= (AI_BRUTAL&AI_PURSUE_NEXT);
				 rando=findSafeSpawn(monster);
				 if (rando==1){
				  gi.linkentity(monster);
				level.monsters_spawned++;
				level.boss_spawned=1;
				 }else{
					 G_FreeEdict(monster);
				 
				 }
				 
			}
			
				if((level.monsters_remaining-level.monsters_spawned)>5)
				{
					i=5;
				}else{
				i=level.monsters_remaining-level.monsters_spawned;
				}
				for(n=0;n<i;n++)
				{
					monster = G_Spawn();
					rando = random()*13;//random monster spawn, cool idea
						
						if (level.wave_number%12 <6)
						{rando=rando/2;}

						//gi.bprintf(PRINT_MEDIUM,"%s %d \n","rando",rando);	
						switch(rando)
							{
							case 1:
								{
									//gi.bprintf(PRINT_MEDIUM,"%s","spawning flyer");
									SP_monster_flyer(monster);
									monster->mtype=2;
									break;
								}
							case 2:
								{
									//gi.bprintf(PRINT_MEDIUM,"%s","spawning soldier");
									SP_monster_soldier_light(monster);							;
									monster->mtype=3;
									break;
								}
							case 3:
								{
									//gi.bprintf(PRINT_MEDIUM,"%s","spawning sssoldier");
									SP_monster_soldier_ss(monster);	
									monster->mtype=4;
									break;
								}
							case 4:
								{
								//gi.bprintf(PRINT_MEDIUM,"%s","spawning gunner");
								SP_monster_infantry(monster);
								monster->mtype=5;
								break;
								}
							case 5:
								{
								//gi.bprintf(PRINT_MEDIUM,"%s","spawning flyer2");
								SP_monster_soldier(monster);
								monster->mtype=6;
								break;
								}
							case 6:
								{
								//gi.bprintf(PRINT_MEDIUM,"%s","spawning chick");
								SP_monster_hover(monster);
								monster->mtype=7;
								break;
								}
							case 7:
								{
								//gi.bprintf(PRINT_MEDIUM,"%s","spawning gunner");
								
									SP_monster_chick(monster);
									monster->mtype=8;
								break;
								}
							case 8:
								{
								//gi.bprintf(PRINT_MEDIUM,"%s","spawning gunner");
								SP_monster_medic(monster);
								monster->mtype=9;
								break;
								}
							case 9:
								{
								//gi.bprintf(PRINT_MEDIUM,"%s","spawning lgt soldier");
								SP_monster_brain(monster);
								monster->mtype=10;
								break;
								}
							case 11:
								{
								//gi.bprintf(PRINT_MEDIUM,"%s","spawning lgt soldier");
								SP_monster_parasite(monster);
								monster->mtype=11;
								break;
								}
							case 12:
								{
								//gi.bprintf(PRINT_MEDIUM,"%s","spawning lgt soldier");
								SP_monster_tank(monster);
								monster->mtype=13;
								break;
								}
								case 13:
								{
									//gi.bprintf(PRINT_MEDIUM,"%s","spawning flyer");
									SP_monster_gunner(monster);
									monster->mtype=12;
									break;
								}
							
							default:
								//gi.bprintf(PRINT_MEDIUM,"%s","defaults spawn tank");
									SP_monster_berserk(monster);
									monster->mtype=3;
								break;
							}
						rando = (random()*28)-1;
						
						monster->s.angles[YAW]=random()*360;
						VectorCopy(spawns[rando],monster->s.origin);// makes monsters spawn facing a random direction
						monster->health+= level.wave_number*5;
				 monster->monsterinfo.aiflags |= (AI_BRUTAL&AI_PURSUE_NEXT);
				rando=findSafeSpawn(monster);
				 if(rando==1)
				 {	 
					 
					 gi.linkentity(monster);
					// gi.bprintf(PRINT_MEDIUM," %s","name ",monster->classname);
					level.monsters_spawned++;
					
					
				}else{
					G_FreeEdict(monster);//good check, very important part that could cause problems if left out
				}
				}
			rando=0;
				n=0;
		for (i=0 ; i<game.maxclients ; i++)
			{
				cl_ent=g_edicts + 1 + i;
				if((cl_ent->inuse))
				{
					n++;
					if(cl_ent->deadflag)
					{
						rando++;
					}
				}
			}
		if(rando == n)
		{
			
			killmonsters();
			level.wave_number=0;
			level.monsters_killed=0;
			level.monsters_remaining=0;
			level.monsters_spawned=0;
			level.boss_spawned=0;
			
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl_ent=g_edicts + 1 + i;
				if((cl_ent->inuse) && (cl_ent->deadflag))
				{
					respawn(cl_ent);
				}
			}
		G_FreeEdict(wave);
		return;
		}
		wave->nextthink= level.time+10;
	}
	
		
}
// Emmanuel velez ev8
// is my timer's think function, when time is up it starts the wave,
void start_Wave(edict_t *timer){
	edict_t *wave;
	gi.bprintf(PRINT_MEDIUM,"%s %d","starting wave :", level.wave_number);
	
		level.monsters_remaining = 10*level.wave_number ;
		level.monsters_spawned =0;
		level.monsters_killed=0;
		level.boss_spawned=0;
		//gi.bprintf(PRINT_MEDIUM,"%s","spawning wave");
		wave=G_Spawn();
		
		
		wave->takedamage=DAMAGE_NO;
		wave->movetype=MOVETYPE_NONE;
		wave->solid = SOLID_NOT;
		VectorClear(wave->s.origin);
		VectorClear(wave->mins);
		VectorClear(wave->maxs);
		wave->think=spawn_monsters;
		wave->nextthink=level.time + 1;
		gi.linkentity(wave);
	G_FreeEdict(timer);
}
//ev8
//debuging function for testing various bits
  void Cmd_Compass_f (edict_t *ent) 

  { edict_t *monster;
  vec3_t spawn;
  spawn[0] = 1217.75;
  spawn[1] = 685.75;
  spawn[2] = 472.125;
  ent->client->resp.ap=50;
	//monster=G_Spawn();
	//VectorCopy(spawn,monster->s.origin);
				//SP_monster_medic(monster);

				//monster->health+= level.wave_number*100;
				// monster->monsterinfo.aiflags |= (AI_BRUTAL&AI_PURSUE_NEXT);
				//gi.linkentity(monster);

				gi.cprintf (ent, PRINT_HIGH, "%g %g %g %s %d %s %d %s %d %s %d %s %d %s %s\n",ent->s.origin[0], ent->s.origin[1], 

					ent->s.origin[2],"remaining",level.monsters_remaining,"spawned",level.monsters_spawned,"wave",level.wave_number, " killed",level.monsters_killed,"experience",ent->client->resp.exp,"level ", ent->client->pers.weapon->classname);  //should be one line 

  }
  //Emmanuel Velez ev8
  //kicks off the first wave of enemies
  // checks to see if the game has started 
  //other wise spawns a timer to the first wave.
void Cmd_Startwave (edict_t *ent){
        edict_t *timer;
	if(level.wave_number){
	    gi.bprintf(PRINT_MEDIUM,"%s","game already started you tit");
		return;
	}
	else
	{
		level.boss_spawned =0;
		gi.bprintf(PRINT_MEDIUM,"%s","starting first wave in 20 seconds");
		level.wave_number = 1;
		//gi.bprintf(PRINT_MEDIUM,"%s","wave 9");
		timer=G_Spawn();
		//gi.bprintf(PRINT_MEDIUM,"%s","timer");
		
		timer->takedamage=DAMAGE_NO;
		timer->movetype=MOVETYPE_NONE;
		timer->solid = SOLID_NOT;
		VectorClear(timer->s.origin);
		VectorClear(timer->mins);
		VectorClear(timer->maxs);
		timer->think=start_Wave;
		timer->nextthink=level.time + 20;
		//gi.bprintf(PRINT_MEDIUM,"%s","timer linking");
		gi.linkentity(timer);
	}
}
/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;
   if (ent->client->showmenu)
   {
      Menu_Sel(ent);
      return;
   }

	

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];

	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		Com_sprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}


/*
=================
ClientCommand
=================
*/
//Emmanuel velez ev8
//this is the call back function for my upgrade menu 
// it checks to see if the player can afford the selected upgrade and prompts them if they cant
void upgrade_Sel(edict_t *ent, int choice)
{
	char buffer[3];
	char str[27];
	int i;
	
	switch (choice)
	{
	case 0:
		


		if(ent->client->resp.ap>=5)
		{
		
			ent->client->resp.ap-=5;
		
			if(!(ent->client->resp.speedmod)){
				ent->client->resp.speedmod = 1;}
			else{
			ent->client->resp.speedmod++;
				 i = 200*((float)1+((float)ent->client->resp.speedmod)/10);
				 //Emmanuel Velez, 
				 //this is how we set the players speed server side to changes the players movement speed
				//pmove is used by the server for player movement prediction and trying to modify the players speed in pmove 
				 // causes that wierd wobbly behavior
				 //stuffcmd provided courtesy of te q devels
				sprintf(buffer,"%u", i );
				strcpy (str,"cl_forwardspeed ");
				strcat (str,buffer);
				strcat (str, "\n");
				stuffcmd (ent, str);
				strcpy (str,"cl_sidespeed ");
				strcat (str,buffer);
				strcat (str, "\n");
				stuffcmd (ent, str);
			}
		}else
			gi.cprintf (ent, PRINT_HIGH, "%s","you can't afford that");
		break;
		case 1:
		if(ent->client->resp.ap>=5)
		{
		
			ent->client->resp.ap-=5;
		
			if(!(ent->client->resp.damagemod)){
				ent->client->resp.damagemod = 1;}
			else{
			ent->client->resp.damagemod++;
			}
		}else
			gi.cprintf (ent, PRINT_HIGH, "%s","you can't afford that");
		break;
		case 2:
		if(ent->client->resp.ap>=5)
		{
		
			ent->client->resp.ap-=5;
		
			if(!(ent->client->resp.jumpmod)){
				ent->client->resp.jumpmod = 1;}
			else{
			ent->client->resp.jumpmod++;
			}
		}else
			gi.cprintf (ent, PRINT_HIGH, "%s","you can't afford that");

		break;
		case 3:
		if(ent->client->resp.ap>=5)
		{
		
			ent->client->resp.ap-=5;
		
			if(!(ent->client->resp.kickmod)){
				ent->client->resp.kickmod = 1;}
			else{
			ent->client->resp.kickmod++;
			}
		}else
			gi.cprintf (ent, PRINT_HIGH, "%s","you can't afford that");
		break;
		case 4:
		if(ent->client->resp.ap>=10)
		{
		
			ent->client->resp.ap-=10;
			ent->max_health+=25;
			
		}else
			gi.cprintf (ent, PRINT_HIGH, "%s","you can't afford that");
		break;
	}

	} 
//Emmanuel Velez ev8
// creates my upgrade menu using the qmenu.c file courtesy of online gaming technologies
// found a citical bug on 9/30 when attempting to demonstrate my mod
// while running on a laptop, trying to create menu items with more than 21 charcters causes 
// strange memory behavior, this bug happened on several laptops. upon several hours of debugging 
// the error was fixed. i did my development on my desktop and never experienced an error and the documentation 
// for qmenu.c explictly said menu item could be ass long as 27 chracters.
void Cmd_upgrade_menu(edict_t *ent)
{

	char buffer1[3];
	char buffer2[5];
	char buffer3[3];
	char buffer4[2];
	char str[27];
	int n ;

	sprintf(buffer1,"%u", level.wave_number);
	sprintf(buffer2,"%u", level.monsters_remaining-level.monsters_killed);
	sprintf(buffer3,"%u", ent->client->resp.lvl);
	sprintf(buffer4,"%u", ent->client->resp.ap);

	strcpy (str,"w: ");
	strcat (str,buffer1);
	strcat (str, " m:");
	strcat (str,buffer2);
	strcat (str," lvl ");
	strcat (str,buffer3);
	strcat (str," points :");
	strcat (str,buffer4);
	str[27]='\n';

   // Check to see if the menu is already open
	
   if (ent->client->showscores || ent->client->showinventory || 
        ent->client->showmenu || ent->client->showmsg)
        return;

   // send the layout
  
   Menu_Clear(ent);
   Menu_Title(ent,str);
   Menu_Add(ent,"upgrd speed   : 5pts");
   Menu_Add(ent,"upgrd damage  : 5pts");
   Menu_Add(ent,"upgrd jump    : 5pts");
   Menu_Add(ent,"upgrd kick    : 5pts");
   Menu_Add(ent,"upgrdmaxhealth:10pts");

    // Setup the User Selection Handler

   ent->client->usr_menu_sel = upgrade_Sel;
   Menu_Open(ent);

}
//Emmanuel Velez ev8
// added wave start and upgrade to client commands
//wave start starts the wave of enemies and upgrade brings up a menu
//tp purchace upgrades to chracter stats
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wavestart") == 0){
		Cmd_Startwave (ent);}
	else if (Q_stricmp (cmd, "compass") == 0) 
		Cmd_Compass_f (ent); 
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	else if (Q_stricmp (cmd, "menu") == 0)
		Menu_Hlp(ent);
	else if (Q_stricmp (cmd, "upgrade") == 0){

		Cmd_upgrade_menu(ent);
	}
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
