-- UNITDEF -- ZHIVE --
--------------------------------------------------------------------------------

unitName = [[zhive]]

--------------------------------------------------------------------------------

builder = true

metalMultiplier = 2
buildSpeedModifier = 2

buildCostMetal = 800
energyUse = 0
energyMake = 0

metalStorage = 200
energyStorage = 200

supplygranted = 40

footprintX = 8
footprintZ = 8

yardMap = [[oooooooo oooooooo oooooooo oooooooo oooooooo oooooooo oooooooo oooooooo]]

primaryCEG = "custom:fusionreactionnuclear-4color"

humanName = [[Zaal Hive]]

objectName = [[zaal/zhive.s3o]]
script = [[zaal/zhive.cob]]

techprovided = [[tech3, tech2, tech1, tech0]]
armortype = [[building]]

VFS.Include("units-configs-basedefs/basedefs/zaal/zhive_basedef.lua")

unitDef.weaponDefs = weaponDefs
--------------------------------------------------------------------------------

return lowerkeys({ [unitName]    = unitDef })

--------------------------------------------------------------------------------