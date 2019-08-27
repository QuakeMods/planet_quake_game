AttackHandler = class(Module)

function AttackHandler:Name()
	return "AttackHandler"
end

function AttackHandler:internalName()
	return "attackhandler"
end

function AttackHandler:Init()
	self.recruits = {attackers = {}, defenders = {}}
	self.counter = 1
	self.ratio = 10
end

function AttackHandler:Update()
	local frame = Spring.GetGameFrame()
	if frame%30 == 0 then -- Refresh commander position
		local x,y,z
		local comms = Spring.GetTeamUnitsByDefs(self.ai.id, {UnitDefNames.ecommanderbattleai.id})
		if comms[1] then
			x,y,z = Spring.GetUnitPosition(comms[1])
			self.commpos = {x = x, y = y, z = z}
		end
	end
	if frame%3000 == 0 then -- Generate random mexpositions
		local positions = self:PickRandomPositionsOnMap()
		local targets = {}
		for ct, position in pairs(positions) do
			targets[ct] = self.ai.metalspothandler:ClosestEnemySpot(self.game:GetTypeByName("eorb") , {x = position[1], y = Spring.GetGroundHeight(position[1], position[2]),z = position[2]} )
		end
		self.targetMexes = targets
	end
	
end

function AttackHandler:GetAggressiveness(atkbehaviour)
	return (math.random(2, 5))
end

function AttackHandler:GetRole(atkbehaviour)
	return ("attacker")
end

function AttackHandler:PickRandomPositionsOnMap()
	local pos = {{math.random(0,Game.mapSizeX), math.random(0,Game.mapSizeZ)},{math.random(0,Game.mapSizeX), math.random(0,Game.mapSizeZ)},{math.random(0,Game.mapSizeX), math.random(0,Game.mapSizeZ)},{math.random(0,Game.mapSizeX), math.random(0,Game.mapSizeZ)},{math.random(0,Game.mapSizeX), math.random(0,Game.mapSizeZ)}}
	return pos
end

function AttackHandler:UnitDead(engineunit)

end

function AttackHandler:DoTargetting()
end

function AttackHandler:DoTargettingOld()

end

function AttackHandler:IsRecruit(attkbehaviour)

end

function AttackHandler:AddRecruit(attkbehaviour)

end

function AttackHandler:RemoveRecruit(attkbehaviour)

end

-- How much is this unit worth?
-- 
-- Idea: add a table with hardcoded values,
-- and use said values if a units found in
-- that table to highlight strategic value
function AttackHandler:ScoreUnit(unit)
	local value = 1
	--[[
	if unit:CanMove() then
		if unit:CanBuild() then
			value = value + 1
		end
	else
		value = value + 2
		if unit:CanBuild() then
			value = value + 2
		end
	end
	--]]
	return value
end
