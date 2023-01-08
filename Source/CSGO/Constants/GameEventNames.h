#pragma once

namespace csgo
{

	#define GAMEEVENT_NAME(name) constexpr auto name = #name
	GAMEEVENT_NAME(player_death);		//event 0
	GAMEEVENT_NAME(player_hurt);		//event 1
	GAMEEVENT_NAME(round_start);		//event 2
	GAMEEVENT_NAME(bomb_planted);		//event 3
	GAMEEVENT_NAME(bomb_defused);		//event 4
	GAMEEVENT_NAME(bomb_exploded);		//event 5
	GAMEEVENT_NAME(player_radio);		//event 6
	GAMEEVENT_NAME(round_freeze_end);	//event 7
	GAMEEVENT_NAME(vote_cast);			//event 8
	GAMEEVENT_NAME(round_mvp);			//event 9
	GAMEEVENT_NAME(item_purchase);		//event 10
	GAMEEVENT_NAME(bullet_impact);		//event 11
	GAMEEVENT_NAME(weapon_fire);		//event 12
	GAMEEVENT_NAME(player_ping);		//event 13
	


	#undef GAMEEVENT_NAME

}
