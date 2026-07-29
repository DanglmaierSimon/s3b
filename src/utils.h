#pragma once

#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_client.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_utils.h"

#include "unit_data.h"
#include <sc2_unit_filters.h>

namespace sc2 {

std::string to_string(const sc2::Point2D &p) {
  std::string ret = "Point2D { x: " + std::to_string(p.x) + ", y: " + std::to_string(p.y) + "}";
  return ret;
}

inline Units get_pending_buildings(const ObservationInterface *obs, const sc2::Filter &filter) {
  auto f = [&filter](const Unit &unit) -> bool {
    return unit.is_alive && unit.build_progress < 1.0 && is_building(unit) && filter(unit);
  };
  return obs->GetUnits(f);
}

inline Units get_pending_buildings(const ObservationInterface *obs) {
  return get_pending_buildings(obs, [](const Unit &) -> bool { return true; });
}

inline const Unit *get_closest_unit(const Point2D &pos, const Units &units) {
  if (units.empty()) {
    return nullptr;
  }

  const Unit *closest = units.front();
  float       distance = DistanceSquared2D(closest->pos, pos);

  // start at one intentionally
  for (int i = 1; i < units.size(); i++) {
    auto candidate = units[i];
    auto d = DistanceSquared2D(candidate->pos, pos);
    if (d < distance) {
      distance = d;
      closest = candidate;
    }
  }

  return closest;
}

inline const Unit *get_closest_unit(const ObservationInterface *obs, const Point2D &pos, UNIT_TYPEID type) {
  Units       units = obs->GetUnits(sc2::IsUnit{type});
  float       distance = 0;
  const Unit *target = nullptr;
  for (const auto &u : units) {
    float d = DistanceSquared2D(u->pos, pos);
    if (!target) {
      distance = d;
      target = u;
    } else if (d < distance) {
      distance = d;
      target = u;
    }
  }

  return target;
}

inline const Unit *get_closest_unit(const ObservationInterface *obs, const Point2D &pos, sc2::Filter filter) {

  auto units = obs->GetUnits(Unit::Alliance::Self, filter);

  return get_closest_unit(pos, units);
}
} // namespace sc2