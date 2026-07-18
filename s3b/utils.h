#pragma once

#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_client.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_utils.h"

namespace sc2 {



	inline bool is_building(UNIT_TYPEID type_) {
		switch (type_) {
			// Terran
		case UNIT_TYPEID::TERRAN_ARMORY:
		case UNIT_TYPEID::TERRAN_BARRACKS:
		case UNIT_TYPEID::TERRAN_BARRACKSFLYING:
		case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
		case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
		case UNIT_TYPEID::TERRAN_BUNKER:
		case UNIT_TYPEID::TERRAN_COMMANDCENTER:
		case UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING:
		case UNIT_TYPEID::TERRAN_ENGINEERINGBAY:
		case UNIT_TYPEID::TERRAN_FACTORY:
		case UNIT_TYPEID::TERRAN_FACTORYFLYING:
		case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
		case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
		case UNIT_TYPEID::TERRAN_FUSIONCORE:
		case UNIT_TYPEID::TERRAN_GHOSTACADEMY:
		case UNIT_TYPEID::TERRAN_MISSILETURRET:
		case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
		case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING:
		case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
		case UNIT_TYPEID::TERRAN_REFINERY:
		case UNIT_TYPEID::TERRAN_SENSORTOWER:
		case UNIT_TYPEID::TERRAN_STARPORT:
		case UNIT_TYPEID::TERRAN_STARPORTFLYING:
		case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
		case UNIT_TYPEID::TERRAN_STARPORTTECHLAB:
		case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
		case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
		case UNIT_TYPEID::TERRAN_REACTOR:
		case UNIT_TYPEID::TERRAN_TECHLAB:

			// Zerg
		case UNIT_TYPEID::ZERG_BANELINGNEST:
		case UNIT_TYPEID::ZERG_CREEPTUMOR:
		case UNIT_TYPEID::ZERG_CREEPTUMORBURROWED:
		case UNIT_TYPEID::ZERG_CREEPTUMORQUEEN:
		case UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER:
		case UNIT_TYPEID::ZERG_EXTRACTOR:
		case UNIT_TYPEID::ZERG_GREATERSPIRE:
		case UNIT_TYPEID::ZERG_HATCHERY:
		case UNIT_TYPEID::ZERG_HIVE:
		case UNIT_TYPEID::ZERG_HYDRALISKDEN:
		case UNIT_TYPEID::ZERG_INFESTATIONPIT:
		case UNIT_TYPEID::ZERG_LAIR:
		case UNIT_TYPEID::ZERG_LURKERDENMP:
		case UNIT_TYPEID::ZERG_NYDUSCANAL:
		case UNIT_TYPEID::ZERG_NYDUSNETWORK:
		case UNIT_TYPEID::ZERG_ROACHWARREN:
		case UNIT_TYPEID::ZERG_SPAWNINGPOOL:
		case UNIT_TYPEID::ZERG_SPINECRAWLER:
		case UNIT_TYPEID::ZERG_SPINECRAWLERUPROOTED:
		case UNIT_TYPEID::ZERG_SPIRE:
		case UNIT_TYPEID::ZERG_SPORECRAWLER:
		case UNIT_TYPEID::ZERG_SPORECRAWLERUPROOTED:
		case UNIT_TYPEID::ZERG_ULTRALISKCAVERN:

			// Protoss
		case sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR:
		case sc2::UNIT_TYPEID::PROTOSS_CYBERNETICSCORE:
		case sc2::UNIT_TYPEID::PROTOSS_DARKSHRINE:
		case sc2::UNIT_TYPEID::PROTOSS_FLEETBEACON:
		case sc2::UNIT_TYPEID::PROTOSS_FORGE:
		case sc2::UNIT_TYPEID::PROTOSS_GATEWAY:
		case sc2::UNIT_TYPEID::PROTOSS_NEXUS:
		case sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON:
		case sc2::UNIT_TYPEID::PROTOSS_PYLON:
		case sc2::UNIT_TYPEID::PROTOSS_PYLONOVERCHARGED:
		case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSBAY:
		case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY:
		case sc2::UNIT_TYPEID::PROTOSS_STARGATE:
		case sc2::UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE:
		case sc2::UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL:
		case sc2::UNIT_TYPEID::PROTOSS_WARPGATE:
		case sc2::UNIT_TYPEID::PROTOSS_SHIELDBATTERY:
			return true;

		default:
			return false;
		}
	}


	inline bool is_building(const Unit& unit_) {
		return is_building(unit_.unit_type);
	}







	inline Units get_pending_buildings(const ObservationInterface* obs, const sc2::Filter& filter)
	{
		auto f = [&filter](const Unit& unit) -> bool { return unit.is_alive && unit.build_progress < 1.0 && is_building(unit) && filter(unit); };
		return obs->GetUnits(f);
	}

	inline Units get_pending_buildings(const ObservationInterface* obs)
	{
		return get_pending_buildings(obs, [](const Unit&) -> bool {return true;});
	}

	inline Units get_pending_units(const ObservationInterface* obs, const sc2::Filter& filter)
	{
		auto f = [&filter](const Unit& unit) -> bool { return unit.is_alive && unit.build_progress < 1.0 && filter(unit); };
		return obs->GetUnits(f);
	}

	inline Units get_pending_units(const ObservationInterface* obs)
	{
		return get_pending_units(obs, [](const Unit&) -> bool {return true; });
	}

	inline bool can_afford(const ObservationInterface* obs, UNIT_TYPEID type)
	{
		const auto& unit_data = obs->GetUnitTypeData();
		auto idx = static_cast<size_t>(type);
		auto mineral_cost = unit_data[idx].mineral_cost;
		auto gas_cost = unit_data[idx].vespene_cost;
		auto supply_cost = unit_data[idx].food_required;

		if (obs->GetMinerals() >= mineral_cost
			&& obs->GetVespene() >= gas_cost
			&& ((obs->GetFoodCap() - obs->GetFoodUsed()) >= supply_cost))
		{
			return true;
		}
		else {
			return false;
		}
	}

	inline bool train(UNIT_TYPEID type)
	{

	}

}