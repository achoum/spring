#include "StdAfx.h"
// Player.cpp: implementation of the CPlayer class.
//
//////////////////////////////////////////////////////////////////////
#include <assert.h>

#include "mmgr.h"

#include "Player.h"
#include "PlayerHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#ifdef DIRECT_CONTROL_ALLOWED
	#include "Game/GameHelper.h"
	#include "Sim/Units/Unit.h"
	#include "Sim/Units/UnitHandler.h"
	#include "Sim/Weapons/Weapon.h"
	#include "Sim/Units/COB/CobInstance.h"
	#include "Sim/Units/COB/CobFile.h"
	#include "UI/MouseHandler.h"
	#include "CameraHandler.h"
	#include "Camera.h"
	#include "myMath.h"
#endif
#include "EventHandler.h"
#include "GlobalUnsynced.h"

CR_BIND(CPlayer,);

CR_REG_METADATA(CPlayer, (
				CR_MEMBER(active),
				CR_MEMBER(name),
				CR_MEMBER(countryCode),
				CR_MEMBER(rank),
				CR_MEMBER(spectator),
				CR_MEMBER(team),
//				CR_MEMBER(readyToStart),
//				CR_MEMBER(cpuUsage),
//				CR_MEMBER(ping),
				CR_MEMBER(currentStats),
				CR_MEMBER(playerNum),
//				CR_MEMBER(controlledTeams),
				CR_RESERVED(32)
				));

CR_BIND(CPlayer::Statistics,);

CR_REG_METADATA_SUB(CPlayer, Statistics, (
					CR_MEMBER(mousePixels),
					CR_MEMBER(mouseClicks),
					CR_MEMBER(keyPresses),
					CR_MEMBER(numCommands),
					CR_MEMBER(unitCommands),
					CR_RESERVED(16)
					));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
{
	memset(&currentStats, 0, sizeof(Statistics));

	active = false;
	readyToStart = false;
	cpuUsage = 0;
	ping = 0;


#ifdef DIRECT_CONTROL_ALLOWED
	playerControlledUnit=0;

	myControl.forward=0;
	myControl.back=0;
	myControl.left=0;
	myControl.right=0;

	myControl.mouse1=0;
	myControl.mouse2=0;
	myControl.viewDir=float3(0,0,1);
	myControl.targetPos=float3(0,0,1);
	myControl.targetDist=1000;
	myControl.target=0;
	myControl.myController=this;
#endif
}

CPlayer::~CPlayer()
{
}


void CPlayer::SetControlledTeams()
{
	controlledTeams.clear();

	if (gs->godMode) {
		// anyone can control any unit
		for (int t = 0; t < MAX_TEAMS; t++) {
			controlledTeams.insert(t);
		}
		return;
	}

	if (!spectator) {
		// my team
		controlledTeams.insert(team);
	}

	// AI teams
	for (int t = 0; t < teamHandler->ActiveTeams(); t++) {
		const CTeam* team = teamHandler->Team(t);
		if (team && team->isAI &&
		    !team->dllAI.empty() && // luaAI does not require client control
		    (team->leader == playerNum)) {
			controlledTeams.insert(t);
		}
	}
}


void CPlayer::UpdateControlledTeams()
{
	for (int p = 0; p < playerHandler->ActivePlayers(); p++) {
		CPlayer* player = playerHandler->Player(p);
		if (player) {
			player->SetControlledTeams();
		}
	}
}


void CPlayer::StartSpectating()
{
	spectator = true;
	if (playerHandler->Player(gu->myPlayerNum) == this) { //TODO bad hack
		gu->spectating           = true;
		gu->spectatingFullView   = true;
		gu->spectatingFullSelect = true;
	}
	eventHandler.PlayerChanged(playerNum);
}

void CPlayer::GameFrame(int frameNum)
{
#ifdef DIRECT_CONTROL_ALLOWED
	if(!active || !playerControlledUnit)
		return;

	CUnit* unit = playerControlledUnit;
	DirectControlStruct* dc = &myControl;

	std::vector<int> args;
	args.push_back(0);
	unit->cob->Call(COBFN_AimFromPrimary/*/COBFN_QueryPrimary+weaponNum/ **/,args);
	float3 relPos=unit->cob->GetPiecePos(args[0]);
	float3 pos=unit->pos+unit->frontdir*relPos.z+unit->updir*relPos.y+unit->rightdir*relPos.x;
	pos+=UpVector*7;

	CUnit* hit;
	float dist=helper->TraceRayTeam(pos,dc->viewDir,unit->maxRange,hit,1,unit,teamHandler->AllyTeam(team));
	dc->target=hit;

	if(uh->limitDgun && unit->unitDef->isCommander && unit->pos.SqDistance(teamHandler->Team(unit->team)->startPos)>Square(uh->dgunRadius)){
		return; //prevents dgunning using fps view if outside dgunlimit
	}

	if(hit){
		dc->targetDist=dist;
		dc->targetPos=hit->pos;
		if(!dc->mouse2){
			unit->AttackUnit(hit,true);
		}
	} else {
		if(dist>unit->maxRange*0.95f)
			dist=unit->maxRange*0.95f;

		dc->targetDist=dist;
		dc->targetPos=pos+dc->viewDir*dc->targetDist;

		if(!dc->mouse2){
			unit->AttackGround(dc->targetPos,true);
			for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi){
				float d=dc->targetDist;
				if(d>(*wi)->range*0.95f)
					d=(*wi)->range*0.95f;
				float3 p=pos+dc->viewDir*d;
				(*wi)->AttackGround(p,true);
			}
		}
	}
#endif
}

#ifdef DIRECT_CONTROL_ALLOWED
void CPlayer::StopControllingUnit()
{
	if (gu->directControl == playerControlledUnit) {
		assert(playerHandler->Player(gu->myPlayerNum) == this);
		gu->directControl = 0;

		/* Switch back to the camera we were using before. */
		camHandler->PopMode();

		if (mouse->locked && !mouse->wasLocked){
			mouse->locked = false;
			mouse->ShowMouse();
		}
	}

	playerControlledUnit = 0;
}
#endif
