To compile:
  import into visual studio and compile
  
Modules/files changed and/or added

Jacky Chung:
  
  CombatCommander.cpp (with Brandon Williams)
    added the function "updateReaverDropSquads"
    
  TransportManager
    updateFrame() -> added checks to make sure we have loaded a unit
    moveTransport(), moveTroops() -> logic to retreat and when to drop unit
    copied over assignTargetsOld(), getTarget(), getAttackPriority() from RangedManager with minor changes
    scarabShot() -> checks if we have shot a scarab
    isSafe() -> checks if the reaver is safe from enemies
   
Jimmy Ho:  
  
  InformationManager.cpp
    added check for rushes based off the enemy number of units with function "enemyIsRushing()"
    enemyIsRushing() -> the function that checks if enemy is rushing
  
  ProductionManager.cpp
    added caps and checks for buildings/units
    update() -> checks for nexus, cybernetics core in case they were destroyed by a rush
             -> checks for enemy rushing and queues up cannons if it determines there is a rush
             -> caps probes at 20 with 1 nexus, 30 with 2
             -> caps number of gateways to 4
             -> will manually expand if there are 20 probes and it is past a certain time
    
