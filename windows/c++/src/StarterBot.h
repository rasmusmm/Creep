#pragma once

#include "MapTools.h"

#include <BWAPI.h>
#include <map>
#include <iterator>
#include "UnitData.h"
using namespace std;
class StarterBot
{
	//bool expansionDestroyed = false;
	int endFrame = 0;
	bool poolbuilt = false;
	int scoutTiming = 10;
	bool raceLogged = false;
	int poolBuiltOnFrame;
	int enemyDistance = 0;
	BWAPI::Player me = BWAPI::Broodwar->self();
	BWAPI::Player foe = BWAPI::Broodwar->enemy();
    MapTools m_mapTools;
	BWAPI::Unit mainBase = nullptr;
	BWAPI::TilePosition poolPos = BWAPI::TilePositions::None;
	BWAPI::Unit leaderLing = nullptr;
	UnitData UD;
	BWAPI::Unit workerScout = nullptr;
	BWAPI::Unit overlordScout = nullptr;
	
	//BWAPI::Race enemyRace;

	BWAPI::Unit poolBuilder = nullptr;
	bool scoutingDone = false;

	BWAPI::UnitType ling = BWAPI::UnitTypes::Zerg_Zergling;
	BWAPI::UnitType drone = BWAPI::UnitTypes::Zerg_Drone;
	BWAPI::UnitType pool = BWAPI::UnitTypes::Zerg_Spawning_Pool;
	BWAPI::Position enemyBase = BWAPI::Positions::None;
	BWAPI::Position possibleEnemyBase = BWAPI::Positions::None;
	//int enemyraceint;

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
	void SmartAttackUnit(BWAPI::Unit attacker, BWAPI::Unit target);
	void SmartAttackMove(BWAPI::Unit attacker, const BWAPI::Position& targetPosition);
	void scourEnemyBase(BWAPI::Unit unit);
	void writeStatisticsToCsv(bool isWinner, int pooltiming, int wintime, int distance);
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