#pragma once

#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_lib.h"
#include <iostream>

using namespace sc2;

//class BlinkerBot;

class UnitData
{
public:
	UnitData();
	~UnitData();
	static bool isOurs(const Unit *unit);
	static bool isWorker(const Unit *unit);
	static bool isStructure(const Unit *unit);
	static bool isFightingUnit(const Unit *unit);
	static bool isTownHall(const Unit *unit);
	static bool isSupplyProvider(const Unit *unit);
	static bool isMinerals(const Unit *unit);
	static bool isVespeneGeyser(const Unit *unit);
	static bool isBuildAbility(AbilityID ability);
	static bool isResearch(AbilityID ability);
	static bool isTrainableUnitType(AbilityID ability);
	static bool isGasStructure(const Unit *unit);
	static bool canCloak(const Unit *unit);
	static bool isComparableUpgrade(AbilityID abilityA, AbilityID abilityB);
	static UnitTypeID getUnitTypeID(AbilityID ability);
	static AbilityID getAbilityID(UpgradeID upgrade);
	static UnitTypeID requiredStructure(AbilityID ability);
private:
	static bool compareUpgrades(AbilityID abilityA, AbilityID abilityB);
};

