#pragma once

#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_client.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_utils.h"

#include "unit_info.h"

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

	inline Units get_pending_units(const ObservationInterface* obs, const sc2::Filter& filter)
	{
		auto f = [&filter](const Unit& unit) -> bool { return unit.is_alive && unit.build_progress < 1.0 && filter(unit); };
		return obs->GetUnits(f);
	}

	inline Units get_pending_units(const ObservationInterface* obs)
	{
		return get_pending_units(obs, [](const Unit&) -> bool {return true; });
	}

	inline bool have_enough_supply(const ObservationInterface* obs, size_t requested_supply)
	{
		auto supply_cap = obs->GetFoodCap();
		auto supply_used = obs->GetFoodUsed();

		// supply block (eg overlord killed)
		if (supply_used >= supply_cap)
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
		auto supply_cost = unit_data[idx].food_required;

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

	inline bool train(const ObservationInterface* obs, sc2::ActionInterface* actions, UNIT_TYPEID type, int amount = 1, bool allow_queueing = true)
	{
		auto builder_filter = IsUnits(unit_trained_from(type));
		auto builders = obs->GetUnits(builder_filter);

		const auto& unit_data = obs->GetUnitTypeData();

		auto supply_needed = unit_data[(uint32_t)type].food_required * amount;

		auto left_to_train = amount;

		for (const auto& builder : builders)
		{
			if (builder->orders.empty())
			{
				builder->
					actions->UnitCommand()
			}
		}
	}




}