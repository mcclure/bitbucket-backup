-- Ensure audio loaded

if not m_ambient then
	m_ambient = Sound("media/liz_ambient.ogg")
	m_ambient:Play(true)
end
if not m_moving then m_moving = Sound("media/liz_moving.ogg") end
if not m_panic then m_panic = Sound("media/liz_panic.ogg") end
if not s_uhit then s_uhit = Sound("media/liz_gunhit.ogg") end
if not s_umiss then s_umiss = Sound("media/liz_gunmiss.ogg") end
if not s_ehit then s_ehit = Sound("media/liz_enemyhit.ogg") end
if not s_land then s_land = Sound("media/jdrums23.ogg") end