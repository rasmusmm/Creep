#pragma once

#include "MapTools.h"

#include <BWAPI.h>
#include <map>
#include <iterator>
using namespace std;
class StarterBot
{
	bool poolbuilt = false;
	int scoutTiming = 10;
	BWAPI::Player me = BWAPI::Broodwar->self();
	BWAPI::Player foe = BWAPI::Broodwar->enemy();
	BWAPI::Position poolPos;
    MapTools m_mapTools;
	BWAPI::Unit mainBase = nullptr;
	BWAPI::Unit worker1 = nullptr;
	BWAPI::Unit worker2 = nullptr;
	BWAPI::Unit worker3 = nullptr;


	BWAPI::Unit workerScout = nullptr;
	bool wScoutUsed = false;
	BWAPI::Unit overlordScout = nullptr;
	bool oScoutUsed = false;
	bool scoutingDone = false;

	BWAPI::Unit poolBuilder = nullptr;


	BWAPI::UnitType ling = BWAPI::UnitTypes::Zerg_Zergling;
	BWAPI::UnitType drone = BWAPI::UnitTypes::Zerg_Drone;
	BWAPI::UnitType pool = BWAPI::UnitTypes::Zerg_Spawning_Pool;
	BWAPI::Position enemyBase;
	BWAPI::TilePosition enemyStartPos;


	deque<BWAPI::UnitType> combatUnitCheck = { BWAPI::UnitTypes::Zerg_Zergling,BWAPI::UnitTypes::Terran_Marine,BWAPI::UnitTypes::Terran_Firebat,BWAPI::UnitTypes::Protoss_Zealot,BWAPI::UnitTypes::Protoss_Dragoon };
	deque<BWAPI::UnitType> importantBuildingCheck = { BWAPI::UnitTypes::Terran_Barracks,BWAPI::UnitTypes::Protoss_Gateway };
	
	map<BWAPI::UnitType, int> priorityCheck;
	

	enum Build {
		fivepool, ninepool
	};
public:

    StarterBot();

    // helper functions to get you started with bot programming and learn the API
    void sendIdleWorkersToMinerals();
    void trainUnits();
    void buildAdditionalSupply();
    void drawDebugInformation();
	void assignPoolBuilderAndScout();
	void buildPool();
	void scoutEnemy();
	void attackEnemy();
	void extractorTrick();
    // functions that are triggered by various BWAPI events from main.cpp
	void onStart();
	void onFrame();
	void onEnd(bool isWinner);
	void onUnitDestroy(BWAPI::Unit unit);
	void onUnitMorph(BWAPI::Unit unit);
	void onSendText(std::string text);
	void onUnitCreate(BWAPI::Unit unit);
	void onUnitComplete(BWAPI::Unit unit);
	void onUnitShow(BWAPI::Unit unit);
	void onUnitHide(BWAPI::Unit unit);
	void onUnitRenegade(BWAPI::Unit unit);
};