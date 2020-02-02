# Tornado server for Spool multiplayer twine thingy
# Written by Andi McClure

import tornado.httpserver
import tornado.ioloop
import tornado.options
import tornado.web
from tornado.options import define, options
from tornado.escape import json_encode, json_decode

import traceback
from collections import deque
from random import randint
from time import time

def randomfrom(s):
	return s[randint(0,len(s)-1)]

define("port", default=8888, help="run on the given port", type=int)
define("history", default='history.log', help="history log filename", type=str) #?
define("load", default='', help="database filename", type=str)  #?

define("twine_path", default='twine', help="twine html dir path", type=str)
define("support_path", default='support', help="support files path", type=str)

define("debug", default=0, help="debug runlevel", type=int)
define("debug_delay", default=0, help="debug: fake response delay", type=float)  #?
define("debug_failure", default=0, help="debug: fake request failure", type=int) #?

history_file = None

class MainHandler(tornado.web.RequestHandler):
	def get(self):
		self.write("Nothing is here.")
		
# Library/utility
		
class JsonHandler(tornado.web.RequestHandler):
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
				post_obj = json_decode(post)
				
			response = self.result(post_obj)

			response = json_encode(response)
			
			if options.debug > 0:
				print("RESPONSE: %s" % response)

			if options.debug_delay > 0: # Fake delay to test stuff 
				from time import sleep
				time.sleep(options.debug_delay)

			return response
		except:
			traceback.print_exc()
			return ""
			
	def result(self, obj):
		return None

	def put(self):
		self.write( self.updateFor( self.request.body ) )
		
	def post(self):
		self.write( self.updateFor( self.request.body ) )
		
	def get(self):
		self.write( self.updateFor( None ) )

def initLog():
	lo = None
	return lo

# Game Goes Here

gamedb = {}
roomdb = {}
pianos = deque()
hopes = ["i'm a magical babe with spells like whoa","falling asleep with someone you love","delicious hamburger","tasty ice cream"]
TOO_DANGEROUS = 3
corpse_id_generator = 1

class AthenaHandler(JsonHandler):
	def regRoom(self, id):
		if id not in roomdb:
			roomdb[id] = {"bodies":{},"danger":0}
		return roomdb[id]

	def result(self, obj):
		result = None
		target = obj["target"] if "target" in obj else None
		action = obj["action"] if "action" in obj else None
		
		# These first three are extremely generic. I expect them to be used in the public spool release.
		if action == "send": # User has given us a value to store
			gamedb[target] = obj["value"]
		elif action == "accumulate": # User has given us a value to *add* to a value we've stored
			value = obj["value"] if "value" in obj else 1
			if target in gamedb:
				value += gamedb[target]
			gamedb[target] = value
		elif action == "receive": # User wants to read a value we stored
			result = {"value": (gamedb[target] if target in gamedb else None)}
		
		# These are extremely specific! I expect them to either be deleted when spool is released,
		# or replaced with more generic equivalents.
		elif action == "roomlook": # Give list of bodies/events for current time/place
			if options.debug > 1:
				print(roomdb)
				print(pianos)
				print(hopes)
			if "id" in obj:
				id = obj["id"]
				kind = obj["kind"] if "kind" in obj else None
				after = obj["time"] if "time" in obj else None
				result = {}
				
				if kind == "danger":
					room = self.regRoom(id)
					result["danger"] = room["danger"]
					room["danger"] = room["danger"] + 1
					if room["danger"] > TOO_DANGEROUS:
						room["danger"] = 0
				elif kind == "hope":
					result["hope"] = randomfrom(hopes)
				
				if id in roomdb:
					result["bodies"] = roomdb[id]["bodies"].values()
					
				pianoresult = []
				result["piano"] = pianoresult
				now = time()
				while len(pianos)>0 and now - pianos[0]["time"] > 60:
					print("DESTROYING OLD PIANO")
					pianos.popleft()
				for body in pianos:
					if not after or (body["time"] > after+1): # 1 second silence buffer
						pianoresult.append(body)
		elif action == "corpse":
			global corpse_id_generator
			id = obj["id"]
			self.regRoom(id)
			body = obj["body"]
			if body:
				body["id"] = corpse_id_generator
				corpse_id_generator = corpse_id_generator + 1
				roomdb[id]["bodies"][body["id"]] = body
		elif action == "eatcorpse":
			id = obj["id"]
			if id in roomdb and target is not None:
				bodies = roomdb[id]["bodies"]
				if target in bodies:
					del bodies[target]
		elif action == "piano":
			body = obj["body"]
			if body:
				now = time()
				body["time"] = now
				result = {"time":now}
				pianos.append(body)
		elif action == "hope":
			hope = ""
			if "hope" in obj:
				hope = obj["hope"]
				hope = hope.strip()
			if hope:
				hopes.append(hope)
			print(hopes)
					
		return result

# public static void main

def main():
	tornado.options.parse_command_line()
	if options.debug > 0:
		print("Debug level %s" % (options.debug))
	if options.history:
		global history_file
		global history_dom
		print("Logging to %s" % (options.history))
		history_file = open(options.history, "a")
	if options.load:
		global hopes
		print("Will load from %s" % (options.load))
		with open(options.load, 'r') as f:
			hopes = json_decode(f.read())
		print(hopes)
	
	# Do any extra setup here
	
	application = tornado.web.Application([
		(r"/", MainHandler),
		(r"/(favicon.ico)",  tornado.web.StaticFileHandler, {'path': options.support_path}),
		(r"/twine/(.*)",   tornado.web.StaticFileHandler, {'path': options.twine_path}),
		(r"/support/(.*)", tornado.web.StaticFileHandler, {'path': options.support_path}),
		(r"/data", AthenaHandler),
	])
	http_server = tornado.httpserver.HTTPServer(application)
	http_server.listen(options.port)
	tornado.ioloop.IOLoop.instance().start()


if __name__ == "__main__":
	main()
