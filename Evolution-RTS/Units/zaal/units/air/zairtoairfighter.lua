-- UNITDEF -- zairtoairfighter --
--------------------------------------------------------------------------------

unitName = "zairtoairfighter"

--------------------------------------------------------------------------------

isUpgraded = [[0]]

humanName = [[Corruptor]]

objectName = "zaal/zairtoairfighter.s3o"
script = "zaal/zairtoairfighter.cob"


tech = [[tech2]]
armortype = [[light]]
supply = [[2]]

VFS.Include("units-configs-basedefs/basedefs/zaal/zairtoairfighter_basedef.lua")

unitDef.weaponDefs = weaponDefs

--------------------------------------------------------------------------------

return lowerkeys({ [unitName] = unitDef })

--------------------------------------------------------------------------------
