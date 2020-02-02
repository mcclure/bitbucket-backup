if oc_down then
	soundEffect2:Play(false)

	wobble_v = wobble_v + 0.2 * ((wobble_v <= 0 or math.abs(wobble_v) < 0.1) and -1 or 1)
end