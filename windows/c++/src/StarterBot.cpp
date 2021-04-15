#include "StarterBot.h"
#include "Tools.h"
#include "MapTools.h"

StarterBot::StarterBot()
{
    
}

// Called when the bot starts!
void StarterBot::onStart()
{
    // Set our BWAPI options here    
	BWAPI::Broodwar->setLocalSpeed(15);
    BWAPI::Broodwar->setFrameSkip(0);
    mainBase = Tools::GetUnitOfType(BWAPI::UnitTypes::Zerg_Hatchery);
    // Enable the flag that tells BWAPI top let users enter input while bot plays
    BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);
    assignPoolBuilderAndScout();
    // Call MapTools OnStart
    m_mapTools.onStart();
    //A quick fix to stop zerglings from running to the upper left corner of the map if the enemy is unscouted.
    enemyBase = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());

    //fill our target priorities
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_Marine, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_Firebat, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Zealot, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Dragoon, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Zerg_Zergling, 0));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Zerg_Hydralisk, 0));

    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Zerg_Drone, 1));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Probe, 1));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_SCV, 1));

    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Terran_Barracks, 2));
    priorityCheck.insert(pair<BWAPI::UnitType, int>(BWAPI::UnitTypes::Protoss_Gateway, 2));



}

// Called whenever the game ends and tells you if you won or not
void StarterBot::onEnd(bool isWinner) 
{
    std::cout << "We " << (isWinner ? "won!" : "lost!") << "\n";
}

// Called on each frame of the game
void StarterBot::onFrame()
{
    //ensure we have poolbuilder and scout
    if ((!poolBuilder && !poolbuilt) || !workerScout){
        assignPoolBuilderAndScout();
    }
    //extractortrick to get an extra zergling out
    extractorTrick();
    //See if our pool is built/under construction
    if (Tools::CountUnitsOfType(pool, me->getUnits()) > 0) {
        poolbuilt = true;
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
}


void StarterBot::scoutEnemy()
{
    if (BWAPI::Broodwar->getFrameCount()>scoutTiming && !scoutingDone){
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
    //if there is only one unexplored location, this is the enemybase.
    if (unexploredLocs.size() == 1)
        enemyBase = BWAPI::Position(unexploredLocs.front());
    for (BWAPI::TilePosition loc : startLocs) {
        if (BWAPI::Broodwar->isExplored(loc)) { continue; }
        unexploredLocs.push_front(loc);
    }
    
    int index = 0;
    //going through unexplored locations, workerscout will take the first, overlordScout the second.
    for (BWAPI::TilePosition loc : unexploredLocs) {
        BWAPI::Position pos(loc);
        /*
        int overlordDist = overlordScout->getDistance(pos);
        int workerDist = workerScout->getDistance(pos);
        if (workerDist <= overlordDist) {
            workerScout->move(pos);
            
        } 
        */
        if (index == 0)
            workerScout->move(pos);
        if (index == 1)
            overlordScout->move(pos);
        index++;
    }

    if (enemyBase != BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation())) {
        workerScout->move(mainBase->getPosition());
        overlordScout->move(mainBase->getPosition());
        scoutingDone = true;
    }
    }
}

void StarterBot::attackEnemy()
{
    int priority = 100;
    BWAPI::Unit topTarget = nullptr;
    std::map<BWAPI::UnitType, int>::iterator it;
    //Cycle through our enemies checking their units in our priority map to find a good target.
    for (auto& unit : foe->getUnits()) {
        BWAPI::UnitType type = unit->getType();
        it = priorityCheck.find(type);
        if (it != priorityCheck.end()) {
            if (priorityCheck[type] < priority) {
                topTarget = unit;
            }
        }
    }

    //(BWAPI::Broodwar->getFrameCount())%10 == 0)
    //Make our lings attack if they are not actively attacking a target at this moment. If no target is found in the previous loop they simple run towards the enemybase.
    for (auto& unit : me->getUnits()) {
        if ((unit->getType() == ling) && !unit->isAttackFrame() ) {
            
            topTarget ? unit->attack(topTarget, false) : unit->attack(enemyBase, false);
        }
    }
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
        if (me->minerals() > 190) {
            //poolBuilder->move(mainBase->getPosition());
            BWAPI::TilePosition desiredPos = BWAPI::TilePosition(poolBuilder->getPosition());

            // Ask BWAPI for a building location near the desired position for the type
            int maxBuildRange = pool.width();
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
    BWAPI::Unit sndClosestMineral = Tools::GetClosestUnitTo(closestMineral, BWAPI::Broodwar->getMinerals());
    int index = 0;
    for (auto& unit : myUnits)
    {
        // Check the unit type, if it is an idle worker, then we want to send it somewhere
        if (unit->getType().isWorker() && unit->isIdle())
        {
            if (unit->isCarryingMinerals()) {
                unit->rightClick(mainBase);
            }
            // If a valid mineral was found, right click it with the unit in order to start harvesting
            if (closestMineral && index<=3) { unit->rightClick(closestMineral); }
            if (sndClosestMineral && index>3) { unit->rightClick(sndClosestMineral); }
            index++;
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
    int workersWanted = poolbuilt ? 6 : 5;
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
    BWAPI::Broodwar->drawTextScreen(BWAPI::Position(10, 10), "Hello, World!\n");
    Tools::DrawUnitCommands();
    Tools::DrawUnitBoundingBoxes();
}

// Called whenever a unit is destroyed, with a pointer to the unit
void StarterBot::onUnitDestroy(BWAPI::Unit unit)
{
    if (unit->getPlayer() == BWAPI::Broodwar->self() && unit->getType() == pool) {
        poolbuilt = false;
    }
}

// Called whenever a unit is morphed, with a pointer to the unit
// Zerg units morph when they turn into other units
void StarterBot::onUnitMorph(BWAPI::Unit unit)
{
	
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
	
}

// Called whenever a unit finished construction, with a pointer to the unit
void StarterBot::onUnitComplete(BWAPI::Unit unit)
{
	
}

// Called whenever a unit appears, with a pointer to the destroyed unit
// This is usually triggered when units appear from fog of war and become visible
void StarterBot::onUnitShow(BWAPI::Unit unit)
{ 
    if (unit->getPlayer() == foe && unit->getType().isResourceDepot())
        enemyBase = unit->getPosition();
}

// Called whenever a unit gets hidden, with a pointer to the destroyed unit
// This is usually triggered when units enter the fog of war and are no longer visible
void StarterBot::onUnitHide(BWAPI::Unit unit)
{ 
	
}

// Called whenever a unit switches player control
// This usually happens when a dark archon takes control of a unit
void StarterBot::onUnitRenegade(BWAPI::Unit unit)
{ 
	
}