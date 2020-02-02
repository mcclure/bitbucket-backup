if mouseDownAt then delete(mouseDownAt) mouseDownAt = nil end
if mouseAt then delete(mouseAt) mouseAt = nil end
if cameraRot then delete(cameraRot) cameraRot = nil end

if music then music:Stop() end
if steps then
	if steps[1] then music:Stop() end
	if steps[2] then music:Stop() end
end

killDos()

memory_teardown()