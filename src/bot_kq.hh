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
			float minimum_distance = std::numeric_limits<float>::max();
			Point3D closest_expansion;
			for (int i = 0; i < expansions_.size(); i++) {
				float current_distance = Distance2D(observation->GetStartLocation(), expansions_[i]);
				if (current_distance < .01f) {
					continue;
				}

				if (current_distance < minimum_distance) {
					if (Query()->Placement(build_ability, expansions_[i])) {
						closest_expansion = expansions_[i];
						minimum_distance = current_distance;
					}
				}
			}
			Point3D staging_location_;
			// only update staging location up till 3 bases.
			if (TryBuildStructure(build_ability, worker_type, closest_expansion, true) &&
				observation->GetUnits(Unit::Self, sc2::IsTownHall()).size() < 4) {
				staging_location_ = Point3D(((staging_location_.x + closest_expansion.x) / 2),
					((staging_location_.y + closest_expansion.y) / 2),
					((staging_location_.z + closest_expansion.z) / 2));
				return true;
			}
			return false;
		}


		bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type);

		const Unit* FindNearestMineralPatch(const Point2D& start);

		GameInfo game_info_;
	};
}