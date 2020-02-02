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
#define("giants", default=0, help="how many giants?", type=int)
define("shelf_after", default=0, help="where is shelf?", type=int)
define("shelf_size", default=0, help="how big is shelf?", type=int)
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

class Board:
	def __init__(self, xdim = 24, ydim = 24, suppress = False):
		self.xdim = xdim
		self.ydim = ydim
		if not suppress:
			self.basic()
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
	def __init__(self, gid, suppress = False):
		self.gid = gid
		self.sid = -1
		self.checked = False
		self.won = False
		if not suppress:
			self.board = Board()
		
	def clone(self):
		c = Spec(self.gid, True)
		c.board = self.board.clone()
		return c

	def xml(self, doc, r):
		r.setAttribute("gid", str(self.gid))
		r.setAttribute("sid", str(self.sid))
		self.board.xml(doc,r)
		
	def archive_xml(self, doc, r):
		slo = history_dom.createElement("spawn")
		slo.setAttribute("sid",str(self.sid))
		self.board.xml(doc,slo)
		r.appendChild(slo)
		
class Genome:
	def __init__(self, _id):
		self.id = _id
		self.specs = {} # Unordered
		self.specs_stack = [] # Ordered
		self.basic = Spec(self.id)

	def any(self):
		return self.basic

class Manager:
	def __init__(self, input=None):
		self.basic = Genome(0)
	
	def any(self, post_obj):
		return self.basic

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
		r.setAttribute("message","\n\n\n        **** SERVER OFFLINE ****\n\n\n\nSorry, the level server is currently\noffline while I fix something.\n\nYou can start the game, but you'll only get an empty room.\n\nPlease try back later.")
		host = self.request.headers['Host']
		um = doc.createElement("um")
		um.setAttribute("level", "http://%s/level" % host)
		r.appendChild(um)
		return r
		
class LevelHandler(LuaHandler):
	def result(self, doc, post_obj):
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
	if options.input:
		print("Loading from %s" % (options.input))
		with open(options.input) as f:
			input_raw = f.read()
			input_raw = ("<wrap>%s</wrap>" % input_raw)
			input = xml.dom.minidom.parseString(input_raw)
			if input:
				input = input.firstChild # wrap			
		
	global m
	m = Manager(input)
	
	if options.norun:
		print("BY REQUEST, WON'T RUN")
		return
		
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
