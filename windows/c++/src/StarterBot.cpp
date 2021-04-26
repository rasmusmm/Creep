#include "StarterBot.h"
#include "Tools.h"
#include "MapTools.h"
#include <iostream>
#include <fstream>
/*
Overall todo in rough priority order:

    Refine target selection/micro
        Consider target HP, distance
        Split own units so they don't try to move to a far away target?
        Damaged units should move backwards a bit?
        Should toptarget be a unitset of top targets?
        Maybe implement FAP/a selfmade simple simulation(do we have 2 zerglings for every marine/zealot they have, then engage etc etc).
        Squad implementation like UAlbertaBot?


    Develop scouting more to include harassing
        Maybe gassteal?
    
    Implement score keeping to track winrate against bots/races for testing.
        export to CSV file?

    Optimize mineral gathering
        3 per node.
        Figure out why they start moving to other nodes after collecting once.
        Is it even worth it?

    Optimize pool placement(builder moving before minerals are collected, placing it as close to builder as possible)
        Continous scanning for a poolplacement closer to worker while gathering the 150 minerals?

    Implement extractortrick
        Is it even worth it?
            Maybe on small maps where initial attack is more crucial. Good testing idea.

    Take expansion?
        if no larvae and >300 minerals, build new hatchery at natural?
        Pivot to 2base hydraspamming

    Research burrow and use it to deny expansions?


    
*/
StarterBot::StarterBot()
{
    
}
inline const char* const BoolToString(bool b)
{
    return b ? "true" : "false";
}
inline const char* const PosToString(BWAPI::Position pos)
{
    std::string x = std::to_string(pos.x);
    std::string y = std::to_string(pos.y);
    std::string xy = x + ":" + y;
    char const* result = xy.c_str();
    return result;
}
inline bool nearStartingLocation(BWAPI::Position pos) {
    std::deque<BWAPI::TilePosition> startLocs = BWAPI::Broodwar->getStartLocations();
    for (auto loc : startLocs) {
        BWAPI::Position locPos(loc);
        if (pos.getApproxDistance(locPos) < 100) {
            return true;
        }
    }
    return false;
}

// Called when the bot starts!
void StarterBot::onStart()
{
    // Set our BWAPI options here    
	BWAPI::Broodwar->setLocalSpeed(0);
    BWAPI::Broodwar->setFrameSkip(0);
    mainBase = Tools::GetUnitOfType(BWAPI::UnitTypes::Zerg_Hatchery);
    
    // Enable the flag that tells BWAPI top let users enter input while bot plays
    BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);
    assignPoolBuilderAndScout();
    // Call MapTools OnStart
    m_mapTools.onStart();

    //fill our target priorities
    //first priority are the combat units that can stop our assault. Perhaps expand to include static defense such as photon/spine.
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_Marine, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_Firebat, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Zealot, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Dragoon, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Zerg_Zergling, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Zerg_Hydralisk, 0));
    //then workers to hamstring economy.
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Zerg_Drone, 1));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Probe, 1));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_SCV, 1));
    //lasty production buildings to really deny the option of creating defense.
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_Barracks, 2));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Gateway, 2));
    //and bases
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Zerg_Hatchery, 3));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Nexus, 3));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_Command_Center, 3));



}

// Called whenever the game ends and tells you if you won or not
void StarterBot::onEnd(bool isWinner) 
{
    std::cout << "We " << (isWinner ? "won!" : "lost!") << "\n";
    //enemyRace = BWAPI::Broodwar->enemy()->getRace();
    writeStatisticsToCsv(isWinner, poolBuiltOnFrame, endFrame,enemyDistance);
}
void StarterBot::writeStatisticsToCsv(bool isWinner, int pooltiming, int wintime, int distance)
{

    std::ofstream myfile;
    myfile.open("test.csv",std::ios_base::app);
    myfile << ","<< (isWinner ? "1" : "0") << "," << pooltiming << "," << wintime << "," << distance << "\n";
    myfile.close();
}   
// Called on each frame of the game
void StarterBot::onFrame()

{
    
    if (BWAPI::Broodwar->getFrameCount() > 100000)
    {
        BWAPI::Broodwar->restartGame();
    }
    int enemyraceint = UD.getNumUnits(drone) > 0 ? 0 : UD.getNumUnits(BWAPI::UnitTypes::Terran_SCV) > 0 ? 1 : UD.getNumUnits(BWAPI::UnitTypes::Protoss_Probe) > 0 ? 2 : 3;
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 100), std::to_string(enemyraceint).c_str());
    if (enemyraceint != 3 && !raceLogged) {
        std::ofstream myfile;
        myfile.open("test.csv", std::ios_base::app);
        myfile << enemyraceint;
        myfile.close();
        raceLogged = true;
    }
    enemyDistance = mainBase->getDistance(enemyBase);
    endFrame = BWAPI::Broodwar->getFrameCount();
    UD.removeBadUnits();
    //ensure we have poolbuilder and scout
    if ((!poolBuilder && !poolbuilt) || !workerScout){
        assignPoolBuilderAndScout();
    }
    if (!leaderLing) { leaderLing = Tools::GetUnitOfType(ling); }
    //extractortrick to get an extra zergling out
    extractorTrick();
    //See if our pool is built/under construction
    if (Tools::CountUnitsOfType(pool, me->getUnits()) > 0 && !poolbuilt) {
        poolbuilt = true;
        poolBuiltOnFrame = BWAPI::Broodwar->getFrameCount();
    }
    else if (Tools::CountUnitsOfType(pool, me->getUnits()) == 0) {
        poolbuilt = false;
    }
    // Update our MapTools information
    m_mapTools.onFrame();
    //Scout
    scoutEnemy();
    //Attack
    attackEnemy();
    // Send our idle workers to mine minerals so they don't just stand there
    sendIdleWorkersToMinerals();

    //Build our pool
    buildPool();
    // Train more workers so we can gather more income
    trainUnits();

    // Build more supply if we are going to run out soon
    buildAdditionalSupply();

    // Draw unit health bars, which brood war unfortunately does not do
    Tools::DrawUnitHealthBars();

    // Draw some relevent information to the screen to help us debug the bot
    drawDebugInformation();
    //update our unitmap to remove dead units.
    
}
void StarterBot::scourEnemyBase(BWAPI::Unit unit)
{
    BWAPI::Position upperleftOffset = BWAPI::Position(enemyBase.x - 100, enemyBase.y + 100);
    BWAPI::Position lowerrightOffset = BWAPI::Position(enemyBase.x + 100, enemyBase.y - 100);
    // get the unit's current command
    BWAPI::UnitCommand currentCommand(unit->getLastCommand());

    // if we've already told this unit to attack this target, ignore this command
    if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Move && (currentCommand.getTargetPosition() == upperleftOffset || currentCommand.getTargetPosition() == lowerrightOffset))
    {
        return;
    }
    if (BWAPI::Broodwar->getFrameCount()%2 == 0){
        SmartAttackMove(unit,upperleftOffset );
    }
    else {
        SmartAttackMove(unit,lowerrightOffset );
    }
}

void StarterBot::scoutEnemy()
{
    if (BWAPI::Broodwar->getFrameCount()>scoutTiming && !scoutingDone){
        if (enemyBase != BWAPI::Positions::None) {
            overlordScout->move(mainBase->getPosition());
            scoutingDone = true;

        }
        std::deque<BWAPI::TilePosition> startLocs = BWAPI::Broodwar->getStartLocations();
    

        //if there are less than 3 starting locations 
        //there is no need to scout since we know where our enemy is
        if (startLocs.size() < 3) {
            for (BWAPI::TilePosition loc : startLocs) {

                if (BWAPI::Broodwar->isExplored(loc)) { continue; }
                BWAPI::Position pos(loc);
                enemyBase = pos;
                scoutingDone = true;
                break;
            }
            return;
        }

    
        //Make deque of unexplored starting locations to explore
        std::deque<BWAPI::TilePosition> unexploredLocs;
        for (BWAPI::TilePosition loc : startLocs) {
            if (BWAPI::Broodwar->isExplored(loc)) { continue; }
            unexploredLocs.push_front(loc);
        }
        //if there is only one unexplored location, this is the enemybase.
        if (unexploredLocs.size() == 1 && enemyBase == BWAPI::Positions::None)
        { 
            enemyBase = BWAPI::Position(unexploredLocs.front());
            return;
        }
        
    
        int index = 0;
        //going through unexplored locations, workerscout will take the first, overlordScout the second.
        for (BWAPI::TilePosition loc : unexploredLocs) {
            BWAPI::Position pos(loc);
        
            if (index == 0)
                possibleEnemyBase = pos;
                if (poolbuilt)
                    workerScout->move(pos);
            if (index == 1)
                overlordScout->move(pos);
            index++;
        }

    
    }
    if (workerScout->getHitPoints() > 25 && scoutingDone && poolbuilt) {
        SmartAttackMove(workerScout, enemyBase);
        
    }
    else {
        if (workerScout->getDistance(mainBase) > 300 && scoutingDone)
            workerScout->move(mainBase->getPosition());
        
    }
    
}

//TODO: Refine function to consider elements such as distance to target and remaining HP so our units will focus down targets.
//      Remove uneccessary isFlyer checks.
void StarterBot::attackEnemy()
{
    
    int priority = 100;
    UnitInfo topTarget = UnitInfo();
    std::map<BWAPI::UnitType, int>::iterator it;

    
    UIMap unitMap = UD.getUnits();
    //Cycle through our enemies in our UnitMap and checking their units in our priority map to find a good target.
    for (const auto& unit : unitMap)
    {
        BWAPI::UnitType type = unit.first->getType();

        it = priorityCheck.find(type);
        if (it != priorityCheck.end()) {
            if (priorityCheck[type] < priority) {
                topTarget = unit.second;
                continue;
            }
            if (priorityCheck[type] == priority) {
                
                
                if (unit.second.lastHealth < topTarget.lastHealth && unit.first->getDistance(leaderLing) < 200) {
                    topTarget = unit.second;
                    continue;
                }
                if (unit.first->getDistance(leaderLing) < topTarget.unit->getDistance(leaderLing)) {
                    topTarget = unit.second;
                    continue;
                }
                
                
            }
            
        }
        
        
        else {
            if (topTarget.type == BWAPI::UnitTypes::None) 
                topTarget = unit.second;
        }
        
    }
   
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 30),topTarget.type.getName().c_str() );
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 40), PosToString(topTarget.lastPosition));
    // && (BWAPI::Broodwar->getFrameCount())%10 == 0
     //(BWAPI::Broodwar->getFrameCount())%10 == 0)
    //Make our lings attack if they are not actively attacking a target at this moment. If no target is found in the previous loop they simple run towards the enemybase.
    for (auto& unit : me->getUnits()) {
        if (unit->getType() == ling)
        {
            //If the target is visible and closeby/a marine, attack that target. (Marines are the only ranged unit we should realistically be facing).
            if (topTarget.visible && (topTarget.unit->getDistance(unit)<40)||topTarget.type == BWAPI::UnitTypes::Terran_Marine) {
                SmartAttackMove(unit, topTarget.lastPosition);

                BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 50), "Attacking");
            } /*else if (topTarget.visible && topTarget.unit->getDistance(unit) >= 40) {
                SmartAttackMove(unit, topTarget.lastPosition);
                BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 60), "Out of range, moving");
            }
            else  if (topTarget.type != BWAPI::UnitTypes::None) {
               SmartAttackMove(unit,topTarget.lastPosition);
            }*/
            else {
                if (UD.getUnits().size() < 8 && UD.getMineralsLost() > 200) {
                    SmartAttackMove(unit, topTarget.lastPosition);
                } else {
                enemyBase == BWAPI::Positions::None ? SmartAttackMove(unit,possibleEnemyBase) : SmartAttackMove(unit,enemyBase);
                }
            }
                    
                
            

        }
    }

}
void StarterBot::SmartAttackUnit(BWAPI::Unit attacker, BWAPI::Unit target)
{

    if (!attacker || !target)
    {
        return;
    }

    // if we have issued a command to this unit already this frame, ignore this one
    if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
    {
        return;
    }

    // get the unit's current command
    BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

    // if we've already told this unit to attack this target, ignore this command
    if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Unit && currentCommand.getTarget() == target)
    {
        return;
    }

    // if nothing prevents it, attack the target
    attacker->attack(target);
}
void StarterBot::SmartAttackMove(BWAPI::Unit attacker, const BWAPI::Position& targetPosition)
{

    if (!attacker || !targetPosition.isValid())
    {
        return;
    }

    // if we have issued a command to this unit already this frame, ignore this one
    if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
    {
        return;
    }

    // get the unit's current command
    BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

    // if we've already told this unit to attack this target, ignore this command
    if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Move && currentCommand.getTargetPosition() == targetPosition)
    {
        return;
    }

    // if nothing prevents it, attack the target
    attacker->attack(targetPosition);

}

void StarterBot::assignPoolBuilderAndScout()
{
    int index = 0;
    // For each unit that we own
    for (auto& unit : BWAPI::Broodwar->self()->getUnits())
    {
        // if the unit is of the correct type, and it actually has been constructed, return it
        if (unit->getType() == drone && unit->isCompleted())
        {
            if (index == 0) {
                workerScout = unit;
            }
            else if (index == 1 && !poolbuilt) {
                poolBuilder = unit;
            }
            index++;

        }
        if (!overlordScout && unit->getType() == BWAPI::UnitTypes::Zerg_Overlord)
            overlordScout = unit;
    }
}

void StarterBot::buildPool()
{
    if (!poolBuilder) { return; }
    if (!poolbuilt) {
        if (me->minerals() >= 200) {
            //poolBuilder->move(mainBase->getPosition());
            BWAPI::TilePosition desiredPos = BWAPI::TilePosition(poolBuilder->getPosition());

            // Ask BWAPI for a building location near the desired position for the type
            int maxBuildRange = 3;
            BWAPI::TilePosition buildPos = BWAPI::Broodwar->getBuildLocation(pool, desiredPos, maxBuildRange, true);
            poolBuilder->build(pool, buildPos);
        }
    }

}

// Send our idle workers to mine minerals so they don't just stand there
void StarterBot::sendIdleWorkersToMinerals()
{
    // Let's send all of our starting workers to the closest mineral to them
    // First we need to loop over all of the units that we (BWAPI::Broodwar->self()) own
    const BWAPI::Unitset& myUnits = BWAPI::Broodwar->self()->getUnits();
    BWAPI::Unitset minerals = BWAPI::Broodwar->getMinerals();

    // Get the closest and second closest mineral to the base
    BWAPI::Unit closestMineral = Tools::GetClosestUnitTo(mainBase, BWAPI::Broodwar->getMinerals());
    for (auto& unit : myUnits)
    {
        // Check the unit type, if it is an idle worker, then we want to send it somewhere
        if (unit->getType().isWorker() && unit->isIdle())
        {
            // If a valid mineral was found, right click it with the unit in order to start harvesting
            if (closestMineral) { unit->rightClick(closestMineral); }
        }
    }
}
//TODO: Fix this. Set on hold for more important features so far.
void StarterBot::extractorTrick()
{
    /*
    BWAPI::Unit builder = Tools::GetUnitOfType(drone);
    
    if (Tools::GetTotalSupply(false) == 9 && BWAPI::Broodwar->self()->supplyUsed() == 9 ) {
        BWAPI::TilePosition desiredPos = BWAPI::Broodwar->self()->getStartLocation();

        // Ask BWAPI for a building location near the desired position for the type
        int maxBuildRange = 64;
        BWAPI::TilePosition buildPos = BWAPI::Broodwar->getBuildLocation(BWAPI::UnitTypes::Zerg_Extractor, desiredPos, maxBuildRange, false);
        builder->morph(BWAPI::UnitTypes::Zerg_Extractor);
    }
    if (Tools::GetTotalSupply(false) == 9 && BWAPI::Broodwar->self()->supplyUsed() == 10) {
        BWAPI::Unit extractor = Tools::GetUnitOfType(BWAPI::UnitTypes::Zerg_Extractor);
        extractor->cancelMorph();
    }*/
}
// Train more workers so we can gather more income
void StarterBot::trainUnits()
{   
    int workersWanted = (poolbuilt && (poolBuiltOnFrame + 150) < BWAPI::Broodwar->getFrameCount()) ? 6 : 5;
    const int workersOwned = Tools::CountUnitsOfType(drone, BWAPI::Broodwar->self()->getUnits());
    
    if (workersOwned < workersWanted)
    {

        // if we have a valid depot unit and it's currently not training something, train a worker
        // there is no reason for a bot to ever use the unit queueing system, it just wastes resources
        if (mainBase && !mainBase->isTraining()) { mainBase->train(drone); }
    }
    else {
        if (mainBase && !mainBase->isTraining()) { mainBase->train(ling); }
    }
    
}

// Build more supply if we are going to run out soon
void StarterBot::buildAdditionalSupply()
{
    // Get the amount of supply supply we currently have unused
    const int unusedSupply = Tools::GetTotalSupply(true) - BWAPI::Broodwar->self()->supplyUsed();
    bool supplyInProgress = false;
    // If we have a sufficient amount of supply, we don't need to do anything
    if (unusedSupply >= 2) { return; }

    for (auto& unit : BWAPI::Broodwar->self()->getUnits())
    {
        if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg)
        {
            if (unit->getBuildType() == BWAPI::UnitTypes::Zerg_Overlord)
            {
                supplyInProgress = true;
            }
        }
    }
    if (!supplyInProgress) {
        auto hatch = Tools::GetUnitOfType(BWAPI::UnitTypes::Zerg_Hatchery);
        hatch->train(BWAPI::UnitTypes::Zerg_Overlord);
    }
}

// Draw some relevent information to the screen to help us debug the bot
void StarterBot::drawDebugInformation()
{
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 10), BoolToString(poolbuilt));
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 20), PosToString(enemyBase));
    Tools::DrawUnitCommands();
    Tools::DrawUnitBoundingBoxes();
}

// Called whenever a unit is destroyed, with a pointer to the unit
void StarterBot::onUnitDestroy(BWAPI::Unit unit)
{
    /*
    if (unit->getPlayer() == BWAPI::Broodwar->self() && unit->getType() == pool) {
        poolbuilt = false;
    }*/
    if (unit->getPlayer() == foe) {
        UD.removeUnit(unit);
        
    }
        
    
}

// Called whenever a unit is morphed, with a pointer to the unit
// Zerg units morph when they turn into other units
void StarterBot::onUnitMorph(BWAPI::Unit unit)
{
    if (unit->getPlayer() == foe && !unit->isFlying())
        UD.updateUnit(unit);
}

// Called whenever a text is sent to the game by a user
void StarterBot::onSendText(std::string text) 
{ 
    if (text == "/map")
    {
        m_mapTools.toggleDraw();
    }
}

// Called whenever a unit is created, with a pointer to the destroyed unit
// Units are created in buildings like barracks before they are visible, 
// so this will trigger when you issue the build command for most units
void StarterBot::onUnitCreate(BWAPI::Unit unit)
{ 
    if (unit->getPlayer() == foe && !unit->isFlying())
        UD.updateUnit(unit);
}

// Called whenever a unit finished construction, with a pointer to the unit
void StarterBot::onUnitComplete(BWAPI::Unit unit)
{
    if (unit->getPlayer() == foe && !unit->isFlying())
        UD.updateUnit(unit);
}

// Called whenever a unit appears, with a pointer to the destroyed unit
// This is usually triggered when units appear from fog of war and become visible
void StarterBot::onUnitShow(BWAPI::Unit unit)
{ 
    /*
    if (unit->getPlayer() == foe)
        enemyRace = unit->getPlayer()->getRace();*/
    if (unit->getPlayer() == foe && unit->getType().isResourceDepot() && nearStartingLocation(unit->getPosition()) )
        enemyBase = unit->getPosition();
    if (unit->getPlayer() == foe && !unit->isFlying())
        UD.updateUnitShow(unit);

}

// Called whenever a unit gets hidden, with a pointer to the destroyed unit
// This is usually triggered when units enter the fog of war and are no longer visible
void StarterBot::onUnitHide(BWAPI::Unit unit)
{ 
    if (unit->getPlayer() == foe && !unit->isFlying())
        UD.updateUnitHide(unit);
}

// Called whenever a unit switches player control
// This usually happens when a dark archon takes control of a unit
void StarterBot::onUnitRenegade(BWAPI::Unit unit)
{ 
    if (unit->getPlayer() == foe && !unit->isFlying())
        UD.updateUnit(unit);
}