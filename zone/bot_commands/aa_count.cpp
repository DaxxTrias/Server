#include "../bot_command.h"

void bot_command_aa_count(Client* c, const Seperator* sep)
{
	if (helper_command_alias_fail(c, "bot_command_aa_count", sep->arg[0], "aacount")) {
		c->Message(Chat::White, "note: Estimates how many AA ranks your bots qualify for at current or specified level. This does not change AAs.");

		return;
	}

	if (helper_is_help_or_usage(sep->arg[1])) {
		BotCommandHelpParams p;

		p.description = { "Estimates how many AA ranks your bots qualify for at current or specified level. This does not modify AAs." };
		p.example_format = {
			fmt::format("{} [level optional] [actionable, default: target]", sep->arg[0])
		};
		p.examples_one = {
			"Count for your targeted bot at its current level:",
			fmt::format("{}", sep->arg[0])
		};
		p.examples_two = {
			"Project counts at level 60 for all spawned bots:",
			fmt::format("{} 60 spawned", sep->arg[0])
		};
		p.examples_three = {
			"Project counts at level 55 for clerics only:",
			fmt::format("{} 55 byclass {}", sep->arg[0], Class::Cleric)
		};
		p.actionables = { "target, byname, ownergroup, ownerraid, targetgroup, namesgroup, healrotationtargets, mmr, byclass, byrace, spawned" };

		std::string popup_text = c->SendBotCommandHelpWindow(p);
		popup_text = DialogueWindow::Table(popup_text);

		c->SendPopupToClient(sep->arg[0], popup_text.c_str());

		return;
	}

	int ab_arg = 1;
	uint8 level_override = 0; // 0 means use bot's current level

	if (sep->IsNumber(1)) {
		level_override = static_cast<uint8>(std::clamp(Strings::ToInt(sep->arg[1]), 1, 255));
		++ab_arg;
	}

	const int ab_mask = ActionableBots::ABM_Type1;
	std::string actionable_arg = sep->arg[ab_arg];
	if (actionable_arg.empty()) {
		actionable_arg = "target";
	}

	std::string class_race_arg = sep->arg[ab_arg];
	bool class_race_check = false;
	if (!class_race_arg.compare("byclass") || !class_race_arg.compare("byrace")) {
		class_race_check = true;
	}

	std::vector<Bot*> sbl;
	if (ActionableBots::PopulateSBL(c, actionable_arg, sbl, ab_mask, !class_race_check ? sep->arg[ab_arg + 1] : nullptr, class_race_check ? atoi(sep->arg[ab_arg + 1]) : 0) == ActionableBots::ABT_None) {
		return;
	}

	uint64 total = 0;
	uint32 counted = 0;

	for (auto* b : sbl) {
		if (!b) {
			continue;
		}

		uint8 lvl = level_override ? level_override : b->GetLevel();
		uint32 bot_total = 0;

		for (const auto& ability_pair : zone->aa_abilities) {
			auto* ability = ability_pair.second.get();
			if (!ability || !ability->first || ability->charges > 0) {
				continue; // skip expendables or malformed
			}

			AA::Rank* current = ability->first;
			if (current->level_req > lvl) {
				continue;
			}

			uint32 points = 0;
			while (current) {
				if (current->level_req > lvl || !b->CanUseAlternateAdvancementRank(current)) {
					current = nullptr;
				} else {
					current = current->next;
					++points;
				}
			}

			bot_total += points;
		}

		total += bot_total;
		++counted;

		c->Message(
			Chat::White,
			fmt::format(
				"{} says, 'At level {}, I qualify for {} AA ranks.'",
				b->GetCleanName(),
				static_cast<int>(lvl),
				bot_total
			).c_str()
		);
	}

	if (counted > 1) {
		c->Message(
			Chat::White,
			fmt::format(
				"{} bots total: {} AA ranks (avg {}).",
				counted,
				total,
				(total / counted)
			).c_str()
		);
	}
}


