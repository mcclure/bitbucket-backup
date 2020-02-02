import tornado.httpserver
import tornado.ioloop
import tornado.options
import tornado.web

from collections import deque
from random import random, randint

# Tornado server for drumcircle xml

from tornado.options import define, options

define("port", default=8888, help="run on the given port", type=int)
define("history", default='history.log', help="history log filename", type=str)
define("database", default='database.db', help="database filename", type=str)
define("tanks", default=5, help="number of tanks", type=int)
define("specs", default=10, help="species per tank", type=int)
define("giants", default=0, help="are there giants?", type=int)
define("shelf_after", default=0, help="where is shelf?", type=int)
define("shelf_size", default=-1, help="how big is shelf?", type=int)
define("input", default='', help="initialize state from log", type=str)
define("norun", default=0, help="print and exit", type=int)
define("debug", default=0, help="debug runlevel", type=int)
define("debug_delay", default=0, help="debug: fake response delay", type=float)
define("debug_failure", default=0, help="debug: fake request failure", type=int)

history_file = None
history_dom = None

class MainHandler(tornado.web.RequestHandler):
	def get(self):
		self.write("Nothing is here.")

# levelution 

import traceback
import xml.dom.minidom
import time

AIR="a"
WALL="b"
LAVA="c"

PLAYER=1
EXIT=2

def randomfrom(s):
	return s[randint(0,len(s)-1)]

def randomfromdict(s):
	return s[randomfrom(s.keys())]
	
def top(s):
	ls = len(s)
	if ls > 0:
		return s[ls-1]
	return None

player_db = {}
def player_hits(post_obj):
	if options.shelf_after <= 0 or not post_obj:
		return 0
	key = None
	if post_obj.hasAttribute("pid"):
		key = post_obj["pid"]
	else:
		for node in post_obj.getElementsByTagName("pid"):
			if node.firstChild:
				key = node.firstChild.nodeValue
				break
	
	if not player_db.has_key(key):
		player_db[key] = 0
	player_db[key] += 1
	if options.debug > 3:
		print ("Visit #%d for player '%s'" % (player_db[key], key))
	return player_db[key]

class Board:
	def __init__(self, xdim = 24, ydim = 24, suppress = False):
		self.xdim = xdim
		self.ydim = ydim
		if not suppress:
			self.basic()
			self.noise(10, WALL)
			self.noise(3, LAVA)
			self.px = randint(3,self.xdim-2) # 3 because client spawns player offset
			self.py = self.ydim-2
			self.ex = randint(1,self.xdim-2)
			self.ey = randint(2,4)
		
	def clone(self):
		c = Board(self.xdim,self.ydim, True)
		c.str = list(self.str)
		c.px = self.px
		c.py = self.py
		c.ex = self.ex
		c.ey = self.ey
		return c
	
	def basic(self):
		cap = [WALL] * self.xdim
		mid = [WALL] + [AIR]*(self.xdim-2) + [WALL]
		self.str = cap + mid*(self.ydim-2) + cap
	
	def recombine(self, fwb):
		i = 0
		strmax = len(fwb.str)
		while i < strmax:
			if self.str[i] != AIR or fwb.str[i] != AIR:
				prefer_fwb = random() < 0.5
				if prefer_fwb:
					prefer = fwb.str
				else:
					prefer = self.str
				i2 = 1
				while not (i+i2 >= strmax or (i+i2)%self.xdim == 0 or prefer[i+i2] == AIR):
					i2 += 1
				if prefer_fwb:
					self.str[i:i+i2] = prefer[i:i+i2]
				i += i2
			else:
				i += 1
		return self
	
	def mutate(self):
		self.noise(1, WALL)
		self.noise(1, AIR)
		if random()<0.3:
			self.noise(1,LAVA)
			self.noise(1,AIR)
		return self
	
	# Dust, don't flip
	def noise(self, c, what):
		for i in range(c):
			x = randint(1,self.xdim-2)
			y = randint(1,self.ydim-2)
			e = randint(1,self.xdim/4)
			x -= e/2
			for i2 in range(e):
				x2 = x+i2
				if x2 >= 1 and x2 < self.xdim-1:
					self.set(y,x2,what)
	
	def clear(self):
		self.str = AIR * (self.xdim*self.ydim)
	
	def i(self, x,y):
		return x*self.xdim+y
	
	def get(self, x,y):
		return self.str[self.i(x,y)]
	
	def set(self, x,y,v):
		self.str[self.i(x,y)] = v
		
	def xml(self, doc, r):
		r.setAttribute("map",''.join(self.str))
		r.setAttribute("mapx",str(self.xdim))
		r.setAttribute("mapy",str(self.ydim))
		e = doc.createElement("e")
		def add(doc, e, x, y, c):
			n = doc.createElement("nil")
			n.setAttribute("x", str(x))
			n.setAttribute("y", str(y))
			n.setAttribute("c", str(c))
			e.appendChild(n)
		add(doc,e,self.px,self.py,"1")
		add(doc,e,self.ex,self.ey,"2")
		r.appendChild(e)
		return r
	
class Spec:
	def __init__(self, gid, suppress = False, size=None):
		self.gid = gid
		self.sid = -1
		self.checked = False
		self.won = False
		if not suppress:
			if not size:
				self.board = Board()
			else:
				self.board = Board(size,size)
		
	def clone(self):
		c = Spec(self.gid, True)
		c.board = self.board.clone()
		return c
		
	def mutate(self):
		self.board.mutate()
		return self
		
	def recombine(self, fwb):
		self.board.recombine(fwb.board)
		return self
		
	def xml(self, doc, r):
		r.setAttribute("gid", str(self.gid))
		r.setAttribute("sid", str(self.sid))
		self.board.xml(doc,r)
		
	def archive_xml(self, doc, r):
		slo = history_dom.createElement("spawn")
		slo.setAttribute("sid",str(self.sid))
		self.board.xml(doc,slo)
		r.appendChild(slo)
		
	def report(self, post_obj):
		self.checked = True
		if post_obj.hasAttribute("won") and int(post_obj.getAttribute("won")):
			self.won = True
			self.deaths = int(post_obj.getAttribute("deaths"))
			self.lastf = int(post_obj.getAttribute("lastf"))
		if history_file:
			history_file.write(post_obj.toxml())
			history_file.write("\n")
	
class Genome:
	def __init__(self, _id, suppress = False):
		self.id = _id
		self.specs = {} # Unordered
		self.specs_stack = [] # Ordered
		size = None
		if options.giants and _id == options.tanks:
			size = 32
		if options.giants and _id == options.tanks-1:
			size = 16
		if not size:
			basic = Spec(self.id)
		else:
			basic = Spec(self.id, False, size)
		self.issue_next = 0
		self.generation_count = 1
		self.id_generator = 0
		if not suppress:
			for i in range(options.specs):
				nid = self.nextid()
				s = basic.clone().mutate()
				s.sid = nid
				self.specs[nid] = s
				self.specs_stack.append(s)
			self.last_valid = self.specs_stack[0]
			self.specs_stack.reverse()

	def nextid(self):
		self.id_generator += 1
		return self.id_generator

	def any(self):
		if len(self.specs_stack) == 0:
			old_specs_list = self.specs.values()
			new_specs_list = old_specs_list # Do a funny dance so we can recover if there's an exception.
			new_specs = self.specs
			try:
				def fitness(x,y): # resort by these criteria
					if not (x.checked or y.checked):
						return 0
					if not (x.won or y.won):
						return 0
					if not x.checked or not x.won:
						return 1
					if not y.checked or not y.won:
						return -1
					if x.deaths != y.deaths:
						return cmp(x.deaths, y.deaths)
					return cmp(x.lastf, y.lastf) # Last ditch
				sorted_old_specs_list = sorted(old_specs_list, cmp=fitness)
				if sorted_old_specs_list[0].checked and sorted_old_specs_list[0].won:
					winner1 = sorted_old_specs_list[0]
					winner2 = sorted_old_specs_list[1]
				else:
					winner1 = self.last_valid
					winner2 = randomfrom(sorted_old_specs_list)
				candidate_specs_list = []
				candidate_specs = {}
				if options.debug > 1:
					print("Recombining genome %d. Winning specs: %d, %d" % (self.id, winner1.sid, winner2.sid))
				for old_spec in old_specs_list:
					if old_spec.checked:
						s = winner1.clone().recombine(winner2).mutate()
						s.sid = self.nextid()
					else:
						s = old_spec
						if options.debug > 2:
							print("Preserving spec %d" % (s.sid))
					candidate_specs_list.append(s)
					candidate_specs[s.sid] = s
				new_specs_list = candidate_specs_list
				self.generation_count = self.generation_count + 1
				
				if history_file:
					lo = history_dom.createElement("breed")
					lo.setAttribute("gid", str(self.id))
					lo.setAttribute("gc", str(self.generation_count))
					lo.setAttribute("winner1", str(winner1.sid))
					lo.setAttribute("winner2", str(winner2.sid))
					for s in candidate_specs_list:
						s.archive_xml(history_dom, lo)
					history_file.write(lo.toxml())
					history_file.write("\n")
					history_file.flush()
					
				candidate_specs_list.reverse()
				new_specs_list = candidate_specs_list
				new_specs = candidate_specs
				self.last_valid = winner1
			finally:
				self.specs_stack = new_specs_list
				self.specs = new_specs
		
		return self.specs_stack.pop()
	
	def report(self, post_obj):
		sid = int(post_obj.getAttribute("sid"))
		self.specs[sid].report(post_obj)

class Manager:
	def __init__(self, input=None):
		self.tanks = {}
		self.alltanks = {}
		if not input: # GENERATE RANDOM
			for _i in range(options.tanks):
				i = _i + 1
				if options.debug > 0:
					print("Generating tank %d" % i)
				self.alltanks[i] = Genome(i)
				if options.shelf_size <= 0 or _i < options.shelf_size:
					self.tanks[i] = self.alltanks[i]
		else: # GENERATE FROM XML SOURCE
			node = input.firstChild # Iter commands
			while node:
				if node.nodeName == "init":
					if options.debug > 1:
						print("Loading: INIT")
					self.alltanks = {}
					tank = node.firstChild # Iter tanks
					while tank:
						if not tank.nodeName == "tank":
							continue
						self.unarchiveTank(tank)
						tank = tank.nextSibling
				elif node.nodeName == "twhy":
					if options.debug > 1:
						print("Loading: TWHY")
					self.report(node)
					gid = int(node.getAttribute("gid"))
					sid = int(node.getAttribute("sid"))
					if self.alltanks.has_key(gid):
						t = self.alltanks[gid]
						s = top(t.specs_stack)
						if s and s.sid == sid:
							if options.debug > 1:
								print("\tLoading: Popping gid %d, sid %d" % (gid, sid))
							s.checked = True # In case report botched.
							t.specs_stack.pop()
					else:
						print("BOTCHED TWHY gid: %d" % gid)
					
				elif node.nodeName == "breed":
					if options.debug > 1:
						print("Loading: BREED")
					self.unarchiveTank(node)
				node = node.nextSibling
			
			self.tanks = {}
			for i,t in self.alltanks.iteritems():
				if options.shelf_size <= 0 or i-1 < options.shelf_size:
					self.tanks[i] = t
	
	def unarchiveTank(self, tank):
		gid = int(tank.getAttribute("gid"))
		if options.debug > 1:
			print("\tLoading: Create tank %d" % gid)
		t = Genome(gid, True)
		t.generation_count = int(tank.getAttribute("gc"))
		spawn = tank.firstChild # Iter spawns
		while spawn:
			s = Spec(gid, True)
			sid = int(spawn.getAttribute("sid"))
			s.sid = sid
			if options.debug > 1:
				print("\t\tLoading: Create spec %d" % sid)
			s.board = Board(int(spawn.getAttribute("mapx")), int(spawn.getAttribute("mapy")), True)
			s.board.str = spawn.getAttribute("map")
			e = spawn.getElementsByTagName("e")[0].firstChild # Iter entities
			while e:
				c = int(e.getAttribute("c"))
				x = int(e.getAttribute("x"))
				y = int(e.getAttribute("y"))
				if c == 1:
					s.board.px = x
					s.board.py = y
				elif c == 2:
					s.board.ex = x
					s.board.ey = y
				e = e.nextSibling
			t.specs[sid] = s
			t.specs_stack.append(s)
			spawn = spawn.nextSibling
		t.id_generator = max(t.id_generator, sid)
		t.last_valid = t.specs_stack[0]
		t.specs_stack.reverse()
		self.alltanks[gid] = t
	
	def any(self, post_obj):
		if player_hits(post_obj) > options.shelf_after:
			return randomfromdict(self.alltanks)
		else:
			return randomfromdict(self.tanks)
		
	def report(self, post_obj):
		for a in ["gid","sid","deaths","lastf"]:
			if not post_obj.hasAttribute(a):
				return
		try:
			gid = int(post_obj.getAttribute("gid"))
			self.alltanks[gid].report(post_obj)
		except (ValueError, IndexError, KeyError):
			if options.debug > 0:
				print("BOTCHED REPORT")

class LuaHandler(tornado.web.RequestHandler):
	def result(self, doc, post_obj):
		return doc.createElement("nil")

	def updateFor(self,post):
#		t = time.time()
		if options.debug_failure:
			if random()<1.0/options.debug_failure:
				return ""
		try:
			post_obj = None
			if (post):
				if options.debug > 0:
					print("POST: %s" % post)
				dom = xml.dom.minidom.parseString(post)
				document = dom.documentElement
				post_obj = document
				
			output = xml.dom.minidom.Document()
			dc = self.result(output, post_obj)
			output.appendChild(dc)
			returnstring = output.toxml()

			response = returnstring
			
			if options.debug > 0:
				print(response)

			if options.debug_delay > 0: # Fake delay to test stuff 
				from time import sleep
				time.sleep(options.debug_delay)

			return response
		except:
			traceback.print_exc()
			return ""

	def put(self):
		self.write( self.updateFor( self.request.body ) )
		
	def get(self):
		self.write( self.updateFor( None ) )

class WelcomeHandler(LuaHandler):
	def result(self, doc, post_obj):
		r = doc.createElement("welcome")
		r.setAttribute("message","This game is an internetwide distributedevolutionary algorithm created with the goal of making the hardest platformer\nlevels possible. You will play a series of levels; in each, you must reach the\nblue dot. Those levels which prove most difficult will be allowed to breed and\ncreate new levels for future players.\n\nThere is no win condition. The purpose\nof this game is not to be won. The\npurpose of this game is to get\nprogressively better at killing you.\n\nControls: arrow keys, s to suicide")
		host = self.request.headers['Host']
		um = doc.createElement("um")
		um.setAttribute("level", "http://%s/level" % host)
		r.appendChild(um)
		return r
		
class LevelHandler(LuaHandler):
	def result(self, doc, post_obj):
		if post_obj:
			m.report(post_obj)
		r = doc.createElement("level")
		s = m.any(post_obj)
		s.any().xml(doc, r)
		return r

class UnUpdateHandler(tornado.web.RequestHandler):
	def get(self):
		self.write("""<dc>
	<board>
		<set>
			<c p="201" x="-0.240000" y="0.000000" i="1056209845" />
		</set>
	</board>
</dc>
""")

def initLog():
	lo = history_dom.createElement("init")
	for gid, g in m.tanks.iteritems():
		lot = history_dom.createElement("tank")
		lot.setAttribute("gid", str(gid))
		lot.setAttribute("gc", str(g.generation_count))
		for sid,s in g.specs.iteritems():
			s.archive_xml(history_dom, lot)
		lo.appendChild(lot)
	return lo

# After the fact

def main():
	tornado.options.parse_command_line()
	if options.debug > 0:
		print("Debug level %s" % (options.debug))
	if options.history:
		global history_file
		global history_dom
		print("Logging to %s" % (options.history))
		history_file = open(options.history, AIR)
		history_dom = xml.dom.minidom.Document()
	if options.database:
		global database
		print("Will save to %s" % (options.database))
		
	input = None
	temp_history = None
	if options.input:
		print("Loading from %s" % (options.input))
		with open(options.input) as f:
			input_raw = f.read()
			input_raw = ("<wrap>%s</wrap>" % input_raw)
			input = xml.dom.minidom.parseString(input_raw)
			if input:
				input = input.firstChild # wrap	
			temp_history = history_file # suppress history while loading
			history_file = None
		
	global m
	m = Manager(input)
	
	if options.norun:
		print("BY REQUEST, WON'T RUN")
		if input:
			lo = initLog()
			print(lo.toxml())
		return
	
	if history_file: # initial logging
		lo = initLog()
		history_file.write(lo.toxml())
		history_file.write("\n")
		history_file.flush()
		lo = None
	
	if temp_history:
		history_file = temp_history
	
	application = tornado.web.Application([
		(r"/", MainHandler),
		(r"/welcome", WelcomeHandler),
		(r"/level", LevelHandler),
	])
	http_server = tornado.httpserver.HTTPServer(application)
	http_server.listen(options.port)
	tornado.ioloop.IOLoop.instance().start()


if __name__ == "__main__":
	main()
