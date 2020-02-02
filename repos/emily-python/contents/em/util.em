# Misc helper functions/classes
# (Most of this should probably be in the stdlib)

profile experimental

# Array utilities

export lastFrom = function(a)
	a (a.length - 1)

export popLeft = function(a)
	let left = a 0
	let idx = 0
	while (idx < a.length - 1)
		a idx = a (idx + 1)
		idx = idx + 1
	a.pop
	left

export appendLeft = function(a, item)
	let idx = a.length
	if (idx == 0)
		a.append item
	else
		idx = idx - 1
		a.append(a idx)
		while (idx > 0)
			let idx2 = idx - 1
			a idx = a idx2
			idx = idx2
		a 0 = item

export appendArray = function (a, b)
	let i = b.iter
	while (i.more)
		a.append (i.next)

export cloneArray = function(a)
	let result = array()
	appendArray(result, a)
	result

export catArray = function (a, b)
	let result = array()
	appendArray(result, a)
	appendArray(result, b)
	result

export catArrayElement = function(a,b)
	let result = cloneArray(a)
	result.append b
	result

# Linked list / stack object
export Linked = inherit Object
	field value = null
	field next = null # This looks like an iterator but is immutable. Is this bad
	method more = this.next != null

export cloneLinked = function(list)
	if (list)
		let head = new Linked
		let node = head
		while (list)
			let next = new Linked(list.value)
			node.next = next
			node = next      # Move copy list forward
			list = list.next # Move to-copy list forward
		head.next
	else
		null

export cmp = function (x, y)
	if (x < y)
		~1
	elif (x > y)
		1
	else
		0

export insertLinked = function(cmp, list, value)
	let worse = function(node)
		!node || 0 > cmp value (node.value)
	let insert = new Linked(value)
	if (worse list)
		insert.next = list
		insert
	else
		let node = list
		let done = false
		while (!done && node)
			if (worse (node.next))
				insert.next = node.next
				node.next = insert
				done = true
			node = node.next
		list

export foldl = function(default, f, ary)
	let i = ary.iter
	if (!i.more)
		default
	else
		let value = i.next
		while (i.more)
			value = f(value, i.next)
		value

export foldr = function(default, f, ary)
	let i = ary.reverseIter
	if (!i.more)
		default
	else
		let value = i.next
		while (i.more)
			value = f(i.next, value)
		value

export map = function(f, ary)
	let i = ary.iter
	let result = array()
	while (i.more)
		result.append
			f(i.next)
	result

export iterCount = function(i)
	let result = 0
	while (i.more)
		result = result + 1
		i.next
	result

export with2 = function(f, a, b) (f b a)

export checkErrors = function(errors)
	if (0 < errors.length)
		let i = errors.iter
		stderr.println "Compilation failed:"
		while (i.more)
			let error = i.next
			stderr.println
				nullJoin array (error.loc, ": ", error.msg)
		exit 1

export nonempty = function (ary)
	if ary (ary.length)

# Iter

let MapIterImpl = inherit Object
	field fn = null
	field i = null
	field cache = null

	method hasCache = this.cache && this.cache.length > 0

	method pump = do
		while (!this.hasCache && this.i.more)
			this.cache = this.fn(this.i.next)

	method more = do
		this.pump
		this.hasCache

	method next = do
		this.pump
		popLeft (this.cache)

# fn returns an array of values or null. i is an iterator
export mapIter = function(fn, i)
	new MapIterImpl(fn, i)

export foldIter = function(fn, default, i)
	if (!i.more)
		default
	else
		let v = i.next
		while (i.more)
			v = fn(v, i.next)
		v

# Dict

export cloneDict = function(dict)
	let result = new Dict
	let i = dict.iter
	while (i.more)
		let key = i.next
		result.set (key) (dict.get key)
	result

# String ops

export join = function(joiner, ary)
	if (ary.length == 1)
		ary(0).toString
	else
		foldl "" function (x,y) ( \+( \+(x.toString, joiner), y.toString) ) ary
export nullJoin = join ""
export startsWith = function(x, y)
	let idx = 0
	let valid = y.length <= x.length # Don't bother if x is shorter
	while (valid && idx < y.length)   # Iterate until difference found
		if (x idx != y idx)
			valid = false
		idx = idx + 1
	valid

export quotedString = function(s)
	let result = "\""
	let i = s.iter
	while (i.more)
		result = result + do
			let ch = i.next
			with (ch) match
				"\n" = "\\n"
				"\r" = "\\r"
				"\"" = "\\\""
				_ = ch
	result = result + "\""
	result

# Chained dictionary
# Variant of dict: when not found redirects to parent object, then returns not found object

export chainParent = inherit Object
export chainNotFound = inherit Object
export ChainedDict = inherit Object
	field method dict = new Dict

	method get = function(index)
		if (this.dict.has index)
			this.dict.get index
		elif (this.dict.has chainParent)
			(this.dict.get chainParent).get index
		else
			chainNotFound
	method set = this.dict.set
	method has = function(index)
		this.dict.has index || (this.dict.has chainParent && (this.dict.get chainParent).has index)

	method shallowHas = this.dict.has
	method shallowIter = mapIter
		function (x)
			if (x != chainParent)
				array(x)
			else
				null
		this.dict.iter

	# TODO: iter