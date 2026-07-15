#include "Archipelago.h"

#ifdef ENABLE_ARCHIPELAGO

#include <apclient.hpp>
#include <apuuid.hpp>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>

#include <algorithm>
#include <functional>
#include <map>

#include "2s2h/BenGui/Notification.h"
#include "2s2h/BenGui/UIWidgets.hpp"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/StaticData/StaticData.h"

extern "C" {
#include "variables.h"
extern PlayState* gPlayState;
}

namespace {
constexpr const char* ServerCVar = "gRemote.Archipelago.ServerAddress";
constexpr const char* SlotCVar = "gRemote.Archipelago.SlotName";
constexpr const char* PasswordCVar = "gRemote.Archipelago.Password";
constexpr const char* LastItemCVar = "gRemote.Archipelago.LastReceivedItem";
constexpr const char* LastSessionCVar = "gRemote.Archipelago.LastSession";
constexpr const char* WorldVersion = "0.4";
constexpr const char* ProgressiveIcon = "Archipelago Progressive Icon";
constexpr const char* UsefulIcon = "Archipelago Useful Icon";
constexpr const char* JunkIcon = "Archipelago Junk Icon";
constexpr const char* ConnectionIcon = "Archipelago Connection Icon";

std::string GetTriforceProgressMessage() {
    const uint32_t required = RANDO_SAVE_OPTIONS[RO_TRIFORCE_PIECES_REQUIRED];
    const uint32_t collected = gSaveContext.save.shipSaveInfo.rando.foundTriforcePieces + 1;
    if (collected >= required) {
        return " You completed the Triforce!";
    }

    const uint32_t remaining = required - collected;
    return " " + std::to_string(remaining) + (remaining == 1 ? " piece remains." : " pieces remain.");
}

unsigned int NormalizeItemFlags(RandoItemId itemId, unsigned int flags) {
    switch (Rando::StaticData::Items[itemId].randoItemType) {
        case RITYPE_MAJOR:
        case RITYPE_BOSS_KEY:
        case RITYPE_SMALL_KEY:
        case RITYPE_MASK:
            flags |= APClient::ItemFlags::FLAG_ADVANCEMENT;
            break;
        case RITYPE_LESSER:
        case RITYPE_HEALTH:
        case RITYPE_STRAY_FAIRY:
        case RITYPE_SKULLTULA_TOKEN:
            flags |= APClient::ItemFlags::FLAG_NEVER_EXCLUDE;
            break;
        default:
            break;
    }
    return flags;
}
} // namespace

namespace Archipelago {

Client& Client::Instance() {
    static Client instance;
    return instance;
}

bool Client::Connect() {
    Disconnect();

    const std::string server = CVarGetString(ServerCVar, "archipelago.gg:38281");
    const std::string slot = CVarGetString(SlotCVar, "");
    if (server.empty() || slot.empty()) {
        status = ConnectionStatus::Error;
        Log("Server address and slot name are required.");
        return false;
    }

    const std::string uuid = ap_get_uuid(Ship::Context::GetPathRelativeToAppDirectory("ap-client-uuid"));
    const std::string cert = Ship::Context::LocateFileAcrossAppDirs("networking/cacert.pem");
    client = std::make_unique<APClient>(uuid, GameName, server, cert);
    status = ConnectionStatus::Connecting;
    ConfigureHandlers();
    Log("Connecting to " + server + "...");
    return true;
}

void Client::ConfigureHandlers() {
    client->set_socket_error_handler([this](const std::string& message) {
        status = ConnectionStatus::Error;
        locationsScouted = false;
        Log("Connection error: " + message);
    });

    client->set_room_info_handler([this]() {
        std::list<std::string> tags;
        client->ConnectSlot(CVarGetString(SlotCVar, ""), CVarGetString(PasswordCVar, ""), 0b0111, tags, { 0, 6, 3 });
    });

    client->set_slot_connected_handler([this](const nlohmann::json slotData) {
        if (slotData.contains("client_version") && slotData["client_version"].get<std::string>().rfind(WorldVersion, 0) != 0) {
            status = ConnectionStatus::Error;
            Log("The APWorld and 2Ship client versions are incompatible.");
            return;
        }
        activeLocations.clear();
        scoutedLocations.clear();
        locationsScouted = false;
        randoOptions.clear();
        if (slotData.contains("active_locations")) {
            for (const int32_t checkId : slotData["active_locations"].get<std::vector<int32_t>>()) {
                activeLocations.insert(checkId);
            }
        }
        if (slotData.contains("rando_options") && slotData["rando_options"].is_object()) {
            for (const auto& [optionName, optionValue] : slotData["rando_options"].items()) {
                if (optionValue.is_number_integer()) {
                    randoOptions[optionName] = optionValue.get<int32_t>();
                }
            }
        }
        const std::string session = client->get_seed() + "|" + client->get_slot();
        if (session != CVarGetString(LastSessionCVar, "")) {
            CVarSetInteger(LastItemCVar, 0);
            CVarSetString(LastSessionCVar, session.c_str());
            Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        }
        status = ConnectionStatus::Connected;
        const auto checkedLocations = client->get_checked_locations();
        RecordCheckedLocations(std::list<int64_t>(checkedLocations.begin(), checkedLocations.end()));
        PrepareRandomizerSettings();
        Log("Connected as " + client->get_slot() + ".");
        SyncCompletedChecks();

        std::list<int64_t> locations;
        for (const int32_t checkId : activeLocations) {
            locations.push_back(LocationIdBase + checkId);
        }
        if (!locations.empty()) {
            client->LocationScouts(locations);
        } else {
            locationsScouted = true;
        }

        const std::string hintKey = "_read_hints_" + std::to_string(client->get_team_number()) + "_" +
                                    std::to_string(client->get_player_number());
        client->SetNotify({ hintKey });
        client->Get({ hintKey });
    });

    client->set_slot_refused_handler([this](const std::list<std::string>& reasons) {
        status = ConnectionStatus::Error;
        locationsScouted = false;
        Log(reasons.empty() ? "The server refused this slot." : reasons.front());
    });

    client->set_items_received_handler([this](const std::list<APClient::NetworkItem>& items) {
        const int nextItem = CVarGetInteger(LastItemCVar, 0);
        for (const auto& item : items) {
            if (item.index < nextItem || item.item < ItemIdBase || item.item >= ItemIdBase + RI_MAX) {
                continue;
            }
            const RandoItemId itemId = static_cast<RandoItemId>(item.item - ItemIdBase);
            receivedItems.push({ itemId, item.index, client->get_player_alias(item.player),
                                 NormalizeItemFlags(itemId, item.flags), item.location });
        }
    });

    client->set_location_checked_handler([this](const std::list<int64_t>& locations) {
        RecordCheckedLocations(locations);
    });

    client->set_location_info_handler([this](const std::list<APClient::NetworkItem>& items) {
        for (const APClient::NetworkItem& item : items) {
            const int64_t rawCheckId = item.location - LocationIdBase;
            if (rawCheckId <= RC_UNKNOWN || rawCheckId >= RC_MAX) {
                continue;
            }

            ScoutedLocation location;
            location.checkId = static_cast<RandoCheckId>(rawCheckId);
            location.receiver = item.player;
            location.playerName = client->get_player_alias(item.player);
            location.itemName = client->get_item_name(item.item, client->get_player_game(item.player));
            location.flags = item.flags;
            location.isLocal = item.player == client->get_player_number();
            if (client->get_player_game(item.player) == GameName && item.item >= ItemIdBase &&
                item.item < ItemIdBase + RI_MAX) {
                location.localItemId = static_cast<RandoItemId>(item.item - ItemIdBase);
                location.flags = NormalizeItemFlags(location.localItemId, location.flags);
            }
            if (location.isLocal && location.localItemId != RI_UNKNOWN) {
                RANDO_SAVE_CHECKS[location.checkId].randoItemId = location.localItemId;
            }
            scoutedLocations[location.checkId] = std::move(location);
        }
        locationsScouted = scoutedLocations.size() >= activeLocations.size();
        Log("Scouted " + std::to_string(scoutedLocations.size()) + " / " +
            std::to_string(activeLocations.size()) + " Archipelago locations.");
    });

    client->set_print_json_handler([this](const APClient::PrintJSONArgs& args) {
        ConsoleLine line;
        for (const auto& node : args.data) {
            ColoredTextNode output;
            output.text = node.text;

            if (node.type == "player_id") {
                const int player = std::stoi(node.text);
                output.text = client->get_player_alias(player);
                output.color = player == client->get_player_number() ? TextColor::Player : TextColor::OtherPlayer;
            } else if (node.type == "item_id") {
                const int64_t item = std::stoll(node.text);
                output.text = client->get_item_name(item, client->get_player_game(node.player));
                if (node.flags & APClient::ItemFlags::FLAG_ADVANCEMENT) {
                    output.color = TextColor::Progression;
                } else if (node.flags & APClient::ItemFlags::FLAG_NEVER_EXCLUDE) {
                    output.color = TextColor::Useful;
                } else if (node.flags & APClient::ItemFlags::FLAG_TRAP) {
                    output.color = TextColor::Trap;
                } else {
                    output.color = TextColor::Item;
                }
            } else if (node.type == "location_id") {
                const int64_t location = std::stoll(node.text);
                output.text = client->get_location_name(location, client->get_player_game(node.player));
                output.color = TextColor::Location;
            } else if (node.type == "ERROR") {
                output.color = TextColor::Error;
            } else if (node.type == "LOG") {
                output.color = TextColor::Log;
            }
            line.push_back(std::move(output));
        }
        if (!line.empty()) {
            Log(std::move(line));
        }
    });

    client->set_set_reply_handler([this](const nlohmann::json data) {
        if (client == nullptr || !data.contains("key") || !data.contains("value")) {
            return;
        }
        const std::string hintKey = "_read_hints_" + std::to_string(client->get_team_number()) + "_" +
                                    std::to_string(client->get_player_number());
        if (data["key"] == hintKey) {
            UpdateHints(data["value"]);
        }
    });

    client->set_retrieved_handler([this](const std::map<std::string, nlohmann::json>& data) {
        if (client == nullptr) {
            return;
        }
        const std::string hintKey = "_read_hints_" + std::to_string(client->get_team_number()) + "_" +
                                    std::to_string(client->get_player_number());
        if (data.contains(hintKey)) {
            UpdateHints(data.at(hintKey));
        }
    });
}

void Client::Disconnect() {
    client.reset();
    status = ConnectionStatus::Disconnected;
    goalSent = false;
    activeLocations.clear();
    serverCheckedLocations.clear();
    randoOptions.clear();
    while (!receivedItems.empty()) {
        receivedItems.pop();
    }
    hintList.clear();
    scoutedLocations.clear();
    presentedLocations.clear();
    receivedPresentationQueued = false;
    locationsScouted = false;
}

void Client::Poll() {
    if (client != nullptr) {
        client->poll();
    }
    ApplyServerCheckedLocations();
    RepairInvalidInventoryState();
    ProcessItemQueue();
}

bool Client::IsConnected() const {
    return client != nullptr && status == ConnectionStatus::Connected &&
           client->get_state() == APClient::State::SLOT_CONNECTED;
}

bool Client::IsReady() const {
    return IsConnected() && locationsScouted;
}

std::string Client::GetServerAddress() const {
    return CVarGetString(ServerCVar, "");
}

std::string Client::GetSlotName() const {
    return CVarGetString(SlotCVar, "");
}

std::string Client::GetSeedName() const {
    return IsConnected() ? client->get_seed() : "";
}

ConnectionStatus Client::GetStatus() const {
    return status;
}

void Client::SyncCompletedChecks() {
    if (!IsConnected() || !IS_RANDO) {
        return;
    }
    std::list<int64_t> locations;
    for (int check = RC_UNKNOWN + 1; check < RC_MAX; ++check) {
        if (activeLocations.contains(check) && RANDO_SAVE_CHECKS[check].obtained) {
            locations.push_back(LocationIdBase + check);
        }
    }
    if (!locations.empty()) {
        client->LocationChecks(locations);
    }
}

void Client::RecordCheckedLocations(const std::list<int64_t>& locations) {
    for (const int64_t location : locations) {
        const int64_t rawCheckId = location - LocationIdBase;
        if (rawCheckId > RC_UNKNOWN && rawCheckId < RC_MAX && activeLocations.contains(rawCheckId)) {
            serverCheckedLocations.insert(static_cast<RandoCheckId>(rawCheckId));
        }
    }
    ApplyServerCheckedLocations();
}

void Client::ApplyServerCheckedLocations() {
    if (!IS_ARCHIPELAGO) {
        return;
    }
    for (const RandoCheckId checkId : serverCheckedLocations) {
        RandoSaveCheck& saveCheck = RANDO_SAVE_CHECKS[checkId];
        saveCheck.obtained = true;
        saveCheck.cycleObtained = true;
        saveCheck.eligible = false;
    }
}

void Client::RepairInvalidInventoryState() {
    if (!IS_ARCHIPELAGO || CUR_UPG_VALUE(UPG_BOMB_BAG) != 0) {
        return;
    }
    if (INV_CONTENT(ITEM_BOMB) == ITEM_BOMB) {
        INV_CONTENT(ITEM_BOMB) = ITEM_NONE;
    }
    AMMO(ITEM_BOMB) = 0;
}

void Client::PrepareRandomizerSettings() {
    const auto hasType = [this](RandoCheckType type) {
        for (const int32_t checkId : activeLocations) {
            const auto it = Rando::StaticData::Checks.find(static_cast<RandoCheckId>(checkId));
            if (it != Rando::StaticData::Checks.end() && it->second.randoCheckType == type) {
                return true;
            }
        }
        return false;
    };
    const auto setOption = [](RandoOptionId option, bool enabled) {
        CVarSetInteger(Rando::StaticData::Options[option].cvar, enabled ? RO_GENERIC_YES : RO_GENERIC_NO);
    };

    CVarSetInteger("gRando.Enabled", 1);
    CVarSetInteger("gRando.Mode", RANDO_RUN_MODE_ARCHIPELAGO);
    CVarSetInteger("gRando.SpoilerFileIndex", 0);
    if (!randoOptions.empty()) {
        for (const auto& [optionName, optionValue] : randoOptions) {
            const RandoOptionId optionId = Rando::StaticData::GetOptionIdFromName(optionName.c_str());
            if (optionId != RO_MAX) {
                CVarSetInteger(Rando::StaticData::Options[optionId].cvar, optionValue);
            }
        }
    } else {
        CVarSetInteger(Rando::StaticData::Options[RO_LOGIC].cvar, RO_LOGIC_NO_LOGIC);
        setOption(RO_SHUFFLE_BARREL_DROPS, hasType(RCTYPE_BARREL));
        setOption(RO_SHUFFLE_COWS, hasType(RCTYPE_COW));
        setOption(RO_SHUFFLE_CRATE_DROPS, hasType(RCTYPE_CRATE));
        setOption(RO_SHUFFLE_ENEMY_DROPS, hasType(RCTYPE_ENEMY_DROP));
        setOption(RO_SHUFFLE_FREESTANDING_ITEMS, hasType(RCTYPE_FREESTANDING));
        setOption(RO_SHUFFLE_FROGS, hasType(RCTYPE_FROG));
        setOption(RO_SHUFFLE_GOLD_SKULLTULAS, hasType(RCTYPE_SKULL_TOKEN));
        setOption(RO_SHUFFLE_GRASS_DROPS, hasType(RCTYPE_GRASS));
        setOption(RO_SHUFFLE_OWL_STATUES, hasType(RCTYPE_OWL));
        setOption(RO_SHUFFLE_POT_DROPS, hasType(RCTYPE_POT));
        setOption(RO_SHUFFLE_BOSS_REMAINS, hasType(RCTYPE_REMAINS));
        setOption(RO_SHUFFLE_SHOPS, hasType(RCTYPE_SHOP));
        setOption(RO_SHUFFLE_SNOWBALL_DROPS, hasType(RCTYPE_SNOWBALL));
        setOption(RO_SHUFFLE_TINGLE_SHOPS, hasType(RCTYPE_TINGLE_SHOP));
        setOption(RO_SHUFFLE_TREE_DROPS, hasType(RCTYPE_TREE));
    }
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

bool Client::InitializeSave() {
    if (!IsReady()) {
        return false;
    }

    while (!receivedItems.empty()) {
        receivedItems.pop();
    }
    CVarSetInteger(LastItemCVar, 0);
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    client->Sync();

    for (const auto& [optionId, option] : Rando::StaticData::Options) {
        RANDO_SAVE_OPTIONS[optionId] = static_cast<uint32_t>(CVarGetInteger(option.cvar, option.defaultValue));
    }
    RANDO_SAVE_OPTIONS[RO_LOGIC] = RO_LOGIC_NO_LOGIC;
    gSaveContext.save.saveInfo.playerData.healthCapacity = gSaveContext.save.saveInfo.playerData.health =
        static_cast<int16_t>(RANDO_SAVE_OPTIONS[RO_STARTING_HEALTH] * 0x10);
    gSaveContext.save.shipSaveInfo.rando.finalSeed =
        static_cast<uint32_t>(std::hash<std::string>{}(client->get_seed()));
    snprintf(gSaveContext.save.shipSaveInfo.rando.archipelagoServer,
             sizeof(gSaveContext.save.shipSaveInfo.rando.archipelagoServer), "%s", GetServerAddress().c_str());
    snprintf(gSaveContext.save.shipSaveInfo.rando.archipelagoSlot,
             sizeof(gSaveContext.save.shipSaveInfo.rando.archipelagoSlot), "%s", GetSlotName().c_str());
    snprintf(gSaveContext.save.shipSaveInfo.rando.archipelagoSeed,
             sizeof(gSaveContext.save.shipSaveInfo.rando.archipelagoSeed), "%s", GetSeedName().c_str());

    for (const auto& [checkId, check] : Rando::StaticData::Checks) {
        RandoSaveCheck& saveCheck = RANDO_SAVE_CHECKS[checkId];
        saveCheck.shuffled = activeLocations.contains(checkId);
        if (!saveCheck.shuffled) {
            continue;
        }

        const ScoutedLocation* location = GetScoutedLocation(checkId);
        saveCheck.randoItemId =
            location != nullptr && location->isLocal && location->localItemId != RI_UNKNOWN ? location->localItemId :
                                                                                              RI_JUNK;
    }

    RANDO_SAVE_CHECKS[RC_STARTING_ITEM_DEKU_MASK].eligible =
        activeLocations.contains(RC_STARTING_ITEM_DEKU_MASK);
    RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].eligible =
        activeLocations.contains(RC_STARTING_ITEM_SONG_OF_HEALING);
    RANDO_SAVE_CHECKS[RC_STARTING_ITEM_DEKU_MASK].shuffled =
        activeLocations.contains(RC_STARTING_ITEM_DEKU_MASK);
    RANDO_SAVE_CHECKS[RC_STARTING_ITEM_SONG_OF_HEALING].shuffled =
        activeLocations.contains(RC_STARTING_ITEM_SONG_OF_HEALING);
    ApplyServerCheckedLocations();
    return true;
}

bool Client::CheckLocation(RandoCheckId checkId) {
    if (IS_ARCHIPELAGO && IsConnected() && activeLocations.contains(checkId)) {
        client->LocationChecks({ LocationIdBase + checkId });
        return true;
    }
    return false;
}

const ScoutedLocation* Client::GetScoutedLocation(RandoCheckId checkId) const {
    if (!IS_ARCHIPELAGO) {
        return nullptr;
    }
    const auto it = scoutedLocations.find(checkId);
    return it == scoutedLocations.end() ? nullptr : &it->second;
}

bool Client::IsImportantLocation(RandoCheckId checkId) const {
    const ScoutedLocation* location = GetScoutedLocation(checkId);
    return location != nullptr && (location->flags & 0x3);
}

void Client::NotifyLocationSent(RandoCheckId checkId) {
    const ScoutedLocation* location = GetScoutedLocation(checkId);
    if (location == nullptr) {
        return;
    }
    EnsureTexturesLoaded();
    Notification::Emit({
        .itemIcon = JunkIcon,
        .prefix = location->itemName,
        .prefixColor = GetTextColor(TextColor::Item),
        .message = location->isLocal ? "found for" : "sent to",
        .suffix = location->playerName,
        .suffixColor = GetTextColor(location->isLocal ? TextColor::Player : TextColor::OtherPlayer),
    });
}

void Client::MarkLocationPresentation(RandoCheckId checkId) {
    const ScoutedLocation* location = GetScoutedLocation(checkId);
    if (location != nullptr && location->isLocal) {
        presentedLocations.insert(checkId);
    }
}

void Client::ProcessItemQueue() {
    if (!IsConnected() || !IS_ARCHIPELAGO || gPlayState == nullptr || receivedItems.empty() ||
        receivedPresentationQueued) {
        return;
    }
    const ReceivedItem item = receivedItems.front();
    receivedItems.pop();
    EnsureTexturesLoaded();

    const int64_t rawCheckId = item.location - LocationIdBase;
    if (rawCheckId > RC_UNKNOWN && rawCheckId < RC_MAX) {
        const RandoCheckId checkId = static_cast<RandoCheckId>(rawCheckId);
        if (presentedLocations.erase(checkId) > 0) {
            Rando::GiveItem(item.itemId);
            CVarSetInteger(LastItemCVar, static_cast<int32_t>(item.index + 1));
            Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
            return;
        }
    }

    if (item.flags & 0x3) {
        QueueReceivedItemPresentation(item);
        return;
    }

    Rando::GiveItem(item.itemId);
    CVarSetInteger(LastItemCVar, static_cast<int32_t>(item.index + 1));
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    const char* icon = JunkIcon;
    ImVec4 itemColor(0.0f, 0.93f, 0.93f, 1.0f);
    if (item.flags & APClient::ItemFlags::FLAG_ADVANCEMENT) {
        icon = ProgressiveIcon;
        itemColor = GetTextColor(TextColor::Progression);
    } else if (item.flags & APClient::ItemFlags::FLAG_NEVER_EXCLUDE) {
        icon = UsefulIcon;
        itemColor = GetTextColor(TextColor::Useful);
    } else if (item.flags & APClient::ItemFlags::FLAG_TRAP) {
        itemColor = GetTextColor(TextColor::Trap);
    }
    Notification::Emit({ .itemIcon = icon,
                         .prefix = Rando::StaticData::GetItemName(item.itemId, false),
                         .prefixColor = itemColor,
                         .message = "received from",
                         .suffix = item.sender,
                         .suffixColor = GetTextColor(TextColor::OtherPlayer) });
}

void Client::QueueReceivedItemPresentation(const ReceivedItem& item) {
    receivedPresentationQueued = true;
    pendingReceivedPresentation = item;
    GameInteractor::Instance->events.emplace_back(GIEventGiveItem{
        .showGetItemCutscene = true,
        .param = static_cast<int16_t>(item.itemId),
        .giveItem = GiveReceivedItemPresentation,
        .drawItem = DrawReceivedItemPresentation,
    });
}

void Client::GiveReceivedItemPresentation(Actor*, PlayState*) {
    Client& self = Instance();
    const ReceivedItem& item = self.pendingReceivedPresentation;
    const std::string itemName = Rando::StaticData::GetItemName(item.itemId, true);
    const bool fromSelf = self.client != nullptr && item.sender == self.client->get_slot();
    const std::string progress = item.itemId == RI_TRIFORCE_PIECE ? GetTriforceProgressMessage() : "";
    CustomMessage::Entry entry = {
        .textboxType = 2,
        .icon = Rando::StaticData::GetIconForZMessage(item.itemId),
        .msg = (fromSelf ? "You received " + itemName + "!" :
                           "You received " + itemName + " from " + item.sender + "!") +
               progress,
    };
    CustomMessage::SetActiveCustomMessage(entry.msg, entry);
    Rando::GiveItem(item.itemId);
    CVarSetInteger(LastItemCVar, static_cast<int32_t>(item.index + 1));
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    self.receivedPresentationQueued = false;
}

void Client::DrawReceivedItemPresentation(Actor* actor, PlayState*) {
    Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
    Rando::DrawItem(static_cast<RandoItemId>(CUSTOM_ITEM_PARAM), RC_UNKNOWN, actor);
}

void Client::SendGoal() {
    if (IS_ARCHIPELAGO && IsConnected() && !goalSent) {
        client->StatusUpdate(APClient::ClientStatus::GOAL);
        goalSent = true;
        Log("Goal completed.");
    }
}

void Client::Log(const std::string& message) {
    logMessage = message;
    TextColor color = TextColor::Default;
    if (message.find("[ERROR]") != std::string::npos || message.find("error") != std::string::npos) {
        color = TextColor::Error;
    } else if (message.find("[LOG]") != std::string::npos) {
        color = TextColor::Log;
    }
    Log({ { message, color } });
    SPDLOG_INFO("[Archipelago] {}", message);
}

void Client::Log(ConsoleLine line) {
    consoleLines.push_back(std::move(line));
    while (consoleLines.size() > 100) {
        consoleLines.pop_front();
    }
}

void Client::EnsureTexturesLoaded() {
    static bool loaded = false;
    if (loaded) {
        return;
    }
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    gui->LoadTextureFromRawImage(ProgressiveIcon, "textures/parameter_static/gArchipelagoProgressive.png");
    gui->LoadTextureFromRawImage(UsefulIcon, "textures/parameter_static/gArchipelagoUseful.png");
    gui->LoadTextureFromRawImage(JunkIcon, "textures/parameter_static/gArchipelagoJunk.png");
    gui->LoadTextureFromRawImage(ConnectionIcon, "textures/parameter_static/gArchipelagoConnection.png");
    loaded = true;
}

void Client::DrawConnectionStatus() {
    if (!IS_ARCHIPELAGO || gPlayState == nullptr) {
        return;
    }

    EnsureTexturesLoaded();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 6.0f,
                                  viewport->Pos.y + viewport->Size.y - 6.0f),
                            ImGuiCond_Always, ImVec2(1.0f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.62f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 0.0f));

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("Archipelago Connection Status", nullptr, flags)) {
        ImGui::Image(Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(ConnectionIcon),
                     ImVec2(20.0f, 20.0f));
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();

        const char* label = "Disconnected";
        ImVec4 color(0.75f, 0.75f, 0.75f, 1.0f);
        if (IsReady()) {
            label = "Connected";
            color = ImVec4(0.45f, 1.0f, 0.55f, 1.0f);
        } else if (status == ConnectionStatus::Connected) {
            label = "Synchronizing...";
            color = ImVec4(1.0f, 0.85f, 0.35f, 1.0f);
        } else if (status == ConnectionStatus::Connecting) {
            label = "Connecting...";
            color = ImVec4(1.0f, 0.85f, 0.35f, 1.0f);
        } else if (status == ConnectionStatus::Error) {
            label = "Connection error";
            color = ImVec4(1.0f, 0.45f, 0.45f, 1.0f);
        }
        ImGui::TextColored(color, "%s", label);
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

ImVec4 Client::GetTextColor(TextColor color) const {
    switch (color) {
        case TextColor::Error: return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        case TextColor::Log: return ImVec4(0.7f, 0.7f, 1.0f, 1.0f);
        case TextColor::Player: return ImVec4(0.93f, 0.0f, 0.93f, 1.0f);
        case TextColor::OtherPlayer: return ImVec4(0.98f, 0.98f, 0.82f, 1.0f);
        case TextColor::Progression: return ImVec4(0.69f, 0.60f, 0.94f, 1.0f);
        case TextColor::Useful: return ImVec4(0.43f, 0.55f, 0.91f, 1.0f);
        case TextColor::Trap: return ImVec4(0.98f, 0.50f, 0.45f, 1.0f);
        case TextColor::Item: return ImVec4(0.0f, 0.93f, 0.93f, 1.0f);
        case TextColor::Location: return ImVec4(0.39f, 0.58f, 0.93f, 1.0f);
        default: return ImVec4(0.93f, 0.93f, 0.93f, 1.0f);
    }
}

void Client::DrawMenu() {
    ImGui::SeparatorText("Connection info");
    ImGui::TextUnformatted("Server Address");
    UIWidgets::CVarInputString("##APServer", ServerCVar,
                               UIWidgets::InputOptions().DefaultValue("archipelago.gg:38281")
                                   .PlaceholderText("archipelago.gg:38281")
                                   .Size(ImVec2(ImGui::GetFontSize() * 18.0f, 0.0f)));
    ImGui::TextUnformatted("Slot Name");
    UIWidgets::CVarInputString("##APSlot", SlotCVar,
                               UIWidgets::InputOptions().Size(ImVec2(ImGui::GetFontSize() * 18.0f, 0.0f)));
    ImGui::TextUnformatted("Password (leave blank for no password)");
    UIWidgets::CVarInputString("##APPassword", PasswordCVar,
                               UIWidgets::InputOptions().IsSecret(true)
                                   .Size(ImVec2(ImGui::GetFontSize() * 18.0f, 0.0f)));

    if (IsConnected()) {
        if (UIWidgets::Button("Disconnect", UIWidgets::ButtonOptions())) {
            Disconnect();
        }
    } else if (UIWidgets::Button("Connect", UIWidgets::ButtonOptions().Color(UIWidgets::Colors::LightBlue))) {
        Connect();
    }

    ImGui::SameLine();
    const char* statusText = IsReady() ? "Ready" :
                             status == ConnectionStatus::Connected ? "Synchronizing" :
                             status == ConnectionStatus::Connecting ? "Connecting" :
                             status == ConnectionStatus::Error ? "Error" : "Disconnected";
    const ImVec4 statusColor = IsReady() ? ImVec4(0.5f, 1.0f, 0.5f, 1.0f) :
                               status == ConnectionStatus::Connected ? ImVec4(1.0f, 0.85f, 0.35f, 1.0f) :
                               status == ConnectionStatus::Connecting ? ImVec4(0.7f, 0.7f, 0.7f, 1.0f) :
                               ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
    ImGui::TextColored(statusColor, "%s", statusText);
    if (!logMessage.empty()) {
        ImGui::SeparatorText("Latest message");
        ImGui::TextWrapped("%s", logMessage.c_str());
    }
}

void Client::SendMessage(const std::string& message) {
    if (message.empty()) {
        return;
    }
    if (client == nullptr) {
        Log("[ERROR] Connect to an Archipelago slot before sending messages.");
        return;
    }
    if (message.starts_with("/")) {
        Log("Local slash commands are unavailable. Use !help for server commands.");
        return;
    }
    client->Say(message);
}

void Client::DrawConsole() {
    static bool autoScroll = true;
    static char input[1024] = {};

    ImGui::Checkbox("Auto-scroll", &autoScroll);
    ImGui::Separator();
    const float inputHeight = ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("APConsoleLog", ImVec2(0.0f, -inputHeight), true,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        for (const ConsoleLine& line : consoleLines) {
            bool first = true;
            for (const ColoredTextNode& node : line) {
                if (!first) {
                    ImGui::SameLine(0.0f, 0.0f);
                }
                ImGui::TextColored(GetTextColor(node.color), "%s", node.text.c_str());
                first = false;
            }
        }
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    ImGui::SetNextItemWidth(-70.0f);
    const bool send = ImGui::InputText("##APChat", input, sizeof(input), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (send || UIWidgets::Button("Send", UIWidgets::ButtonOptions())) {
        SendMessage(input);
        input[0] = '\0';
    }
}

void Client::UpdateHints(const nlohmann::json& hints) {
    if (client == nullptr || !hints.is_array()) {
        return;
    }
    hintList.clear();
    for (const nlohmann::json& data : hints) {
        if (!data.contains("receiving_player") || !data.contains("finding_player") || !data.contains("location") ||
            !data.contains("item")) {
            continue;
        }
        Hint hint;
        hint.receivingPlayer = data["receiving_player"];
        hint.findingPlayer = data["finding_player"];
        hint.location = data["location"];
        hint.receivingPlayerName = client->get_player_alias(hint.receivingPlayer);
        hint.findingPlayerName = client->get_player_alias(hint.findingPlayer);
        hint.itemName = client->get_item_name(data["item"], client->get_player_game(hint.receivingPlayer));
        hint.locationName = client->get_location_name(hint.location, client->get_player_game(hint.findingPlayer));
        hint.itemFlags = data.value("item_flags", 0);
        hint.found = data.value("found", false);
        switch (data.value("status", static_cast<int>(APClient::HINT_UNSPECIFIED))) {
            case APClient::HINT_NO_PRIORITY: hint.status = HintStatus::NoPriority; break;
            case APClient::HINT_AVOID: hint.status = HintStatus::Avoid; break;
            case APClient::HINT_PRIORITY: hint.status = HintStatus::Priority; break;
            case APClient::HINT_FOUND: hint.status = HintStatus::Found; break;
            default: hint.status = HintStatus::Unspecified; break;
        }
        hintList.push_back(std::move(hint));
    }
}

void Client::UpdateHintStatus(int player, int64_t location, HintStatus statusValue) {
    if (!IsConnected()) {
        return;
    }
    APClient::HintStatus status = APClient::HINT_UNSPECIFIED;
    switch (statusValue) {
        case HintStatus::NoPriority: status = APClient::HINT_NO_PRIORITY; break;
        case HintStatus::Avoid: status = APClient::HINT_AVOID; break;
        case HintStatus::Priority: status = APClient::HINT_PRIORITY; break;
        case HintStatus::Found: status = APClient::HINT_FOUND; break;
        default: break;
    }
    client->UpdateHint(player, location, status);
}

void Client::DrawHints() {
    static const char* statusNames[] = { "Unspecified", "No Priority", "Avoid", "Priority", "Found" };
    static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                                   ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersV |
                                   ImGuiTableFlags_ScrollY;

    const float footerHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f;
    if (ImGui::BeginTable("APHints", 5, flags, ImVec2(0.0f, -footerHeight))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Receiving Player");
        ImGui::TableSetupColumn("Item");
        ImGui::TableSetupColumn("Finding Player");
        ImGui::TableSetupColumn("Location");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();

        for (size_t index = 0; index < hintList.size(); ++index) {
            Hint& hint = hintList[index];
            ImGui::PushID(static_cast<int>(index));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(GetTextColor(hint.receivingPlayer == client->get_player_number() ? TextColor::Player :
                                                                                               TextColor::OtherPlayer),
                               "%s", hint.receivingPlayerName.c_str());
            ImGui::TableNextColumn();
            TextColor itemColor = TextColor::Item;
            if (hint.itemFlags & APClient::ItemFlags::FLAG_ADVANCEMENT) itemColor = TextColor::Progression;
            else if (hint.itemFlags & APClient::ItemFlags::FLAG_NEVER_EXCLUDE) itemColor = TextColor::Useful;
            else if (hint.itemFlags & APClient::ItemFlags::FLAG_TRAP) itemColor = TextColor::Trap;
            ImGui::PushStyleColor(ImGuiCol_Text, GetTextColor(itemColor));
            ImGui::TextWrapped("%s", hint.itemName.c_str());
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            ImGui::TextColored(GetTextColor(hint.findingPlayer == client->get_player_number() ? TextColor::Player :
                                                                                             TextColor::OtherPlayer),
                               "%s", hint.findingPlayerName.c_str());
            ImGui::TableNextColumn();
            ImGui::TextColored(GetTextColor(TextColor::Location), "%s", hint.locationName.c_str());
            ImGui::TableNextColumn();
            int selected = static_cast<int>(hint.status);
            if (hint.found || hint.receivingPlayer != client->get_player_number()) {
                ImGui::TextUnformatted(statusNames[selected]);
            } else if (ImGui::Combo("##status", &selected, statusNames, IM_ARRAYSIZE(statusNames))) {
                hint.status = static_cast<HintStatus>(selected);
                UpdateHintStatus(hint.findingPlayer, hint.location, hint.status);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    static char hintItem[128] = {};
    ImGui::SetNextItemWidth(-90.0f);
    const bool submit = ImGui::InputTextWithHint("##APHintItem", "Item name", hintItem, sizeof(hintItem),
                                                ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (submit || UIWidgets::Button("Hint", UIWidgets::ButtonOptions())) {
        if (hintItem[0] != '\0') {
            SendMessage("!hint '" + std::string(hintItem) + "'");
            hintItem[0] = '\0';
        }
    }
    ImGui::SameLine();
    if (client != nullptr) {
        ImGui::Text("Points: %d / Cost: %d", client->get_hint_points(), client->get_hint_cost_points());
    }
}

void ConsoleWindow::DrawElement() {
    Client::Instance().DrawConsole();
}

void HintWindow::DrawElement() {
    Client::Instance().DrawHints();
}

void StatusWindow::Draw() {
    Client::Instance().DrawConnectionStatus();
}

void Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>([]() { Client::Instance().Poll(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameCompletion>([]() { Client::Instance().SendGoal(); });
}

} // namespace Archipelago

#else

namespace Archipelago {
Client& Client::Instance() { static Client instance; return instance; }
bool Client::Connect() { return false; }
void Client::Disconnect() {}
void Client::Poll() {}
bool Client::IsConnected() const { return false; }
bool Client::IsReady() const { return false; }
std::string Client::GetServerAddress() const { return ""; }
std::string Client::GetSlotName() const { return ""; }
std::string Client::GetSeedName() const { return ""; }
ConnectionStatus Client::GetStatus() const { return ConnectionStatus::Disconnected; }
bool Client::CheckLocation(RandoCheckId) { return false; }
bool Client::InitializeSave() { return false; }
const ScoutedLocation* Client::GetScoutedLocation(RandoCheckId) const { return nullptr; }
bool Client::IsImportantLocation(RandoCheckId) const { return false; }
void Client::NotifyLocationSent(RandoCheckId) {}
void Client::MarkLocationPresentation(RandoCheckId) {}
void Client::DrawConnectionStatus() {}
void Client::SendGoal() {}
void Client::DrawMenu() {}
void Client::DrawConsole() {}
void Client::DrawHints() {}
void Client::SendMessage(const std::string&) {}
void ConsoleWindow::DrawElement() {}
void HintWindow::DrawElement() {}
void StatusWindow::Draw() {}
void Init() {}
} // namespace Archipelago

#endif
