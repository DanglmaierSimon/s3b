#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS 1


#include <iostream>



#include "bot_examples.h"
#include "bot_kq.hh"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_gametypes.h"
#include "sc2utils/sc2_manage_process.h"

int main(int argc, char* argv[]) {
	sc2::Coordinator coordinator;
	if (!coordinator.LoadSettings(argc, argv)) {
		return 1;
	}

	coordinator.SetMultithreaded(false);
	coordinator.SetRealtime(true);

	// Add the custom bot, it will control the players.
	sc2::BotKillerQueen bot1;
	auto race = sc2::Race::Zerg;
	//sc2::TerranBot bot1;
	//auto race = sc2::Race::Terran;

	coordinator.SetUseGeneralizedAbilityId(true);

	coordinator.SetParticipants({
		CreateParticipant(race, &bot1),
		CreateComputer(sc2::Race::Random, sc2::Difficulty::VeryEasy, sc2::RandomBuild, "stupid computer"),
		});

	// Start the game.
	coordinator.LaunchStarcraft();

	const auto mapdir = "C:\\Program Files (x86)\\StarCraft II\\Maps\\";

	const char* map_pool[] = {
		"TorchesAIE_v4.SC2Map",
		"PersephoneAIE_v4.SC2Map",
		"Simple128.SC2Map",
		"Flat128.SC2Map" };

	auto pool_size = std::size(map_pool);

	int low_dist = 0;
	int high_dist = pool_size;
	std::srand((unsigned int)std::time(nullptr));
	auto selection = low_dist + std::rand() % (high_dist - low_dist);

	auto path = std::string(mapdir);
	path.append(map_pool[selection]);

	std::printf("Chosen map: %s\n", map_pool[selection]);

	bool do_break = false;
	while (!do_break) {
		if (!coordinator.StartGame(path)) {
			break;
		}
		while (coordinator.Update() && !do_break) {
			if (sc2::PollKeyPress()) {
				do_break = true;
			}
		}

		break;
	}

	bot1.Control()->DumpProtoUsage();

	return 0;
}
