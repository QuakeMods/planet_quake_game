shard_include( "attackers" )

local SpGetUnitNearestEnemy = Spring.GetUnitNearestEnemy

function IsAttacker(unit)
	for i,name in ipairs(attackerlist) do
		if name == unit:Internal():Name() then
			return true
		end
	end
	return false
end

AttackerBehaviour = class(Behaviour)

function AttackerBehaviour:Init()
	--self.ai.game:SendToConsole("attacker!")
	--self.game:AddMarker({ x = startPosx, y = startPosy, z = startPosz }, "my start position")
	CMD.MOVE_STATE = 50
end

function AttackerBehaviour:Update()
	local unitID = self.unit:Internal().id
	-- Spring.Echo(unitID)
	local myRange = Spring.GetUnitMaxRange(unitID)
	local closestUnit = Spring.GetUnitNearestEnemy(unitID, myRange)
	local allyTeamID = self.ai.allyId
	if Spring.GetGameFrame() % 15 == 4 then
		if closestUnit and (Spring.IsUnitInLos(closestUnit, allyTeamID)) then
			local enemyRange = Spring.GetUnitMaxRange(closestUnit)
			if myRange > enemyRange then
				local ex,ey,ez = Spring.GetUnitPosition(closestUnit)
				local ux,uy,uz = Spring.GetUnitPosition(unitID)
				local pointDis = Spring.GetUnitSeparation(unitID,closestUnit)
				local dis = 120
				local f = dis/pointDis
				if (pointDis+dis > Spring.GetUnitMaxRange(unitID)) then
				  f = (Spring.GetUnitMaxRange(unitID)-pointDis)/pointDis
				end
				local cx = ux+(ux-ex)*f
				local cy = uy
				local cz = uz+(uz-ez)*f
				self.unit:Internal():ExecuteCustomCommand(CMD.MOVE, {cx, cy, cz})
			end
		end
	end
end

function AttackerBehaviour:OwnerBuilt()
	self.ai.attackhandler:AddRecruit(self)
	self.unit:Internal():ExecuteCustomCommand(CMD.MOVE_STATE, { 2 }, {})
	self.attacking = true
	self.active = true
end


function AttackerBehaviour:OwnerDead()
	self.ai.attackhandler:RemoveRecruit(self)
end

function AttackerBehaviour:OwnerIdle()
	self.attacking = true
	self.active = true
	self.ai.attackhandler:AddRecruit(self)
end

function AttackerBehaviour:AttackCell(cell)
	local unit = self.unit:Internal()
	local unitID = unit.id
	local teamID = ai.id
	local r = math.random(0,1000)
	if r == 0 then
		local ec = Spring.GetTeamResources(ai.id, "energy")
		if ec > 100 then
			Spring.GiveOrderToUnit(unitID, 31337, {}, {})
		end
	end
	--attack
	if unitID%60 == Spring.GetGameFrame()%60 then
		local currenthealth = unit:GetHealth()
		local maxhealth = unit:GetMaxHealth()
		local startPosx, startPosy, startPosz = Spring.GetTeamStartPosition(self.ai.id)
		local startBoxMinX, startBoxMinZ, startBoxMaxX, startBoxMaxZ = Spring.GetAllyTeamStartBox(self.ai.allyId)
		local ec, es = Spring.GetTeamResources(ai.id, "energy")
		local closestUnit = Spring.GetUnitNearestEnemy(unitID, 50000, false)
		local ex,ey,ez = Spring.GetUnitPosition(closestUnit)
		local enemyDis = Spring.GetUnitSeparation(unitID,closestUnit)
		if (currenthealth >= maxhealth or currenthealth > 3000) and closestUnit then
			if enemyDis < 1500 then
				Spring.GiveOrderToUnit(unitID, CMD.CLOAK, { 1 }, {})
			else
				Spring.GiveOrderToUnit(unitID, CMD.CLOAK, { 0 }, {})
			end
			
			p = api.Position()
			p.x = ex
			p.z = ez
			p.y = ey
			self.target = p
			self.attacking = true
			self.ai.attackhandler:AddRecruit(self)
			if self.active then
				self.unit:Internal():MoveAndFire(self.target)
			else
				self.unit:ElectBehaviour()
			end
		--retreat
		else
			local nanotcx, nanotcy, nanotcz = GG.GetClosestNanoTC(unitID)
			if enemyDis < 1500 then
				Spring.GiveOrderToUnit(unitID, CMD.CLOAK, { 1 }, {})
			else
				Spring.GiveOrderToUnit(unitID, CMD.CLOAK, { 0 }, {})
			end
			if nanotcx and nanotcy and nanotcz then
				p = api.Position()
				p.x, p.y, p.z = nanotcx, nanotcy, nanotcz
			elseif startBoxMinX == 0 and startBoxMinZ == 0 and startBoxMaxZ == Game.mapSizeZ and startBoxMaxX == Game.mapSizeX then
				p = api.Position()
				p.x = startPosx
				p.z = startPosz
				p.y = 0
			else
				p = api.Position()
				p.x = math.random(startBoxMinX, startBoxMaxX)
				p.z = math.random(startBoxMinZ, startBoxMaxZ)
				p.y = 0
			end
			
			self.target = p
			self.attacking = false
			self.ai.attackhandler:AddRecruit(self)
			if self.active then
				self.unit:Internal():Move(self.target)
			else
				self.unit:ElectBehaviour()
			end
		end
	end
end

function AttackerBehaviour:Priority()
	if not self.attacking then
		return 0
	else
		return 100
	end
end

function AttackerBehaviour:Activate()
	self.active = true
	if self.target then
		self.unit:Internal():MoveAndFire(self.target)
		self.target = nil
		self.ai.attackhandler:AddRecruit(self)
	else
		self.ai.attackhandler:AddRecruit(self)
	end
end


function AttackerBehaviour:OwnerDied()
	self.ai.attackhandler:RemoveRecruit(self)
	self.attacking = nil
	self.active = nil
	self.unit = nil
end