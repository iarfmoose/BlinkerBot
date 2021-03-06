#pragma once
#include <iostream>
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_lib.h"
#include "UnitData.h"
#include <sstream>

using namespace sc2;

class BlinkerBot;
enum ArmyStatus;

class ArmyManager
{
	BlinkerBot & blinkerBot;

	const int LOCALRADIUS = 15; //radius used to count the number of nearby units and whether units are close to bases
	const int WAITTIME = 500; //represents the number of game loops we can wait before a regroup order times out
	const float zerglingSupply = 0.75; //increased from 0.5 for army size comparisons to reflect their strength vs stalkers

	struct ArmyUnit
	{
		const Unit *unit;
		ArmyStatus status;
		ArmyUnit(const Unit *armyUnit, ArmyStatus unitStatus): unit(armyUnit), status(unitStatus){};
	};

	struct ZerglingTimer
	{
		const Unit *zergling;
		float startTime;
		float endTime;
		bool onCreep;
		Point2D startPosition;
		ZerglingTimer(const Unit *ling, float sTime, float eTime, Point2D sPos, bool creep):
			zergling(ling), startTime(sTime), endTime(eTime), startPosition(sPos), onCreep(creep){};
	};

	Race enemyRace;
	bool beingRushed;
	bool demolitionDuty;
	bool proxy;
	int regroupStarted;
	bool regroupComplete;
	bool zerglingSpeed;
	bool warpgateTech;
	bool blinkTech;
	bool extendedThermalLanceTech;
	float currentArmyValue;
	float currentEnemyArmyValue;
	float totalZerglingSupply;
	ArmyStatus currentStatus;
	Point2D rallyPoint;
	ZerglingTimer zerglingTimer;

	std::vector<ArmyUnit> army;
	std::vector<Point2D> unexploredEnemyStartLocations;
	std::set<const Unit *> bases;
	std::set<const Unit *> darkTemplars;
	std::set<const Unit *> highTemplars;
	std::set<const Unit *> enemyArmy;
	std::set<const Unit *> enemyStructures;
	std::set<const Unit *> observers;
	std::set<const Unit *> photonCannons;
	std::set<const Unit *> sentries;
	std::set<const Unit *> structures;


public:
	ArmyManager(BlinkerBot & bot);
	void addEnemyStructure(const Unit *structure);
	void addEnemyUnit(const Unit *unit);
	void addStructure(const Unit *structure);
	void addUnit(const Unit *unit);
	bool behind();
	void breakWall(const Unit *blocker);
	bool detectionRequired();
	ArmyStatus getArmyStatus();
	void initialise();
	bool lingSpeed();
	bool massLings();
	void onStep();
	void onUpgradeComplete(UpgradeID upgrade);
	void removeEnemyUnit(const Unit *unit);
	void removeEnemyStructure(const Unit *structure);
	void removeStructure(const Unit *structure);
	void removeUnit(const Unit *unit);
	bool rushDetected();
	void setRallyPoint(Point2D point);
	const Unit *underAttack();

private:
	void activatePrismaticAlignment(const Unit *voidray);
	bool aggressiveBlink(Point2D target);
	void attack();
	void attack(Point2D target);
	float averageUnitDistanceToEnemyBase();
	bool blink(const Unit *unit);
	int calculateEnemyStaticDefence();
	float calculateEnemyStaticDefenceInRadius(Point2D centre);
	std::vector<Point2D> calculateGrid(Point2D centre, int size);
	float calculateSupply(std::set<const Unit *> army);
	float calculateSupply(std::vector<ArmyUnit> army);
	float calculateSupplyAndWorkers(std::set<const Unit *> army);
	float calculateSupplyAndWorkersInRadius(Point2D centre, std::set<const Unit *> army);
	float calculateSupplyInRadius(Point2D centre, std::set<const Unit *> army);
	float calculateSupplyInRadius(Point2D centre, std::vector<ArmyUnit> army);
	bool canAttack();
	bool checkForProxies();
	void checkForZerglingSpeed();
	void darkTemplarHarass();
	void defend(Point2D threatened);
	bool escapeAOE(ArmyUnit armyUnit);
	bool enemyHas(UnitTypeID unitType);
	void feedback();
	Point2D findAttackTarget(const Unit *unit);
	Point2D findNearbyRamp(const Unit *unit);
	void forcefield();
	Point2D getAOETarget(std::set<const Unit *> unitsInRange, float radius);
	const Unit *getClosestBase(const Unit *unit);
	const Unit *getClosestBase(Point2D point);
	const Unit *getClosestEnemy(const Unit *ourUnit);
	const Unit *getClosestEnemy(Point2D point);
	const Unit *getClosestEnemyBase(const Unit *ourUnit);
	const Unit *getClosestEnemyBaseWithoutDetection(const Unit *unit);
	const Unit *getClosestEnemyFlyer(Point2D point);
	Point2D getDefensiveForcefieldTarget(const Unit *sentry);
	std::set<const Unit *> getEnemiesInRange(const Unit *unit, AbilityID spell);
	Point2D getRetreatPoint(const Unit *unit);
	bool hasAttribute(UnitTypeID unitType, Attribute attribute);
	bool includes(UnitTypeID unitType);
	bool inRange(const Unit *attacker, const Unit *target);
	bool inRange(const Unit *attacker, Point2D target);
	bool isUnderHostileSpell(Point2D target);
	bool isUnderPsistorm(Point2D target);
	bool kite(ArmyUnit armyUnit);
	void moveObservers();
	bool outranges(const Unit *attacker, const Unit *target);
	void printDebug();
	void psistorm();
	bool regroup();
	void retreat();
	bool shieldsCritical(const Unit *unit, const Unit *attacker);
	void updateArmyValues();
};

