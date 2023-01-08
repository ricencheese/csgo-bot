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
    bool isbotzon{ false };

    int botState = 0;

    bool shouldwalk{ false };               //walkbot vars
    
    
    float waypointApproximation{ 15.f };
    int editWaypoint{ 1 };

    std::vector<csgo::Vector> waypoints{ {0,0,0} }, nodes{ {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} };
    int nodeRadius{ 33 };
    csgo::Vector lastCheckPos{ 0,0,0 };
    float posDelta{ 0.f };
    float lastCheckTime{ 0.f };
    std::vector<float>gcost{ 0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f }, hcost{ 0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f }, fcost{ 0.f,0.f,0.f,0.f,9999999.f,0.f,0.f,0.f,0.f };
    std::array<bool,9>walk{ false,false,false,false,false,false,false,false,false }, crouch{ false,false,false,false,false,false,false,false,false }, crouchJump{ false,false,false,false,false,false,false,false,false };
    //^if I do everything right,this should be obsolete soon


    std::vector<csgo::Vector> openNodes;
    std::vector<csgo::Vector> closedNodes;
    std::vector<int>openNodesParents, closedNodesParents;
    int currentNode{ 0 };
    bool pathFound{ false };
    float dropdownDmg{ 0.f };
    std::vector<csgo::Trace>tracez;
    csgo::Vector checkOrigin;
    csgo::Vector tempFloorPos{ 0,0,0 };
    //first: same as collisionCheck(); second: true if open, false if closed
    std::vector<std::pair<int, bool>>nodeIndex;


    csgo::Vector playerPingLoc{ 0,0,0 };

    float hitglass, hitdoors;
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

    bool aimAtEvents{ true };               //aimbot shii
    int aimreason{ -1 };                     //-1:not aiming at anything, 0:weapon_fire event 1:enemy 2:teammates 3:windows
    bool isShooterVisible{false};
    float reactionTime{ 0.135f };
    csgo::Vector aimspot{ 0,0,0 };
    float startedAiming{ -1.f };
    std::vector<float>aimtime{ 0.1f};
    csgo::Vector localViewAngles{ 0,0,0 };
    float lastTimeSawEnemy{ 0.f };
    bool shouldFire{ false };

    csgo::Trace traceTesting;

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

void Misc::edgejump(csgo::UserCmd* cmd) noexcept
{
    if (!miscConfig.edgejump || !miscConfig.edgejumpkey.isDown())
        return;

    if (!localPlayer || !localPlayer.get().isAlive())
        return;

    if (const auto mt = localPlayer.get().moveType(); mt == MoveType::LADDER || mt == MoveType::NOCLIP)
        return;

    if ((EnginePrediction::getFlags() & 1) && !localPlayer.get().isOnGround())
        cmd->buttons |= csgo::UserCmd::IN_JUMP;
}

void Misc::slowwalk(csgo::UserCmd* cmd) noexcept
{
    if (!miscConfig.slowwalk || !miscConfig.slowwalkKey.isDown())
        return;

    if (!localPlayer || !localPlayer.get().isAlive())
        return;

    const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon());
    if (activeWeapon.getPOD() == nullptr)
        return;

    const auto weaponData = activeWeapon.getWeaponData();
    if (!weaponData)
        return;

    const float maxSpeed = (localPlayer.get().isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed) / 3;

    if (cmd->forwardmove && cmd->sidemove) {
        const float maxSpeedRoot = maxSpeed * static_cast<float>(M_SQRT1_2);
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
    } else if (cmd->forwardmove) {
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeed : maxSpeed;
    } else if (cmd->sidemove) {
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeed : maxSpeed;
    }
}

void Misc::updateClanTag(bool tagChanged) noexcept
{
    static std::string clanTag;

    if (tagChanged) {
        clanTag = miscConfig.clanTag;
        if (!clanTag.empty() && clanTag.front() != ' ' && clanTag.back() != ' ')
            clanTag.push_back(' ');
        return;
    }
    
    static auto lastTime = 0.0f;

    if (miscConfig.clocktag) {
        if (memory.globalVars->realtime - lastTime < 1.0f)
            return;

        const auto time = std::time(nullptr);
        const auto localTime = std::localtime(&time);
        char s[11];
        s[0] = '\0';
        snprintf(s, sizeof(s), "[%02d:%02d:%02d]", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
        lastTime = memory.globalVars->realtime;
        setClanTag(s, s);
    } else if (miscConfig.customClanTag) {
        if (memory.globalVars->realtime - lastTime < 0.6f)
            return;

        if (miscConfig.animatedClanTag && !clanTag.empty()) {
            if (const auto offset = Helpers::utf8SeqLen(clanTag[0]); offset <= clanTag.length())
                std::rotate(clanTag.begin(), clanTag.begin() + offset, clanTag.end());
        }
        lastTime = memory.globalVars->realtime;
        setClanTag(clanTag.c_str(), clanTag.c_str());
    }
}

void Misc::spectatorList() noexcept
{
    if (!miscConfig.spectatorList.enabled)
        return;

    GameData::Lock lock;

    const auto& observers = GameData::observers();

    if (std::ranges::none_of(observers, [](const auto& obs) { return obs.targetIsLocalPlayer; }) && !gui->isOpen())
        return;

    if (miscConfig.spectatorList.pos != ImVec2{}) {
        ImGui::SetNextWindowPos(miscConfig.spectatorList.pos);
        miscConfig.spectatorList.pos = {};
    }

    if (miscConfig.spectatorList.size != ImVec2{}) {
        ImGui::SetNextWindowSize(ImClamp(miscConfig.spectatorList.size, {}, ImGui::GetIO().DisplaySize));
        miscConfig.spectatorList.size = {};
    }

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
    if (!gui->isOpen())
        windowFlags |= ImGuiWindowFlags_NoInputs;
    if (miscConfig.spectatorList.noTitleBar)
        windowFlags |= ImGuiWindowFlags_NoTitleBar;

    if (!gui->isOpen())
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetColorU32(ImGuiCol_TitleBgActive));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
    ImGui::Begin("Spectator list", nullptr, windowFlags);
    ImGui::PopStyleVar();

    if (!gui->isOpen())
        ImGui::PopStyleColor();

    for (const auto& observer : observers) {
        if (!observer.targetIsLocalPlayer)
            continue;

        if (const auto it = std::ranges::find(GameData::players(), observer.playerHandle, &PlayerData::handle); it != GameData::players().cend()) {
            if (const auto texture = it->getAvatarTexture()) {
                const auto textSize = ImGui::CalcTextSize(it->name.c_str());
                ImGui::Image(texture, ImVec2(textSize.y, textSize.y), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.3f));
                ImGui::SameLine();
                ImGui::TextWrapped("%s", it->name.c_str());
            }
        }
    }

    ImGui::End();
}

static void drawCrosshair(ImDrawList* drawList, const ImVec2& pos, ImU32 color) noexcept
{
    // dot
    drawList->AddRectFilled(pos - ImVec2{ 1, 1 }, pos + ImVec2{ 2, 2 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(pos, pos + ImVec2{ 1, 1 }, color);

    // left
    drawList->AddRectFilled(ImVec2{ pos.x - 11, pos.y - 1 }, ImVec2{ pos.x - 3, pos.y + 2 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(ImVec2{ pos.x - 10, pos.y }, ImVec2{ pos.x - 4, pos.y + 1 }, color);

    // right
    drawList->AddRectFilled(ImVec2{ pos.x + 4, pos.y - 1 }, ImVec2{ pos.x + 12, pos.y + 2 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(ImVec2{ pos.x + 5, pos.y }, ImVec2{ pos.x + 11, pos.y + 1 }, color);

    // top (left with swapped x/y offsets)
    drawList->AddRectFilled(ImVec2{ pos.x - 1, pos.y - 11 }, ImVec2{ pos.x + 2, pos.y - 3 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(ImVec2{ pos.x, pos.y - 10 }, ImVec2{ pos.x + 1, pos.y - 4 }, color);

    // bottom (right with swapped x/y offsets)
    drawList->AddRectFilled(ImVec2{ pos.x - 1, pos.y + 4 }, ImVec2{ pos.x + 2, pos.y + 12 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(ImVec2{ pos.x, pos.y + 5 }, ImVec2{ pos.x + 1, pos.y + 11 }, color);
}

void Misc::noscopeCrosshair(ImDrawList* drawList) noexcept
{
    if (!miscConfig.noscopeCrosshair.asColorToggle().enabled)
        return;

    {
        GameData::Lock lock;
        if (const auto& local = GameData::local(); !local.exists || !local.alive || !local.noScope)
            return;
    }

    drawCrosshair(drawList, ImGui::GetIO().DisplaySize / 2, Helpers::calculateColor(memory.globalVars->realtime, miscConfig.noscopeCrosshair.asColorToggle().asColor4()));
}

void Misc::recoilCrosshair(ImDrawList* drawList) noexcept
{
    if (!miscConfig.recoilCrosshair.asColorToggle().enabled)
        return;

    GameData::Lock lock;
    const auto& localPlayerData = GameData::local();

    if (!localPlayerData.exists || !localPlayerData.alive)
        return;

    if (!localPlayerData.shooting)
        return;

    if (ImVec2 pos; Helpers::worldToScreenPixelAligned(localPlayerData.aimPunch, pos))
        drawCrosshair(drawList, pos, Helpers::calculateColor(memory.globalVars->realtime, miscConfig.recoilCrosshair.asColorToggle().asColor4()));
}

void Misc::watermark() noexcept
{
    if (!miscConfig.watermark.enabled)
        return;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
    if (!gui->isOpen())
        windowFlags |= ImGuiWindowFlags_NoInputs;

    ImGui::SetNextWindowBgAlpha(0.3f);
    ImGui::Begin("Watermark", nullptr, windowFlags);

    static auto frameRate = 1.0f;
    frameRate = 0.9f * frameRate + 0.1f * memory.globalVars->absoluteFrameTime;

    ImGui::Text("Osiris | %d fps | %d ms", frameRate != 0.0f ? static_cast<int>(1 / frameRate) : 0, GameData::getNetOutgoingLatency());
    ImGui::End();
}

void Misc::prepareRevolver(const csgo::Engine& engine, csgo::UserCmd* cmd) noexcept
{
    auto timeToTicks = [this](float time) {  return static_cast<int>(0.5f + time / memory.globalVars->intervalPerTick); };
    constexpr float revolverPrepareTime{ 0.234375f };

    static float readyTime;
    if (miscConfig.prepareRevolver && localPlayer && (!miscConfig.prepareRevolverKey.isSet() || miscConfig.prepareRevolverKey.isDown())) {
        const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon());
        if (activeWeapon.getPOD() != nullptr && activeWeapon.itemDefinitionIndex() == WeaponId::Revolver) {
            if (!readyTime) readyTime = memory.globalVars->serverTime() + revolverPrepareTime;
            auto ticksToReady = timeToTicks(readyTime - memory.globalVars->serverTime() - csgo::NetworkChannel::from(retSpoofGadgets->client, engine.getNetworkChannel()).getLatency(0));
            if (ticksToReady > 0 && ticksToReady <= timeToTicks(revolverPrepareTime))
                cmd->buttons |= csgo::UserCmd::IN_ATTACK;
            else
                readyTime = 0.0f;
        }
    }
}

void Misc::fastPlant(const csgo::EngineTrace& engineTrace, csgo::UserCmd* cmd) noexcept
{
    if (!miscConfig.fastPlant)
        return;

    if (static auto plantAnywhere = csgo::ConVar::from(retSpoofGadgets->client, interfaces.getCvar().findVar(csgo::mp_plant_c4_anywhere)); plantAnywhere.getInt())
        return;

    if (!localPlayer || !localPlayer.get().isAlive() || (localPlayer.get().inBombZone() && localPlayer.get().isOnGround()))
        return;

    if (const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon()); activeWeapon.getPOD() == nullptr || activeWeapon.getNetworkable().getClientClass()->classId != ClassId::C4)
        return;

    cmd->buttons &= ~csgo::UserCmd::IN_ATTACK;

    constexpr auto doorRange = 200.0f;

    csgo::Trace trace;
    const auto startPos = localPlayer.get().getEyePosition();
    const auto endPos = startPos + csgo::Vector::fromAngle(cmd->viewangles) * doorRange;
    engineTrace.traceRay({ startPos, endPos }, 0x46004009, localPlayer.get().getPOD(), trace);

    const auto entity = csgo::Entity::from(retSpoofGadgets->client, trace.entity);
    if (entity.getPOD() == nullptr || entity.getNetworkable().getClientClass()->classId != ClassId::PropDoorRotating)
        cmd->buttons &= ~csgo::UserCmd::IN_USE;
}

void Misc::fastStop(csgo::UserCmd* cmd) noexcept
{
    if (!miscConfig.fastStop)
        return;

    if (!localPlayer || !localPlayer.get().isAlive())
        return;

    if (localPlayer.get().moveType() == MoveType::NOCLIP || localPlayer.get().moveType() == MoveType::LADDER || !localPlayer.get().isOnGround() || cmd->buttons & csgo::UserCmd::IN_JUMP)
        return;

    if (cmd->buttons & (csgo::UserCmd::IN_MOVELEFT | csgo::UserCmd::IN_MOVERIGHT | csgo::UserCmd::IN_FORWARD | csgo::UserCmd::IN_BACK))
        return;
    
    const auto velocity = localPlayer.get().velocity();
    const auto speed = velocity.length2D();
    if (speed < 15.0f)
        return;
    
    csgo::Vector direction = velocity.toAngle();
    direction.y = cmd->viewangles.y - direction.y;

    const auto negatedDirection = csgo::Vector::fromAngle(direction) * -speed;
    cmd->forwardmove = negatedDirection.x;
    cmd->sidemove = negatedDirection.y;
}

void Misc::drawBombTimer() noexcept
{
    if (!miscConfig.bombTimer.enabled)
        return;

    GameData::Lock lock;
    
    const auto& plantedC4 = GameData::plantedC4();
    if (plantedC4.blowTime == 0.0f && !gui->isOpen())
        return;

    if (!gui->isOpen()) {
        ImGui::SetNextWindowBgAlpha(0.3f);
    }

    static float windowWidth = 200.0f;
    ImGui::SetNextWindowPos({ (ImGui::GetIO().DisplaySize.x - 200.0f) / 2.0f, 60.0f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ windowWidth, 0 }, ImGuiCond_Once);

    if (!gui->isOpen())
        ImGui::SetNextWindowSize({ windowWidth, 0 });

    ImGui::SetNextWindowSizeConstraints({ 0, -1 }, { FLT_MAX, -1 });
    ImGui::Begin("Bomb Timer", nullptr, ImGuiWindowFlags_NoTitleBar | (gui->isOpen() ? 0 : ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration));

    std::ostringstream ss; ss << "Bomb on " << (!plantedC4.bombsite ? 'A' : 'B') << " : " << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(plantedC4.blowTime - memory.globalVars->currenttime, 0.0f) << " s";

    ImGui::textUnformattedCentered(ss.str().c_str());

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Helpers::calculateColor(memory.globalVars->realtime, miscConfig.bombTimer.asColor3()));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
    ImGui::progressBarFullWidth((plantedC4.blowTime - memory.globalVars->currenttime) / plantedC4.timerLength, 5.0f);

    if (plantedC4.defuserHandle != -1) {
        const bool canDefuse = plantedC4.blowTime >= plantedC4.defuseCountDown;

        if (plantedC4.defuserHandle == GameData::local().handle) {
            if (canDefuse) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                ImGui::textUnformattedCentered("You can defuse!");
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                ImGui::textUnformattedCentered("You can not defuse!");
            }
            ImGui::PopStyleColor();
        } else if (const auto defusingPlayer = GameData::playerByHandle(plantedC4.defuserHandle)) {
            std::ostringstream ss; ss << defusingPlayer->name << " is defusing: " << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(plantedC4.defuseCountDown - memory.globalVars->currenttime, 0.0f) << " s";

            ImGui::textUnformattedCentered(ss.str().c_str());

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, canDefuse ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
            ImGui::progressBarFullWidth((plantedC4.defuseCountDown - memory.globalVars->currenttime) / plantedC4.defuseLength, 5.0f);
            ImGui::PopStyleColor();
        }
    }

    windowWidth = ImGui::GetCurrentWindow()->SizeFull.x;

    ImGui::PopStyleColor(2);
    ImGui::End();
}

void Misc::stealNames(const csgo::Engine& engine) noexcept
{
    if (!miscConfig.nameStealer)
        return;

    if (!localPlayer)
        return;

    static std::vector<int> stolenIds;

    for (int i = 1; i <= memory.globalVars->maxClients; ++i) {
        const auto entityPtr = clientInterfaces.getEntityList().getEntity(i);
        const auto entity = csgo::Entity::from(retSpoofGadgets->client, entityPtr);

        if (entity.getPOD() == nullptr || entity.getPOD() == localPlayer.get().getPOD())
            continue;

        csgo::PlayerInfo playerInfo;
        if (!engine.getPlayerInfo(entity.getNetworkable().index(), playerInfo))
            continue;

        if (playerInfo.fakeplayer || std::ranges::find(stolenIds, playerInfo.userId) != stolenIds.cend())
            continue;

        if (changeName(engine, false, (std::string{ playerInfo.name } +'\x1').c_str(), 1.0f))
            stolenIds.push_back(playerInfo.userId);

        return;
    }
    stolenIds.clear();
}

void Misc::disablePanoramablur() noexcept
{
    static auto blur = interfaces.getCvar().findVar(csgo::panorama_disable_blur);
    csgo::ConVar::from(retSpoofGadgets->client, blur).setValue(miscConfig.disablePanoramablur);
}

void Misc::quickReload(csgo::UserCmd* cmd) noexcept
{
    if (miscConfig.quickReload) {
        static csgo::EntityPOD* reloadedWeapon = nullptr;

        if (reloadedWeapon) {
            for (auto weaponHandle : localPlayer.get().weapons()) {
                if (weaponHandle == -1)
                    break;

                if (clientInterfaces.getEntityList().getEntityFromHandle(weaponHandle) == reloadedWeapon) {
                    cmd->weaponselect = csgo::Entity::from(retSpoofGadgets->client, reloadedWeapon).getNetworkable().index();
                    cmd->weaponsubtype = csgo::Entity::from(retSpoofGadgets->client, reloadedWeapon).getWeaponSubType();
                    break;
                }
            }
            reloadedWeapon = nullptr;
        }

        if (const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon()); activeWeapon.getPOD() != nullptr && activeWeapon.isInReload() && activeWeapon.clip() == activeWeapon.getWeaponData()->maxClip) {
            reloadedWeapon = activeWeapon.getPOD();

            for (auto weaponHandle : localPlayer.get().weapons()) {
                if (weaponHandle == -1)
                    break;

                if (const auto weapon = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntityFromHandle(weaponHandle)); weapon.getPOD() && weapon.getPOD() != reloadedWeapon) {
                    cmd->weaponselect = weapon.getNetworkable().index();
                    cmd->weaponsubtype = weapon.getWeaponSubType();
                    break;
                }
            }
        }
    }
}

bool Misc::changeName(const csgo::Engine& engine, bool reconnect, const char* newName, float delay) noexcept
{
    static auto exploitInitialized{ false };

    static auto name{ interfaces.getCvar().findVar(csgo::name) };

    if (reconnect) {
        exploitInitialized = false;
        return false;
    }

    if (!exploitInitialized && engine.isInGame()) {
        if (csgo::PlayerInfo playerInfo; localPlayer && engine.getPlayerInfo(localPlayer.get().getNetworkable().index(), playerInfo) && (!strcmp(playerInfo.name, "?empty") || !strcmp(playerInfo.name, "\n\xAD\xAD\xAD"))) {
            exploitInitialized = true;
        } else {
            name->onChangeCallbacks.size = 0;
            csgo::ConVar::from(retSpoofGadgets->client, name).setValue("\n\xAD\xAD\xAD");
            return false;
        }
    }

    if (static auto nextChangeTime = 0.0f; nextChangeTime <= memory.globalVars->realtime) {
        csgo::ConVar::from(retSpoofGadgets->client, name).setValue(newName);
        nextChangeTime = memory.globalVars->realtime + delay;
        return true;
    }
    return false;
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

void Misc::fakeBan(const csgo::Engine& engine, bool set) noexcept
{
    static bool shouldSet = false;

    if (set)
        shouldSet = set;

    if (shouldSet && engine.isInGame() && changeName(engine, false, std::string{ "\x1\xB" }.append(std::string{ static_cast<char>(miscConfig.banColor + 1) }).append(miscConfig.banText).append("\x1").c_str(), 5.0f))
        shouldSet = false;
}

void Misc::nadePredict() noexcept
{
    static auto nadeVar{ interfaces.getCvar().findVar(csgo::cl_grenadepreview) };

    nadeVar->onChangeCallbacks.size = 0;
    csgo::ConVar::from(retSpoofGadgets->client, nadeVar).setValue(miscConfig.nadePredict);
}

void Misc::fixTabletSignal() noexcept
{
    if (miscConfig.fixTabletSignal && localPlayer) {
        if (const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon()); activeWeapon.getPOD() != nullptr && activeWeapon.getNetworkable().getClientClass()->classId == ClassId::Tablet)
            activeWeapon.tabletReceptionIsBlocked() = false;
    }
}

void Misc::killMessage(const csgo::Engine& engine, const csgo::GameEvent& event) noexcept
{
    if (!miscConfig.killMessage)
        return;

    if (!localPlayer || !localPlayer.get().isAlive())
        return;

    if (const auto localUserId = localPlayer.get().getUserId(engine); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    std::string cmd = "say \"";
    cmd += miscConfig.killMessageString;
    cmd += '"';
    engine.clientCmdUnrestricted(cmd.c_str());
}

void Misc::fixMovement(csgo::UserCmd* cmd, float yaw) noexcept
{
    if (miscConfig.fixMovement) {
        float oldYaw = yaw + (yaw < 0.0f ? 360.0f : 0.0f);
        float newYaw = cmd->viewangles.y + (cmd->viewangles.y < 0.0f ? 360.0f : 0.0f);
        float yawDelta = newYaw < oldYaw ? fabsf(newYaw - oldYaw) : 360.0f - fabsf(newYaw - oldYaw);
        yawDelta = 360.0f - yawDelta;

        const float forwardmove = cmd->forwardmove;
        const float sidemove = cmd->sidemove;
        cmd->forwardmove = std::cos(Helpers::deg2rad(yawDelta)) * forwardmove + std::cos(Helpers::deg2rad(yawDelta + 90.0f)) * sidemove;
        cmd->sidemove = std::sin(Helpers::deg2rad(yawDelta)) * forwardmove + std::sin(Helpers::deg2rad(yawDelta + 90.0f)) * sidemove;
    }
}

void Misc::antiAfkKick(csgo::UserCmd* cmd) noexcept
{
    if (miscConfig.antiAfkKick && cmd->commandNumber % 2)
        cmd->buttons |= 1 << 27;
}

void Misc::fixAnimationLOD(const csgo::Engine& engine, csgo::FrameStage stage) noexcept
{
#if IS_WIN32()
    if (miscConfig.fixAnimationLOD && stage == csgo::FrameStage::RENDER_START) {
        if (!localPlayer)
            return;

        for (int i = 1; i <= engine.getMaxClients(); i++) {
            const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(i));
            if (entity.getPOD() == nullptr || entity.getPOD() == localPlayer.get().getPOD() || entity.getNetworkable().isDormant() || !entity.isAlive()) continue;
            *reinterpret_cast<int*>(std::uintptr_t(entity.getPOD()) + 0xA28) = 0;
            *reinterpret_cast<int*>(std::uintptr_t(entity.getPOD()) + 0xA30) = memory.globalVars->framecount;
        }
    }
#endif
}

void Misc::autoPistol(csgo::UserCmd* cmd) noexcept
{
    if (miscConfig.autoPistol && localPlayer) {
        const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon());
        if (activeWeapon.getPOD() != nullptr && activeWeapon.isPistol() && activeWeapon.nextPrimaryAttack() > memory.globalVars->serverTime()) {
            if (activeWeapon.itemDefinitionIndex() == WeaponId::Revolver)
                cmd->buttons &= ~csgo::UserCmd::IN_ATTACK2;
            else
                cmd->buttons &= ~csgo::UserCmd::IN_ATTACK;
        }
    }
}

void Misc::chokePackets(const csgo::Engine& engine, bool& sendPacket) noexcept
{
    if (!miscConfig.chokedPacketsKey.isSet() || miscConfig.chokedPacketsKey.isDown())
        sendPacket = engine.getNetworkChannel()->chokedPackets >= miscConfig.chokedPackets;
}

void Misc::autoReload(csgo::UserCmd* cmd) noexcept
{
    if (miscConfig.autoReload && localPlayer) {
        const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon());
        if (activeWeapon.getPOD() != nullptr && getWeaponIndex(activeWeapon.itemDefinitionIndex()) && !activeWeapon.clip())
            cmd->buttons &= ~(csgo::UserCmd::IN_ATTACK | csgo::UserCmd::IN_ATTACK2); //!!!!!!!!
    }
}

void Misc::revealRanks(csgo::UserCmd* cmd) noexcept
{
    if (miscConfig.revealRanks && cmd->buttons & csgo::UserCmd::IN_SCORE)
        clientInterfaces.getClient().dispatchUserMessage(50, 0, 0, nullptr);
}

void Misc::autoStrafe(csgo::UserCmd* cmd) noexcept
{
    if (localPlayer
        && miscConfig.autoStrafe
        && !localPlayer.get().isOnGround()
        && localPlayer.get().moveType() != MoveType::NOCLIP) {
        if (cmd->mousedx < 0)
            cmd->sidemove = -450.0f;
        else if (cmd->mousedx > 0)
            cmd->sidemove = 450.0f;
    }
}

void Misc::removeCrouchCooldown(csgo::UserCmd* cmd) noexcept
{
    if (miscConfig.fastDuck)
        cmd->buttons |= csgo::UserCmd::IN_BULLRUSH;
}

void Misc::moonwalk(csgo::UserCmd* cmd) noexcept
{
    if (miscConfig.moonwalk && localPlayer && localPlayer.get().moveType() != MoveType::LADDER)
        cmd->buttons ^= csgo::UserCmd::IN_FORWARD | csgo::UserCmd::IN_BACK | csgo::UserCmd::IN_MOVELEFT | csgo::UserCmd::IN_MOVERIGHT;
}

void Misc::playHitSound(const csgo::Engine& engine, const csgo::GameEvent& event) noexcept
{
    if (!miscConfig.hitSound)
        return;

    if (!localPlayer)
        return;

    if (const auto localUserId = localPlayer.get().getUserId(engine); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    static constexpr std::array hitSounds{
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play training/timer_bell",
        "play physics/glass/glass_impact_bullet1"
    };

    if (static_cast<std::size_t>(miscConfig.hitSound - 1) < hitSounds.size())
        engine.clientCmdUnrestricted(hitSounds[miscConfig.hitSound - 1]);
    else if (miscConfig.hitSound == 5)
        engine.clientCmdUnrestricted(("play " + miscConfig.customHitSound).c_str());
}

void Misc::killSound(const csgo::Engine& engine, const csgo::GameEvent& event) noexcept
{
    if (!miscConfig.killSound)
        return;

    if (!localPlayer || !localPlayer.get().isAlive())
        return;

    if (const auto localUserId = localPlayer.get().getUserId(engine); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    static constexpr std::array killSounds{
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play training/timer_bell",
        "play physics/glass/glass_impact_bullet1"
    };

    if (static_cast<std::size_t>(miscConfig.killSound - 1) < killSounds.size())
        engine.clientCmdUnrestricted(killSounds[miscConfig.killSound - 1]);
    else if (miscConfig.killSound == 5)
        engine.clientCmdUnrestricted(("play " + miscConfig.customKillSound).c_str());
}

void Misc::purchaseList(const csgo::Engine& engine, const csgo::GameEvent* event) noexcept
{
    static std::mutex mtx;
    std::scoped_lock _{ mtx };

    struct PlayerPurchases {
        int totalCost;
        std::unordered_map<std::string, int> items;
    };

    static std::unordered_map<int, PlayerPurchases> playerPurchases;
    static std::unordered_map<std::string, int> purchaseTotal;
    static int totalCost;

    static auto freezeEnd = 0.0f;

    if (event) {
        switch (fnv::hashRuntime(event->getName())) {
        case fnv::hash("item_purchase"): {
            if (const auto player = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(engine.getPlayerForUserID(event->getInt("userid")))); player.getPOD() != nullptr && localPlayer && localPlayer.get().isOtherEnemy(memory, player)) {
                if (const auto definition = csgo::EconItemDefinition::from(retSpoofGadgets->client, csgo::ItemSchema::from(retSpoofGadgets->client, memory.itemSystem().getItemSchema()).getItemDefinitionByName(event->getString("weapon"))); definition.getPOD() != nullptr) {
                    auto& purchase = playerPurchases[player.handle()];
                    if (const auto weaponInfo = memory.weaponSystem.getWeaponInfo(definition.getWeaponId())) {
                        purchase.totalCost += weaponInfo->price;
                        totalCost += weaponInfo->price;
                    }
                    const std::string weapon = interfaces.getLocalize().findAsUTF8(definition.getItemBaseName());
                    ++purchaseTotal[weapon];
                    ++purchase.items[weapon];
                }
            }
            break;
        }
        case fnv::hash("round_start"):
            freezeEnd = 0.0f;
            playerPurchases.clear();
            purchaseTotal.clear();
            totalCost = 0;
            break;
        case fnv::hash("round_freeze_end"):
            freezeEnd = memory.globalVars->realtime;
            break;
        }
    } else {
        if (!miscConfig.purchaseList.enabled)
            return;

        if (static const auto mp_buytime = interfaces.getCvar().findVar(csgo::mp_buytime); (!engine.isInGame() || freezeEnd != 0.0f && memory.globalVars->realtime > freezeEnd + (!miscConfig.purchaseList.onlyDuringFreezeTime ? csgo::ConVar::from(retSpoofGadgets->client, mp_buytime).getFloat() : 0.0f) || playerPurchases.empty() || purchaseTotal.empty()) && !gui->isOpen())
            return;

        ImGui::SetNextWindowSize({ 200.0f, 200.0f }, ImGuiCond_Once);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
        if (!gui->isOpen())
            windowFlags |= ImGuiWindowFlags_NoInputs;
        if (miscConfig.purchaseList.noTitleBar)
            windowFlags |= ImGuiWindowFlags_NoTitleBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
        ImGui::Begin("Purchases", nullptr, windowFlags);
        ImGui::PopStyleVar();

        if (miscConfig.purchaseList.mode == PurchaseList::Details) {
            GameData::Lock lock;

            for (const auto& [handle, purchases] : playerPurchases) {
                std::string s;
                s.reserve(std::accumulate(purchases.items.begin(), purchases.items.end(), 0, [](int length, const auto& p) { return length + p.first.length() + 2; }));
                for (const auto& purchasedItem : purchases.items) {
                    if (purchasedItem.second > 1)
                        s += std::to_string(purchasedItem.second) + "x ";
                    s += purchasedItem.first + ", ";
                }

                if (s.length() >= 2)
                    s.erase(s.length() - 2);

                if (const auto player = GameData::playerByHandle(handle)) {
                    if (miscConfig.purchaseList.showPrices)
                        ImGui::TextWrapped("%s $%d: %s", player->name.c_str(), purchases.totalCost, s.c_str());
                    else
                        ImGui::TextWrapped("%s: %s", player->name.c_str(), s.c_str());
                }
            }
        } else if (miscConfig.purchaseList.mode == PurchaseList::Summary) {
            for (const auto& purchase : purchaseTotal)
                ImGui::TextWrapped("%d x %s", purchase.second, purchase.first.c_str());

            if (miscConfig.purchaseList.showPrices && totalCost > 0) {
                ImGui::Separator();
                ImGui::TextWrapped("Total: $%d", totalCost);
            }
        }
        ImGui::End();
    }
}

void Misc::oppositeHandKnife(csgo::FrameStage stage) noexcept
{
    if (!miscConfig.oppositeHandKnife)
        return;

    if (!localPlayer)
        return;

    if (stage != csgo::FrameStage::RENDER_START && stage != csgo::FrameStage::RENDER_END)
        return;

    static const auto cl_righthand = csgo::ConVar::from(retSpoofGadgets->client, interfaces.getCvar().findVar(csgo::cl_righthand));
    static bool original;

    if (stage == csgo::FrameStage::RENDER_START) {
        original = cl_righthand.getInt();

        if (const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon()); activeWeapon.getPOD() != nullptr) {
            if (const auto classId = activeWeapon.getNetworkable().getClientClass()->classId; classId == ClassId::Knife || classId == ClassId::KnifeGG)
                cl_righthand.setValue(!original);
        }
    } else {
        cl_righthand.setValue(original);
    }
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
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    if (!botzConfig.isbotzon)
        return;
    if (!engineInterfaces.getEngine().isInGame())
        return;

    csgo::Trace traceXp; //front,back,right,left
    botzConfig.localViewAngles = cmd->viewangles;
    csgo::Vector viewangleshit{ localPlayer.get().getEyePosition().x + cos(Helpers::deg2rad(botzConfig.localViewAngles.y+19.9f*botzConfig.tracerayAngle)) * 60,
                                localPlayer.get().getEyePosition().y + sin(Helpers::deg2rad(botzConfig.localViewAngles.y + 19.9f * botzConfig.tracerayAngle)) * 60,
                                localPlayer.get().getEyePosition().z };
    const csgo::Vector lPlayerEyes = localPlayer.get().getEyePosition();
    engineInterfaces.engineTrace().traceRay({ lPlayerEyes, viewangleshit}, 0x2, localPlayer.get().getPOD(), traceXp);
    if (botzConfig.tracerayAngle < 9)
        botzConfig.tracerayAngle++;
    else botzConfig.tracerayAngle = -9;
    if (traceXp.fraction < 1.0f&&botzConfig.startedAiming==-1.f) {
        botzConfig.aimspot = viewangleshit;
        botzConfig.isShooterVisible = true;
        botzConfig.aimreason = 3;
    }
    if (botzConfig.shouldFire) {
        cmd->buttons |= csgo::UserCmd::IN_ATTACK;
        botzConfig.shouldFire = false;
    }
}

int fallDamageCheck(const EngineInterfaces& engineInterfaces,csgo::Vector pos) noexcept { 
    if (!localPlayer)//todo: add like a bool to check if you want to print the falldamage value to the console
        return -1;
    if (!localPlayer.get().isAlive())
        return -1;
    //-0.000128943x^{2}+0.341019x-65.806 <- calculates fall damage kinda well 

    csgo::Trace trace;
    float fallHeight,fallDamage;
                                                                
    engineInterfaces.engineTrace().traceRay({ pos,{pos.x,pos.y,pos.z -1500.f} }, 0x46004009, localPlayer.get().getPOD(), trace);
    fallHeight = pos.distTo(trace.endpos);
    botzConfig.tempFloorPos = { pos.x,pos.y,pos.z - fallHeight };
    fallDamage = -0.000128943f * pow(fallHeight, 2.f) + 0.341019f * fallHeight - 65.806f;


    return int(fallDamage);
}


//return 0 if there's no way to get to desired position, 1 if you can walk to get to pos,
//2 if you can get to pos by jumping,3 if you can get to pos by crouching, 4 if dropping
//down will cause you great pain (fall damage is over x% of health left)
int collisionCheck(const EngineInterfaces& engineInterfaces,csgo::Vector pos) noexcept {

    if (!localPlayer)
        return -1;
    //if (!botzConfig.shouldwalk)
    //    return;

    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return -1;
    botzConfig.tracez.clear();
    if (fallDamageCheck(engineInterfaces, pos) > localPlayer.get().health()*botzConfig.dropdownDmg)
        return 4;
    pos.z = botzConfig.tempFloorPos.z;      //set node pos to floor height

    botzConfig.checkOrigin = { pos.x, pos.y, pos.z + 18 };//{ pos.x,pos.y,pos.z + 18.f };
    //H for horizontal, V for vertical, D for diagonal
    csgo::Trace traceHBottom1, traceHBottom2, traceHBottomD1, traceHBottomD2,traceVMiddle, traceHTop1, traceHTop2, traceHTopD1, traceHTopD2;
    const csgo::EngineTrace Trace=engineInterfaces.engineTrace();

    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius/2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x - botzConfig.nodeRadius/2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHBottom1);
    botzConfig.tracez.push_back(traceHBottom1);
    Trace.traceRay({ {botzConfig.checkOrigin.x,botzConfig.checkOrigin.y + botzConfig.nodeRadius/2,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x,botzConfig.checkOrigin.y - botzConfig.nodeRadius/2,botzConfig.checkOrigin.z} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHBottom2);
    botzConfig.tracez.push_back(traceHBottom2);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHBottomD1);
    botzConfig.tracez.push_back(traceHBottomD1);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHBottomD2);
    botzConfig.tracez.push_back(traceHBottomD2);
    Trace.traceRay({ {botzConfig.checkOrigin.x,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z + 36.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceVMiddle);
    botzConfig.tracez.push_back(traceVMiddle);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z+42.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z+42.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHTop1);
    botzConfig.tracez.push_back(traceHTop1);
    Trace.traceRay({ {botzConfig.checkOrigin.x,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f},{botzConfig.checkOrigin.x,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHTop2);
    botzConfig.tracez.push_back(traceHTop2);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHTopD1);
    botzConfig.tracez.push_back(traceHTopD1);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHTopD2);
    botzConfig.tracez.push_back(traceHTopD2);

    if (traceHBottom1.fraction == 1.0f && traceHBottom2.fraction == 1.0f && traceHBottomD1.fraction == 1.0f && traceHBottomD2.fraction == 1.0f)
        return 1;
    else if (traceVMiddle.fraction == 1.0f)
        return 2;
    else if (traceHTop1.fraction == 1.0f && traceHTop2.fraction == 1.0f && traceHTopD1.fraction == 1.0f && traceHTopD2.fraction == 1.0f)
        return 3;
    else return 0;
}

void Misc::drawPathfinding(const EngineInterfaces& engineInterfaces)noexcept {
    if (!botzConfig.isbotzon)
        return;
    if (!botzConfig.pathfindingDebug)
        return;
    if (!localPlayer)
        return;
    
    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;
    
    ImDrawList* dlist;
    dlist = ImGui::GetBackgroundDrawList();
    for (int index = 0; index < botzConfig.openNodes.size(); index++) {
        ImVec2 screenNodePos;
        Helpers::worldToScreenPixelAligned(botzConfig.openNodes[index], screenNodePos);
        dlist->AddRectFilled({ screenNodePos.x - 13.f,screenNodePos.y - 13.f }, { screenNodePos.x + 13.f,screenNodePos.y + 13.f }, 0xCC333333);
        dlist->AddText({ screenNodePos.x - 12.f,screenNodePos.y - 12.f }, 0xFFFFFFFF, std::to_string(index).c_str());
    }
    for (int index = 0; index < botzConfig.closedNodes.size(); index++) {
        ImVec2 screenNodePos;
        Helpers::worldToScreenPixelAligned(botzConfig.closedNodes[index], screenNodePos);
        dlist->AddRectFilled({ screenNodePos.x - 13.f,screenNodePos.y - 13.f }, { screenNodePos.x + 13.f,screenNodePos.y + 13.f }, 0xCC333333);
        dlist->AddText({ screenNodePos.x - 12.f,screenNodePos.y - 12.f }, 0xFFFFFFFF, std::to_string(index).c_str());
    }
    if (botzConfig.tracez.size() == 0||!botzConfig.drawPathfindingTraces)
        return;
    else {
            std::vector<ImVec2>traceScreenPosStart,traceScreenPosEnd;
            for (int index = 0; index < botzConfig.tracez.size(); index++) {
                ImVec2 temp;
                Helpers::worldToScreenPixelAligned(botzConfig.tracez[index].startpos, temp);
                traceScreenPosStart.push_back(temp);
                Helpers::worldToScreenPixelAligned(botzConfig.tracez[index].endpos, temp);
                traceScreenPosEnd.push_back(temp);

                dlist->AddLine(traceScreenPosStart[index], traceScreenPosEnd[index], ((botzConfig.tracez[index].fraction < 1.0f || botzConfig.tracez[0].contents == 1) ? 0xDDFF0000 : 0xDD00FF00));
            }
        }

    }
            
//void Misc::pathfindBackup(const EngineInterfaces& engineInterfaces, const Memory& memory) noexcept {
//    if (!localPlayer)
//        return;
//    if (!localPlayer.get().isAlive())
//        return;
//    const csgo::Engine& engine = engineInterfaces.getEngine();
//    if (!engine.isInGame())
//        return;
//    if (!botzConfig.shouldwalk)
//        return;
//    if (botzConfig.waypoints.size() < 1)
//        return;
//    csgo::Vector checkPos{ localPlayer.get().getAbsOrigin() };
//    checkPos.x += botzConfig.nodeRadius;
//    checkPos.y += botzConfig.nodeRadius;
//
//    for (int dial = 0; dial < 9; dial++) {      //dial bc
//
//        switch (dial) {
//        case 0:break;
//        case 1:checkPos.y -= botzConfig.nodeRadius; break;
//        case 2:checkPos.y -= botzConfig.nodeRadius; break;
//        case 3:checkPos.x -= botzConfig.nodeRadius; break;
//        case 4:continue; break;
//        case 5:checkPos.y += botzConfig.nodeRadius*2; break;
//        case 6:checkPos.x -= botzConfig.nodeRadius; break;
//        case 7:checkPos.y -= botzConfig.nodeRadius; break;
//        case 8:checkPos.y -= botzConfig.nodeRadius; break;
//        }
//        botzConfig.nodes[dial] = checkPos;
//
//        botzConfig.gcost[dial]=checkPos.distTo(localPlayer.get().getAbsOrigin());
//        botzConfig.hcost[dial]=checkPos.distTo(botzConfig.waypoints.front());
//        botzConfig.fcost[dial]=botzConfig.gcost[dial]+botzConfig.hcost[dial];
//        for (int index = 0; index < botzConfig.fcost.size(); index++) {
//            if (botzConfig.walk[index] == false && botzConfig.crouchJump[index] == false)
//                botzConfig.fcost[index] = 99999.f;
//        }
//        if (botzConfig.waypoints.size()==1) {
//            botzConfig.waypoints.push_back(botzConfig.nodes[std::distance(botzConfig.fcost.begin(), std::min_element(botzConfig.fcost.begin(), botzConfig.fcost.end()))]);
//            botzConfig.nodeType.first = std::distance(botzConfig.fcost.begin(), std::min_element(botzConfig.fcost.begin(), botzConfig.fcost.end()));
//        }
//    }
//    if (botzConfig.lastCheckTime + 5 < memory.globalVars->realtime) {   //if bot gets stuck somewhere, set his temp waypoint to abs origin
//        if (botzConfig.lastCheckPos.distTo(localPlayer.get().getAbsOrigin())<150.f) {
//            botzConfig.waypoints[1] = localPlayer.get().getAbsOrigin();
//        }
//        botzConfig.lastCheckPos = localPlayer.get().getAbsOrigin();
//        botzConfig.lastCheckTime = memory.globalVars->realtime;
//    }
//}

void addNeighborNodes(const EngineInterfaces& engineInterfaces) noexcept{
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;

    for (int index = 0; index < 8; index++) {
        csgo::Vector potentialOpen;
        csgo::Vector offset{ 0,0,0 };
        //2 3 4
        //1 d 5
        //0 7 6
        switch (index) {
        case 0:
            offset.x = 0.f - botzConfig.nodeRadius;
            offset.y = 0.f - botzConfig.nodeRadius;
            offset.z = 0.f;
            break;
        case 1:
            offset.x = 0.f - botzConfig.nodeRadius;
            offset.y = 0.f;
            offset.z = 0.f;
            break;
        case 2:
            offset.x = 0.f - botzConfig.nodeRadius;
            offset.y = 0.f + botzConfig.nodeRadius;
            offset.z = 0.f;
            break;
        case 3:
            offset.x = 0.f;
            offset.y = 0.f + botzConfig.nodeRadius;
            offset.z = 0.f;
            break;
        case 4:
            offset.x = 0.f + botzConfig.nodeRadius;
            offset.y = 0.f + botzConfig.nodeRadius;
            offset.z = 0.f;
            break;
        case 5:
            offset.x = 0.f + botzConfig.nodeRadius;
            offset.y = 0.f;
            offset.z = 0.f;
            break;
        case 6:
            offset.x = 0.f + botzConfig.nodeRadius;
            offset.y = 0.f - botzConfig.nodeRadius;
            offset.z = 0.f;
            break;
        case 7:
            offset.x = 0.f;
            offset.y = 0.f - botzConfig.nodeRadius;
            offset.z = 0.f;
            break;
        default:break;
        }
        potentialOpen = botzConfig.openNodes[botzConfig.currentNode] + offset;
        if (std::find(botzConfig.closedNodes.begin(), botzConfig.closedNodes.end(), potentialOpen) != botzConfig.closedNodes.end())
            continue;
        if (collisionCheck(engineInterfaces, potentialOpen) == 0)
            continue;
        potentialOpen.z = botzConfig.tempFloorPos.z;
        botzConfig.closedNodes.push_back(potentialOpen);

        
    }
}

void openNode(const EngineInterfaces& engineInterfaces, int nodeIndex) noexcept {
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;

    if (nodeIndex != 0) {
        botzConfig.openNodes.push_back(botzConfig.closedNodes[nodeIndex]);
        botzConfig.closedNodes.erase(botzConfig.closedNodes.begin() + nodeIndex);
        int staticNodeIndex = std::distance(botzConfig.nodeIndex.begin(),std::find(botzConfig.nodeIndex.begin(), botzConfig.nodeIndex.end(), std::pair(nodeIndex, false)));
        botzConfig.nodeIndex.erase(botzConfig.nodeIndex.begin()+staticNodeIndex);   //todo: crash here fix pls vector assignment out of range
        botzConfig.nodeIndex.push_back(std::pair(nodeIndex, true));
        botzConfig.currentNode = std::distance(botzConfig.openNodes.begin(), botzConfig.openNodes.end());
    }
}

void Misc::pathfind(const EngineInterfaces& engineInterfaces, const Memory& memory,csgo::Vector endpos) noexcept {
    if (!botzConfig.isbotzon)
        return;
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    const csgo::Engine& engine=engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;
    botzConfig.pathFound = false;
    botzConfig.openNodes.clear();
    botzConfig.closedNodes.clear();
    //botzConfig.openNodes.resize(2000);
    //botzConfig.closedNodes.resize(2000);
    botzConfig.currentNode = 0;
    botzConfig.openNodes.push_back(localPlayer.get().getAbsOrigin());
    addNeighborNodes(engineInterfaces);
            //check if the current node is the finish point
    //if (botzConfig.closedNodes.back().x - botzConfig.waypointApproximation<endpos.x &&       //todo: move this to uhhhhh checkIfPathIsFound() or something like that idk
    //    botzConfig.closedNodes.back().x + botzConfig.waypointApproximation>endpos.x &&       //crashes due to attempting to get std::vector::back() of an empty vector :P
    //    botzConfig.closedNodes.back().y - botzConfig.waypointApproximation<endpos.y &&
    //    botzConfig.closedNodes.back().y + botzConfig.waypointApproximation>endpos.y &&
    //    botzConfig.closedNodes.back().z - botzConfig.waypointApproximation<endpos.z &&
    //    botzConfig.closedNodes.back().z + botzConfig.waypointApproximation>endpos.z)
    //        return;
}

void Misc::drawPath(const EngineInterfaces& engineInterfaces) noexcept {
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    ImDrawList* dlist;
    dlist = ImGui::GetBackgroundDrawList();
    const csgo::Engine engine = engineInterfaces.getEngine();
    for (int index = 0; index < botzConfig.closedNodes.size(); index++) {
        ImVec2 point1, point2, point3;
        Helpers::worldToScreenPixelAligned({ botzConfig.closedNodes[index].x-botzConfig.nodeRadius/4.f,botzConfig.closedNodes[index].y     ,botzConfig.closedNodes[index].z}, point1);
        Helpers::worldToScreenPixelAligned({ botzConfig.closedNodes[index].x-botzConfig.nodeRadius/4.f,botzConfig.closedNodes[index].y-botzConfig.nodeRadius/4.f,botzConfig.closedNodes[index].z}, point2);
        Helpers::worldToScreenPixelAligned({ botzConfig.closedNodes[index].x     ,botzConfig.closedNodes[index].y-botzConfig.nodeRadius/4.f,botzConfig.closedNodes[index].z}, point3);
        dlist->AddTriangleFilled(point1,point2,point3, 0xFFAA4444);
    }
    for (int index = 0; index < botzConfig.openNodes.size(); index++) {
        ImVec2 point1, point2, point3;
        Helpers::worldToScreenPixelAligned({ botzConfig.openNodes[index].x - botzConfig.nodeRadius/4.f,botzConfig.openNodes[index].y       ,botzConfig.openNodes[index].z }, point1);
        Helpers::worldToScreenPixelAligned({ botzConfig.openNodes[index].x - botzConfig.nodeRadius/4.f,botzConfig.openNodes[index].y - botzConfig.nodeRadius/4.f,botzConfig.openNodes[index].z }, point2);
        Helpers::worldToScreenPixelAligned({ botzConfig.openNodes[index].x       ,botzConfig.openNodes[index].y - botzConfig.nodeRadius/4.f,botzConfig.openNodes[index].z }, point3);
        dlist->AddTriangleFilled(point1, point2, point3, 0xFF44AA44);
    }
}

void Misc::reportToTeam(const Memory& memory, const EngineInterfaces& engineInterfaces, const csgo::GameEvent& event, bool forceReport) noexcept {

    if (!botzConfig.isbotzon||!botzConfig.shouldReportToTeam)
        return;
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;

    const csgo::Engine& engine = engineInterfaces.getEngine();

    const char* localCallout = localPlayer.get().lastPlaceName();
    std::string say_team;
    if (botzConfig.botState == 2) {
        std::string say_team = "say_team \"i see an enemy at ";
        say_team += localCallout;
        say_team += "\"";
    }
    else if (botzConfig.botState == 6) {
        csgo::PlayerInfo pinfo;
        engine.getPlayerInfo(engine.getPlayerForUserID(event.getInt("attacker")), pinfo);
        say_team = "say_team \"I died ";
        if (botzConfig.reportDetailsCallout) {
            say_team += " at ";
            say_team += localCallout;
        }
        if (botzConfig.reportDetailsDiedTo) {
            say_team += " to ";
            say_team += (pinfo.fakeplayer ? "BOT " : "");
            say_team += (event.getInt("attacker")==localPlayer.get().getUserId(engine) ? " my own " : pinfo.name);
            if (event.getInt("attacker") != localPlayer.get().getUserId(engine))
                say_team += "\'s";
            say_team += event.getString("weapon");
        }
        say_team += "\"";
    }
    switch (botzConfig.botState) {
        case 0:break;   //idling
        case 1:break;   //in combat
        case 2:         //see enemy
            engine.clientCmdUnrestricted(say_team.c_str());
            engine.clientCmdUnrestricted("needbackup");
            botzConfig.botState = 1;
            break;   
        case 3:break;   //reloading
        case 4:break;   //following teammate
        case 5:break;   //throwing a grenade
        case 6:         //bot just died
            engine.clientCmdUnrestricted(say_team.c_str());
            botzConfig.botState = 0;
            break;   
        case 7:break;   //defending bombsite
        case 8:break;   //searching for c4
    }
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

void Misc::drawBotzPos(ImDrawList* dlist) noexcept {

    if (!botzConfig.isbotzon)
        return;
    if (!botzConfig.shouldDebug)
        return;
    if (!localPlayer)
        return;
    for (size_t i = 0; i < botzConfig.waypoints.size(); i++) {
        csgo::Vector worldpos{ botzConfig.waypoints[i] };
        ImVec2 screenpos;
        Helpers::worldToScreenPixelAligned(worldpos, screenpos);
        float distToEyes = worldpos.distTo(localPlayer.get().getEyePosition()) / botzConfig.posDrawSize;


        if (screenpos.x > 0)
            dlist->AddCircleFilled(screenpos, ImGui::GetIO().DisplaySize.x / distToEyes, 0x999999FF, 12);

        csgo::Vector relAngle;
        relAngle = Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), worldpos, localPlayer.get().eyeAngles());
    }
    //dlist->AddText(ImVec2(500, 125), 0xFFFFFFFF, std::to_string(relAngle.x).c_str());//THIS IS VERTICAL
    //dlist->AddText(ImVec2(500, 150), 0xFFFFFFFF, std::to_string(relAngle.y).c_str());//THIS IS HORIZONTAL
    
}

void Misc::gotoBotzPos(csgo::UserCmd* cmd, const EngineInterfaces& engineInterfaces) noexcept{
    
    if (!botzConfig.isbotzon|| !botzConfig.shouldwalk||!botzConfig.pathFound)
        return;
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    if (botzConfig.waypoints.size()==0)
        return;


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

void Misc::handleBotzEvents(const Memory& memory,const EngineInterfaces& engineInterfaces, const csgo::GameEvent& event,const ClientInterfaces& clientInterfaces, int eventType) noexcept {
    if (!localPlayer)                                                                                                    //eventTypes:
        return;                                                                                                          //0: player_death      1: player_hurt      2: round_start
    if (!botzConfig.isbotzon)                                                                                            //3: bomb_planted      4: bomb_defused     5: bomb_exploded
        return;                                                                                                          //6: player_radio      7: round_freeze_end 8: vote_cast
    const csgo::Engine& engine = engineInterfaces.getEngine();                                                           //9: round_mvp         10:item_purchase    11:bullet_impact
    engine.clientCmdUnrestricted(event.getName());                                                                       //12:weapopn_fire      13:player_ping
    const auto localUserId = localPlayer.get().getUserId(engine);
    std::string printToConsole;
    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(engine.getPlayerForUserID(event.getInt("userid"))));
    switch(eventType){                                                                                                   
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
            botzConfig.isShooterVisible = true;}
        else {
            botzConfig.isShooterVisible = false;}

        if (botzConfig.startedAiming + botzConfig.reactionTime < memory.globalVars->realtime) {
            botzConfig.startedAiming = memory.globalVars->realtime;
            botzConfig.aimreason = 0;
        }
        break;
    case 13:
        //if (entity.getPOD() == localPlayer.get().getPOD()) {
            botzConfig.playerPingLoc = { event.getFloat("x"),event.getFloat("y"),event.getFloat("z") };
            if (botzConfig.waypoints.size() > 0) {
                botzConfig.waypoints.front() = botzConfig.playerPingLoc;
            }
            else botzConfig.waypoints.push_back(botzConfig.playerPingLoc);
            botzConfig.waypoints.back() = localPlayer.get().getAbsOrigin();
        //}
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
                if (botzConfig.closedNodes.size() > 0) {
                    ImGui::SliderInt("Waypoint to edit", &botzConfig.editWaypoint, 1, botzConfig.closedNodes.size(),"%d",ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SameLine();
                    if (ImGui::Button("copypos"))
                        botzConfig.closedNodes[botzConfig.editWaypoint - 1] = localPlayer.get().getAbsOrigin();
                    ImGui::SameLine();
                }
                if (ImGui::Button("+")) {
                    botzConfig.closedNodes.push_back({ 0,0,0 });
                    botzConfig.editWaypoint = botzConfig.closedNodes.size();
                    botzConfig.closedNodes[botzConfig.editWaypoint - 1] = localPlayer.get().getAbsOrigin();
                }
                if (ImGui::Button("find path"))
                    Misc::pathfind(engineInterfaces,memory,{0.f,0.f,0.f});
                if(botzConfig.closedNodes.size()>0)
                    ImGui::SliderInt("node # ", &botzConfig.currentNode, 0, botzConfig.closedNodes.size() - 1, "%d", ImGuiSliderFlags_AlwaysClamp);
                if (ImGui::Button("open neighbor nodes of node"))
                    openNode(engineInterfaces,botzConfig.currentNode);
                if (botzConfig.closedNodes.size() > 0) {
                    ImGui::SliderFloat("botz goto position (X)", &botzConfig.closedNodes[botzConfig.editWaypoint - 1].x, -32768.f, 32768.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("botz goto position (Y)", &botzConfig.closedNodes[botzConfig.editWaypoint - 1].y, -32768.f, 32768.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("botz goto position (Z)", &botzConfig.closedNodes[botzConfig.editWaypoint - 1].z, -32768.f, 32768.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);

                }
                ImGui::Separator();
                ImGui::SliderFloat("pos draw size", &botzConfig.posDrawSize, 1.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            }
            ImGui::SliderFloat("Waypoint approximation amount", &botzConfig.waypointApproximation, 1.f, 150.f);
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
        }
    }
    else
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 230.0f);
        if (ImGui::Button("switch to botz"))
            miscConfig.menutodraw = false;
        ImGui::SameLine();
        ImGui::hotkey("Menu Key", miscConfig.menuKey);
        ImGui::Checkbox("Anti AFK kick", &miscConfig.antiAfkKick);
        ImGui::Checkbox("overhead chat", &miscConfig.overheadChat);
        ImGui::Checkbox("Auto strafe", &miscConfig.autoStrafe);
        ImGui::Checkbox("Bunny hop", &miscConfig.bunnyHop);
        ImGui::Checkbox("Fast duck", &miscConfig.fastDuck);
        ImGui::Checkbox("Moonwalk", &miscConfig.moonwalk);
        ImGui::Checkbox("Edge Jump", &miscConfig.edgejump);
        ImGui::SameLine();
        ImGui::PushID("Edge Jump Key");
        ImGui::hotkey("", miscConfig.edgejumpkey);
        ImGui::PopID();
        ImGui::Checkbox("Slowwalk", &miscConfig.slowwalk);
        ImGui::SameLine();
        ImGui::PushID("Slowwalk Key");
        ImGui::hotkey("", miscConfig.slowwalkKey);
        ImGui::PopID();
        ImGuiCustom::colorPicker("Noscope crosshair", miscConfig.noscopeCrosshair);
        ImGuiCustom::colorPicker("Recoil crosshair", miscConfig.recoilCrosshair);
        ImGui::Checkbox("Auto pistol", &miscConfig.autoPistol);
        ImGui::Checkbox("Auto reload", &miscConfig.autoReload);
        ImGui::Checkbox("Auto accept", &miscConfig.autoAccept);
        ImGui::Checkbox("Radar hack", &miscConfig.radarHack);
        ImGui::Checkbox("Reveal ranks", &miscConfig.revealRanks);
        ImGui::Checkbox("Reveal money", &miscConfig.revealMoney);
        ImGui::Checkbox("Reveal suspect", &miscConfig.revealSuspect);
        ImGui::Checkbox("Reveal votes", &miscConfig.revealVotes);

        ImGui::Checkbox("Spectator list", &miscConfig.spectatorList.enabled);
        ImGui::SameLine();

        ImGui::PushID("Spectator list");
        if (ImGui::Button("..."))
            ImGui::OpenPopup("");

        if (ImGui::BeginPopup("")) {
            ImGui::Checkbox("No Title Bar", &miscConfig.spectatorList.noTitleBar);
            ImGui::EndPopup();
        }
        ImGui::PopID();

        ImGui::Checkbox("Watermark", &miscConfig.watermark.enabled);
        ImGuiCustom::colorPicker("Offscreen Enemies", miscConfig.offscreenEnemies.asColor4(), &miscConfig.offscreenEnemies.enabled);
        ImGui::SameLine();
        ImGui::PushID("Offscreen Enemies");
        if (ImGui::Button("..."))
            ImGui::OpenPopup("");

        if (ImGui::BeginPopup("")) {
            ImGui::Checkbox("Health Bar", &miscConfig.offscreenEnemies.healthBar.enabled);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("Type", &miscConfig.offscreenEnemies.healthBar.type, "Gradient\0Solid\0Health-based\0");
            if (miscConfig.offscreenEnemies.healthBar.type == HealthBar::Solid) {
                ImGui::SameLine();
                ImGuiCustom::colorPicker("", miscConfig.offscreenEnemies.healthBar.asColor4());
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
        ImGui::Checkbox("Fix animation LOD", &miscConfig.fixAnimationLOD);
        ImGui::Checkbox("Fix movement", &miscConfig.fixMovement);
        ImGui::Checkbox("Disable model occlusion", &miscConfig.disableModelOcclusion);
        ImGui::SliderFloat("Aspect Ratio", &miscConfig.aspectratio, 0.0f, 5.0f, "%.2f");
        ImGui::NextColumn();
        ImGui::Checkbox("Disable HUD blur", &miscConfig.disablePanoramablur);
        ImGui::Checkbox("Animated clan tag", &miscConfig.animatedClanTag);
        ImGui::Checkbox("Clock tag", &miscConfig.clocktag);
        ImGui::Checkbox("Custom clantag", &miscConfig.customClanTag);
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::PushID(0);

        if (ImGui::InputText("", miscConfig.clanTag, sizeof(miscConfig.clanTag)))
            updateClanTag(true);
        ImGui::PopID();
        ImGui::Checkbox("Kill message", &miscConfig.killMessage);
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::PushID(1);
        ImGui::InputText("", &miscConfig.killMessageString);
        ImGui::PopID();
        ImGui::Checkbox("Name stealer", &miscConfig.nameStealer);
        ImGui::PushID(3);
        ImGui::SetNextItemWidth(100.0f);
        ImGui::Combo("", &miscConfig.banColor, "White\0Red\0Purple\0Green\0Light green\0Turquoise\0Light red\0Gray\0Yellow\0Gray 2\0Light blue\0Gray/Purple\0Blue\0Pink\0Dark orange\0Orange\0");
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(4);
        ImGui::InputText("", &miscConfig.banText);
        ImGui::PopID();
        ImGui::SameLine();
        if (ImGui::Button("Setup fake ban"))
            fakeBan(engineInterfaces.getEngine(), true);
        ImGui::Checkbox("Fast plant", &miscConfig.fastPlant);
        ImGui::Checkbox("Fast Stop", &miscConfig.fastStop);
        ImGuiCustom::colorPicker("Bomb timer", miscConfig.bombTimer);
        ImGui::Checkbox("Quick reload", &miscConfig.quickReload);
        ImGui::Checkbox("Prepare revolver", &miscConfig.prepareRevolver);
        ImGui::SameLine();
        ImGui::PushID("Prepare revolver Key");
        ImGui::hotkey("", miscConfig.prepareRevolverKey);
        ImGui::PopID();
        ImGui::Combo("Hit Sound", &miscConfig.hitSound, "None\0Metal\0Gamesense\0Bell\0Glass\0Custom\0");
        if (miscConfig.hitSound == 5) {
            ImGui::InputText("Hit Sound filename", &miscConfig.customHitSound);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("audio file must be put in csgo/sound/ directory");
        }
        ImGui::PushID(5);
        ImGui::Combo("Kill Sound", &miscConfig.killSound, "None\0Metal\0Gamesense\0Bell\0Glass\0Custom\0");
        if (miscConfig.killSound == 5) {
            ImGui::InputText("Kill Sound filename", &miscConfig.customKillSound);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("audio file must be put in csgo/sound/ directory");
        }
        ImGui::PopID();
        ImGui::SetNextItemWidth(90.0f);
        ImGui::InputInt("Choked packets", &miscConfig.chokedPackets, 1, 5);
        miscConfig.chokedPackets = std::clamp(miscConfig.chokedPackets, 0, 64);
        ImGui::SameLine();
        ImGui::PushID("Choked packets Key");
        ImGui::hotkey("", miscConfig.chokedPacketsKey);
        ImGui::PopID();
        /*
        ImGui::Text("Quick healthshot");
        ImGui::SameLine();
        hotkey(miscConfig.quickHealthshotKey);
        */
        ImGui::Checkbox("Grenade Prediction", &miscConfig.nadePredict);
        ImGui::Checkbox("Fix tablet signal", &miscConfig.fixTabletSignal);
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderFloat("Max angle delta", &miscConfig.maxAngleDelta, 0.0f, 255.0f, "%.2f");
        ImGui::Checkbox("Opposite Hand Knife", &miscConfig.oppositeHandKnife);
        ImGui::Checkbox("Preserve Killfeed", &miscConfig.preserveKillfeed.enabled);
        ImGui::SameLine();

        ImGui::PushID("Preserve Killfeed");
        if (ImGui::Button("..."))
            ImGui::OpenPopup("");

        if (ImGui::BeginPopup("")) {
            ImGui::Checkbox("Only Headshots", &miscConfig.preserveKillfeed.onlyHeadshots);
            ImGui::EndPopup();
        }
        ImGui::PopID();

        ImGui::Checkbox("Purchase List", &miscConfig.purchaseList.enabled);
        ImGui::SameLine();

        ImGui::PushID("Purchase List");
        if (ImGui::Button("..."))
            ImGui::OpenPopup("");

        if (ImGui::BeginPopup("")) {
            ImGui::SetNextItemWidth(75.0f);
            ImGui::Combo("Mode", &miscConfig.purchaseList.mode, "Details\0Summary\0");
            ImGui::Checkbox("Only During Freeze Time", &miscConfig.purchaseList.onlyDuringFreezeTime);
            ImGui::Checkbox("Show Prices", &miscConfig.purchaseList.showPrices);
            ImGui::Checkbox("No Title Bar", &miscConfig.purchaseList.noTitleBar);
            ImGui::EndPopup();
        }
        ImGui::PopID();

        ImGui::Checkbox("Reportbot", &miscConfig.reportbot.enabled);
        ImGui::SameLine();
        ImGui::PushID("Reportbot");

        if (ImGui::Button("..."))
            ImGui::OpenPopup("");

        if (ImGui::BeginPopup("")) {
            ImGui::PushItemWidth(80.0f);
            ImGui::Combo("Target", &miscConfig.reportbot.target, "Enemies\0Allies\0All\0");
            ImGui::InputInt("Delay (s)", &miscConfig.reportbot.delay);
            miscConfig.reportbot.delay = (std::max)(miscConfig.reportbot.delay, 1);
            ImGui::InputInt("Rounds", &miscConfig.reportbot.rounds);
            miscConfig.reportbot.rounds = (std::max)(miscConfig.reportbot.rounds, 1);
            ImGui::PopItemWidth();
            ImGui::Checkbox("Abusive Communications", &miscConfig.reportbot.textAbuse);
            ImGui::Checkbox("Griefing", &miscConfig.reportbot.griefing);
            ImGui::Checkbox("Wall Hacking", &miscConfig.reportbot.wallhack);
            ImGui::Checkbox("Aim Hacking", &miscConfig.reportbot.aimbot);
            ImGui::Checkbox("Other Hacking", &miscConfig.reportbot.other);
            if (ImGui::Button("Reset"))
                Misc::resetReportbot();
            ImGui::EndPopup();
        }
        ImGui::PopID();

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
