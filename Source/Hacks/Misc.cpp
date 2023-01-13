#include <algorithm>
#include <array>
#include <iomanip>
#include <mutex>
#include <numbers>
#include <numeric>
#include <sstream>
#include <vector>

#include <imgui/imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "../ConfigStructs.h"
#include "../InputUtil.h"
#include "../Memory.h"
#include "../ProtobufReader.h"

#include "EnginePrediction.h"
#include "Misc.h"

#include <CSGO/PODs/ConVar.h>
#include <CSGO/Constants/ClassId.h>
#include <CSGO/Client.h>
#include <CSGO/ClientClass.h>
#include <CSGO/ClientMode.h>
#include <CSGO/ConVar.h>
#include <CSGO/Cvar.h>
#include <CSGO/Engine.h>
#include <CSGO/EngineTrace.h>
#include <CSGO/Entity.h>
#include <CSGO/EntityList.h>
#include <CSGO/Constants/ConVarNames.h>
#include <CSGO/Constants/FrameStage.h>
#include <CSGO/Constants/GameEventNames.h>
#include <CSGO/Constants/UserMessages.h>
#include <CSGO/GameEvent.h>
#include <CSGO/GlobalVars.h>
#include <CSGO/ItemSchema.h>
#include <CSGO/Localize.h>
#include <CSGO/LocalPlayer.h>
#include <CSGO/NetworkChannel.h>
#include <CSGO/Panorama.h>
#include <CSGO/UserCmd.h>
#include <CSGO/UtlVector.h>
#include <CSGO/Vector.h>
#include <CSGO/WeaponData.h>
#include <CSGO/WeaponId.h>
#include <CSGO/WeaponSystem.h>
#include <CSGO/PODs/RenderableInfo.h>
#include <CSGO/Entity.h>
#include "../GUI.h"
#include "../Helpers.h"
#include "../Hooks.h"
#include "../GameData.h"
#include "../GlobalContext.h"

#include "../imguiCustom.h"
#include <Interfaces/ClientInterfaces.h>

struct PreserveKillfeed {
    bool enabled = false;
    bool onlyHeadshots = false;
};

struct OffscreenEnemies : ColorToggle {
    OffscreenEnemies() : ColorToggle{ 1.0f, 0.26f, 0.21f, 1.0f } {}
    HealthBar healthBar;
};

struct PurchaseList {
    bool enabled = false;
    bool onlyDuringFreezeTime = false;
    bool showPrices = false;
    bool noTitleBar = false;

    enum Mode {
        Details = 0,
        Summary
    };
    int mode = Details;
};

struct MiscConfig {
    MiscConfig() { clanTag[0] = '\0'; }

    KeyBind menuKey{ KeyBind::INSERT };
    bool menutodraw{ false };//false if bot menu, true if misc menu
    bool antiAfkKick{ false };
    bool autoStrafe{ false };
    bool bunnyHop{ false };
    bool customClanTag{ false };
    bool clocktag{ false };
    bool animatedClanTag{ false };
    bool fastDuck{ false };
    bool moonwalk{ false };
    bool edgejump{ false };
    bool slowwalk{ false };
    bool autoPistol{ false };
    bool autoReload{ false };
    bool autoAccept{ false };
    bool radarHack{ false };
    bool revealRanks{ false };
    bool revealMoney{ false };
    bool revealSuspect{ false };
    bool revealVotes{ false };
    bool fixAnimationLOD{ true };
    bool fixMovement{ false };
    bool disableModelOcclusion{ false };
    bool nameStealer{ false };
    bool disablePanoramablur{ false };
    bool killMessage{ false };
    bool nadePredict{ false };
    bool fixTabletSignal{ false };
    bool fastPlant{ false };
    bool fastStop{ false };
    bool quickReload{ false };
    bool prepareRevolver{ false };
    bool oppositeHandKnife = false;
    bool overheadChat{ false };
    PreserveKillfeed preserveKillfeed;
    char clanTag[16];
    KeyBind edgejumpkey;
    KeyBind slowwalkKey;
    ColorToggleThickness noscopeCrosshair;
    ColorToggleThickness recoilCrosshair;

    struct SpectatorList {
        bool enabled = false;
        bool noTitleBar = false;
        ImVec2 pos;
        ImVec2 size{ 200.0f, 200.0f };
    };

    SpectatorList spectatorList;
    struct Watermark {
        bool enabled = false;
    };
    Watermark watermark;
    float aspectratio{ 0 };
    std::string killMessageString{ "Gotcha!" };
    int banColor{ 6 };
    std::string banText{ "Cheater has been permanently banned from official CS:GO servers." };
    ColorToggle3 bombTimer{ 1.0f, 0.55f, 0.0f };
    KeyBind prepareRevolverKey;
    int hitSound{ 0 };
    int chokedPackets{ 0 };
    KeyBind chokedPacketsKey;
    int quickHealthshotKey{ 0 };
    float maxAngleDelta{ 255.0f };
    int killSound{ 0 };
    std::string customKillSound;
    std::string customHitSound;
    PurchaseList purchaseList;

    struct Reportbot {
        bool enabled = false;
        bool textAbuse = false;
        bool griefing = false;
        bool wallhack = true;
        bool aimbot = true;
        bool other = true;
        int target = 0;
        int delay = 1;
        int rounds = 1;
    } reportbot;

    std::string message="NULL";
    int playeruid=-1;
    float messageLoggedAt = 0.f;
    OffscreenEnemies offscreenEnemies;
} miscConfig;

struct BotzConfig {
    csgo::UserCmd* cmd;

    bool isbotzon{ false };

    int botState = 0;

    bool shouldwalk{ false };               //walkbot vars
  

    std::vector<csgo::Vector> nodes;//node positions
    std::vector<bool> nodesType;    //true if a node is open,false if closed
    std::vector<int>nodesParents;   //index of the parent node
    std::vector<int>walkType;       //same as return of collisionCheck()
    std::vector<float> fcost;       //total cost of node (dist to localplayer+dist to destination)
    int nodeRadius{ 33 };
    int currentNode{ 0 };
    bool pathFound{ false };
    float dropdownDmg{ 0.f };
    std::vector<csgo::Trace>tracez;
    csgo::Trace middleTrace;
    csgo::Vector checkOrigin{ 0,0,0 };
    csgo::Vector tempFloorPos{ 0,0,0 };
    csgo::Vector finalDestination{ 0.f,0.f,0.f};
    //float nodeRadius-1.f{ 32.f };    //todo: replace with nodeRadius-1.f
    std::vector<csgo::Vector> waypoints;
    std::vector<int> waypointWalkType;
    int curWayPoint{ -2 };

    csgo::Vector playerPingLoc{ 0,0,0 };

    float hitglass{ 0.f };
    float tracerayAngle{ 0.f };

    bool shouldDebug{ true };               //debug drawing and ermmmmmm
    float posDrawSize{ 1.5f };
    bool pathfindingDebug{ false };
    bool drawPathfindingTraces{ false };
    bool circlesOrCost{ false };


    bool autoreload{ true };                //reloadbot logic
    float lastReload{ 0.0f };
    int reloadAfterXSeconds{ 5 };
    float reloadIfClipPercent{ 0.75f };

    bool aimAtEvents{ true };               //aimbot things
    int aimreason{ -1 };                     //-1:not aiming at anything, 0:weapon_fire event 1:enemy 2:teammates 3:windows
    bool isShooterVisible{false};
    float reactionTime{ 0.135f };
    csgo::Vector aimspot{ 0,0,0 };
    float startedAiming{ -1.f };
    std::vector<float>aimtime{ 0.1f};
    csgo::Vector localViewAngles{ 0,0,0 };
    float lastTimeSawEnemy{ 0.f };
    bool shouldFire{ false };


    //communication stuff
    const std::array<std::string,31> radioTranslate { "","","go","fallback","sticktog","holdpos","followme", "","roger","negative","cheer","compliment",
                                                     "thanks","","enemyspot","needbackup","takepoint","sectorclear","inposition","coverme","regroup",
                                                     "takingfire","report","", "getout", "enemydown","","","go_a","go_b","needrop"};

    bool shouldReportToTeam{ true };        //report attacker info on death
    bool reportDetailsCallout{ true };
    bool reportDetailsDiedTo{ true };

    float complimentChance{ true };

} botzConfig;


template<typename T>
void pop_front(std::vector<T>& vec)
{
    assert(!vec.empty());
    vec.erase(vec.begin());
}



bool Misc::isRadarHackOn() noexcept
{
    return miscConfig.radarHack;
}

bool Misc::isMenuKeyPressed() noexcept
{
    return miscConfig.menuKey.isPressed();
}

float Misc::maxAngleDelta() noexcept
{
    return miscConfig.maxAngleDelta;
}

float Misc::aspectRatio() noexcept
{
    return miscConfig.aspectratio;
}


void Misc::bunnyHop(csgo::UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    static auto wasLastTimeOnGround{ localPlayer.get().isOnGround() };

    if (miscConfig.bunnyHop && !localPlayer.get().isOnGround() && localPlayer.get().moveType() != MoveType::LADDER && !wasLastTimeOnGround)
        cmd->buttons &= ~csgo::UserCmd::IN_JUMP;

    wasLastTimeOnGround = localPlayer.get().isOnGround();
}

static std::vector<std::uint64_t> reportedPlayers;
static int reportbotRound;

[[nodiscard]] static std::string generateReportString()
{
    std::string report;
    if (miscConfig.reportbot.textAbuse)
        report += "textabuse,";
    if (miscConfig.reportbot.griefing)
        report += "grief,";
    if (miscConfig.reportbot.wallhack)
        report += "wallhack,";
    if (miscConfig.reportbot.aimbot)
        report += "aimbot,";
    if (miscConfig.reportbot.other)
        report += "speedhack,";
    return report;
}

[[nodiscard]] static bool isPlayerReported(std::uint64_t xuid)
{
    return std::ranges::find(std::as_const(reportedPlayers), xuid) != reportedPlayers.cend();
}

[[nodiscard]] static std::vector<std::uint64_t> getXuidsOfCandidatesToBeReported(const csgo::Engine& engine, const ClientInterfaces& clientInterfaces, const OtherInterfaces& interfaces, const Memory& memory)
{
    std::vector<std::uint64_t> xuids;

    for (int i = 1; i <= engine.getMaxClients(); ++i) {
        const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(i));
        if (entity.getPOD() == 0 || entity.getPOD() == localPlayer.get().getPOD())
            continue;

        if (miscConfig.reportbot.target != 2 && (localPlayer.get().isOtherEnemy(memory, entity) ? miscConfig.reportbot.target != 0 : miscConfig.reportbot.target != 1))
            continue;

        if (csgo::PlayerInfo playerInfo; engine.getPlayerInfo(i, playerInfo) && !playerInfo.fakeplayer)
            xuids.push_back(playerInfo.xuid);
    }

    return xuids;
}

void Misc::runReportbot(const csgo::Engine& engine) noexcept
{
    if (!miscConfig.reportbot.enabled)
        return;

    if (!localPlayer)
        return;

    static auto lastReportTime = 0.0f;

    if (lastReportTime + miscConfig.reportbot.delay > memory.globalVars->realtime)
        return;

    if (reportbotRound >= miscConfig.reportbot.rounds)
        return;

    for (const auto& xuid : getXuidsOfCandidatesToBeReported(engine, clientInterfaces, interfaces, memory)) {
        if (isPlayerReported(xuid))
            continue;

        if (const auto report = generateReportString(); !report.empty()) {
            submitReport(LINUX_ARGS(nullptr,) std::to_string(xuid).c_str(), report.c_str());
            lastReportTime = memory.globalVars->realtime;
            reportedPlayers.push_back(xuid);
            return;
        }
    }

    reportedPlayers.clear();
    ++reportbotRound;
}

void Misc::resetReportbot() noexcept
{
    reportbotRound = 0;
    reportedPlayers.clear();
}

void Misc::preserveKillfeed(bool roundStart) noexcept
{
    if (!miscConfig.preserveKillfeed.enabled)
        return;

    static auto nextUpdate = 0.0f;

    if (roundStart) {
        nextUpdate = memory.globalVars->realtime + 10.0f;
        return;
    }

    if (nextUpdate > memory.globalVars->realtime)
        return;

    nextUpdate = memory.globalVars->realtime + 2.0f;

    const auto deathNotice = std::uintptr_t(memory.findHudElement(memory.hud, "CCSGO_HudDeathNotice"));
    if (!deathNotice)
        return;

    const auto deathNoticePanel = csgo::UIPanel::from(retSpoofGadgets->client, (*(csgo::UIPanelPOD**)(*reinterpret_cast<std::uintptr_t*>(deathNotice WIN32_LINUX(-20 + 88, -32 + 128)) + sizeof(std::uintptr_t))));

    const auto childPanelCount = deathNoticePanel.getChildCount();

    for (int i = 0; i < childPanelCount; ++i) {
        const auto childPointer = deathNoticePanel.getChild(i);
        if (!childPointer)
            continue;

        const auto child = csgo::UIPanel::from(retSpoofGadgets->client, childPointer);
        if (child.hasClass("DeathNotice_Killer") && (!miscConfig.preserveKillfeed.onlyHeadshots || child.hasClass("DeathNoticeHeadShot")))
            child.setAttributeFloat("SpawnTime", memory.globalVars->currenttime);
    }
}

void Misc::voteRevealer(const csgo::GameEvent& event) noexcept
{
    if (!miscConfig.revealVotes)
        return;

    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(event.getInt("entityid")));
    if (entity.getPOD() == nullptr || !entity.isPlayer())
        return;
    
    const auto votedYes = event.getInt("vote_option") == 0;
    const auto isLocal = localPlayer && entity.getPOD() == localPlayer.get().getPOD();
    const char color = votedYes ? '\x06' : '\x07';

    csgo::HudChat::from(retSpoofGadgets->client, memory.clientMode->hudChat).printf(0, " \x0C\u2022Osiris\u2022 %c%s\x01 voted %c%s\x01", isLocal ? '\x01' : color, isLocal ? "You" : entity.getPlayerName(interfaces, memory).c_str(), color, votedYes ? "Yes" : "No");
}

void Misc::onVoteStart(const void* data, int size) noexcept
{
    if (!miscConfig.revealVotes)
        return;

    constexpr auto voteName = [](int index) {
        switch (index) {
        case 0: return "Kick";
        case 1: return "Change Level";
        case 6: return "Surrender";
        case 13: return "Start TimeOut";
        default: return "";
        }
    };

    const auto reader = ProtobufReader{ static_cast<const std::uint8_t*>(data), size };
    const auto entityIndex = reader.readInt32(2);

    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(entityIndex));
    if (entity.getPOD() == nullptr || !entity.isPlayer())
        return;

    const auto isLocal = localPlayer && entity.getPOD() == localPlayer.get().getPOD();

    const auto voteType = reader.readInt32(3);
    csgo::HudChat::from(retSpoofGadgets->client, memory.clientMode->hudChat).printf(0, " \x0C\u2022Osiris\u2022 %c%s\x01 call vote (\x06%s\x01)", isLocal ? '\x01' : '\x06', isLocal ? "You" : entity.getPlayerName(interfaces, memory).c_str(), voteName(voteType));
}

void Misc::onVotePass() noexcept
{
    if (miscConfig.revealVotes)
        csgo::HudChat::from(retSpoofGadgets->client, memory.clientMode->hudChat).printf(0, " \x0C\u2022Osiris\u2022\x01 Vote\x06 PASSED");
}

void Misc::onVoteFailed() noexcept
{
    if (miscConfig.revealVotes)
        csgo::HudChat::from(retSpoofGadgets->client, memory.clientMode->hudChat).printf(0, " \x0C\u2022Osiris\u2022\x01 Vote\x07 FAILED");
}



// ImGui::ShadeVertsLinearColorGradientKeepAlpha() modified to do interpolation in HSV
static void shadeVertsHSVColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;

    ImVec4 col0HSV = ImGui::ColorConvertU32ToFloat4(col0);
    ImVec4 col1HSV = ImGui::ColorConvertU32ToFloat4(col1);
    ImGui::ColorConvertRGBtoHSV(col0HSV.x, col0HSV.y, col0HSV.z, col0HSV.x, col0HSV.y, col0HSV.z);
    ImGui::ColorConvertRGBtoHSV(col1HSV.x, col1HSV.y, col1HSV.z, col1HSV.x, col1HSV.y, col1HSV.z);
    ImVec4 colDelta = col1HSV - col0HSV;

    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);

        float h = col0HSV.x + colDelta.x * t;
        float s = col0HSV.y + colDelta.y * t;
        float v = col0HSV.z + colDelta.z * t;

        ImVec4 rgb;
        ImGui::ColorConvertHSVtoRGB(h, s, v, rgb.x, rgb.y, rgb.z);
        vert->col = (ImGui::ColorConvertFloat4ToU32(rgb) & ~IM_COL32_A_MASK) | (vert->col & IM_COL32_A_MASK);
    }
}

void Misc::drawOffscreenEnemies(const csgo::Engine& engine, ImDrawList* drawList) noexcept
{
    if (!miscConfig.offscreenEnemies.enabled)
        return;

    const auto yaw = Helpers::deg2rad(engine.getViewAngles().y);

    GameData::Lock lock;
    for (auto& player : GameData::players()) {
        if ((player.dormant && player.fadingAlpha(memory) == 0.0f) || !player.alive || !player.enemy || player.inViewFrustum)
            continue;

        const auto positionDiff = GameData::local().origin - player.origin;

        auto x = std::cos(yaw) * positionDiff.y - std::sin(yaw) * positionDiff.x;
        auto y = std::cos(yaw) * positionDiff.x + std::sin(yaw) * positionDiff.y;
        if (const auto len = std::sqrt(x * x + y * y); len != 0.0f) {
            x /= len;
            y /= len;
        }

        constexpr auto avatarRadius = 13.0f;
        constexpr auto triangleSize = 10.0f;

        const auto pos = ImGui::GetIO().DisplaySize / 2 + ImVec2{ x, y } * 200;
        const auto trianglePos = pos + ImVec2{ x, y } * (avatarRadius + (miscConfig.offscreenEnemies.healthBar.enabled ? 5 : 3));

        Helpers::setAlphaFactor(player.fadingAlpha(memory));
        const auto white = Helpers::calculateColor(255, 255, 255, 255);
        const auto background = Helpers::calculateColor(0, 0, 0, 80);
        const auto color = Helpers::calculateColor(memory.globalVars->realtime, miscConfig.offscreenEnemies.asColor4());
        const auto healthBarColor = miscConfig.offscreenEnemies.healthBar.type == HealthBar::HealthBased ? Helpers::healthColor(std::clamp(player.health / 100.0f, 0.0f, 1.0f)) : Helpers::calculateColor(memory.globalVars->realtime, miscConfig.offscreenEnemies.healthBar.asColor4());
        Helpers::setAlphaFactor(1.0f);

        const ImVec2 trianglePoints[]{
            trianglePos + ImVec2{  0.4f * y, -0.4f * x } * triangleSize,
            trianglePos + ImVec2{  1.0f * x,  1.0f * y } * triangleSize,
            trianglePos + ImVec2{ -0.4f * y,  0.4f * x } * triangleSize
        };

        drawList->AddConvexPolyFilled(trianglePoints, 3, color);
        drawList->AddCircleFilled(pos, avatarRadius + 1, white & IM_COL32_A_MASK, 40);

        const auto texture = player.getAvatarTexture();

        const bool pushTextureId = drawList->_TextureIdStack.empty() || texture != drawList->_TextureIdStack.back();
        if (pushTextureId)
            drawList->PushTextureID(texture);

        const int vertStartIdx = drawList->VtxBuffer.Size;
        drawList->AddCircleFilled(pos, avatarRadius, white, 40);
        const int vertEndIdx = drawList->VtxBuffer.Size;
        ImGui::ShadeVertsLinearUV(drawList, vertStartIdx, vertEndIdx, pos - ImVec2{ avatarRadius, avatarRadius }, pos + ImVec2{ avatarRadius, avatarRadius }, { 0, 0 }, { 1, 1 }, true);

        if (pushTextureId)
            drawList->PopTextureID();

        if (miscConfig.offscreenEnemies.healthBar.enabled) {
            const auto radius = avatarRadius + 2;
            const auto healthFraction = std::clamp(player.health / 100.0f, 0.0f, 1.0f);

            drawList->AddCircle(pos, radius, background, 40, 3.0f);

            const int vertStartIdx = drawList->VtxBuffer.Size;
            if (healthFraction == 1.0f) { // sometimes PathArcTo is missing one top pixel when drawing a full circle, so draw it with AddCircle
                drawList->AddCircle(pos, radius, healthBarColor, 40, 2.0f);
            } else {
                constexpr float pi = std::numbers::pi_v<float>;
                drawList->PathArcTo(pos, radius - 0.5f, pi / 2 - pi * healthFraction, pi / 2 + pi * healthFraction, 40);
                drawList->PathStroke(healthBarColor, false, 2.0f);
            }
            const int vertEndIdx = drawList->VtxBuffer.Size;

            if (miscConfig.offscreenEnemies.healthBar.type == HealthBar::Gradient)
                shadeVertsHSVColorGradientKeepAlpha(drawList, vertStartIdx, vertEndIdx, pos - ImVec2{ 0.0f, radius }, pos + ImVec2{ 0.0f, radius }, IM_COL32(0, 255, 0, 255), IM_COL32(255, 0, 0, 255));
        }
    }
}

void Misc::autoAccept(const char* soundEntry) noexcept
{
    if (!miscConfig.autoAccept)
        return;

    if (std::strcmp(soundEntry, "UIPanorama.popup_accept_match_beep"))
        return;

    if (const auto idx = memory.registeredPanoramaEvents->find(memory.makePanoramaSymbol("MatchAssistedAccept")); idx != -1) {
        if (const auto eventPtr = retSpoofGadgets->client.invokeCdecl<void*>(std::uintptr_t(memory.registeredPanoramaEvents->memory[idx].value.makeEvent), nullptr))
            csgo::UIEngine::from(retSpoofGadgets->client, interfaces.getPanoramaUIEngine().accessUIEngine()).dispatchEvent(eventPtr);
    }

#if IS_WIN32()
    auto window = FindWindowW(L"Valve001", NULL);
    FLASHWINFO flash{ sizeof(FLASHWINFO), window, FLASHW_TRAY | FLASHW_TIMERNOFG, 0, 0 };
    FlashWindowEx(&flash);
    ShowWindow(window, SW_RESTORE);
#endif
}

//chattin

void Misc::readChat(const void* data, int size) noexcept {
    if (!localPlayer)
        return;
    
    const auto reader = ProtobufReader{ static_cast<const std::uint8_t*>(data),size };
    const auto ent_idx = reader.readInt32(1);
    const auto params = reader.readRepeatedString(4);
    miscConfig.message = params[1];
    miscConfig.playeruid = ent_idx;
    miscConfig.messageLoggedAt = memory.globalVars->realtime;
}

void Misc::chatOverhead(const EngineInterfaces& engineInterfaces,const Memory& memory) noexcept{
    if (!miscConfig.overheadChat)
        return;
    if (!localPlayer)
        return;

    ImDrawList* dlist;
    dlist = ImGui::GetBackgroundDrawList();
    if (miscConfig.message=="NULL"|| miscConfig.playeruid == -1 || miscConfig.messageLoggedAt == 0)
        return;
    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;


    //int entIndex = engine.getPlayerForUserID(miscConfig.playeruid);
    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(miscConfig.playeruid));
    csgo::PlayerInfo pInfo;
    engine.getPlayerInfo(miscConfig.playeruid, pInfo);
    std::string pName = pInfo.name;
    pName += (entity.getTeamNumber() == csgo::Team::CT ? " - CT" : " - T ");
    ImVec2 playerOnScreen;
    Helpers::worldToScreenPixelAligned({ entity.getAbsOrigin().x, entity.getAbsOrigin().y, entity.getEyePosition().z + 15.f }, playerOnScreen);
    
    if (entity.isAlive() && playerOnScreen.x > 0) {
        dlist->AddRectFilledMultiColor(ImVec2(playerOnScreen.x - (ImGui::CalcTextSize(miscConfig.message.c_str()).x / 2) - 8, playerOnScreen.y - (ImGui::CalcTextSize(miscConfig.message.c_str()).y) + 8), ImVec2(playerOnScreen.x + (ImGui::CalcTextSize(miscConfig.message.c_str()).x / 2) + 8, playerOnScreen.y + (ImGui::CalcTextSize(miscConfig.message.c_str()).y) + 8), 0xDD333333, 0xDD292929, 0xDD252525, 0xDD292929);
        dlist->AddText(ImVec2(playerOnScreen.x - (ImGui::CalcTextSize(miscConfig.message.c_str()).x / 2), playerOnScreen.y), 0xFFFFFFFF, miscConfig.message.c_str());
        
    }
    else {
        dlist->AddRectFilledMultiColor(ImVec2(4, 383), ImVec2(4 + ImGui::CalcTextSize(miscConfig.message.c_str()).x + 16, 400 + ImGui::CalcTextSize(miscConfig.message.c_str()).y + 16), 0xDD333333, 0xDD292929, 0xDD252525, 0xDD292929);
        dlist->AddText(ImVec2(12, 391), 0xFFFFFFFF, pName.c_str());
        dlist->AddText(ImVec2(12, 408), 0xFFFFFFFF, miscConfig.message.c_str());
    }
    if (miscConfig.messageLoggedAt + 5.f < memory.globalVars->realtime){
        miscConfig.message = "NULL";
        miscConfig.messageLoggedAt = 0.f;
        miscConfig.playeruid = 0;
    }
}

//botz stuff

void Misc::aimAtEvent(const Memory& memory,const EngineInterfaces& engineInterfaces) noexcept {
    if (!botzConfig.isbotzon)           return;
    if (!localPlayer)                   return;
    if(!localPlayer.get().isAlive())    return;
    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame())             return;
    if (botzConfig.aimreason == 3&&botzConfig.startedAiming<1.f) {
            botzConfig.startedAiming = memory.globalVars->realtime;
    }
    if (botzConfig.startedAiming == -1.f|| botzConfig.startedAiming + botzConfig.reactionTime > memory.globalVars->realtime)
                                        return;
    if (botzConfig.aimreason==0&&!botzConfig.isShooterVisible)
        return;
    
    csgo::Vector relang = Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), botzConfig.aimspot, botzConfig.localViewAngles);
    engine.setViewAngles({ botzConfig.localViewAngles.x + relang.x*sin(memory.globalVars->realtime-botzConfig.startedAiming-botzConfig.aimtime[0] + botzConfig.reactionTime) / 2,
                           botzConfig.localViewAngles.y + relang.y*sin(memory.globalVars->realtime - botzConfig.startedAiming-botzConfig.aimtime[0] + botzConfig.reactionTime) / 2,
                           0.f});
    
    if ( -0.3f<Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), botzConfig.aimspot, botzConfig.localViewAngles).x &&
        Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), botzConfig.aimspot, botzConfig.localViewAngles).x<0.3f&&
         -0.3f<Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), botzConfig.aimspot, botzConfig.localViewAngles).y&&
        Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), botzConfig.aimspot, botzConfig.localViewAngles).y<0.3f){
            botzConfig.startedAiming = -1.f;
            if (botzConfig.aimreason == 3) {
                botzConfig.shouldFire = true;
            }
            botzConfig.aimreason = -1;
            return;
    }
}

//void Misc::findDoors(const EngineInterfaces& engineInterfaces, csgo::UserCmd* cmd) noexcept {
//
//    if (!localPlayer)
//        return;
//    if (!localPlayer.get().isAlive())
//        return;
//    if (!botzConfig.isbotzon)
//        return;
//    if (!engineInterfaces.getEngine().isInGame())
//        return;
//
//    csgo::Trace traceXp; //front,back,right,left
//
//    csgo::Vector viewangleshit{ localPlayer.get().getEyePosition().x + cos(Helpers::deg2rad(localPlayer.get().eyeAngles().y)) * 60,
//                                localPlayer.get().getEyePosition().y + sin(Helpers::deg2rad(localPlayer.get().eyeAngles().y)) * 60,
//                                localPlayer.get().getEyePosition().z };
//    const csgo::Vector lPlayerEyes = localPlayer.get().getEyePosition();
//    engineInterfaces.engineTrace().traceRay({ lPlayerEyes, viewangleshit }, 0x4000, localPlayer.get().getPOD(), traceXp);
//    botzConfig.hitglass = traceXp.fraction;
//
//    if (traceXp.fraction < 1.0f)
//        cmd->buttons |= csgo::UserCmd::IN_USE;
//}

void Misc::findBreakable(const EngineInterfaces& engineInterfaces,csgo::UserCmd* cmd) noexcept {
 
}

int fallDamageCheck(const EngineInterfaces& engineInterfaces,csgo::Vector pos) noexcept { 
    if (!localPlayer)//todo: add like a bool to check if you want to print the falldamage value to the console
        return -1;
    if (!localPlayer.get().isAlive())
        return -1;
    //-0.000128943x^{2}+0.341019x-65.806 <- calculates fall damage kinda well 

    csgo::Trace trace;
    float fallHeight,fallDamage;
                                                                
    engineInterfaces.engineTrace().traceRay({ pos,{pos.x,pos.y,pos.z - 1500.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), trace);
    fallHeight = pos.distTo(trace.endpos);
    botzConfig.tempFloorPos = { pos.x,pos.y,pos.z - fallHeight };
    fallDamage = -0.000128943f * pow(fallHeight, 2.f) + 0.341019f * fallHeight - 65.806f;


    return int(fallDamage);
}

void VectorAngles(const csgo::Vector& forward, csgo::Vector& angles)
{
    float tmp, yaw, pitch;

    if (forward.y == 0 && forward.x == 0)
    {
        yaw = 0;
        if (forward.z > 0)
            pitch = 270;
        else
            pitch = 90;
    }
    else
    {
        yaw = (atan2(forward.y, forward.x) * 180 / 3.141592653f);
        if (yaw < 0)
            yaw += 360;

        tmp = sqrtf(forward.x * forward.x + forward.y * forward.y);
        pitch = (atan2(-forward.z, tmp) * 180 / 3.141592653f);
        if (pitch < 0)
            pitch += 360;
    }

    angles.x = pitch;
    angles.y = yaw;
    angles.z = 0;
}
#define PI			3.14159265358979323846
#define DEG2RAD(x) ((float)(x) * (float)((float)(PI) / 180.0f))
#define RAD2DEG(x) ((float)(x) * (float)(180.0f / (float)(PI)))
void AngleVectors(const csgo::Vector& angles, csgo::Vector& forward)
{


    float sp, sy, cp, cy;

    sy = sin(DEG2RAD(angles.y));
    cy = cos(DEG2RAD(angles.y));

    sp = sin(DEG2RAD(angles.x));
    cp = cos(DEG2RAD(angles.x));

    forward.x = cp * cy;
    forward.y = cp * sy;
    forward.z = -sp;
}
csgo::Vector CalculateAngle(csgo::Vector src, csgo::Vector dst)
{
    csgo::Vector angles;

    csgo::Vector delta = src - dst;
    float hyp = delta.length2D();

    angles.y = std::atanf(delta.y / delta.x) * 57.2957795131f;
    angles.x = std::atanf(-delta.z / hyp) * -57.2957795131f;
    angles.z = 0.0f;

    if (delta.x >= 0.0f)
        angles.y += 180.0f;

    return angles;
}
void Misc::drawPathfinding(const EngineInterfaces& engineInterfaces)noexcept {
    
 }
#include <deque>
#define NODERADIUS 20.f
#define TOTALNODEALONGRAY 8
class CPathFinder {
public:
    struct Node_t {
        csgo::Vector Pos;
        float Radius;
    };
    void MoveTo(csgo::Vector pos, csgo::Vector origin, csgo::UserCmd* cmd, float wish_yaw) {

            auto difference = origin - pos;

        auto velocity = csgo::Vector(difference.x * cos(wish_yaw / 180.0f * 3.141592653f) + difference.y * sin(wish_yaw / 180.0f * 3.141592653f), difference.y * cos(wish_yaw / 180.0f * 3.141592653f) - difference.x * sin(wish_yaw / 180.0f * 3.141592653f), difference.z);
        cmd->forwardmove = std::clamp(-velocity.x * 450, -450.f, 450.f);
        cmd->sidemove = std::clamp(velocity.y * 450, -450.f, 450.f);

    }
    csgo::Vector TraceLineWorldOnly(const EngineInterfaces& engineInterfaces, csgo::Vector start, csgo::Vector end, bool& visible) {


        csgo::Trace trace;


        engineInterfaces.engineTrace().traceRay({ start, end }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), trace);
     
        visible = trace.fraction == 1.f;
        return trace.endpos;
    }
    Node_t* GetClosestNode(csgo::Vector Pos, int* index) {
        Node_t* ClosestNode = nullptr;
        if (!localPlayer)
            return ClosestNode;

        float Closest = FLT_MAX;
        float Distance = Closest;
        int tempindex = 0;
        for (auto& Node : Path) {
            Distance = (Node.Pos - Pos).squareLength();
         
            if (Distance < Closest) {
                Closest = Distance;
                ClosestNode = &Node;
                if (index)
                    *index = tempindex;
            }
            tempindex++;
        }

        return ClosestNode;
    }
    void OptimizePath() {
        CacheNode.clear();
        CacheNode = Path;
        Path.clear();

        auto Node = CacheNode.begin();
        for (int i = 0; i < CacheNode.size(); i++) {
            int Index = 0;
            Path.push_back(Node_t{ Node->Pos, Node->Radius });
            Node_t* Closest = GetClosestNode(Node->Pos, &Index);
            if (Closest) {
                Node = CacheNode.begin() + Index;
            }
            Node++;
        }
        CacheNode.clear();
    }
    Node_t* GetClosestNode() {
        //csgo::UserCmd*
        Node_t* ClosestNode = nullptr;
        if (!localPlayer)
            return ClosestNode;
        if (!localPlayer.get().isAlive())
            return ClosestNode;
        auto Pos = localPlayer.get().getAbsOrigin();
        float Closest = FLT_MAX;
        float ClosestToTarget = FLT_MAX;
        float Distance = Closest;
        float DistanceToTarget = Closest;
        for (auto& Node : Path) {
            Distance = (Node.Pos - Pos).squareLength();
            DistanceToTarget = (Node.Pos - TargetNode.Pos).squareLength();
            if (Distance < Closest && DistanceToTarget < ClosestToTarget) {
                Closest = Distance;
                ClosestToTarget = DistanceToTarget;
                ClosestNode = &Node;
            }
        }

        return ClosestNode;
    }
    void GenerateNodes(Node_t& origin) {
   
    }
    void MovePathForward(const EngineInterfaces& engineInterfaces) {
    
    }
    void GeneratePathAlongRay(csgo::Vector start, csgo::Vector end, bool cache = false) {
        csgo::Vector direction(0, 0, 0);
        AngleVectors(CalculateAngle(start, end), direction);

        for (int i = 1; i < TOTALNODEALONGRAY; i++) {
            float mod = (float)(i / TOTALNODEALONGRAY);

             Path.push_back(Node_t{ start + (direction * mod), NODERADIUS });
            
        }
    }
    void CalculatePath(const EngineInterfaces& engineInterfaces) {
        bool ClearPath = false;
        FoundPath = false;
        TraceLineWorldOnly(engineInterfaces, Path[0].Pos, TargetNode.Pos, ClearPath);

        if (ClearPath) {
            //GeneratePathAlongRay(Path[0].Pos, TargetNode.Pos); doesnt work for some reason
            Path.push_back(TargetNode);
            OptimizePath();
            FoundPath = true;
            return;
        }

    }

    std::deque<Node_t> Path;
    std::deque<Node_t> CacheNode;
    bool FoundPath;
    Node_t TargetNode;

    void GenerateNode(csgo::Vector pos, float radius) {
        CacheNode.push_back(Node_t{ pos, radius });
    }
};
CPathFinder PathFinder; //this is illegal but testing


void Misc::findPath(const EngineInterfaces& engineInterfaces) noexcept {
   //cm
}

void Misc::pathfind(const EngineInterfaces& engineInterfaces, const Memory& memory) noexcept {
  //init
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    PathFinder.Path.clear();
    PathFinder.FoundPath = false;
    PathFinder.CacheNode.clear();
    //push origin node
    //AngleVectors
    csgo::Vector Forward;
    AngleVectors(engineInterfaces.getEngine().getViewAngles(), Forward);
    bool Buf = false;
    csgo::Vector End = PathFinder.TraceLineWorldOnly(engineInterfaces, localPlayer.get().getEyePosition(), localPlayer.get().getEyePosition() + (Forward * 10000.f), Buf);
    PathFinder.TargetNode = CPathFinder::Node_t{ End + csgo::Vector(0.f,0.f,13.f), NODERADIUS };
    PathFinder.Path.push_back(CPathFinder::Node_t{ localPlayer.get().getAbsOrigin() + csgo::Vector(0.f,0.f,13.f), NODERADIUS });
    PathFinder.CalculatePath(engineInterfaces);
}

void Misc::drawPath(const EngineInterfaces& engineInterfaces) noexcept {
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    ImDrawList* dlist;
    dlist = ImGui::GetBackgroundDrawList();
    auto ClosestNode = PathFinder.GetClosestNode();

    if (PathFinder.FoundPath) {

        for (auto& N : PathFinder.Path) {
            ImVec2 tl, tr, bl, br;
            bool IsTargetNode = ClosestNode != nullptr;
            if (IsTargetNode) {
                IsTargetNode = ClosestNode->Pos == N.Pos;
            }
            Helpers::worldToScreenPixelAligned(N.Pos + csgo::Vector(NODERADIUS * 0.5f, -NODERADIUS * 0.5f, 0), tr);
            Helpers::worldToScreenPixelAligned(N.Pos + csgo::Vector(-NODERADIUS * 0.5f, -NODERADIUS * 0.5f, 0), tl);
            Helpers::worldToScreenPixelAligned(N.Pos + csgo::Vector(NODERADIUS * 0.5f, NODERADIUS * 0.5f, 0), br);
            Helpers::worldToScreenPixelAligned(N.Pos + csgo::Vector(-NODERADIUS * 0.5f, NODERADIUS * 0.5f, 0), bl);
            dlist->AddTriangleFilled(tl, br, tr, IsTargetNode ? 0xFF44AA44 : 0xFFAA4444);
            dlist->AddTriangleFilled(tl, br, bl, IsTargetNode ? 0xFF44AA44 : 0xFFAA4444);
        }
    }
}

void Misc::reportToTeam(const Memory& memory, const EngineInterfaces& engineInterfaces, const csgo::GameEvent& event, bool forceReport) noexcept {

}

void Misc::reload(csgo::UserCmd* cmd, const Memory& memory, const EngineInterfaces& engineInterfaces) noexcept {
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    if (!botzConfig.isbotzon|| !botzConfig.autoreload)
        return;
    const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon());
    if (activeWeapon.getPOD() == nullptr)
        return;
    if (activeWeapon.getWeaponType() == WeaponType::Knife || activeWeapon.getWeaponType() == WeaponType::C4 || activeWeapon.getWeaponType() == WeaponType::Grenade || activeWeapon.getWeaponType() == WeaponType::Fists || activeWeapon.getWeaponType() == WeaponType::Melee || activeWeapon.getWeaponType() == WeaponType::Unknown||!activeWeapon.isWeapon())
        return;
    if (activeWeapon.getWeaponData()->maxClip * botzConfig.reloadIfClipPercent < activeWeapon.clip())
        return;

    if (memory.globalVars->realtime - botzConfig.lastReload < 5)
        return;

    for (int i = 1; i <= engineInterfaces.getEngine().getMaxClients(); i++) {
        const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(i));
        if (entity.getPOD() == nullptr || entity.getPOD() == localPlayer.get().getPOD() || entity.getNetworkable().isDormant() || !entity.isAlive()
            || !entity.isOtherEnemy(memory, localPlayer.get()) || entity.gunGameImmunity())
            continue;
        
        if (entity.isVisible(engineInterfaces.engineTrace(), entity.getEyePosition())) {
            botzConfig.lastTimeSawEnemy = memory.globalVars->realtime;
        }
    }
    const csgo::Engine engine = engineInterfaces.getEngine();
    if (memory.globalVars->realtime - botzConfig.lastTimeSawEnemy > botzConfig.reloadAfterXSeconds) {
        cmd->buttons |= csgo::UserCmd::IN_RELOAD;
        botzConfig.lastReload = memory.globalVars->realtime;
    }
    if (EnginePrediction::getFlags() & 13&&memory.globalVars->realtime==botzConfig.lastReload) { engine.clientCmdUnrestricted("coverme");};
    
}

void Misc::gotoBotzPos(const EngineInterfaces& engineInterfaces,csgo::UserCmd* cmd) noexcept{
    
    //actual cm
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
 
    if (PathFinder.FoundPath) {

        float oYaw = engineInterfaces.getEngine().getViewAngles().y;
        auto ClosestNode = PathFinder.GetClosestNode();
        if (!ClosestNode)
            return;

        auto Pos = localPlayer.get().getAbsOrigin();
        PathFinder.MoveTo(ClosestNode->Pos, Pos, cmd, oYaw);
        if (Pos.distTo(PathFinder.TargetNode.Pos) < NODERADIUS) {
            PathFinder.FoundPath = false;
        }
        int ToRemove = -1;
        int PathIndex = 0;
        for (auto& N : PathFinder.Path) {
            if (Pos.distTo(N.Pos) < NODERADIUS) {
                ToRemove = PathIndex;
            }
            PathIndex++;
        }

        if (ToRemove != -1) {
            //PathIndex
            PathFinder.Path.erase(PathFinder.Path.begin() + ToRemove);
        }
    }

}

void Misc::handleRadioCommands(const csgo::GameEvent& event, const EngineInterfaces& engineInterfaces) noexcept {
    if (!localPlayer)                   
        return;                         
    if (!localPlayer.get().isAlive())   
        return;                                                                     //0:unused      1:unused        2:go            3:fallback      4:sticktog
    if (!botzConfig.isbotzon)                                                       //5:holdpos     6:followme      7:unused        8:roger         9:negative
        return;                                                                     //10:cheer      11:compliment   12:thanks       13:unused       14:enemyspot
                                                                                    //15:needbackup 16:takepoint    17:sectorclear  18:inposition   19:coverme
    const csgo::Engine& engine = engineInterfaces.getEngine();                      //20:regroup    21:takingfire   22:report       23:unused       24:enemydown
    if (!engine.isInGame())                                                         //25:unused     26:unused       27:go_a         28:go_b         29:needrop
        return;


    switch (event.getInt("slot")) { 
    case 4:
    case 6:
    case 21:
        //follow teammate
        engine.clientCmdUnrestricted(botzConfig.radioTranslate[8].c_str());
        break;
    case 22:
        //report sectorclear unless engaging an enemy
        engine.clientCmdUnrestricted(botzConfig.radioTranslate[17].c_str());
        break;
    default:break;                
    }
}

void Misc::handleBotzEvents(const Memory& memory, const EngineInterfaces& engineInterfaces, const csgo::GameEvent& event, const ClientInterfaces& clientInterfaces, int eventType) noexcept {
    if (!localPlayer)                                                                                                    //eventTypes:
        return;                                                                                                          //0: player_death      1: player_hurt      2: round_start
    if (!botzConfig.isbotzon)                                                                                            //3: bomb_planted      4: bomb_defused     5: bomb_exploded
        return;                                                                                                          //6: player_radio      7: round_freeze_end 8: vote_cast
    const csgo::Engine& engine = engineInterfaces.getEngine();                                                           //9: round_mvp         10:item_purchase    11:bullet_impact
    std::string eventName = event.getName();                                                                             //12:weapon_fire      13:player_ping
    eventName += "!";
    engine.clientCmdUnrestricted(eventName.c_str());
    const auto localUserId = localPlayer.get().getUserId(engine);
    std::string printToConsole;
    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(engine.getPlayerForUserID(event.getInt("userid"))));
    switch (eventType) {
    case 0:
        if (entity.isOtherEnemy(memory, localPlayer.get())) {
            if (int(fmod(memory.globalVars->realtime, botzConfig.complimentChance)) == 0) {
                engine.clientCmdUnrestricted("compliment");
            }
        }

        if (event.getInt("userid") == localUserId) {//set bot state to dead in case the bot died (wow)
            botzConfig.botState = 6;
            Misc::reportToTeam(memory, engineInterfaces, event, true);
        }
        break;
    case 3:
        //GameData::plantedC4().bombsite doesn't return the bombsite index anymore idk :P
        if (localPlayer.get().getTeamNumber() == csgo::Team::CT) {
            //go towards closest bombsite, if can't hear the bomb go to another bombsite
        }
        else if (localPlayer.get().getTeamNumber() == csgo::Team::TT) {
            //camp
        }
        break;
    case 6:
        printToConsole = "echo User ";
        printToConsole += std::to_string(event.getInt("userid"));
        printToConsole += " used radio command ";
        printToConsole += botzConfig.radioTranslate[event.getInt("slot")];
        engine.clientCmdUnrestricted(printToConsole.c_str());
        Misc::handleRadioCommands(event, engineInterfaces);
        break;
    case 12:

        if (entity.getPOD() == localPlayer.get().getPOD())
            break;
        if (!entity.isOtherEnemy(memory, localPlayer.get()))
            return;
        botzConfig.aimspot = entity.getBonePosition(4);
        csgo::PlayerInfo pinfo;
        engine.getPlayerInfo(engine.getPlayerForUserID(event.getInt("userid")), pinfo);
        printToConsole = "echo ";
        printToConsole += event.getString("weapon");
        printToConsole += " fired by ";
        printToConsole += pinfo.name;
        printToConsole += " at (";
        printToConsole += std::to_string(botzConfig.aimspot.x);
        printToConsole += ", ";
        printToConsole += std::to_string(botzConfig.aimspot.y);
        printToConsole += ", ";
        printToConsole += std::to_string(botzConfig.aimspot.z);
        printToConsole += ")";
        engine.clientCmdUnrestricted(printToConsole.c_str());

        if (entity.isVisible(engineInterfaces.engineTrace(), localPlayer.get().getEyePosition())) {
            botzConfig.isShooterVisible = true;
        }
        else {
            botzConfig.isShooterVisible = false;
        }

        if (botzConfig.startedAiming + botzConfig.reactionTime < memory.globalVars->realtime) {
            botzConfig.startedAiming = memory.globalVars->realtime;
            botzConfig.aimreason = 0;
        }
        break;
    case 13:
        if (entity.getPOD() == localPlayer.get().getPOD()) {
            botzConfig.playerPingLoc = { event.getFloat("x"),event.getFloat("y"),event.getFloat("z") };
            botzConfig.finalDestination = botzConfig.playerPingLoc;
        }
        break;
    default:break;
    }


}

//hooks&gui stuff 

bool Misc::isPlayingDemoHook(ReturnAddress returnAddress, std::uintptr_t frameAddress) const
{
    return miscConfig.revealMoney && returnAddress == demoOrHLTV && *reinterpret_cast<std::uintptr_t*>(frameAddress + WIN32_LINUX(8, 24)) == money;
}

const csgo::DemoPlaybackParameters* Misc::getDemoPlaybackParametersHook(ReturnAddress returnAddress, const csgo::DemoPlaybackParameters& demoPlaybackParameters) const
{
    if (miscConfig.revealSuspect && returnAddress != demoFileEndReached) {
        static csgo::DemoPlaybackParameters customParams;
        customParams = demoPlaybackParameters;
        customParams.anonymousPlayerIdentity = false;
        return &customParams;
    }

    return &demoPlaybackParameters;
}

std::optional<std::pair<csgo::Vector, csgo::Vector>> Misc::listLeavesInBoxHook(ReturnAddress returnAddress, std::uintptr_t frameAddress) const
{
    if (!miscConfig.disableModelOcclusion || returnAddress != insertIntoTree)
        return {};

    const auto info = *reinterpret_cast<csgo::RenderableInfo**>(frameAddress + WIN32_LINUX(0x18, 0x10 + 0x948));
    if (!info || !info->renderable)
        return {};

    const auto ent = VirtualCallable{ retSpoofGadgets->client, std::uintptr_t(info->renderable) - sizeof(std::uintptr_t) }.call<csgo::EntityPOD*, WIN32_LINUX(7, 8)>();
    if (!ent || !csgo::Entity::from(retSpoofGadgets->client, ent).isPlayer())
        return {};

    constexpr float maxCoord = 16384.0f;
    constexpr float minCoord = -maxCoord;
    constexpr csgo::Vector min{ minCoord, minCoord, minCoord };
    constexpr csgo::Vector max{ maxCoord, maxCoord, maxCoord };
    return std::pair{ min, max };
}

void Misc::dispatchUserMessageHook(csgo::UserMessageType type, int size, const void* data)
{
    switch (type) {
    using enum csgo::UserMessageType;
    case VoteStart: return onVoteStart(data, size);
    case VotePass: return onVotePass();
    case VoteFailed: return onVoteFailed();
    //case Text:    return readChat(data,size);
    case SayText2: return readChat(data, size);
    default: break;
    }
}

void Misc::updateEventListeners(const EngineInterfaces& engineInterfaces, bool forceRemove) noexcept
{
    static DefaultEventListener listener;
    static bool listenerRegistered = false;

    if (miscConfig.purchaseList.enabled && !listenerRegistered) {
        engineInterfaces.getGameEventManager(memory.getEventDescriptor).addListener(&listener, csgo::item_purchase);
        listenerRegistered = true;
    } else if ((!miscConfig.purchaseList.enabled || forceRemove) && listenerRegistered) {
        engineInterfaces.getGameEventManager(memory.getEventDescriptor).removeListener(&listener);
        listenerRegistered = false;
    }
}

void Misc::updateInput() noexcept
{

}

static bool windowOpen = false;

void Misc::menuBarItem() noexcept
{
    if (ImGui::MenuItem("Misc")) {
        windowOpen = true;
        ImGui::SetWindowFocus("Misc");
        ImGui::SetWindowPos("Misc", { 100.0f, 100.0f });
    }
}

void Misc::tabItem(Visuals& visuals, inventory_changer::InventoryChanger& inventoryChanger, Glow& glow, const EngineInterfaces& engineInterfaces) noexcept
{
    if (ImGui::BeginTabItem("Misc")) {
        drawGUI(visuals, inventoryChanger, glow, engineInterfaces, true);
        ImGui::EndTabItem();
    }
}

void Misc::drawGUI(Visuals& visuals, inventory_changer::InventoryChanger& inventoryChanger, Glow& glow, const EngineInterfaces& engineInterfaces, bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!windowOpen)
            return;
        ImGui::SetNextWindowSize({ 580.0f, 0.0f });
        ImGui::Begin("Misc", &windowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    }
    if (!miscConfig.menutodraw)
    {
        if (ImGui::Button("switch to misc"))
            miscConfig.menutodraw = true;

        if (ImGui::Button("Toggle botzzz"))
            botzConfig.isbotzon = !botzConfig.isbotzon;
        if (botzConfig.isbotzon) {
            ImGui::Checkbox("Botz debug", &botzConfig.shouldDebug);
            if (botzConfig.shouldDebug) {
                if (ImGui::Button("find path"))
                    Misc::pathfind(engineInterfaces,memory);
              
                ImGui::Separator();
            }
            //ImGui::SliderFloat("Waypoint approximation amount", &botzConfig.nodeRadius, 1.f, 150.f);
            ImGui::Checkbox("Should walk towards pos", &botzConfig.shouldwalk);
            ImGui::Separator();
            ImGui::Text("Pathfinding");
            ImGui::SliderFloat("Max fall damage (%)", &botzConfig.dropdownDmg, 0.0f, 1.0f,"%.2f",ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderInt("Node radius", &botzConfig.nodeRadius, 1, 150);
            ImGui::Checkbox("Pathfinding debug", &botzConfig.pathfindingDebug);
            ImGui::Checkbox("Draw pathfinding traces", &botzConfig.drawPathfindingTraces);
            ImGui::Checkbox("Draw circles", &botzConfig.circlesOrCost);
            ImGui::Separator();
            ImGui::Text("Bot behavior");
            ImGui::Checkbox("Autoreload", &botzConfig.autoreload);
            if (botzConfig.autoreload) {
                ImGui::SliderInt("Reload after X seconds of not seeing an enemy: ", &botzConfig.reloadAfterXSeconds, 0, 30);
                ImGui::SliderFloat("Reload if clip is below x%", &botzConfig.reloadIfClipPercent, 0.01f, 0.99f, "%.2f");
                ImGui::Checkbox("Quickswitch weapon after reload", &miscConfig.quickReload);
            }
            ImGui::Checkbox("Report info to team", &botzConfig.shouldReportToTeam);
            if (botzConfig.shouldReportToTeam) {
                ImGui::Checkbox("Report death position", &botzConfig.reportDetailsCallout);
                ImGui::Checkbox("Report killer's name&weapon", &botzConfig.reportDetailsDiedTo);
            }
            ImGui::Separator();
            ImGui::Text("Aiming");
            ImGui::SliderFloat("Reaction time", &botzConfig.reactionTime, 0.f, 1.f,"%.2f");
            ImGui::SliderFloat("aimtime", &botzConfig.aimtime[0], -2.2f, 0.28f, "%.2f");
            ImGui::Separator();
            ImGui::Text("Communication");
            ImGui::SliderFloat("Compliment chance", &botzConfig.complimentChance, 1.f, 100.f,"%.0f",ImGuiSliderFlags_AlwaysClamp);
            if (botzConfig.shouldwalk) {
                ImGui::SliderFloat("X",&botzConfig.finalDestination.x, -5000.f, 5000.f);
                ImGui::SliderFloat("Y",&botzConfig.finalDestination.y, -5000.f, 5000.f);
                ImGui::SliderFloat("Z",&botzConfig.finalDestination.z, -5000.f, 5000.f);
            }
        }
    }
    else
    {
        if (ImGui::Button("switch to botz"))
            miscConfig.menutodraw = false;
        ImGui::SameLine();
        ImGui::hotkey("Menu Key", miscConfig.menuKey);
        ImGui::Checkbox("overhead chat", &miscConfig.overheadChat);
        ImGui::Checkbox("Bunny hop", &miscConfig.bunnyHop);
        ImGui::Checkbox("Auto accept", &miscConfig.autoAccept);

        if (ImGui::Button("Unhook"))
            hooks->uninstall(*this, glow, engineInterfaces, clientInterfaces, interfaces, memory, visuals, inventoryChanger);
    }
    
    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();
}

static void from_json(const json& j, ImVec2& v)
{
    read(j, "X", v.x);
    read(j, "Y", v.y);
}

static void from_json(const json& j, PurchaseList& pl)
{
    read(j, "Enabled", pl.enabled);
    read(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
    read(j, "Show Prices", pl.showPrices);
    read(j, "No Title Bar", pl.noTitleBar);
    read(j, "Mode", pl.mode);
}

static void from_json(const json& j, OffscreenEnemies& o)
{
    from_json(j, static_cast<ColorToggle&>(o));

    read<value_t::object>(j, "Health Bar", o.healthBar);
}

static void from_json(const json& j, MiscConfig::SpectatorList& sl)
{
    read(j, "Enabled", sl.enabled);
    read(j, "No Title Bar", sl.noTitleBar);
    read<value_t::object>(j, "Pos", sl.pos);
    read<value_t::object>(j, "Size", sl.size);
}

static void from_json(const json& j, MiscConfig::Watermark& o)
{
    read(j, "Enabled", o.enabled);
}

static void from_json(const json& j, PreserveKillfeed& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Only Headshots", o.onlyHeadshots);
}

static void from_json(const json& j, MiscConfig& m)
{
    read(j, "Menu key", m.menuKey);
    read(j, "Anti AFK kick", m.antiAfkKick);
    read(j, "Auto strafe", m.autoStrafe);
    read(j, "Bunny hop", m.bunnyHop);
    read(j, "Custom clan tag", m.customClanTag);
    read(j, "Clock tag", m.clocktag);
    read(j, "Clan tag", m.clanTag, sizeof(m.clanTag));
    read(j, "Animated clan tag", m.animatedClanTag);
    read(j, "Fast duck", m.fastDuck);
    read(j, "Moonwalk", m.moonwalk);
    read(j, "Edge Jump", m.edgejump);
    read(j, "Edge Jump Key", m.edgejumpkey);
    read(j, "Slowwalk", m.slowwalk);
    read(j, "Slowwalk key", m.slowwalkKey);
    read<value_t::object>(j, "Noscope crosshair", m.noscopeCrosshair);
    read<value_t::object>(j, "Recoil crosshair", m.recoilCrosshair);
    read(j, "Auto pistol", m.autoPistol);
    read(j, "Auto reload", m.autoReload);
    read(j, "Auto accept", m.autoAccept);
    read(j, "Radar hack", m.radarHack);
    read(j, "Reveal ranks", m.revealRanks);
    read(j, "Reveal money", m.revealMoney);
    read(j, "Reveal suspect", m.revealSuspect);
    read(j, "Reveal votes", m.revealVotes);
    read<value_t::object>(j, "Spectator list", m.spectatorList);
    read<value_t::object>(j, "Watermark", m.watermark);
    read<value_t::object>(j, "Offscreen Enemies", m.offscreenEnemies);
    read(j, "Fix animation LOD", m.fixAnimationLOD);
    read(j, "Fix movement", m.fixMovement);
    read(j, "Disable model occlusion", m.disableModelOcclusion);
    read(j, "Aspect Ratio", m.aspectratio);
    read(j, "Kill message", m.killMessage);
    read<value_t::string>(j, "Kill message string", m.killMessageString);
    read(j, "Name stealer", m.nameStealer);
    read(j, "Disable HUD blur", m.disablePanoramablur);
    read(j, "Ban color", m.banColor);
    read<value_t::string>(j, "Ban text", m.banText);
    read(j, "Fast plant", m.fastPlant);
    read(j, "Fast Stop", m.fastStop);
    read<value_t::object>(j, "Bomb timer", m.bombTimer);
    read(j, "Quick reload", m.quickReload);
    read(j, "Prepare revolver", m.prepareRevolver);
    read(j, "Prepare revolver key", m.prepareRevolverKey);
    read(j, "Hit sound", m.hitSound);
    read(j, "Choked packets", m.chokedPackets);
    read(j, "Choked packets key", m.chokedPacketsKey);
    read(j, "Quick healthshot key", m.quickHealthshotKey);
    read(j, "Grenade predict", m.nadePredict);
    read(j, "Fix tablet signal", m.fixTabletSignal);
    read(j, "Max angle delta", m.maxAngleDelta);
    read(j, "Fix tablet signal", m.fixTabletSignal);
    read<value_t::string>(j, "Custom Hit Sound", m.customHitSound);
    read(j, "Kill sound", m.killSound);
    read<value_t::string>(j, "Custom Kill Sound", m.customKillSound);
    read<value_t::object>(j, "Purchase List", m.purchaseList);
    read<value_t::object>(j, "Reportbot", m.reportbot);
    read(j, "Opposite Hand Knife", m.oppositeHandKnife);
    read<value_t::object>(j, "Preserve Killfeed", m.preserveKillfeed);
}

static void from_json(const json& j, MiscConfig::Reportbot& r)
{
    read(j, "Enabled", r.enabled);
    read(j, "Target", r.target);
    read(j, "Delay", r.delay);
    read(j, "Rounds", r.rounds);
    read(j, "Abusive Communications", r.textAbuse);
    read(j, "Griefing", r.griefing);
    read(j, "Wall Hacking", r.wallhack);
    read(j, "Aim Hacking", r.aimbot);
    read(j, "Other Hacking", r.other);
}

static void to_json(json& j, const MiscConfig::Reportbot& o, const MiscConfig::Reportbot& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Target", target);
    WRITE("Delay", delay);
    WRITE("Rounds", rounds);
    WRITE("Abusive Communications", textAbuse);
    WRITE("Griefing", griefing);
    WRITE("Wall Hacking", wallhack);
    WRITE("Aim Hacking", aimbot);
    WRITE("Other Hacking", other);
}

static void to_json(json& j, const PurchaseList& o, const PurchaseList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Only During Freeze Time", onlyDuringFreezeTime);
    WRITE("Show Prices", showPrices);
    WRITE("No Title Bar", noTitleBar);
    WRITE("Mode", mode);
}

static void to_json(json& j, const ImVec2& o, const ImVec2& dummy = {})
{
    WRITE("X", x);
    WRITE("Y", y);
}

static void to_json(json& j, const OffscreenEnemies& o, const OffscreenEnemies& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);

    WRITE("Health Bar", healthBar);
}

static void to_json(json& j, const MiscConfig::SpectatorList& o, const MiscConfig::SpectatorList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);

    if (const auto window = ImGui::FindWindowByName("Spectator list")) {
        j["Pos"] = window->Pos;
        j["Size"] = window->SizeFull;
    }
}

static void to_json(json& j, const MiscConfig::Watermark& o, const MiscConfig::Watermark& dummy = {})
{
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const PreserveKillfeed& o, const PreserveKillfeed& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Only Headshots", onlyHeadshots);
}

static void to_json(json& j, const MiscConfig& o)
{
    const MiscConfig dummy;

    WRITE("Menu key", menuKey);
    WRITE("Anti AFK kick", antiAfkKick);
    WRITE("Auto strafe", autoStrafe);
    WRITE("Bunny hop", bunnyHop);
    WRITE("Custom clan tag", customClanTag);
    WRITE("Clock tag", clocktag);

    if (o.clanTag[0])
        j["Clan tag"] = o.clanTag;

    WRITE("Animated clan tag", animatedClanTag);
    WRITE("Fast duck", fastDuck);
    WRITE("Moonwalk", moonwalk);
    WRITE("Edge Jump", edgejump);
    WRITE("Edge Jump Key", edgejumpkey);
    WRITE("Slowwalk", slowwalk);
    WRITE("Slowwalk key", slowwalkKey);
    WRITE("Noscope crosshair", noscopeCrosshair);
    WRITE("Recoil crosshair", recoilCrosshair);
    WRITE("Auto pistol", autoPistol);
    WRITE("Auto reload", autoReload);
    WRITE("Auto accept", autoAccept);
    WRITE("Radar hack", radarHack);
    WRITE("Reveal ranks", revealRanks);
    WRITE("Reveal money", revealMoney);
    WRITE("Reveal suspect", revealSuspect);
    WRITE("Reveal votes", revealVotes);
    WRITE("Spectator list", spectatorList);
    WRITE("Watermark", watermark);
    WRITE("Offscreen Enemies", offscreenEnemies);
    WRITE("Fix animation LOD", fixAnimationLOD);
    WRITE("Fix movement", fixMovement);
    WRITE("Disable model occlusion", disableModelOcclusion);
    WRITE("Aspect Ratio", aspectratio);
    WRITE("Kill message", killMessage);
    WRITE("Kill message string", killMessageString);
    WRITE("Name stealer", nameStealer);
    WRITE("Disable HUD blur", disablePanoramablur);
    WRITE("Ban color", banColor);
    WRITE("Ban text", banText);
    WRITE("Fast plant", fastPlant);
    WRITE("Fast Stop", fastStop);
    WRITE("Bomb timer", bombTimer);
    WRITE("Quick reload", quickReload);
    WRITE("Prepare revolver", prepareRevolver);
    WRITE("Prepare revolver key", prepareRevolverKey);
    WRITE("Hit sound", hitSound);
    WRITE("Choked packets", chokedPackets);
    WRITE("Choked packets key", chokedPacketsKey);
    WRITE("Quick healthshot key", quickHealthshotKey);
    WRITE("Grenade predict", nadePredict);
    WRITE("Fix tablet signal", fixTabletSignal);
    WRITE("Max angle delta", maxAngleDelta);
    WRITE("Fix tablet signal", fixTabletSignal);
    WRITE("Custom Hit Sound", customHitSound);
    WRITE("Kill sound", killSound);
    WRITE("Custom Kill Sound", customKillSound);
    WRITE("Purchase List", purchaseList);
    WRITE("Reportbot", reportbot);
    WRITE("Opposite Hand Knife", oppositeHandKnife);
    WRITE("Preserve Killfeed", preserveKillfeed);
}

json Misc::toJson() noexcept
{
    json j;
    to_json(j, miscConfig);
    return j;
}

void Misc::fromJson(const json& j) noexcept
{
    from_json(j, miscConfig);
}

void Misc::resetConfig() noexcept
{
    miscConfig = {};
}
