# Tornado server for Spool multiplayer twine thingy
# (c) 2013 Andi McClure, http://runhello.com
# You may redistribute, reincorporate, relicense, do anything with this file
# as long as the above copyright notice comment is preserved.

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
define("history", default='history.log', help="history log filename", type=str) # effectively unused
define("load", default='', help="database filename", type=str)  # effectively unused

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

class SpoolHandler(JsonHandler):
	def result(self, obj):
		result = None
		target = obj["target"] if "target" in obj else None
		action = obj["action"] if "action" in obj else None
		
		# Extremely generic commands
		if action == "send": # User has given us a value to store
			gamedb[target] = obj["value"]
		elif action == "accumulate": # User has given us a value to *add* to a value we've stored
			value = obj["value"] if "value" in obj else 1
			if target in gamedb:
				value += gamedb[target]
			gamedb[target] = value
		elif action == "receive": # User wants to read a value we stored
			result = {"value": (gamedb[target] if target in gamedb else None)}
		
		return result

# public static void main

def main():
	tornado.options.parse_command_line()
	if options.debug > 0:
		print("Debug level %s" % (options.debug))
	if options.history:
		global history_file
		print("Logging to %s" % (options.history))
		history_file = open(options.history, "a")
	if options.load:
		print("Will load from %s" % (options.load))
		with open(options.load, 'r') as f:
			pass
	
	# Do any extra setup here
	
	application = tornado.web.Application([
		(r"/", MainHandler),
		(r"/(favicon.ico)",  tornado.web.StaticFileHandler, {'path': options.support_path}),
		(r"/twine/(.*)",   tornado.web.StaticFileHandler, {'path': options.twine_path}),
		(r"/support/(.*)", tornado.web.StaticFileHandler, {'path': options.support_path}),
		(r"/data", SpoolHandler),
	])
	http_server = tornado.httpserver.HTTPServer(application)
	http_server.listen(options.port)
	tornado.ioloop.IOLoop.instance().start()


if __name__ == "__main__":
	main()
