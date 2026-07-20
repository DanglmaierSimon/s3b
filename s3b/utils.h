#pragma once

#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_client.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_utils.h"

#include "unit_data.h"

namespace sc2 {

	inline Units get_pending_buildings(const ObservationInterface* obs, const sc2::Filter& filter)
	{
		auto f = [&filter](const Unit& unit) -> bool { return unit.is_alive && unit.build_progress < 1.0 && is_building(unit) && filter(unit); };
		return obs->GetUnits(f);
	}

	inline Units get_pending_buildings(const ObservationInterface* obs)
	{
		return get_pending_buildings(obs, [](const Unit&) -> bool {return true;});
	}

	inline Units get_pending_units(const ObservationInterface* obs, UNIT_TYPEID type)
	{
		auto f = [type](const Unit& unit) -> bool {
			if (unit.unit_type.ToType() == UNIT_TYPEID::ZERG_EGG)
			{
				if (unit.orders.empty())
				{
					// can this even happen?
					return false;
				}
				auto ability = get_production_abilities(type);
				return unit.orders[0].ability_id == AbilityID(ability);
			}
			else if ((unit.unit_type.ToType() != type))
			{
				return false;
			}

			return unit.build_progress < 1.0;

			};
		return obs->GetUnits(f);
	}

	inline bool have_enough_supply(const ObservationInterface* obs, int requested_supply)
	{
		auto supply_cap = (int)obs->GetFoodCap();
		auto supply_used = (int)obs->GetFoodUsed();

		// some units have negative supply cost
		// because they provide supply (overlord)
		if (requested_supply <= 0)
		{
			return true;
		}

		// supply block (eg overlord killed)
		if (supply_used > supply_cap)
		{
			return false;
		}

		auto free_supply = supply_cap - supply_used;

		if (free_supply < requested_supply)
		{
			return false;
		}
		else
		{
			return true;
		}
	}


	inline bool can_afford(const ObservationInterface* obs, UNIT_TYPEID type)
	{
		const auto& unit_data = obs->GetUnitTypeData();
		auto idx = static_cast<size_t>(type);
		auto mineral_cost = unit_data[idx].mineral_cost;
		auto gas_cost = unit_data[idx].vespene_cost;
		int supply_cost = std::ceil(unit_data[idx].food_required);

		if (obs->GetMinerals() >= mineral_cost
			&& obs->GetVespene() >= gas_cost
			&& (have_enough_supply(obs, supply_cost)))
		{
			return true;
		}
		else {
			return false;
		}
	}

	inline bool train(const ObservationInterface* obs, sc2::ActionInterface* actions, sc2::QueryInterface* queries, UNIT_TYPEID type, int amount = 1, bool allow_queueing = true)
	{
		using namespace std;
		auto p_units = get_production_unit(type);

		if (p_units.empty() || p_units[0] == UNIT_TYPEID::INVALID)
		{
			std::cout << "WARNING: Requested builders for " << static_cast<int>(type) << " and received none!";
			return false;
		}

		auto builder_filter = IsUnits(p_units);
		auto builders = obs->GetUnits(builder_filter);

		const auto& unit_data = obs->GetUnitTypeData();

		auto supply_needed = unit_data[(uint32_t)type].food_required * amount;

		auto left_to_train = amount;

		for (const auto& builder : builders)
		{
			if (left_to_train == 0)
			{
				break;
			}

			// TODO: Allow queuing of stuff
			if (builder->orders.empty())
			{
				// TODO: Use query interface to ask for abilities of unit

				auto abilities = queries->GetAbilitiesForUnit(builder, true, true);

				std::cout << "DEBUG: Available abilities: " << std::endl;
				cout << "Tag: " << abilities.unit_tag << endl;
				cout << "Unit Type: " << abilities.unit_type_id.to_string() << "; " << UnitTypeToName(abilities.unit_type_id) << endl;
				cout << "abilities: " << endl;
				for (auto a : abilities.abilities)
				{
					cout << "ability: " << a.ability_id << "; " << AbilityTypeToName(AbilityID(a.ability_id)) << "; req. point: " << a.requires_point << endl;
				}

				cout << "=================" << endl;


				auto t = builder->unit_type.ToType();
				auto ability = get_production_abilities(type);
				assert(ability != ABILITY_ID::INVALID);
				std::cout << "INFO: Sending command " << AbilityTypeToName(AbilityID(ability)) << "; " << AbilityID(ability) << " to unit " << UnitTypeToName(builder->unit_type) << "; " << UnitTypeID(builder->unit_type) << std::endl;
				actions->UnitCommand(builder, ability);
				left_to_train = std::max(left_to_train - 1, 0);
			}
		}

		return left_to_train == 0;
	}

}