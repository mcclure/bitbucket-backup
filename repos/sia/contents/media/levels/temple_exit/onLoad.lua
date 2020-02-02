pull(km, {roomObj = "temple_exit"})
pull(gm, {door_state = 0})
addScreen("temple_exit", km.roomObj)
addScreen("temple_exit_door_shut", km.roomObj)