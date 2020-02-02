-- CLOSE DOOR

if gm.door_state ~= 1 then return end
gm.door_state = 0
trashScreen("temple_exit_door_door")
actRemoveScreen("temple_exit_door_open")
actExpedite()
addScreen("temple_exit_door_shut", km.roomObj)