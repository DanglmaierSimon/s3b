import itertools
import logging
from random import randint, random, randrange, shuffle

from sc2 import unit_command
from sc2.bot_ai import BotAI
from sc2.data import Result
from sc2.ids.unit_typeid import UnitTypeId
from sc2.position import Point2
from sc2.units import Units
from sc2.ids.ability_id import AbilityId


class CompetitiveBot(BotAI):
    """Main bot class that handles the game logic."""

    iteration: int
    candidate_positions: set[Point2]
    target_list_empty_previously: bool

    def __init__(self):
        super().__init__()
        self.target_list_empty_previously = True
        self.iteration = 0

    def get_hatcheries_ready(self) -> Units:
        return self.structures.filter(
            lambda structure: structure.type_id == UnitTypeId.HATCHERY
            and structure.is_ready
        )

    async def on_step(self, iteration: int):

        self.iteration = iteration

        if iteration < 10:
            self.candidate_positions = set(self.enemy_start_locations)

        self.prevent_supply_block()

        await self.build_spawning_pool_if_possible()

        await self.expand_if_possible()

        await self.build_workers()

        self.build_queens()

        await self.attack_with_queens()

        await self.spread_creep()

    async def spread_creep(self):
        tumors = (self.structures).filter(
            lambda structure: structure.type_id
            in [
                UnitTypeId.CREEPTUMORQUEEN,
                UnitTypeId.CREEPTUMORBURROWED,
                UnitTypeId.CREEPTUMORMISSILE,
            ]
        )

        candidate_positions = self.candidate_positions

        for tumor in tumors:
            if not tumor.is_ready:
                continue

            if tumor.is_using_ability(AbilityId.BUILD_CREEPTUMOR):
                continue

            can_cast = await self.can_cast(
                tumor,
                AbilityId.BUILD_CREEPTUMOR_TUMOR,
                only_check_energy_and_cooldown=True,
            )

            if not can_cast:
                continue

            clist = list(candidate_positions)

            shuffle(clist)

            dist = randrange(6, 8)

            position_towards_enemy_or_friendly_base = (
                tumor.position.towards_with_random_angle(
                    clist[0], distance=dist
                ).rounded
            )

            # funktioniert
            fp1 = await self.find_placement(
                AbilityId.BUILD_CREEPTUMOR_TUMOR,
                position_towards_enemy_or_friendly_base,
                max_distance=4,
                random_alternative=True,
            )

            # funktioniert
            fp2 = await self.find_placement(
                AbilityId.BUILD_CREEPTUMOR_QUEEN,
                position_towards_enemy_or_friendly_base,
                max_distance=4,
                random_alternative=True,
            )
            # funktioniert
            can_place_a = await self.can_place(
                AbilityId.BUILD_CREEPTUMOR_TUMOR,
                [position_towards_enemy_or_friendly_base],
            )

            # funktioniert
            can_place_b = await self.can_place(
                AbilityId.BUILD_CREEPTUMOR_QUEEN,
                [position_towards_enemy_or_friendly_base],
            )

            self.do(
                unit_command.UnitCommand(
                    AbilityId.BUILD_CREEPTUMOR,
                    tumor,
                    position_towards_enemy_or_friendly_base,
                    True,
                )
            )

    async def attack_with_queens(self):
        queens = self.units.filter(lambda unit: unit.type_id == UnitTypeId.QUEEN)

        targets = self.enemy_structures + self.enemy_units

        if len(targets) == 0:
            self.target_list_empty_previously = True
            for queen in queens:
                if queen.is_active:
                    continue

                for loc in self.enemy_start_locations:
                    queen.attack(loc, True)
        else:
            if self.target_list_empty_previously:
                self.target_list_empty_previously = False
                for queen in queens:
                    queen.stop()

        tumors = (
            (self.structures)
            .filter(
                lambda structure: structure.type_id
                in [
                    UnitTypeId.CREEPTUMORQUEEN,
                    UnitTypeId.CREEPTUMORBURROWED,
                    UnitTypeId.CREEPTUMORMISSILE,
                ]
            )
            .amount
        )

        for queen in queens:

            if queen.is_attacking:
                continue

            has_creep = self.has_creep(queen.position)

            if tumors < 100:
                creep_position = await self.find_placement(
                    AbilityId.BUILD_CREEPTUMOR_TUMOR,
                    queen.position,
                    max_distance=1,
                    random_alternative=True,
                )

                is_not_using = not queen.is_using_ability(
                    AbilityId.BUILD_CREEPTUMOR_QUEEN
                )

                if has_creep and is_not_using and creep_position is not None:
                    self.do(
                        unit_command.UnitCommand(
                            AbilityId.BUILD_CREEPTUMOR_QUEEN,
                            queen,
                            creep_position,
                            False,
                        ),
                        False,
                        False,
                        False,
                        False,
                    )

            es = targets.random_or(None)
            if es:
                self.target_list_empty_previously = False
                if not queen.is_attacking:
                    queen.attack(es.position)

    def build_queens(self):
        hatcheries = self.get_hatcheries_ready()

        sp_count = self.structures.filter(
            lambda structure: structure.type_id == UnitTypeId.SPAWNINGPOOL
            and structure.is_ready
        ).amount

        if sp_count == 0:
            return

        hatch_count = hatcheries.amount

        pending = (int)(self.already_pending(UnitTypeId.QUEEN))

        self.train(
            UnitTypeId.QUEEN,
            max(0, hatch_count - pending),
            train_only_idle_buildings=True,
        )

        # for hatch in hatcheries:
        #     if self.can_afford(UnitTypeId.QUEEN, True):
        #         hatch.build(UnitTypeId.QUEEN, queue=False, can_afford_check=False)

    def prevent_supply_block(self):
        if self.supply_used == 200 or self.supply_cap == 200:
            return

        if self.already_pending(UnitTypeId.OVERLORD) > 0:
            return

        if self.supply_left < 5 and self.can_afford(UnitTypeId.OVERLORD):
            self.train(UnitTypeId.OVERLORD)

    async def expand_if_possible(self):

        if await self.get_next_expansion() is None:
            return

        # if self.already_pending(UnitTypeId.HATCHERY) > 0:
        #     return

        hatch_count = self.structures.filter(
            lambda structure: structure.type_id == UnitTypeId.HATCHERY
            and structure.is_ready
        ).amount

        if hatch_count >= len(self.expansion_locations_list):
            return

        if not self.can_afford(UnitTypeId.HATCHERY):
            return

        await self.expand_now(UnitTypeId.HATCHERY)

    async def build_workers(self):

        hatch_count = self.structures.filter(
            lambda structure: structure.type_id == UnitTypeId.HATCHERY
            and structure.is_ready
        ).amount

        if (
            self.larva.amount == 0
            or self.already_pending(UnitTypeId.DRONE) >= hatch_count
        ):
            return

        optimal_worker_count = min(hatch_count * 16, 60)

        cur_workers = self.workers.amount
        await self.simple_dist_workers()

        larva = self.larva.random_or(None)
        if (
            larva
            and self.can_afford(UnitTypeId.DRONE, True)
            and cur_workers < optimal_worker_count
        ):
            logging.debug(f"optimal worker count: {optimal_worker_count}")
            logging.debug(f"current worker count: {cur_workers}")
            logging.debug("training worker...")
            larva.train(UnitTypeId.DRONE)

    async def simple_dist_workers(self):
        hatcheries = self.structures(UnitTypeId.HATCHERY)

        for hatch in hatcheries:
            if (
                hatch.assigned_harvesters > 16
                or hatch.ideal_harvesters < hatch.assigned_harvesters
            ):
                await self.distribute_workers(0)
                return

    async def build_spawning_pool_if_possible(self):
        spawning_pools = self.structures.filter(
            lambda structure: structure.type_id == UnitTypeId.SPAWNINGPOOL
            and structure.is_ready
        )

        if not self.can_afford(UnitTypeId.SPAWNINGPOOL, False):
            # too poor
            return

        if self.already_pending(UnitTypeId.SPAWNINGPOOL) + spawning_pools.amount == 0:
            logging.debug("trying to build spawning pool")
            worker_candidates = self.workers.filter(
                lambda worker: (worker.is_collecting or worker.is_idle)
                and worker.tag not in self.unit_tags_received_action
            )
            # Worker_candidates can be empty
            if worker_candidates:
                map_center = self.game_info.map_center
                position_towards_map_center = self.start_location.towards(
                    map_center, distance=5
                )
                placement_position = await self.find_placement(
                    UnitTypeId.SPAWNINGPOOL,
                    near=position_towards_map_center,
                    placement_step=1,
                )
                # Placement_position can be None
                if placement_position:
                    build_worker = worker_candidates.closest_to(placement_position)
                    build_worker.build(UnitTypeId.SPAWNINGPOOL, placement_position)
                else:
                    logging.warning("could not find a building position")
            else:
                logging.warning("no available workers, not building spawning pool")
