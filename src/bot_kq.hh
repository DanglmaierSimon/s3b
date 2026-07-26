#pragma once


#include "sc2_agent.h"
#include "sc2_interfaces.h"
#include "sc2_map_info.h"
#include "sc2_unit_filters.h"
#include "sc2_utils.h"

namespace sc2 {


	class BotKillerQueen : public sc2::Agent {
	public:
		virtual void OnGameFullStart() final;
		virtual void OnGameStart() final;
		virtual void OnStep() final;
		virtual void OnGameEnd() final;
		virtual void OnUnitIdle(const Unit* unit) final;
		virtual void OnUnitDestroyed(const Unit*) final;
		virtual void OnNeutralUnitCreated(const Unit*) final;
		virtual void OnUnitCreated(const Unit*) final;
		virtual void OnUpgradeCompleted(UpgradeID) final;
		virtual void OnBuildingConstructionComplete(const Unit*) final;
		virtual void OnUnitDamaged(const Unit*, float /*health*/, float /*shields*/) final;
		virtual void OnNydusDetected() final;
		virtual void OnNuclearLaunchDetected() final;
		virtual void OnUnitEnterVision(const Unit*) final;
		virtual void OnError(const std::vector<ClientError>& /*client_errors*/, const std::vector<std::string>& /*protocol_errors*/) final;

	private:
		void PreventSupplyBlock(const ObservationInterface* obs);

		void BuildSpawnPoolIfPossible();

		void ExpandIfPossible();

		void BuildWorkers();

		void BuildQueens();

		void AttackWithQueens();

		void SpreadCreep();

		void distributeWorkers();

		const Unit* find_closest_mineral_patch(const Point2D& pos);

		void sendWorkerToClosestMineralPatch(const Unit* worker);

	private:
		uint32_t iteration = 0;
		std::vector<Point3D> expansions_;

		inline bool CanPathToLocation(const sc2::Unit* unit, sc2::Point2D& target_pos) {
			// Send a pathing query from the unit to that point. Can also query from point to point,
			// but using a unit tag wherever possible will be more accurate.
			// Note: This query must communicate with the game to get a result which affects performance.
			// Ideally batch up the queries (using PathingDistanceBatched) and do many at once.
			float distance = Query()->PathingDistance(unit, target_pos);

			return distance > 0.1f;
		}

		inline void TryMoveRandomUnit() {
			const sc2::ObservationInterface* observation = Observation();
			sc2::ActionInterface* action = Actions();

			sc2::Units my_units = observation->GetUnits(sc2::Unit::Alliance::Self);
			if (my_units.empty()) {
				return;
			}

			const sc2::Unit* unit = sc2::GetRandomEntry(my_units);

			sc2::Point2D move_target = sc2::FindRandomLocation(observation->GetGameInfo());
			if (!CanPathToLocation(unit, move_target)) {
				return;
			}

			action->UnitCommand(unit, sc2::ABILITY_ID::SMART, move_target);
		}

		// Try build structure given a location. This is used most of the time
		inline bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Point2D location,
			bool isExpansion = false) {
			const ObservationInterface* observation = Observation();
			Units workers = observation->GetUnits(Unit::Alliance::Self, sc2::IsUnit(unit_type));

			// if we have no workers Don't build
			if (workers.empty()) {
				return false;
			}

			// Check to see if there is already a worker heading out to build it
			for (const auto& worker : workers) {
				for (const auto& order : worker->orders) {
					if (order.ability_id == ability_type_for_structure) {
						return false;
					}
				}
			}

			// If no worker is already building one, get a random worker to build one
			const Unit* unit = GetRandomEntry(workers);

			// Check to see if unit can make it there
			if (Query()->PathingDistance(unit, location) < 0.1f) {
				return false;
			}

			if (!isExpansion) {
				for (const auto& expansion : expansions_) {
					if (Distance2D(location, Point2D(expansion.x, expansion.y)) < 7) {
						return false;
					}
				}
			}
			// Check to see if unit can build there
			if (Query()->Placement(ability_type_for_structure, location)) {
				Actions()->UnitCommand(unit, ability_type_for_structure, location);
				return true;
			}
			return false;
		}

		inline bool TryBuildOnCreep(AbilityID ability_type_for_structure, UnitTypeID unit_type) {
			float rx = GetRandomScalar();
			float ry = GetRandomScalar();
			const ObservationInterface* observation = Observation();
			auto start_location = observation->GetStartLocation();

			Point2D build_location = Point2D(start_location.x + rx * 15, start_location.y + ry * 15);

			if (observation->HasCreep(build_location)) {
				return TryBuildStructure(ability_type_for_structure, unit_type, build_location);
			}
			return false;
		}

		inline bool TryExpand(AbilityID build_ability, UnitTypeID worker_type) {
			const ObservationInterface* observation = Observation();
			auto max = std::numeric_limits<float>::max();
			// closest expansion at index [0], second closest at [1]
			std::array<Point3D, 3> closest_expansions;
			std::array<float, 3> closest_distances = { max,max,max };

			for (int i = 0; i < expansions_.size(); i++) {
				float current_distance = Distance2D(observation->GetStartLocation(), expansions_[i]);
				if (current_distance < .01f) {
					continue;
				}

				if (current_distance < closest_distances[0]) {
					if (Query()->Placement(build_ability, expansions_[i])) {

						// shift array elements
						closest_expansions[2] = closest_expansions[1];
						closest_expansions[1] = closest_expansions[0];

						closest_distances[2] = closest_distances[1];
						closest_distances[1] = closest_distances[0];

						closest_expansions[0] = expansions_[i];
						closest_distances[0] = current_distance;
					}
				}
				else if (current_distance < closest_distances[1])
				{
					if (Query()->Placement(build_ability, expansions_[i])) {

						// shift array elements
						closest_expansions[2] = closest_expansions[1];

						closest_distances[2] = closest_distances[1];

						closest_expansions[1] = expansions_[i];
						closest_distances[1] = current_distance;
					}
				}
				else if (current_distance < closest_distances[2])
				{
					if (Query()->Placement(build_ability, expansions_[i])) {

						closest_expansions[2] = expansions_[i];
						closest_distances[2] = current_distance;
					}
				}
			}
			Point3D staging_location_;
			// only update staging location up till 3 bases.
			for (int i = 0; i < 3; i++)
			{
				auto closest = closest_expansions[i];

				if (TryBuildStructure(build_ability, worker_type, closest, true) &&
				observation->GetUnits(Unit::Self, sc2::IsTownHall()).size() < 4) {
					staging_location_ = Point3D(((staging_location_.x + closest.x) / 2),
						((staging_location_.y + closest.y) / 2),
						((staging_location_.z + closest.z) / 2));
					return true;
				}
			}
			return false;

		}


		inline bool have_enough_supply(const ObservationInterface* obs, double requested_supply)
		{
			auto supply_cap = (double)obs->GetFoodCap();
			auto supply_used = (double)obs->GetFoodUsed();

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

		inline bool train(const ObservationInterface* obs, sc2::ActionInterface* actions, sc2::QueryInterface* queries, UNIT_TYPEID type, int amount = 1, bool allow_queueing = true)
		{
			using namespace std;
			auto p_units = get_production_unit(type);

			(void)allow_queueing; // TODO Implement queueing of units -> is this even required?

			if (p_units.empty() || p_units[0] == UNIT_TYPEID::INVALID)
			{
				std::cout << "WARNING: Requested builders for " << UnitTypeToName(UnitTypeID(type)) << "(" << UnitTypeID(type) << ")" << " and received none!" << std::endl;
				return false;
			}

			auto builder_filter = [&p_units](const Unit& unit) -> bool {return unit.alliance == Unit::Alliance::Self && unit.build_progress == 1.0 && IsUnits{ p_units }(unit);};
			auto builders = obs->GetUnits(builder_filter);

			const auto& unit_data = obs->GetUnitTypeData();

			auto left_to_train = amount;

			auto pending_units = get_pending_units(obs, type);

			if (pending_units.size() >= builders.size())
			{
				return false;
			}

			int builders_tried = 0;

			while (left_to_train > 0 && have_enough_supply(obs, unit_data[(uint32_t)type].food_required))
			{
				auto builder = GetRandomEntry(builders);

				if (builders_tried >= builders.size())
				{
					break;
				}

				// TODO: Allow queuing of stuff
				if (builder->orders.empty())
				{
					// TODO: Use query interface to ask for abilities of unit

					auto abilities = queries->GetAbilitiesForUnit(builder, true, true);

					auto ability = get_production_ability(type);
					assert(ability != ABILITY_ID::INVALID);
					std::cout << "INFO: Sending command " << AbilityTypeToName(AbilityID(ability)) << "; " << AbilityID(ability) << " to unit " << UnitTypeToName(builder->unit_type) << "; " << UnitTypeID(builder->unit_type) << std::endl;
					actions->UnitCommand(builder, ability);
					left_to_train = std::max(left_to_train - 1, 0);
				}
				else
				{
					builders_tried += 1;
				}
			}

			return left_to_train == 0;
		}


		inline bool can_afford(const ObservationInterface* obs, UNIT_TYPEID type)
		{
			const auto& unit_data = obs->GetUnitTypeData();
			auto idx = static_cast<size_t>(type);
			auto mineral_cost = unit_data[idx].mineral_cost;
			auto gas_cost = unit_data[idx].vespene_cost;
			double supply_cost = std::ceil(unit_data[idx].food_required);

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


		bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type);

		const Unit* FindNearestMineralPatch(const Point2D& start);

		GameInfo game_info_;
	};
}