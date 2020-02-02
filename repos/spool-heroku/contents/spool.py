import os
import functools
from flask import Flask, request, jsonify, send_from_directory

# A basic twinespool implementation. Either execute this script as your server,
# or from spool import * in your server and add custom actions with @makeHandler

app = Flask(__name__, static_folder='support')

app.debug = True

def asJson(f):
	@functools.wraps(f)
	def result():
		post_obj = None
		if request.method == 'POST':
			post_obj = request.get_json(force=True) # Force?

		result = f(post_obj)
		
		return jsonify(**result)
	return result

handlers = {}

def makeHandler(action):
	def functionEater(fn):
		global handlers
		handlers[action] = fn
		return fn
	return functionEater
	
@makeHandler("send")
def doSend(obj):
	target = obj["target"]
	gamedb[target] = obj["value"]

@makeHandler("accumulate")	
def doAccumulate(obj):
	target = obj["target"]
	value = obj["value"] if "value" in obj else 1
	if target in gamedb:
		value += gamedb[target]
	gamedb[target] = value

@makeHandler("receive")
def doReceive(obj):
	target = obj["target"]
	return {"value": (gamedb[target] if target in gamedb else None)}

# Page construction goes here

gamedb = {}

@app.route('/')
def testHtml():
	return send_from_directory("story", "test.html")

@app.route('/data', methods=['GET', 'POST'])
@asJson
def query(obj):
	result = None
	action = obj["action"] if obj and "action" in obj else None
	
	if action in handlers:
		result = handlers[action](obj)
		
	if result is None:
		result = {}
	
	return result

@app.route('/robots.txt')
@app.route('/favicon.ico')
def static_from_root():
    return send_from_directory(app.static_folder, request.path[1:])
