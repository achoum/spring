/* Author: Tobi Vollebregt */

/* based on code from GlobalSynced.{cpp,h} */

#ifndef PLAYERHANDLER_H
#define PLAYERHANDLER_H

#include <assert.h>

#include "creg/creg.h"
#include "Player.h"

class CGameSetup;


class CPlayerHandler
{
public:
	CR_DECLARE(CPlayerHandler);

	CPlayerHandler();
	~CPlayerHandler();

	void LoadFromSetup(const CGameSetup* setup);

	/**
	 * @brief Player
	 * @param i index to fetch
	 * @return CPlayer pointer
	 *
	 * Accesses a CPlayer instance at a given index
	 */
	CPlayer* Player(int i) { assert(unsigned(i) < players.size()); return &players[i]; }

	/**
	 * @brief Player
	 * @param name name of the player
	 * @return his playernumber of -1 if not found
	 *
	 * Search a player by name.
	 */
	int Player(const std::string& name) const;

	void PlayerLeft(int playernum, unsigned char reason);

	int ActivePlayers() const { return activePlayers; }
	int TotalPlayers() const { return players.size(); };

	void GameFrame(int frameNum);

private:

	/**
	 * @brief active players
	 *
	 * The number of active players
	 */
	int activePlayers;

	typedef std::vector<CPlayer> playerVec;
	/**
	 * @brief players
	 *
	 * for all the players in the game
	 */
	playerVec players;
};

extern CPlayerHandler* playerHandler;

#endif // !PLAYERHANDLER_H
