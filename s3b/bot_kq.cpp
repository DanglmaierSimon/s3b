#include "bot_kq.hh"

#include "sc2api/sc2_unit_filters.h"
#include "utils.h"

void sc2::BotKillerQueen::OnGameStart()
{

}

void sc2::BotKillerQueen::OnStep()
{
	const auto* obs = Observation();

	this->iteration = obs->GetGameLoop();

	if (this->iteration < 10)
	{
		for (auto sl : obs->GetGameInfo().enemy_start_locations)
		{
			this->candidate_positions.insert(sl);
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

void sc2::BotKillerQueen::OnGameEnd()
{

}

void sc2::BotKillerQueen::OnUnitIdle(const Unit* unit)
{

}

void sc2::BotKillerQueen::PreventSupplyBlock(const sc2::ObservationInterface* obs)
{

	auto supply_used = obs->GetFoodUsed();
	auto supply_cap = obs->GetFoodCap();

	if (supply_used >= 200 || supply_cap >= 200)
	{
		return;
	}

	auto pending_overlords = get_pending_units(Observation(), IsUnit(UNIT_TYPEID::ZERG_OVERLORD));

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
}

void sc2::BotKillerQueen::ExpandIfPossible()
{
}

void sc2::BotKillerQueen::BuildWorkers()
{
}

void sc2::BotKillerQueen::BuildQueens()
{
}

void sc2::BotKillerQueen::AttackWithQueens()
{
}

void sc2::BotKillerQueen::SpreadCreep()
{
}
