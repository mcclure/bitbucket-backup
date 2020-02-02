-- OPEN DOOR

if gm.door_state ~= 0 then return end
gm.door_state = 1
trashScreen("temple_exit_door_shut") -- Fade out 
actInstantAddScreen("temple_exit_door_open", km.roomObj, true)
addScreen("temple_exit_door_door", km.roomObj)