#include "../bot_command.h"
#include "../zonedb.h"
#include "../zone.h"
#include "../spawn2.h"

void bot_command_spawn_info(Client* c, const Seperator* sep)
{
	if (helper_command_alias_fail(c, "bot_command_spawn_info", sep->arg[0], "spawninfo")) {
		c->Message(Chat::White, "note: Shows spawn2/spawngroup info for the targeted NPC, including PH/rare lineup by chance.");

		return;
	}

	if (helper_is_help_or_usage(sep->arg[1])) {
		BotCommandHelpParams p;

		p.description = { "Shows spawn2/spawngroup info for the targeted NPC." };
		p.notes = {
			"- Targets must be NPCs that spawned from spawn2.",
			"- Looks up spawngroup entries and their chances to infer PH/rare.",
			"- Reports remaining respawn time for this spawn2 if tracked."
		};
		p.example_format = { fmt::format("{} [actionable, default: target]", sep->arg[0]) };
		p.examples_one = { "Basic usage:", fmt::format("{}", sep->arg[0]) };
		p.actionables = { "target" };

		std::string popup_text = c->SendBotCommandHelpWindow(p);
		popup_text = DialogueWindow::Table(popup_text);
		c->SendPopupToClient(sep->arg[0], popup_text.c_str());
		return;
	}

	Mob* t = c->GetTarget();
	if (!t || !t->IsNPC()) {
		c->Message(Chat::Yellow, "You must target a spawned NPC.");
		return;
	}

	NPC* n = t->CastToNPC();
	uint32 spawn2_id = n->GetSpawnPointID();
	uint32 spawngroup_id = n->GetSpawnGroupId();
	if (!spawn2_id || !spawngroup_id) {
		c->Message(Chat::Yellow, "Target is not associated with spawn2/spawngroup (dynamic or quest spawn)." );
		return;
	}

	// Remaining respawn time (if available)
	uint32 remaining = database.GetSpawnTimeLeft(spawn2_id, zone->GetInstanceID());

	c->Message(
		Chat::White,
		fmt::format(
			"spawn2: {}  spawngroup: {}  npc: {} (id {})",
			spawn2_id,
			spawngroup_id,
			n->GetCleanName(),
			n->GetNPCTypeID()
		).c_str()
	);

	if (remaining > 0) {
		uint32 s = remaining % 60;
		uint32 m = (remaining / 60) % 60;
		uint32 h = (remaining / 3600);
		c->Message(Chat::White, fmt::format("respawn remaining: {}h {}m {}s", h, m, s).c_str());
	}

	// Fetch entries for this spawngroup directly via SQL for compactness
	auto results = content_db.QueryDatabase(
		fmt::format(
			"SELECT se.npcID, se.chance, nt.name FROM spawnentry se JOIN npc_types nt ON nt.id = se.npcID WHERE se.spawngroupID = {} ORDER BY se.chance DESC, se.npcID",
			spawngroup_id
		)
	);
	if (!results.Success() || results.RowCount() == 0) {
		c->Message(Chat::White, "No spawnentries found for this spawngroup.");
		return;
	}

	int rows = 0;
	bool has_variants = false;
	for (auto row = results.begin(); row != results.end(); ++row) {
		++rows;
		int npc_id = row[0] ? atoi(row[0]) : 0;
		int chance = row[1] ? atoi(row[1]) : 0;
		std::string name = row[2] ? row[2] : "";
		if (chance != 100) {
			has_variants = true;
		}
		// keep lines short for legacy client wrap
		c->Message(
			Chat::White,
			fmt::format(
				"- {} (npc {}) chance {}%{}",
				name,
				npc_id,
				chance,
				(npc_id == (int)n->GetNPCTypeID() ? "  [target]" : "")
			).c_str()
		);
	}

	if (rows > 1) {
		c->Message(Chat::White, has_variants ? "PH/rare lineup detected (multiple entries)." : "Multi-entry spawngroup." );
	} else {
		c->Message(Chat::White, "Static spawn (single 100% entry)." );
	}
}


