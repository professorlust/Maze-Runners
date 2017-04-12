#ifndef _BASIC_RULES_H
#define _BASIC_RULES_H

#include "../../Interfaces/ruleenforcer.h"
#include "../../types.h"
#include "../../Interfaces/player.h"
#include "../../Interfaces/backend_types.h"
#include<unordered_map>

class BasicRules : public RuleEnforcer<BasicPlayer, BasicPlayerData, MapTile, MazeSettings>
{
    std::unordered_map<BasicPlayer*, unsigned int> _playerIds;
    int playerCount = 0;

public:
    bool playerIsDifferent(const BasicPlayerData& before, const BasicPlayerData& after)
    {
        return (before.x != after.x) || (before.y != after.y);
    }
    MazeSettings getSettings(const maze<MapTile>& m);
    BasicPlayerData initPlayer(BasicPlayer* player, const maze<MapTile>& m);
    bool playerGetsTurn(BasicPlayerData playerData, const maze<MapTile>& m);
    bool playerIsDone(BasicPlayerData playerData, const maze<MapTile>& m);
};

#endif;