#pragma once

#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS 1

#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_utils.h"

template<>
struct std::hash<sc2::Point2D>
{
	std::size_t operator()(const sc2::Point2D& p) const noexcept
	{
		auto hx = std::hash<double>{}(p.x);
		auto hy = std::hash<double>{}(p.y);

		return std::hash<double>{}(hx ^ hy);
	}
};

namespace sc2 {
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
		std::unordered_set<sc2::Point2D> candidate_positions;
		bool target_list_empty_previously = true;
	};


}