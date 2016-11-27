#include "TransportManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;


TransportManager::TransportManager() :
	_transportShip(NULL)
	, _currentRegionVertexIndex(-1)
	, _minCorner(-1,-1)
	, _maxCorner(-1,-1)
	, _to(-1,-1)
	, _from(-1,-1)
	, _leftBase(false)
	, _returning(false)
{
	
	

}

void TransportManager::executeMicro(const BWAPI::Unitset & targets) 
{
	// I assume this is the units in the drop squad. Nope this just finds the shuttle
	const BWAPI::Unitset & transportUnits = getUnits();

	if (transportUnits.empty())
	{
		
		return;
	}	
}

void TransportManager::calculateMapEdgeVertices()
{
	BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());

	if (!enemyBaseLocation)
	{
		return;
	}

	const BWAPI::Position basePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	const std::vector<BWAPI::TilePosition> & closestTobase = MapTools::Instance().getClosestTilesTo(basePosition);

	std::set<BWAPI::Position> unsortedVertices;

	int minX = std::numeric_limits<int>::max(); int minY = minX;
	int maxX = std::numeric_limits<int>::min(); int maxY = maxX;

	//compute mins and maxs
	for(auto &tile : closestTobase)
	{
		if (tile.x > maxX) maxX = tile.x;
		else if (tile.x < minX) minX = tile.x;

		if (tile.y > maxY) maxY = tile.y;
		else if (tile.y < minY) minY = tile.y;
	}

	_minCorner = BWAPI::Position(minX, minY) * 32 + BWAPI::Position(16, 16);
	_maxCorner = BWAPI::Position(maxX, maxY) * 32 + BWAPI::Position(16, 16);

	//add all(some) edge tiles! 
	for (int _x = minX; _x <= maxX; _x += 5)
	{
		unsortedVertices.insert(BWAPI::Position(_x, minY) * 32 + BWAPI::Position(16, 16));
		unsortedVertices.insert(BWAPI::Position(_x, maxY) * 32 + BWAPI::Position(16, 16));
	}

	for (int _y = minY; _y <= maxY; _y += 5)
	{
		unsortedVertices.insert(BWAPI::Position(minX, _y) * 32 + BWAPI::Position(16, 16));
		unsortedVertices.insert(BWAPI::Position(maxX, _y) * 32 + BWAPI::Position(16, 16));
	}

	std::vector<BWAPI::Position> sortedVertices;
	BWAPI::Position current = *unsortedVertices.begin();

	_mapEdgeVertices.push_back(current);
	unsortedVertices.erase(current);

	// while we still have unsorted vertices left, find the closest one remaining to current
	while (!unsortedVertices.empty())
	{
		double bestDist = 1000000;
		BWAPI::Position bestPos;

		for (const BWAPI::Position & pos : unsortedVertices)
		{
			double dist = pos.getDistance(current);

			if (dist < bestDist)
			{
				bestDist = dist;
				bestPos = pos;
			}
		}

		current = bestPos;
		sortedVertices.push_back(bestPos);
		unsortedVertices.erase(bestPos);
	}
    
	_mapEdgeVertices = sortedVertices;
}

void TransportManager::drawTransportInformation(int x = 0, int y = 0)
{
	if (!Config::Debug::DrawUnitTargetInfo)
	{
		return;
	}

	if (x && y)
	{
		//BWAPI::Broodwar->drawTextScreen(x, y, "ScoutInfo: %s", _scoutStatus.c_str());
		//BWAPI::Broodwar->drawTextScreen(x, y + 10, "GasSteal: %s", _gasStealStatus.c_str());
	}
	for (size_t i(0); i < _mapEdgeVertices.size(); ++i)
	{
		BWAPI::Broodwar->drawCircleMap(_mapEdgeVertices[i], 4, BWAPI::Colors::Green, false);
		BWAPI::Broodwar->drawTextMap(_mapEdgeVertices[i], "%d", i);
	}
}

void TransportManager::update()
{
    
	const BWAPI::Unitset & dropUnits = getUnits();
	int transportSpotsRemaining = 8;
	int unitSpace = 0;
	
	// size is either 0 or 1 ... not going to touch this
	if (!_transportShip && getUnits().size() > 0)
    {
        _transportShip = *getUnits().begin();
    }
	
	/*
	this is specifically for when we are in our base
	if we are not returning to our base and we still need to load more units
	*/
	if (_transportShip && (_transportShip->getSpaceRemaining() > 0) && !_returning && !_leftBase)
	{
		return; 
	}

	
	// calculate enemy region vertices if we haven't yet
	if (_mapEdgeVertices.empty())
	{
		calculateMapEdgeVertices();
	}

	moveTroops();

	/* if we want to return but we haven't loaded the reaver */
	if (_transportShip && _returning && _transportShip->getSpaceRemaining() > 5){
		return;
	}

	moveTransport();
	
	drawTransportInformation();
}

void TransportManager::moveTransport()
{
	if (!_transportShip || !_transportShip->exists() || !(_transportShip->getHitPoints() > 0))
	{
		return;
	}

	// If I didn't finish unloading the troops, wait
	/* 
	  (currentCommad = Unload) && (there are units on the shuttle) && (we can unload here)
	*/
	BWAPI::UnitCommand currentCommand(_transportShip->getLastCommand());
	if ( 
		(currentCommand.getType() == BWAPI::UnitCommandTypes::Unload_All || currentCommand.getType() == BWAPI::UnitCommandTypes::Unload_All_Position)
		&& ( _transportShip->getLoadedUnits().size() > 0 )
		&& _transportShip->canUnloadAtPosition(_transportShip->getPosition()) 
		)
	{
		/*
		BWAPI::Broodwar->printf("Can unload here? %d", _transportShip->canUnloadAtPosition(_transportShip->getPosition()));
		BWAPI::Broodwar->printf("%d--%d--%d", (currentCommand.getType() == BWAPI::UnitCommandTypes::Unload_All)
			, (currentCommand.getType() == BWAPI::UnitCommandTypes::Unload_All_Position)
			, (_transportShip->getLoadedUnits().size() > 0) 
			);
		*/

		// if we are safe and enemies are in range of our attacks so we wait until we finish unloading (isSafe == 1)
		// for zealots, the (isSafe == 0) condition might occur as we base isSafe off the transportShip
		if (isSafe(_transportShip) < 2) {
			return;
		}
		// otherwise we keep going moving the shuttle
	}
	
	// we have left the base
	_leftBase = true;

		/*we want to retreat*/ 
	if (_returning)
	{
		// ditch the zealots
		for (auto & unit : _transportShip->getLoadedUnits()) 
		{
			if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot) 
			{
				_transportShip->unload(unit);
			}
		}
		/*make sure we have picked up the reaver*/ 
		if (_transportShip->getSpaceRemaining() > 5)
		{
			return;
		}
		else
		{
			// retreat
			followPerimeter(-1);
		}
	}
	/*not retreating*/
	else 
	{
		// go towards enemy base
		if (_transportShip->getSpaceRemaining() < 5)
		{
			followPerimeter(1);
		}
		else
		{
			// might want to move this back to combat commander?
			// moves the shuttle to the reaver if we are currently using the reaver to attack
			for (auto & reaver : _transportShip->getUnitsInRadius(BWAPI::WeaponTypes::Scarab.maxRange() * 4, BWAPI::Filter::IsAlly))
			{
				if (reaver->getType() == BWAPI::UnitTypes::Protoss_Reaver)
				{
					_transportShip->move(reaver->getPosition());
				}
			}
		}

	}
	
}


void TransportManager::moveTroops()
{
	if (!_transportShip || !_transportShip->exists() || !(_transportShip->getHitPoints() > 0))
	{
		return;
	}

	//unload zealots and reavers if close enough or dying
	int transportHP = _transportShip->getHitPoints() + _transportShip->getShields();
	
	// this should not be needed PLS REMOVE
	BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());

	int weaponRange = BWAPI::WeaponTypes::Scarab.maxRange();
	int radiusOfShuttle = weaponRange; // this can be changed to determine the radius at which we find our enemy units

	// why use radiusOfShuttle here??? we don't need to. We can totally search the all the enemy units if we want?
	BWAPI::Unitset unitsInRange = _transportShip->getUnitsInRadius(radiusOfShuttle * 3, BWAPI::Filter::IsEnemy);
	BWAPI::Unit target = assignTargetsOld(unitsInRange);
	
	/*
	  checks if target is within weapon range of reaver (might want to add a buffer)
	  or if we are low on hp.
	  original UAlbertaBot code conflicts due to its EnemyBaseLocation < 300 and our isSafe == 2 (combat commander)
	*/
	if 
		(
		  ( (target != NULL) && (_transportShip->getDistance(target) < weaponRange) ) ||
		  ( (transportHP < 100) && _transportShip->canUnloadAtPosition(_transportShip->getPosition()) )
		)
	{

		// get the unit's current command
		BWAPI::UnitCommand currentCommand(_transportShip->getLastCommand());

		// if we've already told this unit to unload, wait
		if (currentCommand.getType() == BWAPI::UnitCommandTypes::Unload_All || currentCommand.getType() == BWAPI::UnitCommandTypes::Unload_All_Position)
		{
			return;
		}

		// attacking enemy base
		if (!_returning && _transportShip->getSpaceRemaining() < 5) 
		{
			_transportShip->unloadAll();
			// dude your operator overload is the same as using no parameters.
			// i spent like a whole hour figuring this was causing a bug...
			//_transportShip->unloadAll(_transportShip->getPosition());
		}
	
	}
	
}

// ... how does this even work? what if i want it to go back to my base????
void TransportManager::followPerimeter(int clockwise)
{
	BWAPI::Position goTo = getFleePosition(clockwise);
	
	// theres probably extra conditional checks done here that we don't need
	// we want to return and we have not up the shuttle

	if (Config::Debug::DrawUnitTargetInfo)
	{
		BWAPI::Broodwar->drawCircleMap(goTo, 5, BWAPI::Colors::Brown, true);
	}

	Micro::SmartMove(_transportShip, goTo);
}

// you don't work do you????
void TransportManager::followPerimeter(BWAPI::Position to, BWAPI::Position from)
{
	static int following = 0;
	if (following)
	{
		followPerimeter(following);
		return;
	}

	//assume we're near FROM! 
	if (_transportShip->getDistance(from) < 50 && _waypoints.empty())
	{
		//compute waypoints
		std::pair<int, int> wpIDX = findSafePath(to, from);
		bool valid = (wpIDX.first > -1 && wpIDX.second > -1);
		UAB_ASSERT_WARNING(valid, "waypoints not valid! (transport mgr)");
		_waypoints.push_back(_mapEdgeVertices[wpIDX.first]);
		_waypoints.push_back(_mapEdgeVertices[wpIDX.second]);

		BWAPI::Broodwar->printf("WAYPOINTS: [%d] - [%d]", wpIDX.first, wpIDX.second);

		Micro::SmartMove(_transportShip, _waypoints[0]);
	}
	else if (_waypoints.size() > 1 && _transportShip->getDistance(_waypoints[0]) < 100)
	{
		BWAPI::Broodwar->printf("FOLLOW PERIMETER TO SECOND WAYPOINT!");
		//follow perimeter to second waypoint! 
		//clockwise or counterclockwise? 
		int closestPolygonIndex = getClosestVertexIndex(_transportShip);
		UAB_ASSERT_WARNING(closestPolygonIndex != -1, "Couldn't find a closest vertex");

		if (_mapEdgeVertices[(closestPolygonIndex + 1) % _mapEdgeVertices.size()].getApproxDistance(_waypoints[1]) <
			_mapEdgeVertices[(closestPolygonIndex - 1) % _mapEdgeVertices.size()].getApproxDistance(_waypoints[1]))
		{
			BWAPI::Broodwar->printf("FOLLOW clockwise");
			following = 1;
			followPerimeter(following);
		}
		else
		{
			BWAPI::Broodwar->printf("FOLLOW counter clockwise");
			following = -1;
			followPerimeter(following);
		}

	}
	else if (_waypoints.size() > 1 && _transportShip->getDistance(_waypoints[1]) < 50)
	{	
		//if close to second waypoint, go to destination!
		following = 0;
		Micro::SmartMove(_transportShip, to);
	}

}


int TransportManager::getClosestVertexIndex(BWAPI::UnitInterface * unit)
{
	int closestIndex = -1;
	double closestDistance = 10000000;

	for (size_t i(0); i < _mapEdgeVertices.size(); ++i)
	{
		double dist = unit->getDistance(_mapEdgeVertices[i]);
		if (dist < closestDistance)
		{
			closestDistance = dist;
			closestIndex = i;
		}
	}

	return closestIndex;
}

int TransportManager::getClosestVertexIndex(BWAPI::Position p)
{
	int closestIndex = -1;
	double closestDistance = 10000000;

	for (size_t i(0); i < _mapEdgeVertices.size(); ++i)
	{
		
		double dist = p.getApproxDistance(_mapEdgeVertices[i]);
		if (dist < closestDistance)
		{
			closestDistance = dist;
			closestIndex = i;
		}
	}

	return closestIndex;
}

std::pair<int,int> TransportManager::findSafePath(BWAPI::Position to, BWAPI::Position from)
{
	BWAPI::Broodwar->printf("FROM: [%d,%d]",from.x, from.y);
	BWAPI::Broodwar->printf("TO: [%d,%d]", to.x, to.y);


	//closest map edge point to destination
	int endPolygonIndex = getClosestVertexIndex(to);
	//BWAPI::Broodwar->printf("end indx: [%d]", endPolygonIndex);

	UAB_ASSERT_WARNING(endPolygonIndex != -1, "Couldn't find a closest vertex");
	BWAPI::Position enemyEdge = _mapEdgeVertices[endPolygonIndex];

	BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	BWAPI::Position enemyPosition = enemyBaseLocation->getPosition();

	//find the projections on the 4 edges
	UAB_ASSERT_WARNING((_minCorner.isValid() && _maxCorner.isValid()), "Map corners should have been set! (transport mgr)");
	std::vector<BWAPI::Position> p;
	p.push_back(BWAPI::Position(from.x, _minCorner.y));
	p.push_back(BWAPI::Position(from.x, _maxCorner.y));
	p.push_back(BWAPI::Position(_minCorner.x, from.y));
	p.push_back(BWAPI::Position(_maxCorner.x, from.y));

	//for (auto _p : p)
		//BWAPI::Broodwar->printf("p: [%d,%d]", _p.x, _p.y);

	int d1 = p[0].getApproxDistance(enemyPosition);
	int d2 = p[1].getApproxDistance(enemyPosition);
	int d3 = p[2].getApproxDistance(enemyPosition);
	int d4 = p[3].getApproxDistance(enemyPosition);

	//have to choose the two points that are not max or min (the sides!)
	int maxIndex = (d1 > d2 ? d1 : d2) > (d3 > d4 ? d3 : d4) ?
						  (d1 > d2 ? 0 : 1) : (d3 > d4 ? 2 : 3);
	
	

	int minIndex = (d1 < d2 ? d1 : d2) < (d3 < d4 ? d3 : d4) ?
						   (d1 < d2 ? 0 : 1) : (d3 < d4 ? 2 : 3);


	if (maxIndex > minIndex)
	{
		p.erase(p.begin() + maxIndex);
		p.erase(p.begin() + minIndex);
	}
	else
	{
		p.erase(p.begin() + minIndex);
		p.erase(p.begin() + maxIndex);
	}

	//BWAPI::Broodwar->printf("new p: [%d,%d] [%d,%d]", p[0].x, p[0].y, p[1].x, p[1].y);

	//get the one that works best from the two.
	BWAPI::Position waypoint = (enemyEdge.getApproxDistance(p[0]) < enemyEdge.getApproxDistance(p[1])) ? p[0] : p[1];

	int startPolygonIndex = getClosestVertexIndex(waypoint);

	return std::pair<int, int>(startPolygonIndex, endPolygonIndex);

}

BWAPI::Position TransportManager::getFleePosition(int clockwise)
{
	UAB_ASSERT_WARNING(!_mapEdgeVertices.empty(), "We should have a transport route!");

	// if this is the first flee, we will not have a previous perimeter index
	if (_currentRegionVertexIndex == -1)
	{
		// so return the closest position in the polygon
		int closestPolygonIndex = getClosestVertexIndex(_transportShip);

		UAB_ASSERT_WARNING(closestPolygonIndex != -1, "Couldn't find a closest vertex");

		if (closestPolygonIndex == -1)
		{
			return BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
		}
		else
		{
			// set the current index so we know how to iterate if we are still fleeing later
			_currentRegionVertexIndex = closestPolygonIndex;
			return _mapEdgeVertices[closestPolygonIndex];
		}
	}
	// if we are still fleeing from the previous frame, get the next location if we are close enough
	else
	{
		double distanceFromCurrentVertex = _mapEdgeVertices[_currentRegionVertexIndex].getDistance(_transportShip->getPosition());

		// keep going to the next vertex in the perimeter until we get to one we're far enough from to issue another move command
		while (distanceFromCurrentVertex < 128*2)
		{
			// change made here
			_currentRegionVertexIndex = (_currentRegionVertexIndex + clockwise*1 ) % _mapEdgeVertices.size();

			distanceFromCurrentVertex = _mapEdgeVertices[_currentRegionVertexIndex].getDistance(_transportShip->getPosition());
		}
		
		return _mapEdgeVertices[_currentRegionVertexIndex];
	}

}

void TransportManager::setTransportShip(BWAPI::UnitInterface * unit)
{
	_transportShip = unit;
}

void TransportManager::setFrom(BWAPI::Position from)
{
	if (from.isValid())
		_from = from;
}
void TransportManager::setTo(BWAPI::Position to)
{
	if (to.isValid())
		_to = to;
}


BWAPI::Unit TransportManager::assignTargetsOld(const BWAPI::Unitset & targets)
{
	// do i need this tbh?
	// i can just use _transportShip can't I?
	// i think so
	//const BWAPI::Unitset & rangedUnits = getUnits();

	// figure out targets
	BWAPI::Unitset reaverUnitTargets;
	std::copy_if(targets.begin(), targets.end(), std::inserter(reaverUnitTargets, reaverUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });

	// if there are targets
	if (!reaverUnitTargets.empty())
	{
		// find the best target for the reaver (so the shuttle can drop)
		BWAPI::Unit target = getTarget(_transportShip, reaverUnitTargets);

		if (target && Config::Debug::DrawUnitTargetInfo)
		{
			BWAPI::Broodwar->drawLineMap(_transportShip->getPosition(), _transportShip->getTargetPosition(), BWAPI::Colors::Purple);
		}

		return target;
		// if there are no targets
	}
	else
	{
		return NULL;
	}
	
}

BWAPI::Unit TransportManager::getTarget(BWAPI::Unit shuttleUnit, const BWAPI::Unitset & targets)
{
	int bestPriorityDistance = 1000000;
	int bestPriority = 0;

	double bestLTD = 0;

	int highPriority = 0;
	double closestDist = std::numeric_limits<double>::infinity();
	BWAPI::Unit closestTarget = nullptr;

	for (const auto & target : targets)
	{
		double distance = shuttleUnit->getDistance(target);
		double LTD = UnitUtil::CalculateLTD(target, shuttleUnit);
		int priority = getAttackPriority(shuttleUnit, target);
		// Do we need this? because are we really checking the threat of the enemy unit to the shuttle?
		// or is it the threat of the enemy unit to the reaver???
		bool targetIsThreat = LTD > 0;

		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist))
		{
			closestDist = distance;
			highPriority = priority;
			closestTarget = target;
		}
	}

	return closestTarget;
} 

// get the attack priority of a type in relation to a zergling??
// do we really want to target enemy workers instead? I don't think so
int TransportManager::getAttackPriority(BWAPI::Unit reaverUnit, BWAPI::Unit target)
{
	BWAPI::UnitType rangedType = reaverUnit->getType();
	BWAPI::UnitType targetType = target->getType();

	bool isThreat = rangedType.isFlyer() ? targetType.airWeapon() != BWAPI::WeaponTypes::None : targetType.groundWeapon() != BWAPI::WeaponTypes::None;

	if (target->getType().isWorker())
	{
		isThreat = false;
	}

	if (target->getType() == BWAPI::UnitTypes::Zerg_Larva || target->getType() == BWAPI::UnitTypes::Zerg_Egg)
	{
		return 0;
	}



	// if the target is building something near our base something is fishy
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	if (target->getType().isWorker() && (target->isConstructing() || target->isRepairing()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 100;
	}

	if (target->getType().isBuilding() && (target->isCompleted() || target->isBeingConstructed()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 90;
	}

	// highest priority is something that can attack us or aid in combat
	if (targetType == BWAPI::UnitTypes::Terran_Bunker || isThreat)
	{
		return 11;
	}
	// next priority is worker
	else if (targetType.isWorker())
	{
		return 11;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Zerg_Spawning_Pool)
	{
		return 5;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Protoss_Pylon)
	{
		return 5;
	}
	// next is buildings that cost gas
	else if (targetType.gasPrice() > 0)
	{
		return 4;
	}
	else if (targetType.mineralPrice() > 0)
	{
		return 3;
	}
	// then everything else
	else
	{
		return 1;
	}
}

// does it even need an input param? no
bool TransportManager::scarabShot(BWAPI::Unit shuttle)
{
	
	for (auto & unit : shuttle->getUnitsInRadius(BWAPI::WeaponTypes::Scarab.maxRange(), BWAPI::Filter::IsAlly))
	{
		if (unit->getType() == BWAPI::UnitTypes::Protoss_Scarab)
		{
			// a scarab was shot
			return true;
		}
	}

	/*a scarab was not shot*/
	return false;

}



/*
  checks if the reaver is safe from enemy attacks
  return values:
  0 -> unsafe
  1 -> safe and we can hit the enemy
  2 -> safe but there are no enemies for us to hit
*/
int TransportManager::isSafe(BWAPI::Unit reaver)
{
	// 128 is the scarabs weapon range, need to play around with then numbers
	BWAPI::Unitset enemies, targets; 

	targets = reaver->getUnitsInRadius(BWAPI::WeaponTypes::Scarab.maxRange(), BWAPI::Filter::IsEnemy);
	// check scarabs here or nah?
	// not loaded reaver yet
	// nothing within range and we aren't retreating atm, return false as that will make the shuttle pick it up again
	if ((targets.size() == 0) && _transportShip->getSpaceRemaining() > 5) {
		return 2;
	}
	else
	{
		enemies = reaver->getUnitsInRadius(BWAPI::WeaponTypes::Scarab.maxRange() * 2, BWAPI::Filter::IsEnemy);

	}

	for (auto & enemy : enemies)
	{
		// check if reaver can immediatley be attacked by enemy
		if (enemy->isInWeaponRange(reaver))
		{
			return false;
		}

	}

	return true;

}