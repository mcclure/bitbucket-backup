import tornado.httpserver
import tornado.ioloop
import tornado.options
import tornado.web

from collections import deque
from random import random

# Tornado server for drumcircle xml

from tornado.options import define, options

define("port", default=8888, help="run on the given port", type=int)
define("history", default='', help="history log filename", type=str)
define("debug", default=0, help="debug runlevel", type=int)
define("debug_delay", default=0, help="debug: fake response delay", type=float)
define("debug_failure", default=0, help="debug: fake request failure", type=int)

history_file = None

class MainHandler(tornado.web.RequestHandler):
	def get(self):
		self.write("Nothing is here.")

# Drumcircle 

import traceback
import xml.dom.minidom
import time

class Corpse:
	timeout = 10 # Corpses decay after 60 seconds
	def __init__(self, id, died):
		self.died = died
		self.id = id

class Board:
	def __init__(self):
		self.board = {}
		self.t = 0
		self.dead = deque() # Add recently dead to left, pop from right
		
b = Board()
tempo = "0.4"
tempo_updated = 0

class Chip:
	def __init__(self, i):
		self.i = i
		self.t = 0
		self.x = 0
		self.y = 0
		self.p = None
		self.s = None
	
	def sync(self, t, x, y, p, s):
		self.t = t
		self.x = x
		self.y = y
		self.p = p
		self.s = s

class UpdateHandler(tornado.web.RequestHandler):
	def updateFor(self,post):
		lastt = 0
		t = time.time()
		tempo_just_updated = False
		if options.debug_failure:
			if random()<1.0/options.debug_failure:
				return ""
		try:
			if (post):
				dom = xml.dom.minidom.parseString(post)
				document = dom.documentElement
				board = document.firstChild
				updated = {} # Could be set?
				try:
					lastt = float(document.attributes['last'].value)
				except:
					pass
				lastboardt = b.t
				if options.debug > 2:
					print("t %s lastt %s lastboardt %s" % (t, lastt, lastboardt))
				just_updated = False
				while board:
					if board.tagName == 'board':
						if board.hasAttribute('tempo'):
							global tempo, tempo_updated
							tempo = board.attributes['tempo'].value
							just_updated = True
							tempo_updated = t
							tempo_just_updated = True
						action = board.firstChild
						while action:
							chip = action.firstChild
							while chip:
								if chip.tagName == 'c':
									i = chip.attributes['i'].value
									
									if action.tagName == 'set':
										x = chip.attributes['x'].value
										y = chip.attributes['y'].value
										p = chip.attributes['p'].value if chip.hasAttribute('p') else None
										s = chip.attributes['s'].value if chip.hasAttribute('s') else None

										updated[i] = None
										just_updated = True
										if i in b.board:
											schip = b.board[i]
											if options.debug > 2:
												print("SET UPDATE " + str(i))
										else:
											schip = Chip(i)
											b.board[i] = schip
											if options.debug > 2:
												print("SET NEW " + str(i))
										schip.sync(t, x, y, p, s)
									if action.tagName == 'del':
										if options.debug > 2:
											print("SET DELETE " + str(i))
										updated[i] = None
										just_updated = True
										if i in b.board:
											del b.board[i]
										b.dead.appendleft(Corpse(i, t)) # t will be "time of death"
								chip = chip.nextSibling
							action = action.nextSibling
					board = board.nextSibling
				if just_updated:
					b.t = t
					if history_file:
						history_file.write(post)
						history_file.write("\n")
						history_file.flush()
						
			for i, chip in b.board.iteritems(): # DELETE ME!
				if options.debug > 2:
					print("%s: i %s x %s y %s p %s s %s" % (i, chip.i, chip.x, chip.y, chip.p, chip.s))
			
			output = xml.dom.minidom.Document()
			dc = output.createElement("dc")
			dc.setAttribute("time", str(t))
			dc.setAttribute("delay", str(0))
			output.appendChild(dc)
			if not post or lastt < lastboardt: # FIXME: We can trigger this check by doing an update?
				outset = None
				outdel = None
				outtempo = tempo_updated > lastt and not tempo_just_updated
				for i, chip in b.board.iteritems():
					if (not post) or (i not in updated and chip.t > lastt):
						if options.debug > 2:
							print("Send update %s" % i)
						if not outset:
							outset = output.createElement("set")
						outchip = output.createElement("c")
						outchip.setAttribute("i", str(chip.i))
						outchip.setAttribute("x", str(chip.x))
						outchip.setAttribute("y", str(chip.y))
						if chip.p:
							outchip.setAttribute("p", str(chip.p))
						if chip.s:
							outchip.setAttribute("s", str(chip.s))
						outset.appendChild(outchip)
				if post: # If the remote side cares about deaths
					for corpse in b.dead.__iter__():
						if not corpse.died > lastt:
							break
						if corpse.id in updated:
							continue
						if not outdel:
							outdel = output.createElement("del")
						outchip = output.createElement("c")
						outchip.setAttribute("i", corpse.id)
						if options.debug > 2:
							print("NOTIFY %s" % (corpse.id))
						outdel.appendChild(outchip)
					# Don't keep around corpses more than like a minute old
					while len(b.dead) and t - b.dead[-1].died > Corpse.timeout:
						if options.debug > 2:
							print("DECAY %s" % (b.dead[-1].id))
						b.dead.pop()
				if outset or outdel or outtempo:
					outboard = output.createElement("board")
					if outtempo:
						outboard.setAttribute("tempo", str(tempo))
					if outset:
						outboard.appendChild(outset)
					if outdel:
						outboard.appendChild(outdel)
					dc.appendChild(outboard)
			returnstring = output.toxml()

			response = returnstring 

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

class UnUpdateHandler(tornado.web.RequestHandler):
	def get(self):
		self.write("""<dc>
	<board>
		<set>
			<c p="201" x="-0.240000" y="0.000000" i="1056209845" />
			<c p="201" x="-0.275532" y="0.665193" i="208099763" />
			<c p="201" x="0.000000" y="0.720000" i="861523593" />
			<c p="201" x="0.480000" y="0.000000" i="67903136" />
			<c p="201" x="0.339411" y="-0.339411" i="866807731" />
			<c p="101" x="0.000000" y="0.480000" i="551332811" />
			<c p="101" x="-0.000000" y="-0.480000" i="1558151612" />
			<c p="51" x="0.000000" y="0.240000" i="1870676120" />
			<c p="51" x="-0.000000" y="-0.720000" i="372425184" />
			<c p="51" x="1.450000" y="-0.773333" i="1660434716" />
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
		print("Logging to %s" % (options.history))
		history_file = open(options.history, "a")
	application = tornado.web.Application([
		(r"/", MainHandler),
		(r"/update", UpdateHandler),
	])
	http_server = tornado.httpserver.HTTPServer(application)
	http_server.listen(options.port)
	tornado.ioloop.IOLoop.instance().start()


if __name__ == "__main__":
	main()
