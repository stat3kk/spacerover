#include "Common.h"
#include "BOSSManager.h"

using namespace UAlbertaBot;

// get an instance of this
BOSSManager & BOSSManager::Instance() 
{
	static BOSSManager instance;
	return instance;
}

// constructor
BOSSManager::BOSSManager() 
	: _previousSearchStartFrame(0)
    , _previousSearchFinishFrame(0)
    , _searchInProgress(false)
{
	BOSS::init();
}

void BOSSManager::reset()
{
    _previousSearchResults = BOSS::DFBB_BuildOrderSearchResults();
    _searchInProgress = false;
    _previousSearchStartFrame = 0;
    _previousBuildOrder = std::vector<BOSS::ActionType>();
}

// tell the search to keep going for however long we have this frame
void BOSSManager::update(double timeLimit)
{
    if (isSearchInProgress())
    {
        _smartSearch->setTimeLimit(timeLimit);
    	_smartSearch->search();
        
        if (BWAPI::Broodwar->getFrameCount() > (_previousSearchStartFrame + SEARCH_FRAME_LIMIT))
        {
            _searchInProgress = false;
            _previousSearchFinishFrame = BWAPI::Broodwar->getFrameCount();
            _previousSearchResults = _smartSearch->getResults();
            _previousBuildOrder = _previousSearchResults.buildOrder;

            if (_previousBuildOrder.empty())
            {
                BWAPI::Broodwar->printf("Previous search didn't find a solution, resorting to Naive Build Order");
                _previousBuildOrder = BOSS::Tools::GetNaiveBuildOrder(_smartSearch->getParameters().initialState, _smartSearch->getParameters().goal);
            }
        }
    }
}

// start a new search for a new goal
void BOSSManager::startNewSearch(const std::vector<MetaPair> & goalUnits)
{
    // convert from UAlbertaBot's meta goal type to BOSS ActionType goal
    BOSS::DFBB_BuildOrderSearchGoal goal = getGoal(goalUnits);
    BOSS::GameState initialState(BWAPI::Broodwar, BWAPI::Broodwar->self());

    _smartSearch = SearchPtr(new BOSS::DFBB_BuildOrderSmartSearch(initialState.getRace()));
    _smartSearch->setGoal(getGoal(goalUnits));
    _smartSearch->setState(initialState);

    _searchInProgress = true;
    _previousSearchStartFrame = BWAPI::Broodwar->getFrameCount();
}

BOSS::DFBB_BuildOrderSearchGoal BOSSManager::getGoal(const std::vector<MetaPair> & goalUnits)
{
	BOSS::DFBB_BuildOrderSearchGoal goal(getRace());

	for (size_t i=0; i<goalUnits.size(); ++i)
	{
		goal.setGoal(getActionType(goalUnits[i].first), goalUnits[i].second);
	}

	return goal;
}

// gets the StarcraftState corresponding to the beginning of a Melee game
BOSS::GameState BOSSManager::getStartState()
{
    BOSS::GameState state(getRace());
    state.setStartingState();

	return state;
}

const BOSS::RaceID BOSSManager::getRace() const
{
    BWAPI::Race r = BWAPI::Broodwar->self()->getRace();
    if (r == BWAPI::Races::Protoss)
    {
        return BOSS::Races::Protoss;
    }
    else if (r == BWAPI::Races::Terran)
    {
        return BOSS::Races::Terran;
    }
    else if (r == BWAPI::Races::Zerg)
    {
        return BOSS::Races::Zerg;
    }
    else
    {
        BOSS_ASSERT(false, "We should have had a valid race from BWAPI");
        return BOSS::Races::None;
    }
}

bool BOSSManager::isSearchInProgress()
{
    return _searchInProgress;
}

// converts SearchResults.buildOrder vector into vector of MetaType
std::vector<MetaType> BOSSManager::getMetaVector(const std::vector<BOSS::ActionType> & buildOrder)
{
	std::vector<MetaType> metaVector;
    	
	for (size_t i(0); i<buildOrder.size(); ++i)
	{
		metaVector.push_back(getMetaType(buildOrder[i]));
	}

	return metaVector;
}


std::vector<MetaType> BOSSManager::getBuildOrder()
{
    return getMetaVector(_previousBuildOrder);
}

BOSS::ActionType BOSSManager::getActionType(const MetaType & t)
{
	// set the appropriate type
	if (t.isUnit())
	{
		return BOSS::ActionType(t.unitType);
	}
	else if (t.isUpgrade())
	{
		return BOSS::ActionType(t.upgradeType);
	} 
	else if (t.isTech())
	{
		return BOSS::ActionType(t.techType);
	}
	else
	{
		UAB_ASSERT(false, "Should have found a valid type here");
	}
	
	return BOSS::ActionType();
}

MetaType BOSSManager::getMetaType(const BOSS::ActionType & a)
{
	// set the appropriate type
	if (a.isUnit())
	{
		return MetaType(a.getUnitType());
	}
	else if (a.isUpgrade())
	{
		return MetaType(a.getUpgradeType());
	} 
	else if (a.isTech())
	{
		return MetaType(a.getTechType());
	}
	else
	{
		UAB_ASSERT(false, "Should have found a valid type here");
	}
	
	return MetaType();
}

void BOSSManager::drawSearchInformation(int x, int y)
{
	//starcraftSearchData.drawSearchResults(10, 240);
}

	
		