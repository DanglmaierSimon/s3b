#pragma once

#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS 1

#include "sc2_search.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_utils.h"
#include <sc2_unit_filters.h>

namespace sc2 {

	class MarineMicroBot : public Agent {
	public:
		virtual void OnGameStart() final;
		virtual void OnStep() final;
		virtual void OnUnitDestroyed(const Unit* unit) override;

	private:
		bool GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D& position);
		bool GetNearestZergling(const Point2D& from);

		const Unit* targeted_zergling_;
		bool move_back_;
		Point2D backup_target_;
		Point2D backup_start_;
	};

	// Bot builds supply depots as required.
	// Bot builds 15 SCVs.
	// Bot builds a barracks.
	// Bot builds 10 marines.
	// Bot finds enemy, sends marines.
	class TerranBot : public sc2::Agent {
	public:
		virtual void OnGameStart() final;
		virtual void OnStep() final;
		virtual void OnGameEnd() final;
		virtual void OnUnitIdle(const Unit* unit) final;

		void PrintStatus(std::string msg);

		// Tries to find a random location that can be pathed to on the map.
		// Returns 'true' if a new, random location has been found that is pathable by the unit.
		bool FindEnemyPosition(Point2D& target_pos, int loop);
		void ScoutWithMarines(int loop);
		bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type = UNIT_TYPEID::TERRAN_SCV);
		bool TryBuildSupplyDepot();
		bool TryBuildBarracks();
		bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type);
		bool TryBuildSCV();
		bool TryBuildMarine();

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

		const Unit* FindNearestMineralPatch(const Point2D& start);

		GameInfo game_info_;
	};


	class BotKillerQueen : public sc2::Agent {
	public:
		virtual void OnGameStart() final;
		virtual void OnStep() final;
		virtual void OnGameEnd() final;
		virtual void OnUnitIdle(const Unit* unit) final;

	private:
		void PreventSupplyBlock(const ObservationInterface* obs);

		void BuildSpawnPoolIfPossible();

		void ExpandIfPossible();

		void BuildWorkers();

		void BuildQueens();

		void AttackWithQueens();

		void SpreadCreep();

	private:
		uint32_t iteration = 0;
		// std::unordered_set<sc2::Point2D> candidate_positions;
		bool target_list_empty_previously = true;

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
			Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));

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
			// TODO: dont be stupid, calculate at start or something
			auto expansions = search::CalculateExpansionLocations(Observation(), Query());

			if (!isExpansion) {
				for (const auto& expansion : expansions) {
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
			auto expansions = search::CalculateExpansionLocations(observation, Query());
			Point3D closest_expansion;
			for (int i = 0; i < expansions.size(); i++) {
				float current_distance = Distance2D(observation->GetStartLocation(), expansions[i]);
				if (current_distance < .01f) {
					continue;
				}

				if (current_distance < minimum_distance) {
					if (Query()->Placement(build_ability, expansions[i])) {
						closest_expansion = expansions[i];
						minimum_distance = current_distance;
					}
				}
			}
			Point3D staging_location_;
			// only update staging location up till 3 bases.
			if (TryBuildStructure(build_ability, worker_type, closest_expansion, true) &&
				observation->GetUnits(Unit::Self, IsTownHall()).size() < 4) {
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


	class MultiplayerBot : public sc2::Agent {
	public:
		bool nuke_detected = false;
		uint32_t nuke_detected_frame;

		void PrintStatus(std::string msg);

		virtual void OnGameStart();

		size_t CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type);

		size_t CountUnitTypeBuilding(const ObservationInterface* observation, UNIT_TYPEID production_building,
			ABILITY_ID ability);

		size_t CountUnitTypeTotal(const ObservationInterface* observation, UNIT_TYPEID unit_type, UNIT_TYPEID production,
			ABILITY_ID ability);

		size_t CountUnitTypeTotal(const ObservationInterface* observation, std::vector<UNIT_TYPEID> unit_type,
			UNIT_TYPEID production, ABILITY_ID ability);

		bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type);

		const Unit* FindNearestMineralPatch(const Point2D& start);

		// Tries to find a random location that can be pathed to on the map.
		// Returns 'true' if a new, random location has been found that is pathable by the unit.
		bool FindEnemyPosition(Point2D& target_pos);

		bool TryFindRandomPathableLocation(const Unit* unit, Point2D& target_pos);

		void AttackWithUnitType(UnitTypeID unit_type, const ObservationInterface* observation);

		void ScoutWithUnits(UnitTypeID unit_type, const ObservationInterface* observation);

		void RetreatWithUnits(UnitTypeID unit_type, Point2D retreat_position);

		void AttackWithUnit(const Unit* unit, const ObservationInterface* observation);

		void ScoutWithUnit(const Unit* unit, const ObservationInterface* observation);

		void RetreatWithUnit(const Unit* unit, Point2D retreat_position);

		// Try build structure given a location. This is used most of the time
		bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Point2D location,
			bool isExpansion);
		// Try to build a structure based on tag, Used mostly for Vespene, since the pathing check will fail even though the
		// geyser is "Pathable"

		bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Tag location_tag);
		// Expands to nearest location and updates the start location to be between the new location and old bases.

		bool TryExpand(AbilityID build_ability, UnitTypeID worker_type);
		// Tries to build a geyser for a base

		bool TryBuildGas(AbilityID build_ability, UnitTypeID worker_type, Point2D base_location);

		bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type);

		// Mine the nearest mineral to Town hall.
		// If we don't do this, probes may mine from other patches if they stray too far from the base after building.
		void MineIdleWorkers(const Unit* unit, AbilityID worker_gather_command, UnitTypeID vespene_building_type);

		// An estimate of how many workers we should have based on what buildings we have
		int GetExpectedWorkers(UNIT_TYPEID vespene_building_type);

		// To ensure that we do not over or under saturate any base.
		void ManageWorkers(UNIT_TYPEID worker_type, AbilityID worker_gather_command, UNIT_TYPEID vespene_building_type);

		virtual void OnNuclearLaunchDetected() final;

		uint32_t current_game_loop_ = 0;
		int max_worker_count_ = 70;

		// When to start building attacking units
		int target_worker_count_ = 15;
		GameInfo game_info_;
		std::vector<Point3D> expansions_;
		Point3D startLocation_;
		Point3D staging_location_;

	private:
		std::string last_action_text_;
	};

	class ProtossMultiplayerBot : public MultiplayerBot {
	public:
		bool air_build_ = false;

		bool TryBuildArmy();

		void BuildOrder();

		void ManageUpgrades();

		void ManageArmy();

		bool TryWarpInUnit(ABILITY_ID ability_type_for_unit);

		void ConvertGateWayToWarpGate();

		bool TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID unit_type);

		bool TryBuildPylon();

		// Separated per race due to gas timings
		bool TryBuildAssimilator();

		// Same as above with expansion timings
		bool TryBuildExpansionNexus();

		bool TryBuildProbe();

		virtual void OnStep() final;

		virtual void OnGameEnd() final;

		virtual void OnUnitIdle(const Unit* unit) override;

		virtual void OnUpgradeCompleted(UpgradeID upgrade) final;

	private:
		bool warpgate_reasearched_ = false;
		bool blink_reasearched_ = false;
		int max_colossus_count_ = 5;
		int max_sentry_count_ = 2;
		int max_stalker_count_ = 20;
	};

	class ZergMultiplayerBot : public MultiplayerBot {
	public:
		bool mutalisk_build_ = false;

		bool TryBuildDrone();

		bool TryBuildOverlord();

		void BuildArmy();

		bool TryBuildOnCreep(AbilityID ability_type_for_structure, UnitTypeID unit_type);

		void BuildOrder();

		void ManageUpgrades();

		void ManageArmy();

		void TryInjectLarva();

		bool TryBuildExpansionHatch();

		bool BuildExtractor();

		virtual void OnStep() final;

		virtual void OnUnitIdle(const Unit* unit) override;

	private:
		std::vector<UNIT_TYPEID> hatchery_types = { UNIT_TYPEID::ZERG_HATCHERY, UNIT_TYPEID::ZERG_HIVE,
												   UNIT_TYPEID::ZERG_LAIR };
	};

	class TerranMultiplayerBot : public MultiplayerBot {
	public:
		bool mech_build_ = false;

		bool TryBuildSCV();

		bool TryBuildSupplyDepot();

		void BuildArmy();

		bool TryBuildAddOn(AbilityID ability_type_for_structure, uint64_t base_structure);

		bool TryBuildStructureRandom(AbilityID ability_type_for_structure, UnitTypeID unit_type);

		void BuildOrder();

		void ManageUpgrades();

		void ManageArmy();

		bool TryBuildExpansionCom();

		bool BuildRefinery();

		virtual void OnStep() final;

		virtual void OnUnitIdle(const Unit* unit) override;

		virtual void OnUpgradeCompleted(UpgradeID upgrade) override;

	private:
		std::vector<UNIT_TYPEID> barrack_types = { UNIT_TYPEID::TERRAN_BARRACKSFLYING, UNIT_TYPEID::TERRAN_BARRACKS };
		std::vector<UNIT_TYPEID> factory_types = { UNIT_TYPEID::TERRAN_FACTORYFLYING, UNIT_TYPEID::TERRAN_FACTORY };
		std::vector<UNIT_TYPEID> starport_types = { UNIT_TYPEID::TERRAN_STARPORTFLYING, UNIT_TYPEID::TERRAN_STARPORT };
		std::vector<UNIT_TYPEID> supply_depot_types = { UNIT_TYPEID::TERRAN_SUPPLYDEPOT,
													   UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED };
		std::vector<UNIT_TYPEID> bio_types = { UNIT_TYPEID::TERRAN_MARINE, UNIT_TYPEID::TERRAN_MARAUDER,
											  UNIT_TYPEID::TERRAN_GHOST, UNIT_TYPEID::TERRAN_REAPER /*reaper*/ };
		std::vector<UNIT_TYPEID> widow_mine_types = { UNIT_TYPEID::TERRAN_WIDOWMINE, UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED };
		std::vector<UNIT_TYPEID> siege_tank_types = { UNIT_TYPEID::TERRAN_SIEGETANK, UNIT_TYPEID::TERRAN_SIEGETANKSIEGED };
		std::vector<UNIT_TYPEID> viking_types = { UNIT_TYPEID::TERRAN_VIKINGASSAULT, UNIT_TYPEID::TERRAN_VIKINGFIGHTER };
		std::vector<UNIT_TYPEID> hellion_types = { UNIT_TYPEID::TERRAN_HELLION, UNIT_TYPEID::TERRAN_HELLIONTANK };

		bool nuke_built = false;
		bool stim_researched_ = false;
		bool ghost_cloak_researched_ = true;
		bool banshee_cloak_researched_ = true;
	};

}  // namespace sc2
