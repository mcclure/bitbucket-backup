#!/usr/bin/env python
#
# Copyright 2007 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.	
# You may obtain a copy of the License at
#
#	 http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import webapp2
import json
import logging

# Constants
xb = 32
yb = 32

from google.appengine.ext import ndb

class Board(ndb.Model):
	"""Models an individual whiteboard-- last user and board grid."""
	cookie_generator = ndb.IntegerProperty(indexed=False)
	last_dirty_cookie = ndb.IntegerProperty(indexed=False)
	data = ndb.BlobProperty(indexed=False)

	@classmethod
	def query_book(cls, ancestor_key):
		return cls.query(ancestor=ancestor_key).order(-cls.date)

class SetupHandler(webapp2.RequestHandler):
	def get(self):
		board = Board(id="g",
				cookie_generator=1,
                last_dirty_cookie=1,
                data=("1"*(xb*yb)))
		board.put()
		self.response.write( "Success" )

class MainHandler(webapp2.RequestHandler):
	def post(self):
		board = ndb.Key("Board", "g").get()
		
		obj = json.loads(self.request.body)
		ret = {}
		want_board = False
#		logging.info("BEFORE gen %s dirty %s cookie %s" % (board.cookie_generator, board.last_dirty_cookie, "cookie" in obj and obj["cookie"] or "null"))
		if "k" in obj and "v" in obj and "cookie" in obj:
			if board.last_dirty_cookie > obj["cookie"]:
				want_board = True
			board.cookie_generator = board.cookie_generator + 1
			board.last_dirty_cookie = board.cookie_generator
			
			idx = int(obj["k"])
			data = board.data
			board.data = data[:idx] + str(obj["v"]) + data[idx+1:]
			board.put()
		elif "cookie" not in obj:
			board.cookie_generator = board.cookie_generator + 1
			want_board = True
			board.put()
			
		if want_board:
			ret['board'] = board.data
#			logging.info("SENT BOARD")

#		logging.info("AFTER gen %s dirty %s cookie %s" % (board.cookie_generator, board.last_dirty_cookie, "cookie" in obj and obj["cookie"] or "null"))
		
		ret['cookie'] = board.cookie_generator
		self.response.write( json.dumps(ret, separators=(',',':')) )

app = webapp2.WSGIApplication([
	('/board', MainHandler),
	('/deploy', SetupHandler)
], debug=True)