from dataclasses import dataclass
import enum
import json
from typing import Any, Dict, NewType, Set, Tuple, TypeAlias, Union


class Race(enum.StrEnum):
    TERRAN = "Terran"
    ZERG = "Zerg"
    PROTOSS = "Protoss"


AbilityId = NewType("AbilityId", int)
UnitId = NewType("UnitId", int)
UpgradeId = NewType("UpgradeId", int)


@dataclass(frozen=True)
class Unit:
    id: UnitId
    name: str
    race: Race
    unit_alias: None | UnitId
    tech_alias: list[UnitId]
    abilities: list[AbilityId]


@dataclass(frozen=True)
class Upgrade:
    id: UpgradeId
    name: str


@dataclass(frozen=True)
class Ability:
    id: AbilityId
    name: str
    remaps_to: None | AbilityId
    trains: None | UnitId
    researches: None | UpgradeId
    morphs: None | UnitId


ABILITIES: Dict[AbilityId, Ability] = {}
UNITS: Dict[UnitId, Unit] = {}
UPGRADES: Dict[UpgradeId, Upgrade] = {}

# which ability produces which unit?
MAP_ABILITY_TO_TRAIN_UNIT: Dict[AbilityId, UnitId] = {}
# inverse of the above: which Ability is needed to create which unit?
MAP_UNIT_TO_CREATION_ABILITY: Dict[UnitId, list[AbilityId]] = {}

# Some abilities remap to one another
MAP_ABILITY_TO_ABILITY_REMAPS: Dict[AbilityId, AbilityId] = {}

# same as with units: which ability researches which upgrade
MAP_ABILITY_TO_UPGRADE: Dict[AbilityId, UpgradeId] = {}
# inverse of above
MAP_UPGRADE_FROM_ABILITY: Dict[UpgradeId, AbilityId] = {}

# set off all ids, that are structures (buildings)
STRUCTURES: Set[UnitId] = set()


def get_morph(ability: Dict[str, Any]) -> UnitId | None:
    if not "target" in ability:
        return None

    t = ability["target"]
    if isinstance(t, str):
        return None

    assert isinstance(t, dict)

    if not "Morph" in t:
        return None

    assert "produces" in t["Morph"]

    id = (int)(t["Morph"]["produces"])

    if id == 0:
        return None

    return UnitId(id)


def get_trains(ability: Dict[str, Any]) -> UnitId | None:
    if not "target" in ability:
        return None

    t = ability["target"]
    if isinstance(t, str):
        return None

    assert isinstance(t, dict)

    if not "Train" in t:
        return None

    assert "produces" in t["Train"]

    id = (int)(t["Train"]["produces"])

    if id == 0:
        return None

    return UnitId(id)


def get_research(ability: Dict[str, Any]) -> UpgradeId | None:
    if not "target" in ability:
        return None

    t = ability["target"]
    if isinstance(t, str):
        return None

    assert isinstance(t, dict)

    if not "Research" in t:
        return None

    assert "upgrade" in t["Research"]

    id = (int)(t["Research"]["upgrade"])
    assert id != 0

    return UpgradeId(id)


def get_remaps_to(ability: Dict[str, Any]) -> AbilityId | None:
    if not "remaps_to_ability_id" in ability:
        return None

    id = (int)(ability["remaps_to_ability_id"])
    assert not id == 0

    return AbilityId(id)


def handle_abilities(abilities: list[Dict[str, Any]]):
    for ability in abilities:
        assert "id" in ability
        assert "name" in ability

        id = ability["id"]
        name = ability["name"]
        remaps = get_remaps_to(ability)
        trains = get_trains(ability)
        morphs = get_morph(ability)
        researches = get_research(ability)

        if trains:
            assert morphs is None
            assert researches is None

        if morphs:
            assert trains is None
            assert researches is None

        if researches:
            assert trains is None
            assert morphs is None

        ab = Ability(id, name, remaps, trains, researches, morphs)
        ABILITIES[id] = ab

        if ab.remaps_to:
            assert ab.id not in MAP_ABILITY_TO_ABILITY_REMAPS
            MAP_ABILITY_TO_ABILITY_REMAPS[ab.id] = ab.remaps_to

        if ab.trains:
            MAP_ABILITY_TO_TRAIN_UNIT[ab.id] = ab.trains

            if ab.trains in MAP_UNIT_TO_CREATION_ABILITY:
                MAP_UNIT_TO_CREATION_ABILITY[ab.trains].append(ab.id)
            else:
                MAP_UNIT_TO_CREATION_ABILITY[ab.trains] = [ab.id]

        if ab.morphs:
            if ab.morphs in MAP_UNIT_TO_CREATION_ABILITY:
                MAP_UNIT_TO_CREATION_ABILITY[ab.morphs].append(ab.id)
            else:
                MAP_UNIT_TO_CREATION_ABILITY[ab.morphs] = [ab.id]

        if ab.researches:
            assert ab.id not in MAP_ABILITY_TO_UPGRADE
            MAP_ABILITY_TO_UPGRADE[ab.id] = ab.researches

            assert ab.researches not in MAP_UPGRADE_FROM_ABILITY
            MAP_UPGRADE_FROM_ABILITY[ab.researches] = ab.id


def handle_unit_abilities(
    abilities: list[Dict[str, Any]],
) -> list[AbilityId]:
    ret: list[AbilityId] = []

    for a in abilities:
        id = (int)(a["ability"])

        ret.append(AbilityId(id))

    return ret


def handle_units(units: list[Dict[str, Any]]):
    for unit in units:
        assert "id" in unit
        assert "name" in unit

        id = (int)(unit["id"])
        name = unit["name"]
        unit_alias = (str)(unit["unit_alias"])

        if unit_alias == "0":
            unit_alias = None
        else:
            unit_alias = UnitId((int)(unit_alias))

        ta_int: list[int] = unit["tech_alias"]

        ta = [UnitId(x) for x in ta_int]

        abilities = handle_unit_abilities(unit["abilities"])

        u = Unit(UnitId(id), name, unit["race"], unit_alias, ta, abilities)

        UNITS[u.id] = u

        if "true" in (str)(unit["is_structure"]).lower():
            STRUCTURES.add(UnitId(id))


def handle_upgrades(upgrades: list[Dict[str, Any]]):
    for up in upgrades:
        id = (int)(up["id"])
        name = up["name"]

        upgrade = Upgrade(UpgradeId(id), name)

        UPGRADES[upgrade.id] = upgrade


def gen_file_header() -> str:

    s = "#pragma once\n//autogenerated: do not edit\n"
    s += """
#include<vector>
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_utils.h"
namespace sc2 {
"""

    return s


def gen_file_footer() -> str:
    return "}\n\n"


# find out if a unit is a structure:
def gen_is_building_function() -> str:
    string = "constexpr inline bool is_building(UNIT_TYPEID id) {\n"

    string += "    switch(id) {\n"

    # TODO: Sort units by race
    for s in STRUCTURES:

        unit = UNITS[s]

        # special cases: ignore these
        unu = unit.name.upper()
        if (
            "RAVENREPAIRDRONE" in unu
            or "ELSECARO" in unu
            or "BYPASSARMORDRONE" in unu
            or "NYDUSCANALATTACKER" in unu
            or "NYDUSCANALCREEPER" in unu
            or "RESOURCEBLOCKER" in unu
        ):
            continue

        assert not unit is None

        idname = "UNIT_TYPEID::" + unit.race.upper() + "_" + unit.name.upper()

        string += f"    case {idname}:\n"

    string += "        return true;\n"

    string += "    default:\n        return false;\n"

    # close bracket switch
    string += "    }\n"

    # close bracket function
    string += "}\n\n"

    string += "inline bool is_building(const sc2::Unit& unit) { return is_building(unit.unit_type.ToType()); }\n"

    return string


# get_production_unit: UnitId -> UnitId
def get_production_unit() -> str:

    string = "inline std::vector<UNIT_TYPEID> get_production_unit(UNIT_TYPEID id) {\n"

    string += "    switch (id) {\n"

    unitcollection: Dict[str, list[str]] = {}

    for unit in UNITS.values():
        if unit.unit_alias or len(unit.tech_alias) > 0:
            # assumption: units with tech aliases will be handled implicitely, once the
            # actual unit (the one the alias refers to) is handled
            # example: a siege tank exists in 2 variants: Sieged and Normal, we dont care about the
            # sieged variant and it has a tech alias set, so we ignore it
            continue

        for unit_ability in unit.abilities:
            ability = ABILITIES[unit_ability]

            if (ability.trains is None) and (ability.morphs is None):
                continue

            produced_unit_id = ability.trains if ability.trains else ability.morphs

            assert produced_unit_id

            produced_unit = UNITS[produced_unit_id]
            assert produced_unit

            lhs = (
                "UNIT_TYPEID::"
                + produced_unit.race.upper()
                + "_"
                + produced_unit.name.upper()
            )

            rhs = "UNIT_TYPEID::" + unit.race.upper() + "_" + unit.name.upper()

            # special cases:
            if "DEFILER" in lhs or "DEFILER" in rhs or "INFESTORTERRANBURROWED" in lhs:
                continue

            # special case 2: apparently the C++ Constant for Burrowed Ravagers is the only one
            # which is missing the ZERG_ prefix
            if "ZERG_RAVAGERBURROWED" in lhs:
                lhs = "UNIT_TYPEID::RAVAGERBURROWED"

            if lhs in unitcollection:
                unitcollection[lhs].append(rhs)
            else:
                unitcollection[lhs] = [rhs]

    for key in unitcollection:
        val = unitcollection[key]

        string += f"    case {key}: return {{"
        vals = ",".join(val)
        string += vals
        string += "};\n"

    string += "    default: return {};\n"

    # closing brace switch
    string += "    }\n"

    # closing brace function
    string += "}\n"

    return string


def get_production_ability() -> str:
    return ""


with open("data.json", "r", encoding="utf-8") as f:
    data = json.load(f)

handle_abilities(data.get("Ability"))

handle_units(data.get("Unit"))

handle_upgrades(data.get("Upgrade"))


with open("s3b/unit_data.h", "w") as f:

    f.write(gen_file_header())
    f.write("\n\n")
    f.flush()

    f.write(gen_is_building_function())
    f.write("\n\n")
    f.flush()

    f.write(get_production_unit())
    f.write("\n\n")
    f.flush()

    f.write(get_production_ability())
    f.write("\n\n")
    f.flush()

    f.write(gen_file_footer())
    f.flush()


# goal:
# produce the following mapping functions in c++ code:

# get ability, which produces unit
# get_production_ability: UnitId -> AbilityId

# get remapped ability (or itself, if no remap)
# get_ability_remap: AbilityId -> AbilityId

# which ability produces which upgrade?
# get_upgrade_from_ability: UpgradeId -> AbilityId

# which unit researches which upgrade
# get_structure_for_research -> UpgradeId -> UnitId


# TODO: Future ideas
# also generate costs (minerals, gas, supply, time) from this data
# can be queried in game, but still would be nice as static table
