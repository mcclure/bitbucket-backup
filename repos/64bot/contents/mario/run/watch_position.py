# Continuously print Mario's position to the console.

load("mario/basics")

class WatchPosition(Runnable):
    def onBlank(self):
        trace(mario_at())

result(WatchPosition())
