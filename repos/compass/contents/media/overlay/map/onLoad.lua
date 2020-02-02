-- Game map

-- Remember: LEFT UP RIGHT DOWN

local divider = {"The cubicle divider feels like cloth."}

km.map = {
	start = {
		goto={ STAR="realstart_b4" },
	},
	
	realstart_b4 = {
		"You are sitting in a chair.",
		goto={
			{"The cubicle wall is slightly damp."},
			{"You poke listlessly at the keys on the computer in front of you. After a moment you realize you are not pressing them in any actual order."},
			"hallway_c4",
			{"You lean back in your chair."},
		},
	},
	
	hallway_c4 = {
		"You step out of your cubicle. Another, identical cubicle is across a short walkway.",
		goto={
			"realstart_b4",
			"hallway_c3",
			{"The coworker whose cubicle sits across from you is hunched over his laptop, typing intently. He doesn't notice you standing there. You leave him alone."},
			{"Through the window you see trees and foot-worn paths through grass. A flock of birds lights from one tree, lands in another."},
		},
	},
	
	hallway_c3 = {
		"You work in a small room divided into four cubicles, with a break area at one side. At one time this might have been a single person's office, since converted; it's hard to say.",
		goto={
			"hallway_a3",
			"hallway_c2",
			"deadend_d3",
			"hallway_c4",
		},
	},
	
	deadend_d3 = {
		"You stand in front of a office supply cabinet.",
		goto={
			"hallway_c3",
			divider,
			{"You swing open the little metal door. The cabinet is well stocked, probably because it is full of highlighters and little staple-clamps and generally speaking goods fit for a paper office of a kind that hasn't existed for twenty years. You note, as you usually do, that they have not given you any cutting implements."},
			divider,
		},
	},

}