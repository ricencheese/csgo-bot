#pragma once

// https://github.com/SteamDatabase/Protobufs/blob/master/csgo/cstrike15_usermessages.proto

namespace csgo
{

    enum class UserMessageType {
        SayText2 = 6,
        Text = 7,
        VoteStart = 46,
        VotePass = 47,
        VoteFailed = 48,
    };

}
