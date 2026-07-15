#pragma once

#include <cstdint>
#include <deque>
#include <list>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <ship/window/gui/GuiWindow.h>

#include "2s2h/Rando/Types.h"

class APClient;
struct Actor;
struct PlayState;

namespace Archipelago {

constexpr const char* GameName = "2 Ship 2 Harkinian";
constexpr int64_t ItemIdBase = 74200000;
constexpr int64_t LocationIdBase = 74210000;

enum class ConnectionStatus : uint8_t {
    Disconnected,
    Connecting,
    Connected,
    Error,
};

enum class TextColor : uint8_t {
    Default,
    Error,
    Log,
    Player,
    OtherPlayer,
    Progression,
    Useful,
    Trap,
    Item,
    Location,
};

struct ColoredTextNode {
    std::string text;
    TextColor color = TextColor::Default;
};

using ConsoleLine = std::vector<ColoredTextNode>;

enum class HintStatus : uint8_t {
    Unspecified,
    NoPriority,
    Avoid,
    Priority,
    Found,
};

struct Hint {
    int receivingPlayer = 0;
    int findingPlayer = 0;
    int64_t location = 0;
    std::string receivingPlayerName;
    std::string findingPlayerName;
    std::string itemName;
    std::string locationName;
    unsigned int itemFlags = 0;
    HintStatus status = HintStatus::Unspecified;
    bool found = false;
};

struct ScoutedLocation {
    RandoCheckId checkId = RC_UNKNOWN;
    RandoItemId localItemId = RI_UNKNOWN;
    int receiver = 0;
    std::string itemName;
    std::string playerName;
    unsigned int flags = 0;
    bool isLocal = false;
};

class Client {
  public:
    static Client& Instance();

    bool Connect();
    void Disconnect();
    void Poll();
    bool IsConnected() const;
    bool IsReady() const;
    std::string GetServerAddress() const;
    std::string GetSlotName() const;
    std::string GetSeedName() const;
    ConnectionStatus GetStatus() const;

    bool CheckLocation(RandoCheckId checkId);
    void SendGoal();
    void DrawMenu();
    void DrawConsole();
    void DrawHints();
    void SendMessage(const std::string& message);
    const ScoutedLocation* GetScoutedLocation(RandoCheckId checkId) const;
    bool IsImportantLocation(RandoCheckId checkId) const;
    void NotifyLocationSent(RandoCheckId checkId);
    void MarkLocationPresentation(RandoCheckId checkId);
    void DrawConnectionStatus();
    bool InitializeSave();

  private:
    struct ReceivedItem {
        RandoItemId itemId;
        int index;
        std::string sender;
        unsigned int flags;
        int64_t location;
    };

    Client() = default;
    void ConfigureHandlers();
    void SyncCompletedChecks();
    void RecordCheckedLocations(const std::list<int64_t>& locations);
    void ApplyServerCheckedLocations();
    void RepairInvalidInventoryState();
    void PrepareRandomizerSettings();
    void ProcessItemQueue();
    void QueueReceivedItemPresentation(const ReceivedItem& item);
    static void GiveReceivedItemPresentation(Actor* actor, PlayState* play);
    static void DrawReceivedItemPresentation(Actor* actor, PlayState* play);
    void Log(const std::string& message);
    void Log(ConsoleLine line);
    void EnsureTexturesLoaded();
    ImVec4 GetTextColor(TextColor color) const;
    void UpdateHints(const nlohmann::json& hints);
    void UpdateHintStatus(int player, int64_t location, HintStatus status);

    std::unique_ptr<APClient> client;
    std::queue<ReceivedItem> receivedItems;
    std::unordered_set<int32_t> activeLocations;
    std::unordered_set<RandoCheckId> serverCheckedLocations;
    std::unordered_map<std::string, int32_t> randoOptions;
    ConnectionStatus status = ConnectionStatus::Disconnected;
    bool goalSent = false;
    std::string logMessage;
    std::deque<ConsoleLine> consoleLines;
    std::vector<Hint> hintList;
    std::unordered_map<RandoCheckId, ScoutedLocation> scoutedLocations;
    std::unordered_set<RandoCheckId> presentedLocations;
    ReceivedItem pendingReceivedPresentation{};
    bool receivedPresentationQueued = false;
    bool locationsScouted = false;
};

class ConsoleWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override {};
    void DrawElement() override;
    void UpdateElement() override {};
};

class HintWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override {};
    void DrawElement() override;
    void UpdateElement() override {};
};

class StatusWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override {}
    void DrawElement() override {}
    void Draw() override;
    void UpdateElement() override {}
};

void Init();

} // namespace Archipelago
