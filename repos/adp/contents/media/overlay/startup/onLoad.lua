resetPtrLookup()
memory_setup()
if not did_seed then
	math.randomseed( use_seed or os.time() )
	did_seed = true
end