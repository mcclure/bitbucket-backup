gm = {poff = 1}
km = {tglide = -0.02,}

function clamp(a,b,c)
	if b < a then return a end
	if b > c then return c end
	return b
end

function thresher(b)
	shader.thresh:set( clamp(0,b,1) )
end

gm.px1 = shader.px:get()
gm.py1 = shader.py:get()

function sizer(b)
	gm.poff = clamp(1,b,256)
	shader.px:set( gm.poff*gm.px1 )
	shader.py:set( gm.poff*gm.py1 )
end

thresher(1.0)