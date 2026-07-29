#include "bot_kq.hh"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ostream>
#include <random>
#include <string>

#include "utils.h"
#include <fstream>
#include <numeric>
#include <ostream>

#include "sc2_api.h"
#include "sc2_lib.h"
#include "sc2_search.h"
#include "sc2_unit_filters.h"

#include "utils.h"

using namespace std;
using namespace sc2;

namespace {

struct IsNotUnits {
  inline explicit IsNotUnits(const std::vector<sc2::UNIT_TYPEID> &types_) : m_types{types_} {}

  inline bool operator()(const sc2::Unit &unit_) const {
    for (auto t : m_types) {
      if (unit_.unit_type == t) {
        return false;
      }
    }

    return true;
  }

private:
  std::vector<sc2::UNIT_TYPEID> m_types;
};

struct IsMilitary {

  inline bool operator()(const sc2::Unit &unit_) const {

    return unit_.alliance == sc2::Unit::Alliance::Self &&
           IsNotUnits{
               {UNIT_TYPEID::ZERG_DRONE, UNIT_TYPEID::ZERG_EGG, UNIT_TYPEID::ZERG_LARVA, UNIT_TYPEID::ZERG_OVERLORD}
    }(unit_);
  }
};

constexpr float GEYSER_SEARCH_DISTANCE = 17.0f;

struct GasInfo {
  int wanted_geysers;
  int gas_workers;
};

constexpr GasInfo get_gas_info(int assigned_workers, int ideal_harvesters) {

  if (assigned_workers == ideal_harvesters) {
    return {.wanted_geysers = 2, .gas_workers = 6};
  }

  switch (assigned_workers) {
    // to early for gas
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
    return {.wanted_geysers = 0, .gas_workers = 0};

    // 1 gas
  case 9:
    return {.wanted_geysers = 1, .gas_workers = 1};
  case 10:
    return {.wanted_geysers = 1, .gas_workers = 1};
  case 11:
    return {.wanted_geysers = 1, .gas_workers = 1};
  case 12:
    return {.wanted_geysers = 1, .gas_workers = 2};
  case 13:
    return {.wanted_geysers = 1, .gas_workers = 3};
  // 2 gas
  case 14:
    return {.wanted_geysers = 2, .gas_workers = 4};
  case 15:
    return {.wanted_geysers = 2, .gas_workers = 5};
  case 16:
    return {.wanted_geysers = 2, .gas_workers = 6};

    // oversaturated
  default:
    return {.wanted_geysers = 2, .gas_workers = 6};
  }
}

static string to_string(sc2::Attribute a) {
  switch (a) {
  case sc2::Attribute::Light:
    return "Light(1)";
  case sc2::Attribute::Armored:
    return "Armored(2)";
  case sc2::Attribute::Biological:
    return "Biological(3)";
  case sc2::Attribute::Mechanical:
    return "Mechanical(4)";
  case sc2::Attribute::Robotic:
    return "Robotic(5)";
  case sc2::Attribute::Psionic:
    return "Psionic(6)";
  case sc2::Attribute::Massive:
    return "Massive(7)";
  case sc2::Attribute::Structure:
    return "Structure(8)";
  case sc2::Attribute::Hover:
    return "Hover(9)";
  case sc2::Attribute::Heroic:
    return "Heroic(10)";
  case sc2::Attribute::Summoned:
    return "Summoned(11)";
  case sc2::Attribute::Invalid:
    return "Invalid(12)";
  default:
    return string("Unknown(") + ::to_string(static_cast<int>(a)) + ")";
  }
}

static string to_string(sc2::Weapon w) {
  // TODO Rest of this stuff
  switch (w.type) {
  case sc2::Weapon::TargetType::Air:
    return "Air";
  case sc2::Weapon::TargetType::Ground:
    return "Ground";
  case sc2::Weapon::TargetType::Any:
    return "Any";
  default:
    return "Invalid";
  }
}

static string to_string(sc2::Race race) {
  switch (race) {
  case sc2::Race::Terran:
    return "Terran";
  case sc2::Race::Zerg:
    return "Zerg";
  case sc2::Race::Protoss:
    return "Protoss";
  case sc2::Race::Random:
    return "Random";
  default:
    return "Unknown";
  }
}

static string debugUnitTypeData(const sc2::UnitTypeData &ud) {

  string ret = "UnitTypeData {\n";
  ret += "  unit_type_id: " + std::to_string(ud.unit_type_id) + "\n";
  ret += "  name: " + (ud.name) + "\n";
  ret += "  available: " + std::to_string(ud.available) + "\n";
  ret += "  cargo_size: " + std::to_string(ud.cargo_size) + "\n";
  ret += "  mineral_cost: " + std::to_string(ud.mineral_cost) + "\n";
  ret += "  vespene_cost: " + std::to_string(ud.vespene_cost) + "\n";
  ret += "  attributes: [";
  for (auto a : ud.attributes) {
    ret += to_string(a) + ", ";
  }
  ret += "]\n";

  ret += "  movement_speed: " + std::to_string(ud.movement_speed) + "\n";
  ret += "  armor: " + std::to_string(ud.armor) + "\n";

  ret += "  weapons: [";
  for (const auto &w : ud.weapons) {
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
  for (auto ta : ud.tech_alias) {
    ret += std::to_string(ta) + ", ";
  }
  ret += "]\n";
  ret += "  unit_alias: " + std::to_string(ud.unit_alias) + "\n";
  ret += "  tech_requirement: " + std::to_string(ud.tech_requirement) + "\n";
  ret += "  require_attached: " + std::to_string(ud.require_attached) + "\n";

  ret += "}\n";

  return ret;
}

static string to_string(sc2::AbilityData::Target t) {
  switch (t) {
  case sc2::AbilityData::Target::None:
    return "None";
  case sc2::AbilityData::Target::Point:
    return "Point";
  case sc2::AbilityData::Target::Unit:
    return "Unit";
  case sc2::AbilityData::Target::PointOrUnit:
    return "PointOrUnit";
  case sc2::AbilityData::Target::PointOrNone:
    return "PointOrNone";
  default:
    return "Unknown";
  }
}

static string debugAbilityData(const sc2::AbilityData &a) {
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
  for (auto r : a.remaps_from_ability_id) {
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

bool GetRandomUnit(const sc2::Unit *&unit_out, const sc2::ObservationInterface *observation, sc2::UnitTypeID unit_type) {
  sc2::Units my_units = observation->GetUnits(sc2::Unit::Alliance::Self, sc2::IsUnit(unit_type));
  if (!my_units.empty()) {
    unit_out = GetRandomEntry(my_units);
    return true;
  }
  return false;
}

} // namespace

namespace sc2 {

void BotKillerQueen::OnGameFullStart() { cout << "OnGameFullStart()" << endl; }

void BotKillerQueen::OnGameStart() {
  cout << "OnGameStart()" << endl;

  auto params = search::ExpansionParameters();
  // params.circle_step_size_ = 0.25;
  // params.radiuses_ = { 2,3,4,5,6,7,9 };
  // params.cluster_distance_ = 14;
  params.debug_ = Debug();

  expansions_ = search::CalculateExpansionLocations(Observation(), Query(), params);
  Debug()->SendDebug();
  cout << "calculated expansion locations" << endl;

  auto start = Observation()->GetStartLocation();

  std::sort(expansions_.begin(), expansions_.end(), [start](const sc2::Point3D &l, const sc2::Point3D &r) {
    return DistanceSquared3D(start, l) < DistanceSquared3D(start, r);
  });
}

void BotKillerQueen::OnStep() {
  const auto *obs = Observation();

  if (obs->GetGameLoop() == 0) {
    stuffTodoOnGameStart();
  }

  if (Observation()->GetGameLoop() < 10) {
    for (auto sl : obs->GetGameInfo().enemy_start_locations) {
      // this->candidate_positions.insert(sl);
    }
  }

  PreventSupplyBlock(obs);

  BuildSpawnPoolIfPossible();

  BuildBuildingIfPossible(UNIT_TYPEID::ZERG_LAIR);

  BuildBuildingIfPossible(UNIT_TYPEID::ZERG_ROACHWARREN);

  ExpandIfPossible();

  BuildWorkers();

  BuildQueens();

  AttackWithQueens();

  SpreadCreep();

  buildGases();
}

static string to_string(const Unit &unit) {
  std::string r;

  r += "Unit [" + std::to_string(unit.tag) + "]: { type: " + UnitTypeToName(unit.unit_type) + " }";
  return r;
}

void BotKillerQueen::OnGameEnd() { cout << "OnGameEnd()" << endl; }

void BotKillerQueen::OnUnitIdle(const Unit *unit) {
  cout << "OnUnitIdle()" << endl;
  cout << "Idle Unit: " << to_string(*unit) << endl;

  if (IsUnit{UNIT_TYPEID::ZERG_DRONE}(*unit)) {
    sendWorkerToClosestMineralPatch(unit);
  }
}

void BotKillerQueen::OnUnitDestroyed(const Unit *unit) {
  cout << "OnUnitDestroyed()" << endl;
  cout << "Unit destroyed: " << to_string(*unit) << endl;
}

void BotKillerQueen::OnNeutralUnitCreated(const Unit *unit) {
  cout << "OnNeutralUnitCreated()" << endl;
  cout << "Neutral Unit created: " << to_string(*unit) << endl;
}

void BotKillerQueen::OnUnitCreated(const Unit *unit) {
  cout << "OnUnitCreated()" << endl;
  cout << "Unit created: " << to_string(*unit) << endl;
}

void BotKillerQueen::OnUpgradeCompleted(UpgradeID upgrade) {
  cout << "OnUpgradeCompleted()" << endl;
  cout << "Upgrade Completed: " << UpgradeIDToName(upgrade) << endl;
}

void BotKillerQueen::OnBuildingConstructionComplete(const Unit *unit) {
  cout << "OnBuildingConstructionComplete()" << endl;
  cout << "Building finished: " << to_string(*unit) << endl;
}

void BotKillerQueen::OnUnitDamaged(const Unit *unit, float health, float shields) {
  cout << "OnUnitDamaged()" << endl;
  cout << "Unit damaged: " << to_string(*unit) << "health: " << health << "; shields: " << shields << endl;
}

void BotKillerQueen::OnNydusDetected() {
  cout << "OnNydusDetected()" << endl;
  cout << "oh oh, not good" << endl;
}

void BotKillerQueen::OnNuclearLaunchDetected() {
  cout << "OnNuclearLaunchDetected()" << endl;
  cout << "RUN!" << endl;
}

void BotKillerQueen::OnUnitEnterVision(const Unit *unit) {
  cout << "OnUnitEnterVision()" << endl;
  cout << "Unit entered vision: " << to_string(*unit) << endl;
}

void BotKillerQueen::OnError(const std::vector<ClientError> &client_errors, const std::vector<std::string> &protocoll_errors) {
  cout << "OnError()" << endl;

  cout << "Received ERRORS!" << endl;

  for (const auto &ce : client_errors) {
    cout << "Client Error: " << static_cast<int>(ce) << endl;
  }

  for (const auto &pe : protocoll_errors) {
    cout << "Protocoll Error: " << pe << endl;
  }
}

void BotKillerQueen::stuffTodoOnGameStart() {
  auto obs = Observation();

  auto os = ofstream("data\\stuff.log");

  os << "Abilities:" << endl;
  for (const auto &a : obs->GetAbilityData()) {
    os << debugAbilityData(a) << endl;
  }
  os << "====================" << endl;

  os << "UnitTypes:" << endl;
  for (const auto &u : obs->GetUnitTypeData()) {
    os << debugUnitTypeData(u) << endl;
  }
  os << "====================" << endl;

  os << "Upgrades:" << endl;
  for (const auto &u : obs->GetUpgradeData()) {
    os << u.Log() << endl;
  }
  os << "====================" << endl;

  os << "Buffs:" << endl;
  for (const auto &b : obs->GetBuffData()) {
    os << b.Log() << endl;
  }
  os << "====================" << endl;

  os << "Effects:" << endl;
  for (const auto &e : obs->GetEffectData()) {
    os << e.Log() << endl;
  }
  os << "====================" << endl;

  os.flush();
  os.close();

  buildUnitInfo();
}

void BotKillerQueen::buildUnitInfo() {
  unitinfo_ = Observation()->GetUnitTypeData();
  abilities_ = Observation()->GetAbilityData();
  upgrades_ = Observation()->GetUpgradeData();
  buffs_ = Observation()->GetBuffData();
  effects_ = Observation()->GetEffectData();
}

void BotKillerQueen::PreventSupplyBlock(const ObservationInterface *obs) {

  auto supply_used = obs->GetFoodUsed();
  auto supply_cap = obs->GetFoodCap();

  if (supply_used >= 200 || supply_cap >= 200) {
    return;
  }

  auto units = obs->GetUnits(Unit::Alliance::Self);
  using namespace std;
  // for (auto u : units)
  //{
  //	cout << "Unit( " << u->tag << "):" << endl;
  //	cout << "  Type: " << UnitTypeToName(u->unit_type) << " (" <<
  // u->unit_type << ")" << endl; 	cout << "  Progress: " <<
  // u->build_progress << endl; 	cout << ")" << endl;
  // }

  auto pending_overlords = get_pending_units(Observation(), UNIT_TYPEID::ZERG_OVERLORD);
  // std::cout << "========================" << std::endl;

  if (pending_overlords.size() > 0) {
    return;
  }

  auto supply_left = obs->GetFoodCap() - obs->GetFoodUsed();

  if (supply_left < 5 && can_afford(obs, UNIT_TYPEID::ZERG_OVERLORD)) {
    train(obs, Actions(), Query(), UNIT_TYPEID::ZERG_OVERLORD);
  }
}

void BotKillerQueen::BuildBuildingIfPossible(UNIT_TYPEID unit) {

  auto obs = Observation();

  if (!can_afford(obs, unit)) {
    return;
  }

  if (get_pending_buildings(obs, IsUnit(unit)).size() > 0 || obs->GetUnits(Unit::Alliance::Self, IsUnit(unit)).size() > 0) {
    return;
  }

  auto ability_id = get_production_ability(unit);

  TryBuildOnCreep(ability_id, UNIT_TYPEID::ZERG_DRONE);
}

void sc2::BotKillerQueen::BuildSpawnPoolIfPossible() {
  auto obs = Observation();

  if (!can_afford(obs, UNIT_TYPEID::ZERG_SPAWNINGPOOL)) {
    return;
  }

  if (get_pending_buildings(obs, IsUnit(UNIT_TYPEID::ZERG_SPAWNINGPOOL)).size() > 0 ||
      obs->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_SPAWNINGPOOL)).size() > 0) {
    return;
  }

  TryBuildOnCreep(ABILITY_ID::BUILD_SPAWNINGPOOL, UNIT_TYPEID::ZERG_DRONE);
}

void sc2::BotKillerQueen::ExpandIfPossible() {

  if (!can_afford(Observation(), UNIT_TYPEID::ZERG_HATCHERY)) {
    return;
  }

  auto pending_hatches = get_pending_units(Observation(), UNIT_TYPEID::ZERG_HATCHERY);
  auto ready_hatches = get_ready_units(Observation(), IsTownHall{});

  if (pending_hatches.size() + ready_hatches.size() >= expansions_.size()) {
    return;
  }
  TryExpand(ABILITY_ID::BUILD_HATCHERY, UNIT_TYPEID::ZERG_DRONE);
}

void sc2::BotKillerQueen::BuildWorkers() {
  auto obs = Observation();
  auto hatcheries = get_ready_units(obs, IsTownHall{});
  auto hatch_count = hatcheries.size();
  auto larva_count = obs->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_LARVA)).size();
  auto pending_workers = get_pending_units(obs, UNIT_TYPEID::ZERG_DRONE).size();

  if (larva_count == 0 || pending_workers > hatch_count) {
    return;
  }

  auto calculated_workers = std::accumulate(hatcheries.cbegin(), hatcheries.cend(), 0, [](int val, const Unit *unit) {
    if (unit->alliance != Unit::Alliance::Self) {
      return val;
    }
    return unit->ideal_harvesters + val;
  });
  auto optimal_worker_count = std::min(calculated_workers, 70);
  auto current_workers = (int)obs->GetFoodWorkers();

  // todo: distribute workers evenly to bases
  auto larva = obs->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_LARVA));

  if (!larva.empty() && can_afford(obs, UNIT_TYPEID::ZERG_DRONE) && current_workers < optimal_worker_count) {
    if (train(obs, Actions(), Query(), UNIT_TYPEID::ZERG_DRONE)) {
      std::cout << "optimal worker count: " << optimal_worker_count << std::endl;
      std::cout << "current worker count: " << current_workers << std::endl;
      std::cout << "training worker..." << std::endl;
    }
  }

  distributeWorkers();

  // for (auto hatch : hatcheries) {
  //   if (hatch->assigned_harvesters > hatch->ideal_harvesters) {
  //     break;
  //   }
  // }
}

void BotKillerQueen::BuildQueens() {
  auto spawning_pools = Observation()
                            ->GetUnits([](const Unit &unit) -> bool {
                              return unit.alliance == Unit::Alliance::Self && unit.build_progress == 1.0 &&
                                     IsUnit{UNIT_TYPEID::ZERG_SPAWNINGPOOL}(unit);
                            })
                            .size();

  if (spawning_pools > 0 && can_afford(Observation(), UNIT_TYPEID::ZERG_QUEEN)) {
    auto pending_queens = get_pending_units(Observation(), UNIT_TYPEID::ZERG_QUEEN);
    auto finished_queens = Observation()->GetUnits(Unit::Alliance::Self, IsUnit{UNIT_TYPEID::ZERG_QUEEN});
    auto hatcheries = get_ready_units(Observation(), IsTownHall{});

    if (pending_queens >= hatcheries) {
      return;
    }

    train(Observation(), Actions(), Query(), UNIT_TYPEID::ZERG_QUEEN, 1);
  }
}

void BotKillerQueen::AttackWithQueens() {
  auto obs = Observation();

  auto queens = obs->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_QUEEN));

  auto targets = obs->GetUnits([](const Unit &unit) -> bool { return unit.alliance == Unit::Alliance::Enemy; });

  // intentional copy
  auto start_locations = obs->GetGameInfo().enemy_start_locations;

  if (queens.empty()) {
    return;
  }

  auto random_queen = GetRandomEntry(queens);

  if (obs->HasCreep(random_queen->pos) &&
      (random_queen->orders.empty() || random_queen->orders.front().ability_id == ABILITY_ID::MOVE_MOVE ||
       random_queen->orders.front().ability_id == ABILITY_ID::ATTACK)) {
    auto dx = GetRandomScalar();
    auto dy = GetRandomScalar();

    auto offset = Point2D(random_queen->pos.x + dx, random_queen->pos.y + dy);

    auto tumors = obs->GetUnits(Unit::Alliance::Self,
                                IsUnits{
                                    {UNIT_TYPEID::ZERG_CREEPTUMORQUEEN,
                                     UNIT_TYPEID::ZERG_CREEPTUMOR,
                                     UNIT_TYPEID::ZERG_CREEPTUMORBURROWED,
                                     UNIT_TYPEID::ZERG_HATCHERY,
                                     UNIT_TYPEID::ZERG_HIVE,
                                     UNIT_TYPEID::ZERG_LAIR}
    });

    auto closest_tumor = get_closest_unit(offset, tumors);
    auto distance = 0.0;

    if (closest_tumor) {
      distance = Distance2D(closest_tumor->pos, offset);
    }

    ABILITY_ID tumor_prod_ability = get_production_ability(UNIT_TYPEID::ZERG_CREEPTUMORQUEEN);

    auto general_id = static_cast<ABILITY_ID>(abilities_.at((int)tumor_prod_ability).remaps_to_ability_id);

    if (distance > 8 && can_cast(random_queen, general_id)) {
      Actions()->UnitCommand(random_queen, general_id, offset);
    }
  }

  // dont attack with too few queens
  if (queens.size() < 10) {
    return;
  }

  if (targets.empty()) {
    for (auto q : queens) {
      if (q->orders.empty()) {
        auto target = GetRandomEntry(start_locations);

        Actions()->UnitCommand(q, ABILITY_ID::ATTACK, target);
      }
    }
  } else {
    for (auto q : queens) {
      if (q->orders.empty()) {
        auto target = GetRandomEntry(targets);

        Actions()->UnitCommand(q, ABILITY_ID::ATTACK, target->pos);
      }
    }
  }
}

void BotKillerQueen::SpreadCreep() {

  auto tumors = Observation()->GetUnits(
      Unit::Alliance::Self,
      IsUnits{
          {UNIT_TYPEID::ZERG_CREEPTUMOR, UNIT_TYPEID::ZERG_CREEPTUMORBURROWED, UNIT_TYPEID::ZERG_CREEPTUMORQUEEN}
  });

  if (tumors.empty()) {
    return;
  }

  int tumorcount = 0;

  while (true) {

    auto t = GetRandomEntry(tumors);

    if (tumorcount >= tumors.size()) {
      break;
    }
    tumorcount++;
    if (t->build_progress < 1.0) {
      continue;
    }

    auto abilites = Query()->GetAbilitiesForUnit(t, false, false);

    if (abilites.abilities.empty()) {
      continue;
    }

    for (auto a : abilites.abilities) {
      cout << "Available ability: " << AbilityTypeToName(a.ability_id) << " [" << a.ability_id.to_string() << "]" << endl;
    }

    cout << t->tag << endl;

    auto p = t->pos + Point3D(0, 0, 2);

    constexpr auto CHECK_DISTANCE = 12.0f;
    constexpr auto CAST_DISTANCE = 10.0f;

    // i am ashamed of this shit
    std::array<int, 4> indizes = {0, 1, 2, 3};

    std::array<Point3D, 4> directions = {p + Point3D(CHECK_DISTANCE, 0, 0),
                                         p + Point3D(0, CHECK_DISTANCE, 0),
                                         p + Point3D(-CHECK_DISTANCE, 0, 0),
                                         p + Point3D(0, -CHECK_DISTANCE, 0)};

    // TODO: become smart enough to figure out Vector arithmetic to create
    // a point exactly 10 Units away in the given direction
    // TODO: Figure out a way to not always make it the 4 cardinal directions but rotate
    // the 4 search "arms" randomly
    // to do this, use this formula to translate the vector to [0,0], rotate by an angle and then
    // translate back:
    // x_rotated = ((x - dx) * cos(angle)) - ((dy - y) * sin(angle)) + dx
    // y_rotated = dy - ((dy - y) * cos(angle)) + ((x - dx) * sin(angle

    std::array<Point2D, 4> target_points = {p + Point3D(CAST_DISTANCE, 0, 0),
                                            p + Point3D(0, CAST_DISTANCE, 0),
                                            p + Point3D(-CAST_DISTANCE, 0, 0),
                                            p + Point3D(0, -CAST_DISTANCE, 0)};

    int counter = 0;

    while (true) {
      auto index = GetRandomEntry(indizes);

      auto direction = directions.at(index);
      auto target_point = target_points.at(index);

      if (!Observation()->HasCreep(direction)) {

        Debug()->DebugLineOut(t->pos, Point3D{target_point.x, target_point.y, t->pos.z}, Colors::Purple);
        Debug()->DebugSphereOut(Point3D{target_point.x, target_point.y, t->pos.z}, 1, Colors::Blue);
        Debug()->SendDebug();

        if (Query()->Placement(ABILITY_ID::BUILD_CREEPTUMOR_TUMOR, target_point)) {
          Actions()->UnitCommand(t, ABILITY_ID::BUILD_CREEPTUMOR_TUMOR, target_point, false);
        }

        return;
      } else {
        counter += 1;
      }

      if (counter >= 4) {
        break;
      }
    }
  }
}

void BotKillerQueen::distributeWorkers() {
  auto hatcheries = Observation()->GetUnits(Unit::Alliance::Self, IsTownHall{});

  for (const auto &source_hatchery : hatcheries) {
    if (source_hatchery->build_progress < 1.0) {
      continue;
    }

    // distribute workers to gas extractors
    auto gas_info = get_gas_info(source_hatchery->assigned_harvesters, source_hatchery->ideal_harvesters);
    auto extractor_filter = IsUnits{
        {UNIT_TYPEID::ZERG_EXTRACTORRICH, UNIT_TYPEID::ZERG_EXTRACTOR}
    };

    auto extractors = get_units_closer_than(Unit::Alliance::Self, extractor_filter, source_hatchery->pos, GEYSER_SEARCH_DISTANCE);

    auto total_gas_miners = 0;

    for (auto ex : extractors) {
      total_gas_miners += ex->assigned_harvesters;
    }

    if (total_gas_miners < gas_info.gas_workers) {
      for (auto ex : extractors) {
        if (ex->build_progress < 1.0) {
          continue;
        }

        if (ex->assigned_harvesters < 3) {

          auto close_workers = get_units_closer_than(Unit::Alliance::Self, UNIT_TYPEID::ZERG_DRONE, source_hatchery->pos, 4);

          if (close_workers.empty()) {
            continue;
          }

          auto worker = GetRandomEntry(close_workers);

          if (worker) {
            if (!IsCarryingMinerals(*worker)) {

              Actions()->UnitCommand(worker, ABILITY_ID::SMART, ex);
            }
            return;
          }
        }
      }
    }

    // distribute workers to other bases
    if (source_hatchery->assigned_harvesters > source_hatchery->ideal_harvesters && source_hatchery->assigned_harvesters != 0) {
      for (const auto &target_hatch : hatcheries) {
        if (target_hatch->build_progress < 1.0) {
          continue;
        }

        if (target_hatch->assigned_harvesters >= target_hatch->ideal_harvesters) {
          continue;
        }

        if (target_hatch == source_hatchery) {
          // dont send to self
          continue;
        }

        const auto *closest_worker = get_closest_unit(Observation(), source_hatchery->pos, UNIT_TYPEID::ZERG_DRONE);

        if (!closest_worker) {
          cout << "WARN: could not find a worker close to " << source_hatchery->pos.x << ", " << source_hatchery->pos.y << ", "
               << source_hatchery->pos.z;
          return;
        }

        const auto *closest_min_patch = find_closest_mineral_patch(target_hatch->pos);

        if (!closest_min_patch) {
          cout << "WARN: Could not find a mineral patch close to " << to_string(*target_hatch);
          return;
        }

        cout << "Dist Workers: Moving worker " << to_string(*closest_worker) << " to " << to_string(*closest_min_patch) << endl;
        Actions()->UnitCommand(closest_worker, ABILITY_ID::SMART, &*closest_min_patch);
        // Debug()->DebugTextOut(std::to_string(closest_worker->tag), closest_worker->pos, Colors::Yellow);
        // Debug()->DebugSphereOut(closest_min_patch->pos, 1, Colors::Green);
        // Debug()->DebugLineOut(closest_worker->pos, closest_min_patch->pos, Colors::Red);
        // Debug()->SendDebug();
        //	Actions()->SendActions();
        return;
      }
    }
  }
}

const Unit *BotKillerQueen::find_closest_mineral_patch(const Point2D &pos) {
  auto f = [](const Unit &u) -> bool {
    return u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD || u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
           u.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD || u.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750 ||
           u.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD ||
           u.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD750 ||
           u.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD ||
           u.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD750 ||
           u.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD || u.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750 ||
           u.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD ||
           u.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750;
  };

  Units       units = Observation()->GetUnits(Unit::Alliance::Neutral, f);
  float       distance = std::numeric_limits<float>::max();
  const Unit *target = nullptr;
  for (const auto &u : units) {
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

void BotKillerQueen::sendWorkerToClosestMineralPatch(const Unit *worker) {
  if (worker->unit_type != UnitTypeID(UNIT_TYPEID::ZERG_DRONE)) {
    cout << "WARN: Received invalid unit to distribute to mineral patch! Unit: " << to_string(*worker) << endl;
  }

  auto closest_min_patch = find_closest_mineral_patch(worker->pos);

  if (!closest_min_patch) {
    cout << "WARN: Could not find a mineral patch near " << to_string(*worker);
    return;
  }

  cout << "Idle Worker detected! Sending worker " << to_string(*worker) << " to " << to_string(closest_min_patch->pos) << endl;

  if (worker->orders.empty() || worker->orders[0].ability_id != ABILITY_ID::HARVEST_GATHER) {
    Actions()->UnitCommand(worker, ABILITY_ID::SMART, &*closest_min_patch);
  }
}

void BotKillerQueen::buildGases() {
  auto bases = Observation()->GetUnits(Unit::Alliance::Self, IsTownHall{});

  for (auto base : bases) {

    if (!base->IsBuildFinished()) {
      continue;
    }
    auto gas_info = get_gas_info(base->assigned_harvesters, base->ideal_harvesters);
    // build new gases

    auto filter = IsUnits{
        {UNIT_TYPEID::NEUTRAL_VESPENEGEYSER,
         UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER,
         UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER,
         UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER,
         UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER,
         UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER,
         UNIT_TYPEID::NEUTRAL_VESPENEGEYSER}
    };

    auto geysers = get_units_closer_than(Unit::Alliance::Neutral, filter, base->pos, GEYSER_SEARCH_DISTANCE);

    if (geysers.size() > 2) {
      cout << "WARN: Found more than 2 gases at " << to_string(base->pos) << endl;
      return;
    }

    if (geysers.empty()) {
      continue;
    }

    int free_gases = 0;

    for (auto g : geysers) {
      if (!Query()->Placement(ABILITY_ID::BUILD_EXTRACTOR, g->pos)) {
        // extractor there already
        continue;
      }
      free_gases += 1;
    }

    auto extractors_built = 2 - free_gases;

    if (gas_info.wanted_geysers > extractors_built) {
      auto pos = (base->pos);
      cout << "Base at " << to_string(base->pos) << " has only " << extractors_built << " gases, but want "
           << gas_info.wanted_geysers << endl;
      buildGasAtBase(pos);
    }
  }
}

void BotKillerQueen::buildGasAtBase(sc2::Point3D &base_location) {

  auto filter = IsUnits{
      {UNIT_TYPEID::NEUTRAL_VESPENEGEYSER,
       UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER,
       UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER,
       UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER,
       UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER,
       UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER,
       UNIT_TYPEID::NEUTRAL_VESPENEGEYSER}
  };

  auto geysers = Observation()->GetUnits(Unit::Alliance::Neutral, filter);

  const Unit *closest_geyser = nullptr;
  float       closest_distance = 100000;

  for (auto g : geysers) {

    auto distance = Distance3D(base_location, g->pos);

    if (distance < GEYSER_SEARCH_DISTANCE && distance < closest_distance) {
      if (Query()->Placement(ABILITY_ID::BUILD_EXTRACTOR, g->pos)) {
        // close enough and can actually place something there
        closest_distance = distance;
        closest_geyser = g;
      }
    }
  }

  if (closest_geyser == nullptr) {
    cout << "WARN: Couldnt find a suitable geyser in range of base! Base Location: " << to_string(base_location)
         << "; Search radius: " << GEYSER_SEARCH_DISTANCE << endl;
    return;
  }

  if (can_afford(Observation(), UNIT_TYPEID::ZERG_EXTRACTOR)) {

    auto ability = get_production_ability(UNIT_TYPEID::ZERG_EXTRACTOR);

    Units workers = Observation()->GetUnits(Unit::Alliance::Self, sc2::IsUnit(UNIT_TYPEID::ZERG_DRONE));

    auto worker = GetRandomEntry(workers);

    // TODO: make function to build extractors
    Actions()->UnitCommand(worker, ability, closest_geyser);
  }
}

Units BotKillerQueen::get_units_closer_than(Unit::Alliance alliance, const Filter &f, const Point2D &pos, float distance) {

  auto filter = [&f, distance, &pos](const Unit &unit) -> bool { return f(unit) && Distance2D(unit.pos, pos) < distance; };

  return Observation()->GetUnits(alliance, filter);
}

Units BotKillerQueen::get_units_closer_than(Unit::Alliance alliance, UNIT_TYPEID type, const Point2D &pos, float distance) {
  return get_units_closer_than(alliance, IsUnit{type}, pos, distance);
}

bool BotKillerQueen::can_cast(const Unit *unit, ABILITY_ID id) {
  auto available_abilities = Query()->GetAbilitiesForUnit(unit, false, true);

  for (const auto &aa : available_abilities.abilities) {
    if (aa.ability_id == id) {
      return true;
    }
  }
  return false;
}

ABILITY_ID BotKillerQueen::get_production_ability(UNIT_TYPEID unit) const {
  auto &unitinfo = unitinfo_.at(static_cast<int>(unit));
  auto  id = unitinfo.ability_id;

  if (id == ABILITY_ID::INVALID) {
    cout << "WARN: " << "Unit " << (UnitTypeID(id)) << " does not have a production_ability!" << endl;
    return id;
  }

  //  cout << "DEBUG: " << "Unit " << (UnitTypeID(id)) << " is produced by Ability " << AbilityID(id) << endl;
  return id;
}

std::vector<UNIT_TYPEID> BotKillerQueen::get_requirements(UNIT_TYPEID unit) const {
  std::vector<UNIT_TYPEID> ret;

  UNIT_TYPEID unit_to_check = unit;
  while (true) {
    auto &unitinfo = unitinfo_.at(static_cast<int>(unit_to_check));

    if (static_cast<int>(unitinfo.tech_requirement) == 0) {
      break;
    } else {
      ret.push_back(unitinfo.tech_requirement);
    }

    unit_to_check = unitinfo.tech_requirement;
  }

  std::reverse(ret.begin(), ret.end());

  return ret;
}

bool BotKillerQueen::TryBuildOnCreep(AbilityID ability_type_for_structure, UnitTypeID unit_type) {
  float       rx = GetRandomScalar();
  float       ry = GetRandomScalar();
  const auto *observation = Observation();

  Filter base_filter = [](const Unit &unit) -> bool { return unit.IsBuildFinished() && IsTownHall{}(unit); };

  auto bases = Observation()->GetUnits(Unit::Alliance::Self, base_filter);

  if (bases.empty()) {
    cout << "WARN: Could not find any finished bases, we probably have other problems then..." << endl;
    return false;
  }

  auto start_location = GetRandomEntry(bases);

  // Try 10 times to find a suitable build location
  for (int i = 0; i < 10; i++) {
    Point3D build_location = Point3D(start_location->pos.x + rx * 15, start_location->pos.y + ry * 15, start_location->pos.z);

    auto building_filter = [](const Unit &unit) -> bool { return unit.alliance == Unit::Alliance::Self && IsBuilding{}(unit); };

    auto closest_other_building = get_closest_unit(Observation(), build_location, building_filter);

    if (closest_other_building && (Distance2D(closest_other_building->pos, build_location) < 4)) {
      // dont build too close to other buildings
      continue;
    }

    if (observation->HasCreep(build_location)) {
      return TryBuildStructure(ability_type_for_structure, unit_type, build_location);
    }
    return false;
  }

  return false;
}

bool BotKillerQueen::TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
  const ObservationInterface *observation = Observation();

  // If we are at supply cap, don't build anymore units, unless its an overlord.
  if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
    return false;
  }
  const Unit *unit = nullptr;
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

} // namespace sc2