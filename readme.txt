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
   
