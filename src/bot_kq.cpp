#include "bot_kq.hh"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iostream>
#include <iterator>
#include <ostream>
#include <random>
#include <string>
#include <string>

#include "utils.h"
#include <fstream>
#include <numeric>
#include <ostream>

#include "sc2_api.h"
#include "sc2_lib.h"
#include "sc2_search.h"
#include "sc2_unit_filters.h"
#include "sc2_unit_filters.h"

#include "utils.h"


using namespace std;


namespace
{

	static string to_string(sc2::Attribute a)
	{
		switch (a)
		{
		case sc2::Attribute::Light: return "Light(1)";
		case sc2::Attribute::Armored: return "Armored(2)";
		case sc2::Attribute::Biological: return "Biological(3)";
		case sc2::Attribute::Mechanical: return "Mechanical(4)";
		case sc2::Attribute::Robotic: return "Robotic(5)";
		case sc2::Attribute::Psionic: return "Psionic(6)";
		case sc2::Attribute::Massive: return "Massive(7)";
		case sc2::Attribute::Structure: return "Structure(8)";
		case sc2::Attribute::Hover: return "Hover(9)";
		case sc2::Attribute::Heroic: return "Heroic(10)";
		case sc2::Attribute::Summoned: return "Summoned(11)";
		case sc2::Attribute::Invalid: return "Invalid(12)";
		default: return string("Unknown(") + ::to_string(static_cast<int>(a)) + ")";
		}
	}

	static string to_string(sc2::Weapon w)
	{
		// TODO Rest of this stuff
		switch (w.type)
		{
		case sc2::Weapon::TargetType::Air: return "Air";
		case sc2::Weapon::TargetType::Ground: return "Ground";
		case sc2::Weapon::TargetType::Any: return "Any";
		default: return "Invalid";
		}
	}

	static	string to_string(sc2::Race  race)
	{
		switch (race) {
		case sc2::Race::Terran: return "Terran";
		case sc2::Race::Zerg: return "Zerg";
		case sc2::Race::Protoss: return "Protoss";
		case sc2::Race::Random: return "Random";
		default: return "Unknown";
		}
	}


	static string debugUnitTypeData(const sc2::UnitTypeData& ud) {

		string ret = "UnitTypeData {\n";
		ret += "  unit_type_id: " + std::to_string(ud.unit_type_id) + "\n";
		ret += "  name: " + (ud.name) + "\n";
		ret += "  available: " + std::to_string(ud.available) + "\n";
		ret += "  cargo_size: " + std::to_string(ud.cargo_size) + "\n";
		ret += "  mineral_cost: " + std::to_string(ud.mineral_cost) + "\n";
		ret += "  vespene_cost: " + std::to_string(ud.vespene_cost) + "\n";
		ret += "  attributes: [";
		for (auto a : ud.attributes)
		{
			ret += to_string(a) + ", ";
		}
		ret += "]\n";

		ret += "  movement_speed: " + std::to_string(ud.movement_speed) + "\n";
		ret += "  armor: " + std::to_string(ud.armor) + "\n";

		ret += "  weapons: [";
		for (auto w : ud.weapons)
		{
			ret += to_string(w) + ", ";
		}
		ret += "]\n";

		ret += "  food_required: " + std::to_string(ud.food_required) + "\n";
		ret += "  food_provided: " + std::to_string(ud.food_provided) + "\n";
		ret += "  ability_id: " + std::to_string(ud.ability_id) + "\n";
		ret += "  race: " + to_string(ud.race) + "\n";
		ret += "  build_time: " + std::to_string(ud.build_time) + "\n";
		ret += "  has_minerals: " + std::to_string(ud.has_minerals) + "\n";
		ret += "  has_vespene: " + std::to_string(ud.has_vespene) + "\n";
		ret += "  sight_range: " + std::to_string(ud.sight_range) + "\n";

		ret += "  tech_alias: [";
		for (auto ta : ud.tech_alias)
		{
			ret += std::to_string(ta) + ", ";
		}
		ret += "]\n";
		ret += "  unit_alias: " + std::to_string(ud.unit_alias) + "\n";
		ret += "  tech_requirement: " + std::to_string(ud.tech_requirement) + "\n";
		ret += "  require_attached: " + std::to_string(ud.require_attached) + "\n";

		ret += "}\n";

		return ret;
	}


	static string to_string(sc2::AbilityData::Target t)
	{
		switch (t)
		{
		case sc2::AbilityData::Target::None: return "None";
		case sc2::AbilityData::Target::Point: return "Point";
		case sc2::AbilityData::Target::Unit: return "Unit";
		case sc2::AbilityData::Target::PointOrUnit: return "PointOrUnit";
		case sc2::AbilityData::Target::PointOrNone: return "PointOrNone";
		default: return "Unknown";
		}
	}

	static string debugAbilityData(const sc2::AbilityData& a)
	{
		string ret = "AbilityData {\n";
		ret += "  available: " + std::to_string(a.available) + "\n";
		ret += "  ability_id: " + std::to_string(a.ability_id) + "\n";
		ret += "  link_name: " + (a.link_name) + "\n";
		ret += "  link_index: " + std::to_string(a.link_index) + "\n";
		ret += "  button_name: " + (a.button_name) + "\n";
		ret += "  friendly_name: " + (a.friendly_name) + "\n";
		ret += "  hotkey: " + (a.hotkey) + "\n";
		ret += "  remaps_to_ability_id: " + std::to_string(a.remaps_to_ability_id) + "\n";
		ret += "  remaps_from_ability_id: [";
		for (auto r : a.remaps_from_ability_id)
		{
			ret += std::to_string(r) + ", ";
		}
		ret += "]\n";
		ret += "  target: " + to_string(a.target) + "\n";
		ret += "  allow_autocast: " + std::to_string(a.allow_autocast) + "\n";
		ret += "  is_building: " + std::to_string(a.is_building) + "\n";
		ret += "  footprint_radius: " + std::to_string(a.footprint_radius) + "\n";
		ret += "  is_instant_placement: " + std::to_string(a.is_instant_placement) + "\n";
		ret += "  cast_range: " + std::to_string(a.cast_range) + "\n";

		ret += "}\n";
		return ret;
	}


	bool GetRandomUnit(const sc2::Unit*& unit_out, const sc2::ObservationInterface* observation, sc2::UnitTypeID unit_type) {
		sc2::Units my_units = observation->GetUnits(sc2::Unit::Alliance::Self, sc2::IsUnit(unit_type));
		if (!my_units.empty()) {
			unit_out = GetRandomEntry(my_units);
			return true;
		}
		return false;
	}

}




namespace sc2 {



	void BotKillerQueen::OnGameFullStart()
	{
		cout << "OnGameFullStart()" << endl;
	}

	void BotKillerQueen::OnGameStart()
	{
		cout << "OnGameStart()" << endl;

		auto params = search::ExpansionParameters();
		//params.circle_step_size_ = 0.25;
		//params.radiuses_ = { 2,3,4,5,6,7,9 };
		//params.cluster_distance_ = 14;
		params.debug_ = Debug();

		expansions_ = search::CalculateExpansionLocations(Observation(), Query(), params);
		Debug()->SendDebug();
		cout << "calculated expansion locations" << endl;

		auto start = Observation()->GetStartLocation();

		std::sort(expansions_.begin(), expansions_.end(), [start](const sc2::Point3D& l, const sc2::Point3D& r) {return DistanceSquared3D(start, l) < DistanceSquared3D(start, r);});
	}

	void BotKillerQueen::OnStep()
	{
		const auto* obs = Observation();

		if (obs->GetGameLoop() == 0)
		{
			auto os = ofstream("data\\stuff.log");

			os << "Abilities:" << endl;
			for (const auto& a : obs->GetAbilityData())
			{
				os << debugAbilityData(a) << endl;

			}
			os << "====================" << endl;

			os << "UnitTypes:" << endl;
			for (const auto& u : obs->GetUnitTypeData())
			{
				os << debugUnitTypeData(u) << endl;
			}
			os << "====================" << endl;

			os << "Upgrades:" << endl;
			for (const auto& u : obs->GetUpgradeData())
			{
				os << u.Log() << endl;
			}
			os << "====================" << endl;

			os << "Buffs:" << endl;
			for (const auto& b : obs->GetBuffData())
			{
				os << b.Log() << endl;

			}
			os << "====================" << endl;

			os << "Effects:" << endl;
			for (const auto& e : obs->GetEffectData())
			{
				os << e.Log() << endl;

			}
			os << "====================" << endl;

			os.flush();
			os.close();
		}




		this->iteration = obs->GetGameLoop();

		if (this->iteration < 10)
		{
			for (auto sl : obs->GetGameInfo().enemy_start_locations)
			{
				//this->candidate_positions.insert(sl);
			}
		}

		PreventSupplyBlock(obs);

		BuildSpawnPoolIfPossible();

		ExpandIfPossible();

		BuildWorkers();

		BuildQueens();

		AttackWithQueens();

		SpreadCreep();


	}

	static string to_string(const Unit& unit)
	{
		std::string r;

		r += "Unit [" + std::to_string(unit.tag) + "]: { type: " + UnitTypeToName(unit.unit_type) + " }";
		return r;
	}


	void BotKillerQueen::OnGameEnd()
	{
		cout << "OnGameEnd()" << endl;

	}

	void BotKillerQueen::OnUnitIdle(const Unit* unit)
	{
		cout << "OnUnitIdle()" << endl;
		cout << "Idle Unit: " << to_string(*unit) << endl;

		if (IsUnit{ UNIT_TYPEID::ZERG_DRONE }(*unit))
		{
			sendWorkerToClosestMineralPatch(unit);
		}
	}

	void BotKillerQueen::OnUnitDestroyed(const Unit* unit)
	{
		cout << "OnUnitDestroyed()" << endl;
		cout << "Unit destroyed: " << to_string(*unit) << endl;

	}

	void BotKillerQueen::OnNeutralUnitCreated(const Unit* unit)
	{
		cout << "OnNeutralUnitCreated()" << endl;
		cout << "Neutral Unit created: " << to_string(*unit) << endl;
	}

	void BotKillerQueen::OnUnitCreated(const Unit* unit)
	{
		cout << "OnUnitCreated()" << endl;
		cout << "Unit created: " << to_string(*unit) << endl;
	}

	void BotKillerQueen::OnUpgradeCompleted(UpgradeID upgrade)
	{
		cout << "OnUpgradeCompleted()" << endl;
		cout << "Upgrade Completed: " << UpgradeIDToName(upgrade) << endl;
	}

	void BotKillerQueen::OnBuildingConstructionComplete(const Unit* unit)
	{
		cout << "OnBuildingConstructionComplete()" << endl;
		cout << "Building finished: " << to_string(*unit) << endl;
	}

	void BotKillerQueen::OnUnitDamaged(const Unit* unit, float health, float shields)
	{
		cout << "OnUnitDamaged()" << endl;
		cout << "Unit damaged: " << to_string(*unit) << "health: " << health << "; shields: " << shields << endl;

	}

	void BotKillerQueen::OnNydusDetected()
	{
		cout << "OnNydusDetected()" << endl;
		cout << "oh oh, not good" << endl;
	}

	void BotKillerQueen::OnNuclearLaunchDetected()
	{
		cout << "OnNuclearLaunchDetected()" << endl;
		cout << "RUN!" << endl;

	}

	void BotKillerQueen::OnUnitEnterVision(const Unit* unit)
	{
		cout << "OnUnitEnterVision()" << endl;
		cout << "Unit entered vision: " << to_string(*unit) << endl;

	}

	void BotKillerQueen::OnError(const std::vector<ClientError>& client_errors, const std::vector<std::string>& protocoll_errors)
	{
		cout << "OnError()" << endl;

		cout << "Received ERRORS!" << endl;

		for (const auto& ce : client_errors)
		{
			cout << "Client Error: " << static_cast<int>(ce) << endl;
		}

		for (const auto& pe : protocoll_errors)
		{
			cout << "Protocoll Error: " << pe << endl;
		}

	}

	void BotKillerQueen::PreventSupplyBlock(const ObservationInterface* obs)
	{

		auto supply_used = obs->GetFoodUsed();
		auto supply_cap = obs->GetFoodCap();

		if (supply_used >= 200 || supply_cap >= 200)
		{
			return;
		}

		auto units = obs->GetUnits(Unit::Alliance::Self);
		using namespace std;
		//for (auto u : units)
		//{
		//	cout << "Unit( " << u->tag << "):" << endl;
		//	cout << "  Type: " << UnitTypeToName(u->unit_type) << " (" << u->unit_type << ")" << endl;
		//	cout << "  Progress: " << u->build_progress << endl;
		//	cout << ")" << endl;
		//}

		auto pending_overlords = get_pending_units(Observation(), UNIT_TYPEID::ZERG_OVERLORD);
		// std::cout << "========================" << std::endl;

		if (pending_overlords.size() > 0)
		{
			return;
		}

		auto supply_left = obs->GetFoodCap() - obs->GetFoodUsed();

		if (supply_left < 5 && can_afford(obs, UNIT_TYPEID::ZERG_OVERLORD))
		{
			train(obs, Actions(), Query(), UNIT_TYPEID::ZERG_OVERLORD);
		}
	}

	void sc2::BotKillerQueen::BuildSpawnPoolIfPossible()
	{
		auto obs = Observation();

		if (!can_afford(obs, UNIT_TYPEID::ZERG_SPAWNINGPOOL))
		{
			return;
		}

		if (get_pending_buildings(obs, IsUnit(UNIT_TYPEID::ZERG_SPAWNINGPOOL)).size() > 0 || obs->GetUnits(IsUnit(UNIT_TYPEID::ZERG_SPAWNINGPOOL)).size() > 0)
		{
			return;
		}

		TryBuildOnCreep(ABILITY_ID::BUILD_SPAWNINGPOOL, UNIT_TYPEID::ZERG_DRONE);
	}

	void sc2::BotKillerQueen::ExpandIfPossible()
	{

		if (!can_afford(Observation(), UNIT_TYPEID::ZERG_HATCHERY))
		{
			return;
		}

		auto pending_hatches = get_pending_units(Observation(), UNIT_TYPEID::ZERG_HATCHERY);
		auto ready_hatches = get_ready_units(Observation(), UNIT_TYPEID::ZERG_HATCHERY);


		if (pending_hatches.size() + ready_hatches.size() >= expansions_.size())
		{
			return;
		}
		TryExpand(ABILITY_ID::BUILD_HATCHERY, UNIT_TYPEID::ZERG_DRONE);
	}

	void sc2::BotKillerQueen::BuildWorkers()
	{
		auto obs = Observation();
		const auto hatcheries = obs->GetUnits(Unit::Alliance::Self, [](const Unit& unit) { return unit.build_progress == 1.0 && unit.unit_type == UnitTypeID(UNIT_TYPEID::ZERG_HATCHERY); });
		auto hatch_count = hatcheries.size();
		auto larva_count = obs->GetUnits(IsUnit(UNIT_TYPEID::ZERG_LARVA)).size();
		auto pending_workers = get_pending_units(obs, UNIT_TYPEID::ZERG_DRONE).size();
		if (larva_count == 0 || pending_workers > hatch_count)
		{
			return;
		}

		auto calculated_workers = std::accumulate(hatcheries.cbegin(), hatcheries.cend(), 0, [](int val, const Unit* unit) { if (unit->alliance != Unit::Alliance::Self) { return val; } return unit->ideal_harvesters + val; });
		auto optimal_worker_count = std::min(calculated_workers, 70);
		auto current_workers = obs->GetFoodWorkers();

		// todo: distribute workers evenly to bases
		auto larva = obs->GetUnits(IsUnit(UNIT_TYPEID::ZERG_LARVA));

		if (!larva.empty() && can_afford(obs, UNIT_TYPEID::ZERG_DRONE) && current_workers < optimal_worker_count)
		{
			if (train(obs, Actions(), Query(), UNIT_TYPEID::ZERG_DRONE))
			{
				std::cout << "optimal worker count: " << optimal_worker_count << std::endl;
				std::cout << "current worker count: " << current_workers << std::endl;
				std::cout << "training worker..." << std::endl;
			}
		}

		for (auto hatch : hatcheries)
		{
			if (hatch->assigned_harvesters > hatch->ideal_harvesters)
			{
				distributeWorkers();
				break;
			}
		}
	}

	void BotKillerQueen::BuildQueens()
	{
		auto spawning_pools = Observation()->GetUnits([](const Unit& unit) -> bool {return unit.build_progress == 1.0 && IsUnit{ UNIT_TYPEID::ZERG_SPAWNINGPOOL }(unit);}).size();


		if (spawning_pools > 0 && can_afford(Observation(), UNIT_TYPEID::ZERG_QUEEN))
		{
			auto pending_queens = get_pending_units(Observation(), UNIT_TYPEID::ZERG_QUEEN);
			auto hatcheries = get_ready_units(Observation(), UNIT_TYPEID::ZERG_HATCHERY);

			if (pending_queens >= hatcheries)
			{
				return;
			}

			train(Observation(), Actions(), Query(), UNIT_TYPEID::ZERG_QUEEN, 1);
		}
	}

	void BotKillerQueen::AttackWithQueens()
	{
		auto obs = Observation();

		auto queens = obs->GetUnits(IsUnit(UNIT_TYPEID::ZERG_QUEEN));

		auto targets = obs->GetUnits([](const Unit& unit)->bool {return unit.alliance == Unit::Alliance::Enemy;});

		auto start_locations = obs->GetGameInfo().enemy_start_locations;


		if (queens.empty())
		{
			return;
		}


		auto random_queen = GetRandomEntry(queens);


		if (obs->HasCreep(random_queen->pos) && (random_queen->orders.empty() || random_queen->orders.front().ability_id == ABILITY_ID::MOVE_MOVE || random_queen->orders.front().ability_id == ABILITY_ID::ATTACK))
		{
			auto dx = GetRandomScalar();
			auto dy = GetRandomScalar();

			auto offset = Point2D(random_queen->pos.x + dx, random_queen->pos.y + dy);

			auto tumors = obs->GetUnits(IsUnits{ { UNIT_TYPEID::ZERG_CREEPTUMOR, UNIT_TYPEID::ZERG_CREEPTUMOR, UNIT_TYPEID::ZERG_CREEPTUMORBURROWED, UNIT_TYPEID::ZERG_HATCHERY} });

			auto closest_tumor = get_closest_unit(obs, offset, tumors);
			auto distance = 0.0;

			if (closest_tumor)
			{
				distance = Distance2D(closest_tumor->pos, offset);
			}

			if (distance > 8)
			{
				Actions()->UnitCommand(random_queen, ABILITY_ID::BUILD_CREEPTUMOR_QUEEN, offset);
			}
		}

		if (targets.empty())
		{
			for (auto q : queens)
			{
				if (q->orders.empty())
				{
					auto target = GetRandomEntry(start_locations);

					Actions()->UnitCommand(q, ABILITY_ID::ATTACK, target);
				}
			}
		}
		else
		{
			for (auto q : queens)
			{
				if (q->orders.empty())
				{
					auto target = GetRandomEntry(targets);

					Actions()->UnitCommand(q, ABILITY_ID::ATTACK, target->pos);
				}

			}
		}
	}

	void BotKillerQueen::SpreadCreep()
	{
	}

	inline static string to_string(const Point2D& p2)
	{
		return "Point2D { x: " + std::to_string(p2.x) + ", y: " + std::to_string(p2.y) + " }";
	}


	void BotKillerQueen::distributeWorkers()
	{
		auto hatcheries = Observation()->GetUnits(IsTownHall{});

		for (const auto& source_hatchery : hatcheries)
		{
			if (source_hatchery->build_progress < 1.0)
			{
				continue;
			}

			if (source_hatchery->assigned_harvesters > source_hatchery->ideal_harvesters && source_hatchery->assigned_harvesters != 0)
			{
				for (const auto& target_hatch : hatcheries)
				{
					if (target_hatch->build_progress < 1.0)
					{
						continue;
					}

					if (target_hatch->assigned_harvesters >= target_hatch->ideal_harvesters)
					{
						continue;
					}

					if (target_hatch == source_hatchery)
					{
						// dont send to self
						continue;
					}

					const auto* closest_worker = get_closest_unit(Observation(), source_hatchery->pos, UNIT_TYPEID::ZERG_DRONE);

					if (!closest_worker)
					{
						cout << "WARN: could not find a worker close to " << source_hatchery->pos.x << ", " << source_hatchery->pos.y << ", " << source_hatchery->pos.z;
						return;
					}

					const auto* closest_min_patch = find_closest_mineral_patch(target_hatch->pos);

					if (!closest_min_patch)
					{
						cout << "WARN: Could not find a mineral patch close to " << to_string(*target_hatch);
						return;
					}

					cout << "Dist Workers: Moving worker " << to_string(*closest_worker) << " to " << to_string(*closest_min_patch) << endl;
					Actions()->UnitCommand(closest_worker, ABILITY_ID::SMART, &*closest_min_patch);
					Debug()->DebugTextOut(std::to_string(closest_worker->tag), closest_worker->pos, Colors::Yellow);
					Debug()->DebugSphereOut(closest_min_patch->pos, 1, Colors::Green);
					Debug()->DebugLineOut(closest_worker->pos, closest_min_patch->pos, Colors::Red);
					Debug()->SendDebug();
					//	Actions()->SendActions();
					return;
				}
			}
		}
	}


	const Unit* BotKillerQueen::find_closest_mineral_patch(const Point2D& pos)
	{
		auto f = [](const Unit& u) -> bool {
			return u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750 ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD750 ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD750 ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750 ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD ||
				u.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750;
			};


		Units units = Observation()->GetUnits(Unit::Alliance::Neutral, f);
		float distance = std::numeric_limits<float>::max();
		const Unit* target = nullptr;
		for (const auto& u : units) {
			if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
				float d = DistanceSquared2D(u->pos, pos);
				if (d < distance) {
					distance = d;
					target = u;
				}
			}
		}
		// If we never found one return false;
		if (distance == std::numeric_limits<float>::max()) {
			cout << "WARN: Could not find a mineral patch!" << endl;
			return nullptr;
		}

		return target;
	};


	void BotKillerQueen::sendWorkerToClosestMineralPatch(const Unit* worker)
	{
		if (worker->unit_type != UnitTypeID(UNIT_TYPEID::ZERG_DRONE))
		{
			cout << "WARN: Received invalid unit to distribute to mineral patch! Unit: " << to_string(*worker) << endl;
		}

		auto closest_min_patch = find_closest_mineral_patch(worker->pos);

		if (!closest_min_patch)
		{
			cout << "WARN: Could not find a mineral patch near " << to_string(*worker);
			return;
		}

		cout << "Idle Worker detected! Sending worker " << to_string(*worker) << " to " << to_string(closest_min_patch->pos) << endl;

		if (worker->orders.empty() || worker->orders[0].ability_id != ABILITY_ID::HARVEST_GATHER)
		{
			Actions()->UnitCommand(worker, ABILITY_ID::SMART, &*closest_min_patch);
		}
	}

	bool BotKillerQueen::TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
		const ObservationInterface* observation = Observation();

		// If we are at supply cap, don't build anymore units, unless its an overlord.
		if (observation->GetFoodUsed() >= observation->GetFoodCap() &&
			ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
			return false;
		}
		const Unit* unit = nullptr;
		if (!GetRandomUnit(unit, observation, unit_type)) {
			return false;
		}
		if (!unit->orders.empty()) {
			return false;
		}

		if (unit->build_progress != 1) {
			return false;
		}

		Actions()->UnitCommand(unit, ability_type_for_unit);
		return true;
	}



}