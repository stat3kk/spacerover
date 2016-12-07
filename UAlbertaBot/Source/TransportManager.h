#pragma once;

#include <Common.h>
#include "MicroManager.h"


namespace UAlbertaBot
{
class MicroManager;

class TransportManager : public MicroManager
{
	std::vector<BWAPI::Position>    _mapEdgeVertices; 
	
	int                             _currentRegionVertexIndex;
	BWAPI::Position					_minCorner;
	BWAPI::Position					_maxCorner;
	std::vector<BWAPI::Position>	_waypoints;
	BWAPI::Position					_to;
	BWAPI::Position					_from;



	void							calculateMapEdgeVertices();
	void							drawTransportInformation(int x, int y);
	void							moveTransport();
	void							moveTroops();
	BWAPI::Position                 getFleePosition(int clockwise = 1);
	void                            followPerimeter(int clockwise = 1);
	void							followPerimeter(BWAPI::Position to, BWAPI::Position from);
	int                             getClosestVertexIndex(BWAPI::UnitInterface * unit);
	int								getClosestVertexIndex(BWAPI::Position p);
	std::pair<int, int>				findSafePath(BWAPI::Position from, BWAPI::Position to);
	
public:

	TransportManager();

	bool							_leftBase;
	bool							_returning;
	BWAPI::UnitInterface*	        _transportShip;

	void							executeMicro(const BWAPI::Unitset & targets);
	void							update();
	void							setTransportShip(BWAPI::UnitInterface * unit);
	void							setFrom(BWAPI::Position from);
	void							setTo(BWAPI::Position to);

	void							clearAll();


	// similar to ranged manager, but this time we want to return a target
	// changes are made here
	BWAPI::Unit						assignTargetsOld(const BWAPI::Unitset & targets);
	BWAPI::Unit						getTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets);
	int								getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target);
	// checks if scarab was shot
	bool							scarabShot(BWAPI::Unit shuttle);
	
	/*
	checks if the reaver is safe from enemy attacks
	0 -> unsafe			1 -> safe and we can hit the enemy		2 -> safe but there are no enemies for us to hit
	*/
	int								isSafe(BWAPI::Unit reaver);

};
}
