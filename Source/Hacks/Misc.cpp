#include <algorithm>
#include <array>
#include <iomanip>
#include <mutex>
#include <numbers>
#include <numeric>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>

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

#include "CSGO/IUIPanel.h"
#include "CSGO/IUIEvent.h"

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

    bool justInjected{ false };
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
    int guiTab = 0;
    std::string panoramaEvent = "OpenSidebarPanel";
    float matchmakingStartTime{ 0 };
    float drawstartTime = 0;
    bool langWindowOpen{ false };
    bool walkbotTypeOpen{ false };
    bool botListOpen{ false };
    bool botControlOpen{ false };
    int language = 0;
} miscConfig;

struct BotzConfig {

    bool isbotzon{ false };

    int botState = 0;
    int botStatePrev = 0;

    int roundCounter = 0;


        //walkbot vars
    bool shouldwalk{ false };

    //common walkbot vars
    int walkbotType{ 0 };
    int nodeRadius{ 33 };
    std::vector<int> waypointWalkType;
    float dropdownDmg{ 0.f };
    bool improvedPathfinding{ false };

        //walkbot type 0 (dynamic node pathfinding)
    std::vector<csgo::Vector> nodes;//node positions
    std::vector<bool> nodesType;    //true if a node is open,false if closed
    std::vector<int>nodesParents;   //index of the parent node
    std::vector<int>walkType;       //same as return of collisionCheck()
    std::vector<float> fcost;       //total cost of node (dist to localplayer+dist to destination)
    int currentNode{ 0 };
    bool pathFound{ false };
    std::vector<csgo::Trace>tracez;
    csgo::Trace middleTrace;
    csgo::Vector checkOrigin{ 0,0,0 };
    csgo::Vector tempFloorPos{ 0,0,0 };
    csgo::Vector finalDestination{ 0.f,0.f,0.f};
    std::vector<csgo::Vector> waypoints;
    std::vector<csgo::Vector> waypointParentOffset;
    int curWayPoint{ -2 };
    bool shouldGoTowardsPing{ false };
    bool traceToParentIntersects{ false };
    std::vector<csgo::Vector>parentOffset;
    bool stopWalking{ false };


        //walkbot type 1 (static node pathfinding)
    int targetStaticNode{0};                  //the static node that we should walk towards
    std::vector<csgo::Vector> presetNodes;          //list of all static nodes
    std::vector<int> nodeGroup;                     //common/camping spot/bombsite a/bombsite b
    const std::array<std::string,3> maplist{ "borbot","de_mirage",""};//supported maps
    std::string currentMap{ "idk" };
    float maxDistToCalculate{ 1500.f };             //don't calculate fcost for static node if distance from lPlayer to node is > value
    std::vector<float> staticNodesFcosts;           //999999 if node is further than maxDistToCalculate units away (so that

    csgo::Vector playerPingLoc{ 0,0,0 };

    float hitglass{ 0.f };
    float tracerayAngle{ 0.f };

    bool shouldDebug{ false };               //debug drawing and ermmmmmm
    float posDrawSize{ 1.5f };
    bool pathfindingDebug{ false };
    bool drawPathfindingTraces{ false };
    bool circlesOrCost{ false };             


    bool autoreload{ false };                    //reloadbot logic
    float lastReload{ 0.0f };                      //I THINK(!) this is the last second when no enemy was seen 
    int reloadAfterXSeconds{ 5 };            
    float reloadIfClipPercent{ 0.75f };

    bool aimAtEvents{ true };                    //aimbot things
    int aimreason{ -1 };                           //-1:not aiming at anything, 0:weapon_fire event 1:enemy 2:teammates 3:windows 4:path
    bool isShooterVisible{false};
    float reactionTime{ 0.135f };
    csgo::Vector aimspot{ 0,0,0 };
    float startedAiming{ -1.f };
    std::vector<float>aimtime{ 0.1f};
    csgo::Vector localViewAngles{ 0,0,0 };
    float lastTimeSawEnemy{ 0.f };
    bool shouldFire{ false };
    bool aimAtPath{ false };

    std::vector<csgo::Entity> enemyEntities;       //list of enemy csgo::Entities that we can aim at (line of sight+fov)

    
    int enemyToAim{ -1 };
    bool shouldScope{ false };
    int shouldUnscope{ 0 };

    float roundStartTime{ 0.f };
    float buyAfter{ 0.f };
    //communication stuff
    const std::array<std::string,31> radioTranslate { "","","go","fallback","sticktog","holdpos","followme", "","roger","negative","cheer","compliment",
                                                     "thanks","","enemyspot","needbackup","takepoint","sectorclear","inposition","coverme","regroup",
                                                     "takingfire","report","", "getout", "enemydown","","","go_a","go_b","needrop"};

    bool shouldReportToTeam{ true };        //report attacker info on death
    bool reportDetailsCallout{ true };
    bool reportDetailsDiedTo{ true };

    bool shouldCompliment{ true };
    float complimentChance{1.f};
    
    std::vector<int>botsList;
    bool isListeningForCommands{ false };
    std::vector<int>botsListeningToMe;

} botzConfig;

struct Translate{
    std::array<std::string, 4>tabWalkbot        { "WALKBOT",                                "WALKBOT",                                      "WALKBOT",                                       "WALKBOT"};
    std::array<std::string, 4>tabEvents         { "EVENTS",                                 "EREIGNISSE",                                   "EVENTS",                                        "VERANSTALTUNGEN"};
    std::array<std::string, 4>tabChat           { "CHAT",                                   "CHAT",                                         "CHAT",                                          "PLAUDERN"};
    std::array<std::string, 4>tabMisc           { "MISC",                                   "VERSCHIEDENES",                                "MISC",                                          "MISC" };
                            
                            
    std::array<std::string, 4>walkToggleBot     { "Toggle bot",                             "Toggle bot",                                   "Включить бота",                                 "Bot umschalten"};
    std::array<std::string, 4>walkAutoPath      { "Automatic pathfinding",                  "Automatischer Wegfinder",                      "Автоматическое нахождение маршрута",            "Automatische Wegfindung" };
    std::array<std::string, 4>walkPresetNodes   { "Pre-set nodes",                          "Voreingestellte Nodes",                        "Навмеш",                                        "Voreingestellte Knoten" };
    std::array<std::string, 4>walkShouldWalk    { "Should walk towards pos",                "In Richtung Position gehen",                   "Идти до позиции",                               "Sollte in Richtung pos gehen" };
    std::array<std::string, 4>walkGotoPings     { "Go towards teammate\'s pings",           "Gehe zu Markierungen von Teammates",           "Идти до пингов тиммейтов",                      "Gehen Sie zu den Pings Ihres Teamkollegen" };
    std::array<std::string, 4>walkImprovedPfind { "Improved pathfinding",                   "Verbesserte Wegfindung",                       "Улучшенный патхфайндинг",                       "Verbesserte Wegfindung" };
    std::array<std::string, 4>walkImprovedPWarn { "Will get stuck on uneven ground!!!",     "Wird auf unebenem Untergrund stecken bleiben!","Застревает на неровной поверхности!",           "Wird auf unebenem Untergrund stecken bleiben!!!" };
    std::array<std::string, 4>walkAimAtPath     { "Aim at path",                            "Sollte auf den Weg zielen",                    "Нацеливаться на путь",                          "Pfad anpeilen"};
    std::array<std::string, 4>walkMaxFallDamage { "Max fall damage percent",                "Maximaler Fallschaden %",                      "Максимальный урон от падения (%)",              "Maximaler Sturzschaden in Prozent" };
    std::array<std::string, 4>walkNodeSpacing   { "Node spacing",                           "Node Abstand",                                 "Расстояние между точками",                      "Knotenabstand"};
    std::array<std::string, 4>walkDrawNodes     { "Draw nodes",                             "Zeichne Nodes",                                "Прорисовка точек",                              "Knoten zeichnen" };
    std::array<std::string, 4>walkDebugFeatures { "Debug features",                         "Debug Features",                               "Дебаг-функции",                                 "Debug-Funktionen" };
    std::array<std::string, 4>walkDrawDebugInfo { "Draw debug information",                 "Zeige Debug-Informationen",                    "Прорисовка дебаг-информации",                   "Zeichne Debug-Informationen" };
    std::array<std::string, 4>walkDrawTraces    { "Draw collision check traces",            "Zeige Kollision Prüfe Spuren",                 "Прорисовка коллижн-трейсов",                    "Zeichnen Sie Kollisionskontrollspuren" };
    std::array<std::string, 4>walkForceResetPos { "Force reset local pos",                  "Erzwinge den reset der lokalen Position",      "Сбросить локальную позицию",                    "Zurücksetzen lokaler Position erzwingen" };
                            
    std::array<std::string, 4>eventsAutoreload  { "Autoreload",                             "Auto-nachladen",                               "Автоматическая перезарядка",                    "Automatisch neu laden" };
    std::array<std::string, 4>eventsReloadTime  { "Reload after X seconds",                 "Nachladen nach X Sekunden",                    "Перезаряжаться через Х секунд",                 "Nach X Sekunden neu laden" };
    std::array<std::string, 4>eventsReloadClip  { "Reload if clip is below x%",             "Nachladen wenn Magazin unter X% ist",          "Перезаряжаться если в магазине <X патронов",    "Neu laden, wenn der Clip unter x % liegt" };
    std::array<std::string, 4>eventsReportDeath { "Report death info",                      "Zeige Todesinfo an",                           "Сообщать информацию о смерти",                  "Todesfall melden" };
    std::array<std::string, 4>eventsReportPos   { "Report death callout",                   "Zeige Todesort an",                            "Сообщать позицию смерти",                       "Todesruf melden" };
    std::array<std::string, 4>eventsReportKiller{ "Report killer info",                     "Zeige Name und Waffe des Killers",             "Сообщать информацию об убийце",                 "Mörderinfo melden" };
    std::array<std::string, 4>eventsCompliment  { "Compliment teammates on kill",           "Komplimentiere Teammates bei kill",            "Хвалить тиммейтов за киллы",                    "Machen Sie Teamkollegen Komplimente, wenn sie getötet wurden" };
    std::array<std::string, 4>eventsComplimentPC{ "Compliment chance",                      "Komplimentiere Chance",                        "Шанс похвалы",                                  "Kompliment Chance" };
                            
    std::array<std::string, 4>miscOverheadChat  { "Overhead chat",                          "Overhead chat",                                "Чат над головой",                               "Overhead-Chat" };
    std::array<std::string, 4>miscMenuKey       { "Menu key",                               "Menütaste",                                    "Кнопка меню",                                   "Menu key" };
    std::array<std::string, 4>miscBhop          { "Bunnyhop",                               "Bunnyhop",                                     "Баннихоп",                                      "Hasenhop" };
    std::array<std::string, 4>miscAutoaccept    { "Autoaccept",                             "Automatisch akzeptieren",                      "Автопринятие игры",                             "Automatisch akzeptieren" };
    std::array<std::string, 4>miscStartMM       { "Start queue",                            "Starte Matchmaking",                           "Начать поиск",                                  "Warteschlange starten" };
    std::array<std::string, 4>miscImAddicted    { "I think I might be addicted to csgo...", "Ich glaube ich bin süchtig nach csgo...",      "Я зависим от кс...",                            "Ich glaube, ich könnte csgo-süchtig sein ..."};
    std::array<std::string, 4>miscLanguage      { "Language",                               "Sprache",                                      "Язык",                                          "Sprache" };
    std::array<std::string, 4>miscLanguageEng   { "English",                                "Englisch",                                     "Английский",                                    "Englisch" };
    std::array<std::string, 4>miscLanguageGer   { "German",                                 "Deutsche",                                     "Немецкий",                                      "Deutsch" };
    std::array<std::string, 4>miscLanguageRus   { "Russian",                                "Russische",                                    "Русский",                                       "Russisch" };
    std::array<std::string, 4>miscLanguageAus   { "Austrian",                               "österreichisch",                               "Австрийский",                                   "österreichisch" };
    std::array<std::string, 4>miscBotsList      { "Bots list",                              "Bots-Liste",                                   "Список ботов",                                  "Bots-Liste"};
                            
    std::array<std::string, 4>blistCheckForBots { "Check for bots",                         "Suchen Sie nach Bots",                         "Обновить список боотв",                         "Suchen Sie nach Bots" };
    std::array<std::string, 4>blistYourEncUid   { "Your encrypted userid: ",                "Ihre verschlüsselte userid: ",                 "Ваш зашифрованный userid: ",                    "Ihre verschlüsselte userid: " };
    std::array<std::string, 4>blistAskToListen  { "Ask to listen",                          "Bitten Sie den Bot, zuzuhören",                "Попросить бота ждать команды",                  "Bitten Sie den Bot, zuzuhören" };
    std::array<std::string, 4>blistAskToCome    { "Ask to come to you",                     "Bitten Sie den Bot, zu Ihnen zu kommen",       "Попросить бота идти к вам",                     "Bitten Sie den Bot, zu Ihnen zu kommen"};



}translate;

struct Encrypted{
    //used for encrypted userid transmit, emoticon index=userid
    //bot says :^) in chat, another bot in the match translates this to userid 11
    const std::array<std::string, 101>uid{   ":-)", ":)",   ":-]",  ":]",   ":->",  ":>",   "8-)",  "8)",   ":-}",  ":}",
                                             ":o)", ":c)",  ":^)",  "=]",   "=)",   ":-D",  ":D",   "8-D",  "8D",   "=D",
                                             "8^D", "c:",   "C:",   "x-D",  "xD",   "X-D",  "XD",   ":-))", ":-(",  ":(",
                                             ":-c", ":c",   ":-<",  ":<",   ":-[",  ":[",   ":-|",  ":{",   ":@",   ";(",
                                             ":'-(",":'(",  ":=(",  ":'-)", ":')",  ":'D",  ">:(",  ">:[",  "D-':", "D:<",
                                             "D:",  "D8",   "D;",   "D=",   "DX",   ":-O",  ":O",   ":-o",  ":o",   ":-0",
                                             ":0",  "8-0",  ">:O",  "=O",   "=o",   "=0",   ":-3",  ":3",   "=3",   "x3",
                                             "X3",  ">:3",  ":-*",  ":*",   ":x",   ";-)",  ";)",   "*-)",  "*)",   ";-]",
                                             ";]",  ";^)",  ";>",   ";-,",  ";D",   ";3",   ":-P",  ":P",   "X-P",  "XP",
                                             "x-p", "xp",   ":-p",  ":p",   ":-b",  ":b",   "d:",   "=p",   ">:P",  ":-/",
                                             ":/" };
    //sicko to sicko communication phrases
    const std::array<std::string, 8>phraseList{ ";^3?",";^3!",":0?",">:D!","D:<!",":B!",":V!",":l!"};
    #define helloQuestion   0     
    #define helloAnswer     1 
    #define iAmListening    2 
    #define yesOk           3 
    #define noCantDoThat    4 
    #define goB             5 
    #define goA             6 
    #define comeToMe        7

}encrypted;

struct DiscordBot {
    std::string messageToSend = "";
    bool isMessageSent = true;
}discordBot;


template<typename T>
void pop_front(std::vector<T>& vec)
{
    assert(!vec.empty());
    vec.erase(vec.begin());
}

bool doesFileExist(const std::string& filePath) noexcept
{
    std::ifstream f(filePath.c_str());
    return f.good();
}

panorama::IUIPanel* Misc::GetRoot(bool inGame) noexcept
{
    auto panoramaEngine = interfaces.getPanoramaUIEngine();
    
    
    //return nullptr;
    //auto test = eng->IsValidPanelPointer((panorama::IUIPanel*)0x1337);
    //return NULL;
    //auto tes = eng->GetApplicationInstallPath();
    //return 0;
    auto panel = csgo::UIEngine::from(retSpoofGadgets->client, interfaces.getPanoramaUIEngine().accessUIEngine()).GetLastDispatchedEventTargetPanel();
    //return nullptr;
    if (!csgo::UIEngine::from(retSpoofGadgets->client, interfaces.getPanoramaUIEngine().accessUIEngine()).IsValidPanelPointer(panel))
    {
        return NULL;
    }
    panorama::IUIPanel* itr = panel;
    panorama::IUIPanel* ret = nullptr;
    while (itr && csgo::UIEngine::from(retSpoofGadgets->client, interfaces.getPanoramaUIEngine().accessUIEngine()).IsValidPanelPointer(itr))
    {
        if (inGame)
        {
            if (!strcmp(itr->GetID(), "CSGOHud"))
            {
                ret = itr;
                break;
            }
        }
        else
        {
            if (!strcmp(itr->GetID(), "CSGOMainMenu"))
            {
                ret = itr;
                break;
            }
        }
        itr = itr->GetParent();
    }
    return ret;
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
    csgo::HudChat::from(retSpoofGadgets->client, memory.clientMode->hudChat).printf(0, " \x0C\u2022boted\u2022 %c%s\x01 call vote (\x06%s\x01)", isLocal ? '\x01' : '\x06', isLocal ? "You" : entity.getPlayerName(interfaces, memory).c_str(), voteName(voteType));
    
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

//testing
void Misc::autoqueue() noexcept {

    const auto panoramaEngine = interfaces.getPanoramaUIEngine();
    memory.conColorMsg({ 0,255,255,255 }, miscConfig.panoramaEvent.c_str());
    memory.conColorMsg({ 255,255,0,255 }, "\ntestin\n");

    if (const auto idx = memory.registeredPanoramaEvents->find(memory.makePanoramaSymbol(miscConfig.panoramaEvent.c_str())); idx != -1) {
        memory.conColorMsg({ 255,255,0,255 }, "idx is ok\n");
        if (const auto eventPtr = retSpoofGadgets->client.invokeCdecl<void*>(std::uintptr_t(memory.registeredPanoramaEvents->memory[idx].value.makeEvent), nullptr)) {
            memory.conColorMsg({ 255,255,0,255 }, "eventPtr is ok\n");
            csgo::UIEngine::from(retSpoofGadgets->client, panoramaEngine.accessUIEngine()).dispatchEvent(eventPtr);
        }
    }
}

//discord implementation

void Misc::repostMessageInChat(const EngineInterfaces& engineInterfaces) noexcept {
    const csgo::Engine engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;
    std::filesystem::path path;
    if (PWSTR pathToDocs; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocs))) {
        path = pathToDocs;
        CoTaskMemFree(pathToDocs);
    }
    std::string strPath;
    strPath = (path.string() + "/slippery/messageBack.txt");

    std::ifstream t(strPath);
    std::stringstream buffer;
    buffer << t.rdbuf();
    if (buffer.str().size() < 1)
        return;
    std::string messageToSend = "say ";
    messageToSend += buffer.str();
    engine.clientCmdUnrestricted(messageToSend.c_str());

    std::ofstream messageFile;
    messageFile.open(strPath);
    messageFile << "";
    messageFile.close();


}

void Misc::populateGameInfo(const EngineInterfaces& engineInterfaces) noexcept {

    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;

    std::filesystem::path path;
    if (PWSTR pathToDocs; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocs))) {
        path = pathToDocs;
        CoTaskMemFree(pathToDocs);
    }
    std::string strPath;
    strPath = (path.string() + "/slippery/gameinfo.txt");

    csgo::PlayerInfo pInfo;
    engine.getPlayerInfo(engine.getPlayerForUserID(localPlayer.get().getUserId(engine)), pInfo);
    std::string pName = pInfo.name;
    std::ofstream messageFile;
    std::string fullMessage="";
    messageFile.open(strPath);
    fullMessage += "\nName:";
    fullMessage += pName;
    fullMessage += "\nMap:";
    fullMessage += engine.getLevelName();
    fullMessage += "\nKills:not implemented yet sorry";
    fullMessage += "\nDeaths:not implemented yet sorry";
    fullMessage += "\nScore:not implemented yet sorry";
        
    for (int i = 1; i <= engineInterfaces.getEngine().getMaxClients(); i++) {
        const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(i));
        if (entity.getPOD() == nullptr)
            continue;
        csgo::PlayerInfo pInfo2;
        engine.getPlayerInfo(engine.getPlayerForUserID(entity.getUserId(engine)), pInfo2);
        std::string pName2 = pInfo2.name;

        fullMessage += "\nPlayer:";
        fullMessage += pName2;
    }
    messageFile << fullMessage;
    messageFile.close();

 }


void Misc::clearGameInfo() noexcept{

    std::filesystem::path path;
    if (PWSTR pathToDocs; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocs))) {
        path = pathToDocs;
        CoTaskMemFree(pathToDocs);
    }
    std::string strPath;
    strPath = (path.string() + "/slippery/gameinfo.txt");

    std::ofstream messageFile;
    messageFile.open(strPath);
    messageFile << "";
    messageFile.close();
}
//misc

void Misc::antiaddiction() noexcept {
    if (const auto idx = memory.registeredPanoramaEvents->find(memory.makePanoramaSymbol("PanoramaComponent_MyPersona_GameMustExitNowForAntiAddiction")); idx != -1) {
        if (const auto eventPtr = retSpoofGadgets->client.invokeCdecl<void*>(std::uintptr_t(memory.registeredPanoramaEvents->memory[idx].value.makeEvent), nullptr))
            csgo::UIEngine::from(retSpoofGadgets->client, interfaces.getPanoramaUIEngine().accessUIEngine()).dispatchEvent(eventPtr);
        
    }
}

//chattin

void Misc::readChat(const void* data, int size) noexcept {
    if (!localPlayer)
        return;
    
    const auto reader = ProtobufReader{ static_cast<const std::uint8_t*>(data),size };
    const auto ent_idx = reader.readInt32(1);
    const auto params = reader.readRepeatedString(4);
    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(ent_idx));

    //if (entity.getPOD() == localPlayer.get().getPOD())
     //   return;


    miscConfig.message = params[1];
    miscConfig.playeruid = ent_idx;
    miscConfig.messageLoggedAt = memory.globalVars->realtime;
    discordBot.isMessageSent = false;


}

void Misc::chatOverhead(const EngineInterfaces& engineInterfaces,const Memory& memory) noexcept{
    if (!localPlayer)
        return;

    ImDrawList* dlist;
    dlist = ImGui::GetBackgroundDrawList();
    if (miscConfig.message=="NULL"|| miscConfig.playeruid == -1 || miscConfig.messageLoggedAt == 0)
        return;
    if (std::find(encrypted.uid.begin(), encrypted.uid.end(), miscConfig.message) != encrypted.uid.end())
        return;
    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;




    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(miscConfig.playeruid));
    csgo::PlayerInfo pInfo;
    engine.getPlayerInfo(miscConfig.playeruid, pInfo);
    std::string pName = pInfo.name;

    if (!discordBot.isMessageSent) {
        std::filesystem::path path;
        if (PWSTR pathToDocs; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocs))) {
            path = pathToDocs;
            CoTaskMemFree(pathToDocs);
        }
        std::string strPath;
        strPath = (path.string() + "/slippery/message.txt");

        std::ofstream messageFile;
        std::string messageOut = pName;
        messageOut += ": ";
        messageOut += miscConfig.message;
        messageFile.open(strPath);
        messageFile << messageOut;
        messageFile.close();

        discordBot.isMessageSent = true;
    }

    if (!miscConfig.overheadChat)
        return;

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

void Misc::chatBot(const EngineInterfaces& engineInterfaces, const Memory& memory) noexcept {
    if (!localPlayer)
        return;
    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isConnected())
        return;

    if (miscConfig.message == "NULL" || miscConfig.playeruid == -1 || miscConfig.messageLoggedAt == 0)
        return;

    if (std::find(encrypted.uid.begin(), encrypted.uid.end(), miscConfig.message) == encrypted.uid.end()&&
        std::find(encrypted.phraseList.begin(),encrypted.phraseList.end(),miscConfig.message)==encrypted.phraseList.end())
            return;
    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(miscConfig.playeruid));

    std::string messageToSend="say \"";
    if (miscConfig.message == encrypted.phraseList[helloQuestion]) {
        botzConfig.botsList.clear();
        botzConfig.botsList.push_back(entity.getUserId(engine));
        messageToSend += encrypted.phraseList[helloAnswer];
    }

    if (miscConfig.message == encrypted.phraseList[helloAnswer]) {
        botzConfig.botsList.push_back(entity.getUserId(engine));
    }

    if (miscConfig.message == encrypted.uid[localPlayer.get().getUserId(engine)]) {
        messageToSend += encrypted.phraseList[iAmListening];
        botzConfig.isListeningForCommands = true;
    }

    if (miscConfig.message == encrypted.phraseList[comeToMe]&&botzConfig.isListeningForCommands) {
        if (entity.getNetworkable().isDormant())
            messageToSend += encrypted.phraseList[noCantDoThat];
        else {
            messageToSend += encrypted.phraseList[yesOk];
            botzConfig.finalDestination = entity.getAbsOrigin();
            Misc::pathfind(engineInterfaces, memory);
            botzConfig.isListeningForCommands = false;
        }
    }

    messageToSend += "\"";
    miscConfig.message = "NULL"; miscConfig.playeruid = -1; miscConfig.messageLoggedAt = 0;
    engine.clientCmdUnrestricted(messageToSend.c_str());
}


void Misc::reportDeath(const Memory& memory, const EngineInterfaces& engineInterfaces, const csgo::GameEvent& event, bool forceReport) noexcept {

    if (!botzConfig.isbotzon || !botzConfig.shouldReportToTeam)
        return;
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;

    const csgo::Engine& engine = engineInterfaces.getEngine();
    char* localCallout = localPlayer.get().lastPlaceName();

    std::string say_team;
    if (botzConfig.botState == 6) {
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
            say_team += (event.getInt("attacker") == localPlayer.get().getUserId(engine) ? " my own " : pinfo.name);
            if (event.getInt("attacker") != localPlayer.get().getUserId(engine))
                say_team += "\'s ";
            say_team += event.getString("weapon");
        }
        say_team += "\"";
        engine.clientCmdUnrestricted(say_team.c_str());
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
    const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon());
    if (botzConfig.aimreason == 1) {

        if (!localPlayer.get().isScoped()&&activeWeapon.getWeaponType()==WeaponType::SniperRifle)
            botzConfig.shouldScope = true;


        if (activeWeapon.getWeaponType() == WeaponType::Knife || activeWeapon.getWeaponType() == WeaponType::C4)
            engine.clientCmdUnrestricted("slot1");

    }
    if (botzConfig.startedAiming == -1.f|| botzConfig.startedAiming + (botzConfig.aimreason == 0||botzConfig.aimreason==1 ? botzConfig.reactionTime:0.f) > memory.globalVars->realtime)
                                        return;
    if (botzConfig.aimreason==0&&!botzConfig.isShooterVisible)
        return;
    csgo::Vector relang = Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), botzConfig.aimspot, botzConfig.localViewAngles);
    engine.setViewAngles({ botzConfig.localViewAngles.x + relang.x * sin(memory.globalVars->realtime - botzConfig.startedAiming - (botzConfig.aimreason == 3 ? 0.28f : botzConfig.aimtime[0]) + botzConfig.reactionTime) / 2,
                           botzConfig.localViewAngles.y + relang.y * sin(memory.globalVars->realtime - botzConfig.startedAiming - (botzConfig.aimreason == 3 ? 0.28f : botzConfig.aimtime[0]) + botzConfig.reactionTime) / 2,
                           0.f });
    
    if ( -0.3f<relang.x && relang.x<0.3f&&
         -0.3f<relang.y && relang.y<0.3f){
            botzConfig.startedAiming = -1.f;
            if (botzConfig.aimreason == 3||botzConfig.aimreason==1) {
                botzConfig.shouldFire = true;
            }
            botzConfig.aimreason = -1;
            return;
    }
}

//populates a vector of enemy entities if any enemy is in sight (line of sight+fov check)
void Misc::enemiesRadar(const Memory& memory,const EngineInterfaces& engineInterfaces,const OtherInterfaces& interfaces) noexcept {
    if (!botzConfig.isbotzon)
        return;
    if (!localPlayer || !localPlayer.get().isAlive())
        return;
    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame() || !engine.isConnected())
        return;

    botzConfig.enemyEntities.clear();
    for (int i = 1; i <= engineInterfaces.getEngine().getMaxClients(); i++) {
        const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(i));
        if (entity.getPOD() == nullptr||!entity.isAlive()||!entity.isOtherEnemy(memory,localPlayer.get()))
            continue;
        if (fabs(Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), entity.getEyePosition(), localPlayer.get().eyeAngles()).y) > 80)
            continue;

        csgo::Trace trace;
        engineInterfaces.engineTrace().traceRay({ localPlayer.get().getEyePosition(),{entity.getEyePosition().x,entity.getEyePosition().y,entity.getEyePosition().z+20.f}}, MASK_OPAQUE, localPlayer.get().getPOD(), trace);
        if (trace.contents != 0)
            continue;
        
        botzConfig.enemyEntities.push_back(entity);
        
    }
    
}

void Misc::handleLocatedEnemies(const Memory& memory, const EngineInterfaces& engineInterfaces, const OtherInterfaces& interfaces) noexcept {
    if (!localPlayer || !localPlayer.get().isAlive())
        return;
    const auto engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;
    if (botzConfig.aimreason == 1)
        return;
    if (botzConfig.enemyEntities.size() < 1) {
        botzConfig.stopWalking = false;
        return;
    }
    botzConfig.stopWalking = true;
    if(botzConfig.enemyToAim>botzConfig.enemyEntities.size()-1)
        botzConfig.enemyToAim = fmod(std::rand(), float(botzConfig.enemyEntities.size()));
    botzConfig.aimreason = 1;
    std::vector<int> bones = { 8, 8, 8, 4, 3, 7, 6, 5 };//head is defined three times so that it gets picked as the aim spot more often than other bones 
    botzConfig.aimspot = botzConfig.enemyEntities[botzConfig.enemyToAim].getBonePosition(bones[fmod(std::rand(), float(bones.size()))]);
    botzConfig.startedAiming = memory.globalVars->realtime;

    botzConfig.botState = 2;
}

//proper pathfinding
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
    engineInterfaces.engineTrace().traceRay({ lPlayerEyes, viewangleshit}, CONTENTS_WINDOW, localPlayer.get().getPOD(), traceXp);
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
    if (!localPlayer)
        return -1;
    if (!localPlayer.get().isAlive())
        return -1;
    //-0.000128943x^{2}+0.341019x-65.806 <- calculates fall damage kinda well 

    csgo::Trace trace;
    float fallHeight,fallDamage;

    engineInterfaces.engineTrace().traceRay({ {pos.x,pos.y,pos.z+40.f},{pos.x,pos.y,pos.z - 1000.f}}, MASK_PLAYERSOLID, localPlayer.get().getPOD(), trace);
    fallHeight = pos.distTo(trace.endpos);
    botzConfig.tempFloorPos = { trace.endpos };
    fallDamage = -0.000128943f * pow(fallHeight, 2.f) + 0.341019f * (fallHeight-40.f) - 65.806f;


    return int(fallDamage);
}

//return 0 if there's no way to get to desired position, 1 if you can walk to get to pos,
//2 if you can get to pos by jumping,3 if you can get to pos by crouching, 4 if dropping
//down will cause you great pain (fall damage is over x% of health left)
int collisionCheck(const EngineInterfaces& engineInterfaces,csgo::Vector pos,csgo::Vector parentpos) noexcept {

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

    pos.z = botzConfig.tempFloorPos.z+18.f;      //set node pos to floor height

    botzConfig.checkOrigin = { pos.x, pos.y, pos.z};//{ pos.x,pos.y,pos.z + 18.f };
    //H for horizontal, V for vertical, D for diagonal
    csgo::Trace traceHBottom1, traceHBottom2, traceHBottomD1, traceHBottomD2,traceHMiddle1,traceHMiddle2,traceHMiddleD1,traceHMiddleD2, traceHTop1, traceHTop2, traceHTopD1, traceHTopD2,traceToParent;
    const csgo::EngineTrace Trace=engineInterfaces.engineTrace();

    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius/2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x - botzConfig.nodeRadius/2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHBottom1);
    Trace.traceRay({ {botzConfig.checkOrigin.x,botzConfig.checkOrigin.y + botzConfig.nodeRadius/2,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x,botzConfig.checkOrigin.y - botzConfig.nodeRadius/2,botzConfig.checkOrigin.z} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHBottom2);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHBottomD1);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHBottomD2);

    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z+38.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z+38.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHMiddle1);
    Trace.traceRay({ {botzConfig.checkOrigin.x,  botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+38.f},{botzConfig.checkOrigin.x,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+38.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHMiddle2);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+38.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+38.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHMiddleD1);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+38.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+38.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHMiddleD2);

    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z+42.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y,botzConfig.checkOrigin.z+42.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHTop1);
    Trace.traceRay({ {botzConfig.checkOrigin.x,  botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f},{botzConfig.checkOrigin.x,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHTop2);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHTopD1);
    Trace.traceRay({ {botzConfig.checkOrigin.x + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f},{botzConfig.checkOrigin.x - botzConfig.nodeRadius / 2,botzConfig.checkOrigin.y + botzConfig.nodeRadius / 2,botzConfig.checkOrigin.z+42.f} }, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceHTopD2);

    Trace.traceRay({botzConfig.checkOrigin,parentpos}, MASK_PLAYERSOLID, localPlayer.get().getPOD(), traceToParent);
    if (traceToParent.contents != 0)
        botzConfig.traceToParentIntersects = true;
    else botzConfig.traceToParentIntersects = false;
    const bool walkable{ (traceHBottom1.contents==0 && traceHBottom2.contents == 0 && traceHBottomD1.contents == 0 && traceHBottomD2.contents == 0) },
               crouchable{ (traceHMiddle1.contents == 0 && traceHMiddle2.contents == 0 && traceHMiddleD1.contents == 0 && traceHMiddleD2.contents == 0) },
               jumpable{ (traceHTop1.contents == 0 && traceHTop2.contents == 0 && traceHTopD1.contents == 0 && traceHTopD2.contents == 0) },
               forcejumpable{parentpos.z+36.f < pos.z };

    //    const bool walkable{ (traceHBottom1.fraction == 1.0f && traceHBottom2.fraction == 1.0f && traceHBottomD1.fraction == 1.0f && traceHBottomD2.fraction == 1.0f) }, 
    //    crouchable{ (traceHMiddle1.fraction == 1.0f && traceHMiddle2.fraction == 1.0f && traceHMiddleD1.fraction == 1.0f && traceHMiddleD2.fraction == 1.0f) },
    //    jumpable{ (traceHTop1.fraction == 1.0f && traceHTop2.fraction == 1.0f && traceHTopD1.fraction == 1.0f && traceHTopD2.fraction == 1.0f) },

    if (forcejumpable)              //jump
        return 2;
    else if (walkable && crouchable&&jumpable) //just walk
        return 1;
    else if (walkable && crouchable&&!jumpable)    //crouch
        return 3;
    else return 0;
}

void Misc::drawPathfinding(const EngineInterfaces& engineInterfaces)noexcept {
    if (!botzConfig.isbotzon)
        return;
    if (!botzConfig.pathfindingDebug||!botzConfig.shouldDebug)
        return;
    if (!localPlayer)
        return;
    
    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;
    
    ImDrawList* dlist;
    dlist = ImGui::GetBackgroundDrawList();
    if (botzConfig.nodes.size() > 0) {
        for (uint32_t index = 0; index < botzConfig.nodes.size(); index++) {
            ImVec2 screenNodePos;
            Helpers::worldToScreenPixelAligned(botzConfig.nodes[index], screenNodePos);
            dlist->AddRectFilled({ screenNodePos.x - 13.f,screenNodePos.y - 13.f }, { screenNodePos.x + 13.f,screenNodePos.y + 20.f }, 0xCC333333);
            dlist->AddText({ screenNodePos.x - 12.f,screenNodePos.y - 13.f }, 0xFFFFFFFF, std::to_string(index).c_str());
            dlist->AddText({ screenNodePos.x - 12.f,screenNodePos.y }, 0xFFFFFFFF, std::to_string(botzConfig.nodesParents[index]).c_str());
            const char* walker{ "-" };
            switch (botzConfig.walkType[index]) {
            case 1:walker = "WALK"; break;
            case 2:walker = "JUMP"; break;
            case 3:walker = "CROUCH"; break;
            default:walker = "-"; break;
            }
            dlist->AddText({ screenNodePos.x - 12.f,screenNodePos.y + 13.f }, 0xFFFFFFFF, walker);
            dlist->AddText({ screenNodePos.x - 12, screenNodePos.y + 20.f }, 0xFFFFFFFF, std::to_string(botzConfig.fcost[index]).c_str());
            dlist->AddText(ImVec2(500, 125), 0xFFFFFFFF, std::to_string(std::distance(botzConfig.fcost.begin(), std::min_element(botzConfig.fcost.begin(), botzConfig.fcost.end()))).c_str());
        }
    }
    if (botzConfig.tracez.size() == 0||!botzConfig.drawPathfindingTraces)
        return;
    else {
            std::vector<ImVec2>traceScreenPosStart,traceScreenPosEnd;
            for (uint32_t index = 0; index < botzConfig.tracez.size(); index++) {
                ImVec2 temp;
                Helpers::worldToScreenPixelAligned(botzConfig.tracez[index].startpos, temp);
                traceScreenPosStart.push_back(temp);
                Helpers::worldToScreenPixelAligned(botzConfig.tracez[index].endpos, temp);
                traceScreenPosEnd.push_back(temp);

                dlist->AddLine(traceScreenPosStart[index], traceScreenPosEnd[index], ((botzConfig.tracez[index].fraction < 1.0f || botzConfig.tracez[0].contents == 1) ? 0xDDFF0000 : 0xDD00FF00));
            }
        }

    }

void Misc::addNeighborNodes(const EngineInterfaces& engineInterfaces) noexcept{
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    const csgo::EngineTrace eTrace = engineInterfaces.engineTrace();
    for (int index = 0; index < 8; index++) {
        csgo::Vector potentialOpen;
        csgo::Vector offset{ 0,0,0 };
        //2 3 4
        //1 d 5
        //0 7 6
        switch (index) {
        case 0:
            offset.x -= botzConfig.nodeRadius;
            offset.y -= botzConfig.nodeRadius;
            break;
        case 1:
            offset.x -= botzConfig.nodeRadius;
            break;
        case 2:
            offset.x -= botzConfig.nodeRadius;
            offset.y += botzConfig.nodeRadius;
            break;
        case 3:
            offset.y += botzConfig.nodeRadius;
            break;
        case 4:
            offset.x += botzConfig.nodeRadius;
            offset.y += botzConfig.nodeRadius;
            break;
        case 5:
            offset.x += botzConfig.nodeRadius;
            break;
        case 6:
            offset.x += botzConfig.nodeRadius;
            offset.y -= botzConfig.nodeRadius;
            break;   
        case 7:      
            offset.y -=botzConfig.nodeRadius;
            break;
        default:break;
        }
        potentialOpen = botzConfig.nodes[botzConfig.currentNode] + offset;
        if (botzConfig.currentNode != -1)
            botzConfig.fcost[botzConfig.currentNode] = 99999.f;
        if (std::find(botzConfig.nodes.begin(), botzConfig.nodes.end(), potentialOpen) != botzConfig.nodes.end())
            if(botzConfig.nodes[std::distance(botzConfig.nodes.begin(), std::find(botzConfig.nodes.begin(), botzConfig.nodes.end(), potentialOpen))].z < potentialOpen.z + botzConfig.nodeRadius&&
               botzConfig.nodes[std::distance(botzConfig.nodes.begin(), std::find(botzConfig.nodes.begin(), botzConfig.nodes.end(), potentialOpen))].z > potentialOpen.z - botzConfig.nodeRadius)
                continue;
        int collides = collisionCheck(engineInterfaces, potentialOpen,botzConfig.nodes[botzConfig.currentNode]);
        if (botzConfig.traceToParentIntersects&&potentialOpen.z - botzConfig.tempFloorPos.z > 30.f) {
            potentialOpen += offset;
            collides = collisionCheck(engineInterfaces, potentialOpen, botzConfig.nodes[botzConfig.currentNode]);
            if (collides == 0 || collides == 4)
                potentialOpen -= offset;
        }

        if (collides == 0||collides==4)
            continue;
        csgo::Vector checkOffset = potentialOpen - offset;
        potentialOpen.z = botzConfig.tempFloorPos.z;
        
        botzConfig.nodes.push_back(potentialOpen);
        botzConfig.nodesType.push_back(false);
        botzConfig.nodesParents.push_back(botzConfig.currentNode);
        botzConfig.walkType.push_back(collides);
        botzConfig.parentOffset.push_back(offset);
            
        //unused right now, going to use it for optimization
                                    //uncomment  whenever a better "node-already-exists" detection is added, improves pathfinding 10000x
        float fcost=(/*botzConfig.improvedPathfinding ? botzConfig.nodes.back().distTo(localPlayer.get().getAbsOrigin()) : 0.f) + */botzConfig.nodes.back().distTo(botzConfig.finalDestination));
        if (collides == 3)
            fcost += botzConfig.nodeRadius;
;        botzConfig.fcost.push_back(fcost);
        
        if (botzConfig.nodes.back().distTo(botzConfig.finalDestination) < botzConfig.nodeRadius-1.f)
        {
            botzConfig.pathFound = true;
            return;
        }
    }
}

void Misc::openNode(const EngineInterfaces& engineInterfaces, int nodeIndex) noexcept {
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;

    if (nodeIndex != 0&&botzConfig.nodesType[nodeIndex]==false) {
        botzConfig.currentNode = nodeIndex;
        botzConfig.nodesType[nodeIndex] = true;
        Misc::addNeighborNodes(engineInterfaces);
    }
}

void Misc::findPath(const EngineInterfaces& engineInterfaces) noexcept {
    if (!localPlayer||!localPlayer.get().isAlive())
        return;
    if (!(botzConfig.nodes.size() >0))
        return;
    if (!botzConfig.shouldwalk)
        return;
    if (botzConfig.finalDestination.distTo({0.f,0.f,0.f}) == 0.f)
        return;

    
    if (!botzConfig.pathFound)
        openNode(engineInterfaces, std::distance(botzConfig.fcost.begin(), std::min_element(botzConfig.fcost.begin(), botzConfig.fcost.end())));
    else {
        if(botzConfig.curWayPoint == -2)
            botzConfig.curWayPoint = (botzConfig.nodes.size() - 1);
        while (botzConfig.curWayPoint != -1) {
            botzConfig.waypoints.push_back(botzConfig.nodes[botzConfig.curWayPoint]);
            botzConfig.waypointWalkType.push_back(botzConfig.walkType[botzConfig.curWayPoint]);
            botzConfig.waypointParentOffset.push_back(botzConfig.parentOffset[botzConfig.curWayPoint]);
            botzConfig.curWayPoint = botzConfig.nodesParents[botzConfig.curWayPoint];
        }
    }
}

void Misc::pathfind(const EngineInterfaces& engineInterfaces, const Memory& memory) noexcept {
    if (!botzConfig.isbotzon)
        return;
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    const csgo::Engine& engine=engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;
    //resett
    botzConfig.pathFound = false;
    botzConfig.nodes.clear();
    botzConfig.nodesParents.clear();
    botzConfig.nodesType.clear();
    botzConfig.walkType.clear();
    botzConfig.fcost.clear();
    botzConfig.waypoints.clear();
    botzConfig.currentNode = 0;
    botzConfig.curWayPoint = -2;

    botzConfig.nodes.push_back(localPlayer.get().getAbsOrigin());
    fallDamageCheck(engineInterfaces, botzConfig.nodes.back());
    botzConfig.nodes.back().z = botzConfig.tempFloorPos.z;
    botzConfig.nodesType.push_back(true);
    botzConfig.nodesParents.push_back(-1);
    botzConfig.walkType.push_back(1);
    botzConfig.fcost.push_back(999999.f);
    addNeighborNodes(engineInterfaces);
}

void Misc::drawPath(const EngineInterfaces& engineInterfaces) noexcept {
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    if (!botzConfig.circlesOrCost)
        return;
    ImDrawList* dlist;
    ImDrawList* dlistfg;
    dlist = ImGui::GetBackgroundDrawList();
    dlistfg = ImGui::GetForegroundDrawList();
    const csgo::Engine engine = engineInterfaces.getEngine();
    for (uint32_t index = 0; index < botzConfig.nodes.size(); index++) {
        ImVec2 screenPosTL, screenPosTR, screenPosBL, screenPosBR, screenPosMID;
        Helpers::worldToScreenPixelAligned({ botzConfig.nodes[index].x,       botzConfig.nodes[index].y,       botzConfig.nodes[index].z + 13.f }, screenPosMID);
        Helpers::worldToScreenPixelAligned({ botzConfig.nodes[index].x - botzConfig.nodeRadius/2.f,botzConfig.nodes[index].y + botzConfig.nodeRadius/2.f,botzConfig.nodes[index].z }, screenPosTL);
        Helpers::worldToScreenPixelAligned({ botzConfig.nodes[index].x + botzConfig.nodeRadius/2.f,botzConfig.nodes[index].y + botzConfig.nodeRadius/2.f,botzConfig.nodes[index].z }, screenPosTR);
        Helpers::worldToScreenPixelAligned({ botzConfig.nodes[index].x - botzConfig.nodeRadius/2.f,botzConfig.nodes[index].y - botzConfig.nodeRadius/2.f,botzConfig.nodes[index].z }, screenPosBL);
        Helpers::worldToScreenPixelAligned({ botzConfig.nodes[index].x + botzConfig.nodeRadius/2.f,botzConfig.nodes[index].y - botzConfig.nodeRadius/2.f,botzConfig.nodes[index].z }, screenPosBR);

        if (screenPosMID.x < 5 || screenPosTL.x < 5 || screenPosTR.x < 5 || screenPosBL.x < 5 || screenPosBR.x < 5||
            screenPosMID.y < 5 || screenPosTL.y < 5 || screenPosTR.y < 5 || screenPosBL.y < 5 || screenPosBR.y < 5)
            continue;
        if (botzConfig.nodesType[index]) {
            dlistfg->AddTriangleFilled(screenPosTL, screenPosTR, screenPosMID, 0x9944AA44);
            dlistfg->AddTriangleFilled(screenPosTL, screenPosBL, screenPosMID, 0x9944AA44);
            dlistfg->AddTriangleFilled(screenPosBL, screenPosBR, screenPosMID, 0x9944AA44);
            dlistfg->AddTriangleFilled(screenPosBR, screenPosTR, screenPosMID, 0x9944AA44);
            if (botzConfig.nodes[index].distTo(localPlayer.get().getEyePosition()) < 400.f) {
                dlistfg->AddLine(screenPosTR, screenPosBR, 0x99000000, 1.f);
                dlistfg->AddLine(screenPosTL, screenPosBL, 0x99000000, 1.f);
                dlistfg->AddLine(screenPosBL, screenPosMID, 0x99000000, 1.f);
                dlistfg->AddLine(screenPosBR, screenPosMID, 0x99000000, 1.f);
                dlistfg->AddLine(screenPosBR, screenPosBL, 0x99000000, 1.f);
                dlistfg->AddLine(screenPosTL, screenPosMID, 0x99000000, 1.f);
                dlistfg->AddLine(screenPosTR, screenPosMID, 0x99000000, 1.f);
                dlistfg->AddLine(screenPosTR, screenPosTL, 0x99000000, 1.f);
            }
        }
        else if (!botzConfig.nodesType[index]) {
            dlist->AddTriangleFilled(screenPosTL, screenPosTR, screenPosMID,0x99AA4444);
            dlist->AddTriangleFilled(screenPosTL, screenPosBL, screenPosMID,0x99AA4444);
            dlist->AddTriangleFilled(screenPosBL, screenPosBR, screenPosMID,0x99AA4444);
            dlist->AddTriangleFilled(screenPosBR, screenPosTR, screenPosMID,0x99AA4444);
            if (botzConfig.nodes[index].distTo(localPlayer.get().getEyePosition()) < 400.f) {
            dlist->AddLine(screenPosTR, screenPosBR,    0x99000000, 1.f);
            dlist->AddLine(screenPosTL, screenPosBL,    0x99000000, 1.f);
            dlist->AddLine(screenPosBL, screenPosMID,   0x99000000, 1.f);
            dlist->AddLine(screenPosBR, screenPosMID,   0x99000000, 1.f);
            dlist->AddLine(screenPosBR, screenPosBL,    0x99000000, 1.f);
            dlist->AddLine(screenPosTL, screenPosMID,   0x99000000, 1.f);
            dlist->AddLine(screenPosTR, screenPosMID,   0x99000000, 1.f);
            dlist->AddLine(screenPosTR, screenPosTL,    0x99000000, 1.f);
            }
        }

    }
    std::vector<ImVec2> points;
    if (botzConfig.waypoints.size() > 0) {
        for (uint32_t index = 0; index < botzConfig.waypoints.size(); index++) {
            points.push_back({ 0.f,0.f });
            Helpers::worldToScreenPixelAligned(botzConfig.waypoints[index], points.back());
        }
        for (uint32_t index = 0; index < points.size(); index++) {
            if (index != points.size() - 1)
                dlistfg->AddLine(points[index], points[index + 1], 0xFF8888FF, 3.f);
        }

    }
    ImVec2 screenPosFinal;
    Helpers::worldToScreenPixelAligned(botzConfig.finalDestination, screenPosFinal);
    dlist->AddCircleFilled(screenPosFinal, 15.f, 0xFFAA4444, 8);
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
    if (botzConfig.shouldScope) {
        cmd->buttons |= csgo::UserCmd::IN_ATTACK2;
        botzConfig.shouldScope = false;
    }
    if (botzConfig.shouldUnscope > 0) {
        cmd->buttons |= csgo::UserCmd::IN_ATTACK2;
        botzConfig.shouldUnscope--;
    }
    if (!botzConfig.isbotzon|| !botzConfig.shouldwalk||!botzConfig.pathFound||botzConfig.stopWalking)
        return;
    if (localPlayer.get().isScoped())
        botzConfig.shouldUnscope = 2;
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    if (botzConfig.nodes.size()<1)
       return;

    const csgo::Engine& engine = engineInterfaces.getEngine();
    csgo::Vector relAngle;
    csgo::Vector worldpos;
    worldpos = (botzConfig.waypoints.size() > 0 ? botzConfig.waypoints.back() :localPlayer.get().getAbsOrigin());
    relAngle = Aimbot::calculateRelativeAngle(localPlayer.get().getEyePosition(), worldpos, localPlayer.get().eyeAngles());
    //waypoint approximation will consider the point reached if the player is
    //within x units from the point i.e. the point is at x 1035 and approx is
    //15, the player will stop moving at 1035±15 and consider the point reached
    if (botzConfig.waypoints.size() > 0) {
        if ((localPlayer.get().getAbsOrigin().x - botzConfig.nodeRadius-1.f < botzConfig.waypoints.back().x && botzConfig.waypoints.back().x < localPlayer.get().getAbsOrigin().x + botzConfig.nodeRadius-1.f) &&
            (localPlayer.get().getAbsOrigin().y - botzConfig.nodeRadius-1.f < botzConfig.waypoints.back().y && botzConfig.waypoints.back().y < localPlayer.get().getAbsOrigin().y + botzConfig.nodeRadius-1.f) &&
            (localPlayer.get().getAbsOrigin().z-50.f<botzConfig.waypoints.back().z&&botzConfig.waypoints.back().z<localPlayer.get().getAbsOrigin().z+50.f)) {
                botzConfig.waypoints.pop_back();
                botzConfig.waypointWalkType.pop_back();
        }
        else {
            if (botzConfig.aimAtPath) {
                botzConfig.startedAiming = memory.globalVars->realtime - botzConfig.reactionTime;
                if(botzConfig.aimreason==-1)
                    botzConfig.aimreason = 4;
                botzConfig.aimspot = { botzConfig.waypoints.back().x,botzConfig.waypoints.back().y,botzConfig.waypoints.back().z + 62.f };
            }
            //not in position yet, move closer/do other stuff while you're walking(todo)
            if (botzConfig.waypointWalkType.back() > 1) {
                cmd->buttons |= csgo::UserCmd::IN_DUCK;
            }
            if (botzConfig.waypointWalkType.back() == 2)
                cmd->buttons |= csgo::UserCmd::IN_JUMP;
            cmd->forwardmove = 250 * cos(Helpers::deg2rad(relAngle.y));
            cmd->sidemove = 250 * sin(Helpers::deg2rad(relAngle.y)) * -1;
        }
    }




}



//static nodes pathfinding


void Misc::addNewNode(const EngineInterfaces& engineInterfaces,csgo::Vector pingPos) noexcept {
    if (botzConfig.walkbotType != 1)
        return;
    if (!localPlayer)
        return;
    if (!localPlayer.get().isAlive())
        return;
    const csgo::Engine& engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;
    const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon());
    int nodeType{ 0 };
    switch (activeWeapon.getWeaponData()->price) {
    case 50:
        engine.clientCmdUnrestricted("say holding a decoy rn"); 
        nodeType = 0;   //common
        break;
    case 200:
        engine.clientCmdUnrestricted("say holding a flash grenade rn"); 
        nodeType = 1;   //bombsite A
        break;
    case 300:
        engine.clientCmdUnrestricted("say holding a smoke/he grenade rn");
        nodeType = 2;   //bombsite B
        break;
    case 400:
    case 600:
        engine.clientCmdUnrestricted("say holding a firebomb rn");
        nodeType = 3;   //camping spot
        break;
    default:engine.clientCmdUnrestricted(std::to_string(activeWeapon.getWeaponData()->price).c_str());
    }
    botzConfig.presetNodes.push_back(pingPos);
    botzConfig.nodeGroup.push_back(nodeType);
}

void Misc::drawPresetNodes(const EngineInterfaces& engineInterfaces) noexcept {
    if (!botzConfig.circlesOrCost)
        return;
    if (botzConfig.walkbotType != 1)
        return;
    if (!localPlayer)
        return;
    const csgo::Engine& engine = engineInterfaces.getEngine();

    if (!engine.isInGame())
        return;


    ImDrawList* dlist;
    dlist = ImGui::GetBackgroundDrawList();
    for (uint32_t index = 0; index < botzConfig.presetNodes.size(); index++) {
        ImVec2 screenPosTL,screenPosTR,screenPosBL,screenPosBR,screenPosMID;
        ImU32 color;
        std::string nodetype;
        switch (botzConfig.nodeGroup[index]) {
        case 0:nodetype = "Common"; color = 0x99BBBBBB; break;
        case 1:nodetype = "Bombsite A"; color = 0x99FFA500; break;
        case 2:nodetype = "Bombiste B"; color = 0x99EBD934; break;
        case 3:nodetype = "Camping\n spot"; color = 0x99A534EB; break;
        }
        Helpers::worldToScreenPixelAligned({ botzConfig.presetNodes[index].x,botzConfig.presetNodes[index].y,botzConfig.presetNodes[index].z+13.f}, screenPosMID);
        Helpers::worldToScreenPixelAligned({ botzConfig.presetNodes[index].x - 16.f,botzConfig.presetNodes[index].y+16.f,botzConfig.presetNodes[index].z }, screenPosTL);
        Helpers::worldToScreenPixelAligned({ botzConfig.presetNodes[index].x + 16.f,botzConfig.presetNodes[index].y+16.f,botzConfig.presetNodes[index].z }, screenPosTR);
        Helpers::worldToScreenPixelAligned({ botzConfig.presetNodes[index].x - 16.f,botzConfig.presetNodes[index].y-16.f,botzConfig.presetNodes[index].z }, screenPosBL);
        Helpers::worldToScreenPixelAligned({ botzConfig.presetNodes[index].x + 16.f,botzConfig.presetNodes[index].y-16.f,botzConfig.presetNodes[index].z }, screenPosBR);
        if (screenPosMID.x < 1 || screenPosTL.x < 1 || screenPosTR.x < 1 || screenPosBL.x < 1 || screenPosBL.x < 1)
            continue;

        dlist->AddTriangleFilled(screenPosTL,screenPosTR,screenPosMID,color);
        dlist->AddTriangleFilled(screenPosTL,screenPosBL,screenPosMID,color);
        dlist->AddTriangleFilled(screenPosBL,screenPosBR,screenPosMID,color);
        dlist->AddTriangleFilled(screenPosBR,screenPosTR,screenPosMID,color);
        dlist->AddLine(screenPosTR, screenPosBR, 0xCC000000, 2.f);
        dlist->AddLine(screenPosTL, screenPosBL, 0xCC000000, 2.f);
        dlist->AddLine(screenPosBL, screenPosMID, 0xCC000000, 2.f);
        dlist->AddLine(screenPosBR, screenPosMID, 0xCC000000, 2.f);
        dlist->AddLine(screenPosBR, screenPosBL, 0xCC000000, 2.f);
        dlist->AddLine(screenPosTL, screenPosMID, 0xCC000000, 2.f);
        dlist->AddLine(screenPosTR, screenPosMID, 0xCC000000, 2.f);
        dlist->AddLine(screenPosTR, screenPosTL, 0xCC000000, 2.f);

        //if(index=botzConfig.targetStaticNode)
          //  dlist->AddText(screenPosMID, 0xFFFFFF00, "TARGET");

        if (screenPosMID.x > ImGui::GetIO().DisplaySize.x / 2.f - 40.f && screenPosMID.x<ImGui::GetIO().DisplaySize.x / 2.f + 40.f && screenPosMID.y>ImGui::GetIO().DisplaySize.y / 2.f - 40.f && screenPosMID.y < ImGui::GetIO().DisplaySize.y / 2.f + 40.f) {
            dlist->AddText(ImVec2(screenPosMID.x + 1.f, screenPosMID.y + 1.f), 0xFF000000, nodetype.c_str());
            dlist->AddText(screenPosMID, 0xFFFFFFFF, nodetype.c_str());
            if (GetKeyState(VK_DELETE) & 0x8000){
            botzConfig.presetNodes.erase(botzConfig.presetNodes.begin() + index);
            botzConfig.nodeGroup.erase(botzConfig.nodeGroup.begin() + index);
            }
        }
    }

}

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <string>
#include <codecvt>
#include <nlohmann/json.hpp>

void Misc::savePresetNodes() noexcept {
    json_t PresetNodes;

    auto& json = PresetNodes["BotNodes"];
    json["Mapname"] = botzConfig.currentMap.c_str();

    auto& nodes = json["Nodes"];
    for (uint32_t index = 0; index < botzConfig.presetNodes.size(); index++) {
        auto& node = nodes[std::to_string(index)];
        node["nodeGroup"] = botzConfig.nodeGroup[index];
        node["X"] = botzConfig.presetNodes[index].x;
        node["Y"] = botzConfig.presetNodes[index].y;
        node["Z"] = botzConfig.presetNodes[index].z;
    }


    TCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string folderPath = converter.to_bytes(path) + "\\slippery";
        std::filesystem::create_directory(folderPath);
        std::filesystem::create_directory(folderPath + "\\PresetNodes");

        std::ofstream file_out(folderPath + "\\PresetNodes" + "\\" + botzConfig.currentMap.c_str() + ".json");
            if (file_out.good())
                file_out << PresetNodes;
            file_out.close();

    }
}

void Misc::readPresetNodes() noexcept {
    json_t loadedPresetNodes;
    TCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string folderPath = converter.to_bytes(path) + "\\slippery\\PresetNodes";
        std::string filePath = folderPath + "\\" + botzConfig.currentMap.c_str() + ".json";

        std::ifstream file_in(filePath);
        if (file_in.good()) {
            file_in >> loadedPresetNodes;
        }
        file_in.close();
    }
    //if (botzConfig.presetNodes.size() == 0) {
    //    botzConfig.presetNodes = std::vector<csgo::Vector>();
    //}
    //if (botzConfig.nodeGroup.size() == 0) {
    //    botzConfig.nodeGroup = std::vector<int> nodeGroup;
    //}


    botzConfig.presetNodes.clear(); // clear current nodes

    Json::Value loadedNodes = loadedPresetNodes["BotNodes"]["Nodes"];

    //botzConfig.presetNodes.resize(loadedPresetNodes.size());
    //botzConfig.nodeGroup.resize(loadedPresetNodes.size());

    for (auto& nodeNumber : loadedNodes.getMemberNames()) {
        Json::Value node = loadedNodes[nodeNumber];
        //botzConfig.nodeGroup[std::stoul(nodeNumber)] = node["nodeGroup"].asInt();
        //botzConfig.presetNodes[std::stoul(nodeNumber)] = csgo::Vector{ node["X"].asFloat(), node["Y"].asFloat(), node["Z"].asFloat() };
        botzConfig.presetNodes.push_back(csgo::Vector{ node["X"].asFloat(), node["Y"].asFloat(), node["Z"].asFloat() });
        botzConfig.nodeGroup.push_back(node["nodeGroup"].asInt());
    }




}

void Misc::getMapNameOnce(const EngineInterfaces& engineInterfaces) noexcept {
    const csgo::Engine& engine=engineInterfaces.getEngine();
    botzConfig.currentMap = engine.getLevelName();
}


//misc bot shiz
void Misc::runBuybot(const EngineInterfaces& engineInterfaces) noexcept {
    if (!localPlayer || !localPlayer.get().isAlive())
        return;
    const csgo::Engine engine = engineInterfaces.getEngine();
    if (!engine.isInGame())
        return;

    if (botzConfig.roundStartTime + botzConfig.buyAfter > memory.globalVars->realtime || botzConfig.roundStartTime + 15.f < memory.globalVars->realtime)
        return;

    int playerMoney = localPlayer.get().account();

    if (playerMoney > 650 && !localPlayer.get().hasHelmet() && localPlayer.get().armor() < 1) {
        engine.clientCmdUnrestricted("buy vest");
        return;
    }
    if (playerMoney > 350 && !localPlayer.get().hasHelmet()) {
        engine.clientCmdUnrestricted("echo buy vesthelm");
        return;
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
        if (botzConfig.botState != 1 && botzConfig.botState != 2)
            engine.clientCmdUnrestricted(botzConfig.radioTranslate[17].c_str());
        else engine.clientCmdUnrestricted(botzConfig.radioTranslate[21].c_str());
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
    std::string eventName = event.getName();                                                                             //12:weapon_fire       13:player_ping      14:player_connect
    
    eventName += "!";
    //engine.clientCmdUnrestricted(eventName.c_str());

    const auto localUserId = localPlayer.get().getUserId(engine);
    const csgo::EngineTrace& eTrace = engineInterfaces.engineTrace();
    csgo::Trace trace;
    std::string printToConsole;
    const auto entity = csgo::Entity::from(retSpoofGadgets->client, clientInterfaces.getEntityList().getEntity(engine.getPlayerForUserID(event.getInt("userid"))));

    const auto activeWeapon = csgo::Entity::from(retSpoofGadgets->client, localPlayer.get().getActiveWeapon());
    switch(eventType){                                                                                                   
    case 0:
        if (entity.isOtherEnemy(memory, localPlayer.get())) {
            if (event.getInt("attacker") == localUserId) {
                engine.clientCmdUnrestricted("enemydown");
                break;
            }
            
            if (int(fmod(memory.globalVars->realtime, botzConfig.complimentChance)) == 0) {
                engine.clientCmdUnrestricted("compliment");
                
            }
        }

        if (event.getInt("userid") == localUserId) {//set bot state to dead in case the bot died (wow)
            botzConfig.botState = 6;
            Misc::reportDeath(memory, engineInterfaces, event, true);
        }
        break;
    case 2:
        Misc::getMapNameOnce(engineInterfaces);
        Misc::populateGameInfo(engineInterfaces);
        
        botzConfig.roundStartTime = memory.globalVars->realtime;
        botzConfig.buyAfter = fmod(memory.globalVars->realtime, 13);
        if (activeWeapon.getWeaponType() != WeaponType::Knife)
            engine.clientCmdUnrestricted("slot3");
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
        if (!entity.isOtherEnemy(memory, localPlayer.get()))
            return;
        botzConfig.aimspot = entity.getBonePosition(4);
        eTrace.traceRay({ {botzConfig.aimspot.x,botzConfig.aimspot.y,botzConfig.aimspot.z+50.f},localPlayer.get().getEyePosition()}, MASK_OPAQUE, localPlayer.get().getPOD(), trace);
        botzConfig.isShooterVisible = (trace.contents == 0);

        if (botzConfig.startedAiming + botzConfig.reactionTime < memory.globalVars->realtime) {
            botzConfig.startedAiming = memory.globalVars->realtime;
            botzConfig.aimreason = 0;
        }
        break;
    case 13:
        if (botzConfig.walkbotType == 0) {
            if (!botzConfig.shouldGoTowardsPing)
                return;
            
            if (!entity.isOtherEnemy(memory, localPlayer.get())) {
                botzConfig.playerPingLoc = { event.getFloat("x"),event.getFloat("y"),event.getFloat("z") };
                botzConfig.finalDestination = botzConfig.playerPingLoc;
                Misc::pathfind(engineInterfaces, memory);
            }
            break;
        }
        else if (botzConfig.walkbotType == 1) {
            Misc::addNewNode(engineInterfaces, { event.getFloat("x"),event.getFloat("y"),event.getFloat("z") });
            break;
        }
        break;
    case 14:
        Misc::clearGameInfo();
        break;
    default:break;
    }
    

}


void Misc::handleBotState(const Memory& memory, const EngineInterfaces& engineInterfaces) noexcept {

    if (!localPlayer || !localPlayer.get().isAlive())
        return;

    const csgo::Engine& engine = engineInterfaces.getEngine();

    if (!engine.isConnected())
        return;

    std::string say_team = "say_team ";
    if (botzConfig.botState == botzConfig.botStatePrev)
        return;

    switch (botzConfig.botState) {
    case 0:break;   //idling
    case 1:break;   //in combat
    case 2:         //see enemy
        if (botzConfig.enemyEntities.size() < 1) {
            botzConfig.botState = 0;
            break;
        }
        say_team += "I see an enemy at ";
        say_team += botzConfig.enemyEntities.front().lastPlaceName();
        engine.clientCmdUnrestricted(say_team.c_str());
        engine.clientCmdUnrestricted("needbackup");
        
        break;
    case 3:break;   //reloading
    case 4:break;   //following teammate
    case 5:break;   //throwing a grenade
    case 6:         //bot just died
        break;
    case 7:break;   //defending bombsite
    case 8:break;   //searching for c4
    }
    botzConfig.botStatePrev = botzConfig.botState;
}
//debug
void Misc::debugDraw(const Memory& memory, const EngineInterfaces& engineInterfaces) noexcept {
    if (!localPlayer)
        return;
    ImDrawList* dlist=ImGui::GetBackgroundDrawList();
    
    //dlist->AddText(ImVec2(500, 125), 0xFFCCCCCC, std::to_string(botzConfig.buyAfter).c_str());
    //dlist->AddText(ImVec2(500, 145), 0xFFCCCCCC, std::to_string(botzConfig.roundStartTime).c_str());
    //dlist->AddText(ImVec2(500, 165), 0xFFCCCCCC, std::to_string(memory.globalVars->realtime).c_str());
    //dlist->AddText(ImVec2(500, 185), (botzConfig.roundStartTime + botzConfig.buyAfter > memory.globalVars->realtime || botzConfig.roundStartTime + 15.f < memory.globalVars->realtime?0xFFCC0000:0xFF00CC00), std::to_string(botzConfig.buyAfter+botzConfig.roundStartTime).c_str());
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

static bool windowOpen = true;

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
    ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize - ImVec2(50, 25), ImGuiCond_Once);
    ImGui::Begin("unhook", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove);
    if (ImGui::Button("unload"))
        hooks->uninstall(*this, glow, engineInterfaces, clientInterfaces, interfaces, memory, visuals, inventoryChanger);
    ImGui::End();
    if (!contentOnly) {
        if (!windowOpen)
            return;
        ImGui::SetNextWindowSize({ 580.0f, 390.0f });
        ImGui::Begin("Misc", &windowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 4.f));
    ImGui::SetCursorPos(ImVec2(4.f, 4.f));


    ImGui::BeginChild("##tabs", ImVec2(572.f, 36.f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 3.f);
    if (ImGui::Button(translate.tabWalkbot[miscConfig.language].c_str(), ImVec2(140.f, 28.f)))
        miscConfig.guiTab = 0;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()-6.f);
    if (ImGui::Button(translate.tabEvents[miscConfig.language].c_str(), ImVec2(141.f, 28.f)))
        miscConfig.guiTab = 1;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()-6.f);
    if (ImGui::Button(translate.tabChat[miscConfig.language].c_str(), ImVec2(142.f, 28.f)))
        miscConfig.guiTab = 2;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()-6.f);
    if (ImGui::Button(translate.tabMisc[miscConfig.language].c_str(), ImVec2(140.f, 28.f)))
        miscConfig.guiTab = 3;
    ImGui::PopStyleVar();
    ImGui::PopFont();
    ImGui::EndChild();

    
    ImGui::SetCursorPosX(4.f);
    ImGui::BeginChild("##main", ImVec2(572, 341),true,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    switch (miscConfig.guiTab) {
    case 0:
        ImGui::Checkbox(translate.walkToggleBot[miscConfig.language].c_str(), &botzConfig.isbotzon);
        if (botzConfig.isbotzon) {
            if (ImGui::Button("Walkbot Type", ImVec2(161.f, 20.f))) {
                miscConfig.walkbotTypeOpen = true;
            }
            if (miscConfig.walkbotTypeOpen) {
                ImGui::SetNextWindowPos(ImGui::GetCursorPos() + ImGui::GetWindowPos());
                ImGui::Begin("##walkbotSelector", &miscConfig.walkbotTypeOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
                if (ImGui::Button(translate.walkAutoPath[miscConfig.language].c_str(), ImVec2(145.f, 20.f)))
                    botzConfig.walkbotType = 0;
                if (ImGui::Button(translate.walkPresetNodes[miscConfig.language].c_str(), ImVec2(145.f, 20.f)))
                    botzConfig.walkbotType = 1;
                if (!ImGui::IsWindowFocused())
                    miscConfig.walkbotTypeOpen = false;
                ImGui::End();
            }
            if (botzConfig.walkbotType == 0) {
                ImGui::Checkbox(translate.walkShouldWalk[miscConfig.language].c_str(), &botzConfig.shouldwalk);

                //ImGui::Checkbox(translate.walkImprovedPfind[miscConfig.language].c_str(), &botzConfig.improvedPathfinding);
                //if (ImGui::IsItemHovered()) {
                //    ImGui::BeginTooltip();
                //    ImGui::Text(translate.walkImprovedPWarn[miscConfig.language].c_str());
                //    ImGui::EndTooltip();
                //}
                    
                ImGui::Checkbox(translate.walkGotoPings[miscConfig.language].c_str(), &botzConfig.shouldGoTowardsPing);
                ImGui::Checkbox(translate.walkAimAtPath[miscConfig.language].c_str(), &botzConfig.aimAtPath);
                ImGui::Separator();
                
                ImGui::PushItemWidth(200.f);
                ImGui::SliderFloat(translate.walkMaxFallDamage[miscConfig.language].c_str(), &botzConfig.dropdownDmg, 0.0f, 0.99f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
                ImGui::SliderInt(translate.walkNodeSpacing[miscConfig.language].c_str(), &botzConfig.nodeRadius, 2, 150, "%d", ImGuiSliderFlags_AlwaysClamp);
                ImGui::PopItemWidth();
                ImGui::Separator();

                ImGui::Checkbox(translate.walkDrawNodes[miscConfig.language].c_str(), &botzConfig.circlesOrCost);
                ImGui::Checkbox(translate.walkDebugFeatures[miscConfig.language].c_str(), &botzConfig.shouldDebug);
                if (botzConfig.shouldDebug) {
                    ImGui::Text("|");
                    ImGui::SameLine();
                    ImGui::Checkbox(translate.walkDrawDebugInfo[miscConfig.language].c_str(), &botzConfig.pathfindingDebug);
                    ImGui::Text("|");
                    ImGui::SameLine();
                    ImGui::Checkbox(translate.walkDrawTraces[miscConfig.language].c_str(), &botzConfig.drawPathfindingTraces);
                    ImGui::Text("|");
                    ImGui::SameLine();
                    if (ImGui::Button(translate.walkForceResetPos[miscConfig.language].c_str()))
                        pathfind(engineInterfaces, memory);

                }
                ImGui::Separator();
            }
            else {
                ImGui::Checkbox(translate.walkDrawNodes[miscConfig.language].c_str(), &botzConfig.circlesOrCost);
                ImGui::Text("Current map: ");
                ImGui::SameLine();
                ImGui::Text(botzConfig.currentMap.c_str());
                if(ImGui::Button("Update map name"))
                    Misc::getMapNameOnce(engineInterfaces);
                if (ImGui::Button("Save Nodes"))
                    Misc::savePresetNodes();
                if (ImGui::Button("Load Nodes"))
                    Misc::readPresetNodes();
                std::string mapInList="Is the map in list: ";
                mapInList += (std::find(botzConfig.maplist.begin(), botzConfig.maplist.end(), botzConfig.currentMap) != botzConfig.maplist.end() ? "yes" : "no");
                ImGui::Text(mapInList.c_str());
                if (ImGui::Button("Bot control panel"))
                    miscConfig.botControlOpen = true;
                if (miscConfig.botControlOpen) {
                    ImGui::SetNextWindowSize({ 150.f,75.f }, ImGuiCond_Once);
                    ImGui::Begin("Bot Control Panel", &miscConfig.botControlOpen);
                    ImGui::SliderInt("target node:", &botzConfig.targetStaticNode,0,botzConfig.presetNodes.size()-1,"%d",ImGuiSliderFlags_AlwaysClamp);

                    ImGui::End();
                }
            }   
        }
        break;
    case 1:
        ImGui::Checkbox(translate.eventsAutoreload[miscConfig.language].c_str(), &botzConfig.autoreload);
        if (botzConfig.autoreload) {
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::PushItemWidth(200.f);
            ImGui::SliderInt(translate.eventsReloadTime[miscConfig.language].c_str(), &botzConfig.reloadAfterXSeconds, 0, 30);
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::SliderFloat(translate.eventsReloadClip[miscConfig.language].c_str(), &botzConfig.reloadIfClipPercent, 0.01f, 0.99f, "%.2f");
            ImGui::PopItemWidth();
        }
        ImGui::Separator();
        ImGui::Checkbox(translate.eventsReportDeath[miscConfig.language].c_str(), &botzConfig.shouldReportToTeam);
        if (botzConfig.shouldReportToTeam) {
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::Checkbox(translate.eventsReportPos[miscConfig.language].c_str(), &botzConfig.reportDetailsCallout);
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::Checkbox(translate.eventsReportKiller[miscConfig.language].c_str(), &botzConfig.reportDetailsDiedTo);
        }
        ImGui::Separator();
        ImGui::Checkbox(translate.eventsCompliment[miscConfig.language].c_str(), &botzConfig.shouldCompliment);
        if (botzConfig.shouldCompliment) {
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::PushItemWidth(200.f);
            ImGui::SliderFloat(translate.eventsComplimentPC[miscConfig.language].c_str(), &botzConfig.complimentChance, 1.f, 100.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopItemWidth();
        }
        ImGui::Separator();
        ImGui::Checkbox("Aim at events", &botzConfig.aimAtEvents);
        if (botzConfig.aimAtEvents) {
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::PushItemWidth(200.f);
            ImGui::SliderFloat("Reaction time", &botzConfig.reactionTime, 0.f, 1.f, "%.2f");
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::SliderFloat("Aim time", &botzConfig.aimtime[0], -2.2f, 0.28f, "%.2f");
        }
            break;
    case 2:break;
    case 3:
        ImGui::Checkbox(translate.miscOverheadChat[miscConfig.language].c_str(), &miscConfig.overheadChat);
        ImGui::hotkey(translate.miscMenuKey[miscConfig.language].c_str(), miscConfig.menuKey);
        ImGui::Checkbox(translate.miscBhop[miscConfig.language].c_str(), &miscConfig.bunnyHop);
        ImGui::Checkbox(translate.miscAutoaccept[miscConfig.language].c_str(), &miscConfig.autoAccept);

        ImGui::InputText("##input", &miscConfig.panoramaEvent);
        if (ImGui::Button("test"))
            Misc::autoqueue();
        if(!engineInterfaces.getEngine().isInGame())
            if (ImGui::Button(translate.miscStartMM[miscConfig.language].c_str())) {
                //csgo::UIEngine::from(retSpoofGadgets->client, interfaces.getPanoramaUIEngine().accessUIEngine()).RunScript(Misc::GetRoot(engineInterfaces.getEngine().isInGame()), "LobbyAPI.LaunchTrainingMap()", "panorama/layout/base.xml", 8, 10, false);
                miscConfig.matchmakingStartTime = memory.globalVars->realtime;
            }
        if (miscConfig.matchmakingStartTime > 0.f) {
            ImGui::Begin("mmShit", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
            ImGui::Text("in dev atm sorry!!");
            ImGui::End();
        }
        if (miscConfig.matchmakingStartTime + 1.7f < memory.globalVars->realtime)
            miscConfig.matchmakingStartTime = 0.f;
        if (ImGui::Button(translate.miscBotsList[miscConfig.language].c_str()))
            miscConfig.botListOpen = true;

        if (ImGui::Button(translate.miscImAddicted[miscConfig.language].c_str()))
            Misc::antiaddiction();

        if (ImGui::Button("Populate game info"))
            Misc::populateGameInfo(engineInterfaces);
        
        if (ImGui::Button("Clear game info"))
            Misc::clearGameInfo();

        ImGui::SetCursorPos(ImVec2(410, 319));
        if (ImGui::Button(translate.miscLanguage[miscConfig.language].c_str(), ImVec2(161.f, 20.f))) {
            miscConfig.langWindowOpen = true;
        }
        if (miscConfig.langWindowOpen) {
            ImGui::SetNextWindowPos(ImVec2(410.f,340.f) + ImGui::GetWindowPos());
            ImGui::Begin("##languageSelector", &miscConfig.langWindowOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
            if (ImGui::Button(translate.miscLanguageEng[miscConfig.language].c_str(),ImVec2(145.f,20.f)))
                miscConfig.language = 0;
            if (ImGui::Button(translate.miscLanguageGer[miscConfig.language].c_str(), ImVec2(145.f, 20.f)))
                miscConfig.language = 1;
            if (ImGui::Button(translate.miscLanguageRus[miscConfig.language].c_str(), ImVec2(145.f, 20.f)))
                miscConfig.language = 2;
            if (ImGui::Button(translate.miscLanguageAus[miscConfig.language].c_str(), ImVec2(145.f, 20.f)))
                miscConfig.language = 3;
            if (!ImGui::IsWindowFocused())
                miscConfig.langWindowOpen = false;
            ImGui::End();
        }
        break;
    }

    ImGui::EndChild();

    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();

    if (miscConfig.botListOpen) {
        ImGui::Begin("bot list", &miscConfig.botListOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |ImGuiWindowFlags_NoTitleBar);
        if (ImGui::Button(translate.blistCheckForBots[miscConfig.language].c_str())) {
            botzConfig.botsList.clear();
            engineInterfaces.getEngine().clientCmdUnrestricted("say \";^3?\"");
        }
        ImGui::Text(translate.blistYourEncUid[miscConfig.language].c_str());
        ImGui::SameLine();
        if(localPlayer)
            ImGui::Text(encrypted.uid[localPlayer.get().getUserId(engineInterfaces.getEngine())].c_str());

        ImGui::Text(translate.miscBotsList[miscConfig.language].c_str()+':');
        ImGui::Separator();
        const csgo::Engine& engine = engineInterfaces.getEngine();
        std::string message = "say ";
        if(localPlayer&&engineInterfaces.getEngine().isInGame())
            for (uint32_t index = 0; index < botzConfig.botsList.size(); index++) {
                csgo::PlayerInfo pInfo;
                engine.getPlayerInfo(engine.getPlayerForUserID(botzConfig.botsList[index]), pInfo);

                ImGui::Text(pInfo.name);
                ImGui::SameLine();
                ImGui::Text(std::to_string(pInfo.userId).c_str());
                ImGui::SameLine();
                ImGui::Text(encrypted.uid[pInfo.userId].c_str());
                ImGui::SameLine();
                ImGui::Button(translate.blistAskToListen[miscConfig.language].c_str());
                if (ImGui::IsItemClicked()) {
                    message += encrypted.uid[pInfo.userId];
                    engineInterfaces.getEngine().clientCmdUnrestricted(message.c_str());
                    botzConfig.botsListeningToMe.push_back(pInfo.userId);
                    botzConfig.botsListeningToMe.push_back(-1); //padding for std::find
                }
                if (std::find(botzConfig.botsListeningToMe.begin(), botzConfig.botsListeningToMe.end(), pInfo.userId) != botzConfig.botsListeningToMe.end()) {
                    ImGui::SameLine();
                    ImGui::Button(translate.blistAskToCome[miscConfig.language].c_str());
                    if (ImGui::IsItemClicked()) {
                        message += encrypted.phraseList[comeToMe];
                        engineInterfaces.getEngine().clientCmdUnrestricted(message.c_str());
                        botzConfig.botsListeningToMe.erase(std::find(botzConfig.botsListeningToMe.begin(), botzConfig.botsListeningToMe.end(), pInfo.userId) + 1);
                        botzConfig.botsListeningToMe.erase(std::find(botzConfig.botsListeningToMe.begin(), botzConfig.botsListeningToMe.end(), pInfo.userId));
                    }
                }
                ImGui::Separator();
                    
            }
        ImGui::End();
    }

}
//
//static void from_json(const json& j, ImVec2& v)
//{
//    read(j, "X", v.x);
//    read(j, "Y", v.y);
//}
//
//static void from_json(const json& j, PurchaseList& pl)
//{
//    read(j, "Enabled", pl.enabled);
//    read(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
//    read(j, "Show Prices", pl.showPrices);
//    read(j, "No Title Bar", pl.noTitleBar);
//    read(j, "Mode", pl.mode);
//}
//
//static void from_json(const json& j, OffscreenEnemies& o)
//{
//    from_json(j, static_cast<ColorToggle&>(o));
//
//    read<value_t::object>(j, "Health Bar", o.healthBar);
//}
//
//static void from_json(const json& j, MiscConfig::SpectatorList& sl)
//{
//    read(j, "Enabled", sl.enabled);
//    read(j, "No Title Bar", sl.noTitleBar);
//    read<value_t::object>(j, "Pos", sl.pos);
//    read<value_t::object>(j, "Size", sl.size);
//}
//
//static void from_json(const json& j, MiscConfig::Watermark& o)
//{
//    read(j, "Enabled", o.enabled);
//}
//
//static void from_json(const json& j, PreserveKillfeed& o)
//{
//    read(j, "Enabled", o.enabled);
//    read(j, "Only Headshots", o.onlyHeadshots);
//}
//
//static void from_json(const json& j, MiscConfig& m)
//{
//    read(j, "Menu key", m.menuKey);
//    read(j, "Anti AFK kick", m.antiAfkKick);
//    read(j, "Auto strafe", m.autoStrafe);
//    read(j, "Bunny hop", m.bunnyHop);
//    read(j, "Custom clan tag", m.customClanTag);
//    read(j, "Clock tag", m.clocktag);
//    read(j, "Clan tag", m.clanTag, sizeof(m.clanTag));
//    read(j, "Animated clan tag", m.animatedClanTag);
//    read(j, "Fast duck", m.fastDuck);
//    read(j, "Moonwalk", m.moonwalk);
//    read(j, "Edge Jump", m.edgejump);
//    read(j, "Edge Jump Key", m.edgejumpkey);
//    read(j, "Slowwalk", m.slowwalk);
//    read(j, "Slowwalk key", m.slowwalkKey);
//    read<value_t::object>(j, "Noscope crosshair", m.noscopeCrosshair);
//    read<value_t::object>(j, "Recoil crosshair", m.recoilCrosshair);
//    read(j, "Auto pistol", m.autoPistol);
//    read(j, "Auto reload", m.autoReload);
//    read(j, "Auto accept", m.autoAccept);
//    read(j, "Radar hack", m.radarHack);
//    read(j, "Reveal ranks", m.revealRanks);
//    read(j, "Reveal money", m.revealMoney);
//    read(j, "Reveal suspect", m.revealSuspect);
//    read(j, "Reveal votes", m.revealVotes);
//    read<value_t::object>(j, "Spectator list", m.spectatorList);
//    read<value_t::object>(j, "Watermark", m.watermark);
//    read<value_t::object>(j, "Offscreen Enemies", m.offscreenEnemies);
//    read(j, "Fix animation LOD", m.fixAnimationLOD);
//    read(j, "Fix movement", m.fixMovement);
//    read(j, "Disable model occlusion", m.disableModelOcclusion);
//    read(j, "Aspect Ratio", m.aspectratio);
//    read(j, "Kill message", m.killMessage);
//    read<value_t::string>(j, "Kill message string", m.killMessageString);
//    read(j, "Name stealer", m.nameStealer);
//    read(j, "Disable HUD blur", m.disablePanoramablur);
//    read(j, "Ban color", m.banColor);
//    read<value_t::string>(j, "Ban text", m.banText);
//    read(j, "Fast plant", m.fastPlant);
//    read(j, "Fast Stop", m.fastStop);
//    read<value_t::object>(j, "Bomb timer", m.bombTimer);
//    read(j, "Quick reload", m.quickReload);
//    read(j, "Prepare revolver", m.prepareRevolver);
//    read(j, "Prepare revolver key", m.prepareRevolverKey);
//    read(j, "Hit sound", m.hitSound);
//    read(j, "Choked packets", m.chokedPackets);
//    read(j, "Choked packets key", m.chokedPacketsKey);
//    read(j, "Quick healthshot key", m.quickHealthshotKey);
//    read(j, "Grenade predict", m.nadePredict);
//    read(j, "Fix tablet signal", m.fixTabletSignal);
//    read(j, "Max angle delta", m.maxAngleDelta);
//    read(j, "Fix tablet signal", m.fixTabletSignal);
//    read<value_t::string>(j, "Custom Hit Sound", m.customHitSound);
//    read(j, "Kill sound", m.killSound);
//    read<value_t::string>(j, "Custom Kill Sound", m.customKillSound);
//    read<value_t::object>(j, "Purchase List", m.purchaseList);
//    read<value_t::object>(j, "Reportbot", m.reportbot);
//    read(j, "Opposite Hand Knife", m.oppositeHandKnife);
//    read<value_t::object>(j, "Preserve Killfeed", m.preserveKillfeed);
//}
//
//static void from_json(const json& j, MiscConfig::Reportbot& r)
//{
//    read(j, "Enabled", r.enabled);
//    read(j, "Target", r.target);
//    read(j, "Delay", r.delay);
//    read(j, "Rounds", r.rounds);
//    read(j, "Abusive Communications", r.textAbuse);
//    read(j, "Griefing", r.griefing);
//    read(j, "Wall Hacking", r.wallhack);
//    read(j, "Aim Hacking", r.aimbot);
//    read(j, "Other Hacking", r.other);
//}
//
//static void to_json(json& j, const MiscConfig::Reportbot& o, const MiscConfig::Reportbot& dummy = {})
//{
//    WRITE("Enabled", enabled);
//    WRITE("Target", target);
//    WRITE("Delay", delay);
//    WRITE("Rounds", rounds);
//    WRITE("Abusive Communications", textAbuse);
//    WRITE("Griefing", griefing);
//    WRITE("Wall Hacking", wallhack);
//    WRITE("Aim Hacking", aimbot);
//    WRITE("Other Hacking", other);
//}
//
//static void to_json(json& j, const PurchaseList& o, const PurchaseList& dummy = {})
//{
//    WRITE("Enabled", enabled);
//    WRITE("Only During Freeze Time", onlyDuringFreezeTime);
//    WRITE("Show Prices", showPrices);
//    WRITE("No Title Bar", noTitleBar);
//    WRITE("Mode", mode);
//}
//
//static void to_json(json& j, const ImVec2& o, const ImVec2& dummy = {})
//{
//    WRITE("X", x);
//    WRITE("Y", y);
//}
//
//static void to_json(json& j, const OffscreenEnemies& o, const OffscreenEnemies& dummy = {})
//{
//    to_json(j, static_cast<const ColorToggle&>(o), dummy);
//
//    WRITE("Health Bar", healthBar);
//}
//
//static void to_json(json& j, const MiscConfig::SpectatorList& o, const MiscConfig::SpectatorList& dummy = {})
//{
//    WRITE("Enabled", enabled);
//    WRITE("No Title Bar", noTitleBar);
//
//    if (const auto window = ImGui::FindWindowByName("Spectator list")) {
//        j["Pos"] = window->Pos;
//        j["Size"] = window->SizeFull;
//    }
//}
//
//static void to_json(json& j, const MiscConfig::Watermark& o, const MiscConfig::Watermark& dummy = {})
//{
//    WRITE("Enabled", enabled);
//}
//
//static void to_json(json& j, const PreserveKillfeed& o, const PreserveKillfeed& dummy = {})
//{
//    WRITE("Enabled", enabled);
//    WRITE("Only Headshots", onlyHeadshots);
//}
//
//static void to_json(json& j, const MiscConfig& o)
//{
//    const MiscConfig dummy;
//
//    WRITE("Menu key", menuKey);
//    WRITE("Anti AFK kick", antiAfkKick);
//    WRITE("Auto strafe", autoStrafe);
//    WRITE("Bunny hop", bunnyHop);
//    WRITE("Custom clan tag", customClanTag);
//    WRITE("Clock tag", clocktag);
//
//    if (o.clanTag[0])
//        j["Clan tag"] = o.clanTag;
//
//    WRITE("Animated clan tag", animatedClanTag);
//    WRITE("Fast duck", fastDuck);
//    WRITE("Moonwalk", moonwalk);
//    WRITE("Edge Jump", edgejump);
//    WRITE("Edge Jump Key", edgejumpkey);
//    WRITE("Slowwalk", slowwalk);
//    WRITE("Slowwalk key", slowwalkKey);
//    WRITE("Noscope crosshair", noscopeCrosshair);
//    WRITE("Recoil crosshair", recoilCrosshair);
//    WRITE("Auto pistol", autoPistol);
//    WRITE("Auto reload", autoReload);
//    WRITE("Auto accept", autoAccept);
//    WRITE("Radar hack", radarHack);
//    WRITE("Reveal ranks", revealRanks);
//    WRITE("Reveal money", revealMoney);
//    WRITE("Reveal suspect", revealSuspect);
//    WRITE("Reveal votes", revealVotes);
//    WRITE("Spectator list", spectatorList);
//    WRITE("Watermark", watermark);
//    WRITE("Offscreen Enemies", offscreenEnemies);
//    WRITE("Fix animation LOD", fixAnimationLOD);
//    WRITE("Fix movement", fixMovement);
//    WRITE("Disable model occlusion", disableModelOcclusion);
//    WRITE("Aspect Ratio", aspectratio);
//    WRITE("Kill message", killMessage);
//    WRITE("Kill message string", killMessageString);
//    WRITE("Name stealer", nameStealer);
//    WRITE("Disable HUD blur", disablePanoramablur);
//    WRITE("Ban color", banColor);
//    WRITE("Ban text", banText);
//    WRITE("Fast plant", fastPlant);
//    WRITE("Fast Stop", fastStop);
//    WRITE("Bomb timer", bombTimer);
//    WRITE("Quick reload", quickReload);
//    WRITE("Prepare revolver", prepareRevolver);
//    WRITE("Prepare revolver key", prepareRevolverKey);
//    WRITE("Hit sound", hitSound);
//    WRITE("Choked packets", chokedPackets);
//    WRITE("Choked packets key", chokedPacketsKey);
//    WRITE("Quick healthshot key", quickHealthshotKey);
//    WRITE("Grenade predict", nadePredict);
//    WRITE("Fix tablet signal", fixTabletSignal);
//    WRITE("Max angle delta", maxAngleDelta);
//    WRITE("Fix tablet signal", fixTabletSignal);
//    WRITE("Custom Hit Sound", customHitSound);
//    WRITE("Kill sound", killSound);
//    WRITE("Custom Kill Sound", customKillSound);
//    WRITE("Purchase List", purchaseList);
//    WRITE("Reportbot", reportbot);
//    WRITE("Opposite Hand Knife", oppositeHandKnife);
//    WRITE("Preserve Killfeed", preserveKillfeed);
//}
//
//json Misc::toJson() noexcept
//{
//    json j;
//    to_json(j, miscConfig);
//    return j;
//}
//
//void Misc::fromJson(const json& j) noexcept
//{
//    from_json(j, miscConfig);
//}

void Misc::resetConfig() noexcept
{
    miscConfig = {};
}
