#include "BuildOrderManager.h"
#include "Blinkerbot.h"

BuildOrderManager::BuildOrderManager(BlinkerBot & bot) : blinkerBot(bot), enemyHasCloak(false),
currentBases(0), currentGases(0), currentProductionFacilities(0), currentCannons(0)
{
	initialiseKeyTechs();
}

BuildOrderManager::~BuildOrderManager()
{
}

ProductionGoal::ProductionGoal(AbilityID type, int quantity)
{
	this->type = type;
	this->quantity = quantity;
}

ProductionGoal::~ProductionGoal()
{
}

/*
count the numbers of various types of structures under our control
*/
void BuildOrderManager::updateStructureCounts()
{
	int bases = 0;
	int productionFacilities = 0;
	int gases = 0;
	int cannons = 0;

	for (auto structure : blinkerBot.Observation()->GetUnits())
	{
		if (UnitData::isOurs(structure) && UnitData::isStructure(structure))
		{
			if (structure->unit_type == UNIT_TYPEID::PROTOSS_NEXUS)
			{
				bases++;
			}
			else if (structure->unit_type == UNIT_TYPEID::PROTOSS_GATEWAY ||
				structure->unit_type == UNIT_TYPEID::PROTOSS_WARPGATE ||
				structure->unit_type == UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)
			{
				productionFacilities++;
			}
			else if (structure->unit_type == UNIT_TYPEID::PROTOSS_ASSIMILATOR)
			{
				gases++;
			}
			else if (structure->unit_type == UNIT_TYPEID::PROTOSS_PHOTONCANNON)
			{
				cannons++;
			}
		}
	}

	currentBases = bases;
	currentGases = gases;
	currentProductionFacilities = productionFacilities;
	currentCannons = cannons;
}

/*
returns a set of AbilityID and int pairs representing things we want to produce and the quantity of each thing we want
*/
std::vector<ProductionGoal> BuildOrderManager::generateGoal()
{
	std::vector<ProductionGoal> buildOrderGoal;

	//first let's count the number of things we currently have
	updateStructureCounts();

	//next let's calculate how many extra things we want
	int extraCannons = currentBases - currentCannons;
	int extraProductionFacilities = (currentBases * 2) - currentProductionFacilities;
	int extraGases = (currentBases * 2) - currentGases;

	//if there are any necessary techs, let's add the next one to the queue
	//if we only have one base then we don't want to start teching to colossus yet
	AbilityID tech = getNextTech();
	if (tech != ABILITY_ID::INVALID && !(currentBases == 1 && getNextTech() == ABILITY_ID::RESEARCH_EXTENDEDTHERMALLANCE))
	{
		if (currentBases == 1 && extraGases > 0)
		{
			buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_ASSIMILATOR, 1));
			extraGases--;
		}
		buildOrderGoal.push_back(ProductionGoal(tech, 1));
	}

	//if the enemy has cloaked units, we want to make sure we have cannons at each base
	if (enemyHasCloak || currentBases > 2)
	{
		if (extraCannons > 0)
		{
			buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_PHOTONCANNON, extraCannons));
		}
	}

	//add extra gateways at a 2:1 gateway to base ratio as our economy improves
	if (extraProductionFacilities > 0)
	{
		//we don't wanna add too many extra production facilities at one time as this will slow down our production
		if (extraProductionFacilities > 2)
		{
			buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_GATEWAY, 2));
		}
		else
		{
			buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_GATEWAY, extraProductionFacilities));
		}
	}

	//calculate any additional gases we want to take
	if (extraGases > 0)
	{
		//buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_ASSIMILATOR, extraGases));
		buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_ASSIMILATOR, 1));
	}

	//if we've already got plenty of production facilities then we want to think about expanding
	if ((extraProductionFacilities < 1 && extraGases < 1) || miningOut)
	{
		buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_NEXUS, 1));
	}

	if (buildOrderGoal.empty())
	{
		buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_PYLON, 1));
	}

	/*
	std::cerr << "current bases: " << currentBases << std::endl;
	std::cerr << "current gateways: " << currentProductionFacilities << std::endl;
	std::cerr << "goal state: " << currentBases << " bases and " << currentProductionFacilities + extraProductionFacilities << " gateways" << std::endl;
	std::cerr << "adding to production queue:" << std::endl;
	for (auto item : buildOrderGoal)
	{
	for (int i = 0; i != item.second; i++)
	{
	std::cerr << AbilityTypeToName(item.first) << std::endl;
	}
	}
	*/

	return buildOrderGoal;
}

/*
called when an upgrade is completed
*/
void BuildOrderManager::onUpgradeComplete(UpgradeID upgrade)
{
	removeKeyTech(upgrade);
}

/*
checks the vector of key techs, removes any that have already been researched, and returns the next item
*/
AbilityID BuildOrderManager::getNextTech()
{
	//check that there are some remaining techs
	if (!keyTechs.empty())
	{
		//check through to see if any techs are already researched (and remove them if they are)
		std::vector<AbilityID>::iterator nextTech = keyTechs.begin();
		while (alreadyResearched(*nextTech) && nextTech != keyTechs.end())
		{
			//std::cerr << "removing already completed tech: " << AbilityTypeToName(*nextTech) << std::endl;
			removeKeyTech(*nextTech++);
		}
		//we might have removed some so make sure we still have some left
		if (!keyTechs.empty())
		{
			//check through to see if any techs are already in progress (and skip them (but don't remove them) if they are)
			std::vector<AbilityID>::iterator nextTech = keyTechs.begin();
			while (inProgress(*nextTech) && nextTech != keyTechs.end())
			{
				//std::cerr << "skipping already started tech: " << AbilityTypeToName(*nextTech) << std::endl;
				nextTech++;
			}
			return *nextTech;
		}
		else
		{
			return ABILITY_ID::INVALID;
		}
	}
	else
	{
		return ABILITY_ID::INVALID;
	}
	
}

//checks our structures to see if we already have this ability in progress
bool BuildOrderManager::inProgress(AbilityID ability)
{
	for (auto unit : blinkerBot.Observation()->GetUnits())
	{
		if (UnitData::isOurs(unit) && UnitData::isStructure(unit))
		{
			for (auto order : unit->orders)
			{
				if (order.ability_id == ability || UnitData::isComparableUpgrade(order.ability_id, ability))
				{
					return true;
				}
			}
		}
	}
	return false;
}

/*
1/2 removes a key tech from the vector
*/
void BuildOrderManager::removeKeyTech(UpgradeID upgrade)
{
	for (std::vector<AbilityID>::iterator tech = keyTechs.begin(); tech != keyTechs.end();)
	{
		if (UnitData::getAbilityID(upgrade) == *tech)
		{
			//std::cerr << "removing a key tech: " << AbilityTypeToName(*tech) << std::endl;
			tech = keyTechs.erase(tech);
		}
		else
		{
			++tech;
		}
	}
}

/*
2/2 removes a key tech from the vector
*/
void BuildOrderManager::removeKeyTech(AbilityID ability)
{
	for (std::vector<AbilityID>::iterator tech = keyTechs.begin(); tech != keyTechs.end();)
	{
		if (ability == *tech)
		{
			//std::cerr << "removing key tech: " << AbilityTypeToName(*tech) << std::endl;
			tech = keyTechs.erase(tech);
		}
		else
		{
			++tech;
		}
	}
}

/*
returns true if a tech has already been researched
*/
bool BuildOrderManager::alreadyResearched(AbilityID ability)
{
	for (auto completed : completedTechs)
	{
		if (UnitData::getAbilityID(completed) == ability)
		{
			return true;
		}
	}
	return false;
}

/*
tells us if the enemy has cloak or not
*/
void BuildOrderManager::receiveCloakSignal(bool signal)
{
	enemyHasCloak = signal;
}

/*
returns a set of our bases
*/
std::set<const Unit *> BuildOrderManager::getBases()
{
	std::set<const Unit *> bases;
	for (auto unit : blinkerBot.Observation()->GetUnits())
	{
		if (UnitData::isOurs(unit) && UnitData::isTownHall(unit))
		{
			bases.insert(unit);
		}
	}
	return bases;
}

/*
tells us when one of our bases is mining out
*/
void BuildOrderManager::receiveMiningOutSignal(bool signal)
{
	miningOut = signal;
}

/*
creates an ordered list of important upgrades that need to be researched throughout the game
*/
void BuildOrderManager::initialiseKeyTechs()
{
	keyTechs.push_back(ABILITY_ID::RESEARCH_WARPGATE);
	keyTechs.push_back(ABILITY_ID::RESEARCH_BLINK);
	keyTechs.push_back(ABILITY_ID::RESEARCH_EXTENDEDTHERMALLANCE);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONSLEVEL1);
	keyTechs.push_back(ABILITY_ID::RESEARCH_CHARGE);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONSLEVEL2);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONSLEVEL3);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMORLEVEL1);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMORLEVEL2);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMORLEVEL3);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSSHIELDSLEVEL1);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSSHIELDSLEVEL2);
	keyTechs.push_back(ABILITY_ID::RESEARCH_PROTOSSSHIELDSLEVEL3);
}

/*
add an upgrade to the set of completed ones
*/
void BuildOrderManager::addCompletedTech(UpgradeID upgrade)
{
	completedTechs.insert(upgrade);
}

/*
returns true if the ability is one of our key techs
*/
bool BuildOrderManager::isKeyTech(AbilityID ability)
{
	for (auto tech : keyTechs)
	{
		if (tech == ability)
		{
			return true;
		}
	}
	return false;
}

/*
alters our build order to deal with rushes
*/
std::vector<ProductionGoal> BuildOrderManager::generateRushDefenceGoal()
{
	std::vector<ProductionGoal> buildOrderGoal;

	//first count what we have
	updateStructureCounts();

	int extraProductionFacilities = 4 - currentProductionFacilities;
	int extraGases = 1 - currentGases;

	//if we don't have any gas yet, then we want to add one
	if (extraGases > 0)
	{
		buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_ASSIMILATOR, extraGases));
	}

	//we wanna get warpgate if we don't have it, but don't tech any further than that yet
	AbilityID tech = getNextTech();
	if (tech == ABILITY_ID::RESEARCH_WARPGATE)
	{
		//put in an extra gateway first so we don't get stuck on 1 gate while teching
		if (extraProductionFacilities > 0)
		{
			int earlyGates = 2 - currentProductionFacilities;
			buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_GATEWAY, earlyGates));
			extraProductionFacilities -= earlyGates;
		}
		buildOrderGoal.push_back(ProductionGoal(tech, 1));
	}

	//we can go up to 4 gateways to make sure we can defend
	if (extraProductionFacilities > 0)
	{
		buildOrderGoal.push_back(ProductionGoal(ABILITY_ID::BUILD_GATEWAY, extraProductionFacilities));
	}

	return buildOrderGoal;
}