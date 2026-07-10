#include "MiscBehavior.h"
#include "Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h_assets.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "2s2h/BenGui/Notification.h"
#include "2s2h/Network/Archipelago/Archipelago.h"

extern "C" {
#include "z64save.h"
#include "functions.h"
#include "macros.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"
#include "misc/title_static/title_static.h"
#include "gfxprint.h"
extern s16 sWindowContentColors[3];
extern FileSelectState* gFileSelectState;
}

#define dgFileSelArchiButtonTex "__OTR__textures/title_static/gFileSelArchiButtonTex"
static const ALIGN_ASSET(2) char gFileSelArchiButtonTex[] = dgFileSelArchiButtonTex;

typedef struct {
    char tex[512];
    uint16_t width;
    uint16_t height;
    uint8_t im_fmt;
    uint8_t im_siz;
    uint8_t id;
} Sprite;

// Image Icons

#include <array>
#include "assets/archives/icon_item_static/icon_item_static_yar.h"
#include "assets/archives/icon_item_24_static/icon_item_24_static_yar.h"

inline std::array<Sprite, 100> gSeedTextures = { {
    { dgItemIconOcarinaOfTimeTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 0 },
    { dgItemIconBowTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 1 },
    { dgItemIconFireArrowTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 2 },
    { dgItemIconIceArrowTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 3 },
    { dgItemIconLightArrowTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 4 },
    { dgItemIconFairyOcarinaTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 5 },
    { dgItemIconBombTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 6 },
    { dgItemIconBombchuTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 7 },
    { dgItemIconDekuStickTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 8 },
    { dgItemIconDekuNutTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 9 },
    { dgItemIconMagicBeansTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 10 },
    { dgItemIconSlingshotTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 11 },
    { dgItemIconPowderKegTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 12 },
    { dgItemIconPictographBoxTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 13 },
    { dgItemIconLensofTruthTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 14 },
    { dgItemIconHookshotTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 15 },
    { dgItemIconGreatFairysSwordTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16 },
    { dgItemIconLongshotTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 17 },
    { dgItemIconEmptyBottleTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 18 },
    { dgItemIconRedPotionTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 19 },
    { dgItemIconGreenPotionTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 20 },
    { dgItemIconBluePotionTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 21 },
    { dgItemIconBottledFairyTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 22 },
    { dgItemIconBottledDekuPrincessTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 23 },
    { dgItemIconBottledFullMilkTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 24 },
    { dgItemIconBottledHalfMilkTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 25 },
    { dgItemIconBottledFishTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 26 },
    { dgItemIconBottledBugTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 27 },
    { dgItemIconBottledBlueFireTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 28 },
    { dgItemIconBottledPoeTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 29 },
    { dgItemIconBottledBigPoeTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 30 },
    { dgItemIconSpringWaterTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 31 },
    { dgItemIconHotSpringWaterTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32 },
    { dgItemIconBottledZoraEggTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 33 },
    { dgItemIconBottledGoldDustTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 34 },
    { dgItemIconBottledMushroomTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 35 },
    { dgItemIconBottledSeahorseTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 36 },
    { dgItemIconChateauRomaniTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 37 },
    { dgItemIconBottledHylianLoachTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 38 },
    { dgItemIconEmptyBottle2Tex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 39 },
    { dgItemIconLandDeedTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 40 },
    { dgItemIconMoonsTearTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 41 },
    { dgItemIconSwampDeedTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 42 },
    { dgItemIconMountainDeedTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 43 },
    { dgItemIconOceanDeedTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 44 },
    { dgItemIconRoomKeyTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 45 },
    { dgItemIconLetterToMamaTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 46 },
    { dgItemIconLetterToKafeiTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 47 },
    { dgItemIconPendantOfMemoriesTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 48 },
    { dgItemIconTingleMapTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 49 },
    { dgItemIconDekuMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 50 },
    { dgItemIconGoronMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 51 },
    { dgItemIconZoraMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 52 },
    { dgItemIconFierceDeityMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 53 },
    { dgItemIconMaskOfTruthTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 54 },
    { dgItemIconKafeisMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 55 },
    { dgItemIconAllNightMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 56 },
    { dgItemIconBunnyHoodTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 57 },
    { dgItemIconKeatonMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 58 },
    { dgItemIconGaroMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 59 },
    { dgItemIconRomaniMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 60 },
    { dgItemIconCircusLeaderMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 61 },
    { dgItemIconPostmansHatTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 62 },
    { dgItemIconCouplesMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 63 },
    { dgItemIconGreatFairyMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 64 },
    { dgItemIconGibdoMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 65 },
    { dgItemIconDonGeroMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 66 },
    { dgItemIconKamaroMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 67 },
    { dgItemIconCaptainsHatTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 68 },
    { dgItemIconStoneMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 69 },
    { dgItemIconBremenMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 70 },
    { dgItemIconBlastMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 71 },
    { dgItemIconMaskOfScentsTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 72 },
    { dgItemIconGiantsMaskTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 73 },
    { dgItemIconBowFireTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 74 },
    { dgItemIconBowIceTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 75 },
    { dgItemIconBowLightTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 76 },
    { dgItemIconKokiriSwordTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 77 },
    { dgItemIconRazorSwordTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 78 },
    { dgItemIconGildedSwordTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 79 },
    { dgItemIconFierceDeitySwordTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 80 },
    { dgItemIconHerosShieldTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 81 },
    { dgItemIconMirrorShieldTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 82 },
    { dgItemIconQuiver30Tex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 83 },
    { dgItemIconQuiver40Tex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 84 },
    { dgItemIconQuiver50Tex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 85 },
    { dgItemIconBombBag20Tex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 86 },
    { dgItemIconBombBag30Tex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 87 },
    { dgItemIconBombBag40Tex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 88 },
    { dgItemIconDefaultWalletTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 89 },
    { dgItemIconAdultsWalletTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 90 },
    { dgItemIconGiantsWalletTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 91 },
    { dgItemIconFishingRodTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 92 },
    { dgItemIconOdolwasRemainsTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 93 },
    { dgItemIconGohtsRemainsTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 94 },
    { dgItemIconGyorgsRemainsTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 95 },
    { dgItemIconTwinmoldsRemainsTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 96 },
    { dgItemIconBombersNotebookTex, 32, 32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 97 },
    { dgQuestIconBossKeyTex, 24, 24, G_IM_FMT_RGBA, G_IM_SIZ_32b, 98 },
    { dgQuestIconSmallKeyTex, 24, 24, G_IM_FMT_RGBA, G_IM_SIZ_32b, 99 },
} };

Sprite* GetSeedTexture(const uint8_t index) {
    return &gSeedTextures[index];
}

u8 isRando[FILE_NUM_MAX_WITH_OWL_SAVE];
u8 isArchipelago[FILE_NUM_MAX_WITH_OWL_SAVE];
u32 seedHashes[FILE_NUM_MAX_WITH_OWL_SAVE];
char archipelagoServers[FILE_NUM_MAX_WITH_OWL_SAVE][128];
char archipelagoSlots[FILE_NUM_MAX_WITH_OWL_SAVE][64];
char archipelagoSeeds[FILE_NUM_MAX_WITH_OWL_SAVE][64];

static bool IsArchipelagoCreationMode() {
    return CVarGetInteger("gRando.Enabled", 0) &&
           CVarGetInteger("gRando.Mode", RANDO_RUN_MODE_LOCAL) == RANDO_RUN_MODE_ARCHIPELAGO;
}

static bool CurrentConnectionMatches(int fileIndex) {
    Archipelago::Client& client = Archipelago::Client::Instance();
    return client.IsReady() && client.GetServerAddress() == archipelagoServers[fileIndex] &&
           client.GetSlotName() == archipelagoSlots[fileIndex] && client.GetSeedName() == archipelagoSeeds[fileIndex];
}

static void EmitArchipelagoFileSelectError(const char* message) {
    Notification::Emit({
        .prefix = "Archipelago:",
        .prefixColor = ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
        .message = message,
    });
}

extern "C" bool Archipelago_CanCreateFile() {
    if (!IsArchipelagoCreationMode()) {
        return true;
    }
    Archipelago::Client& client = Archipelago::Client::Instance();
    if (!client.IsReady()) {
        if (client.GetStatus() == Archipelago::ConnectionStatus::Disconnected ||
            client.GetStatus() == Archipelago::ConnectionStatus::Error) {
            client.Connect();
        }
        EmitArchipelagoFileSelectError("connecting; wait for synchronization before starting this file.");
        return false;
    }
    return true;
}

extern "C" bool Archipelago_CanOpenFile(s32 fileIndex) {
    if (fileIndex < 0 || fileIndex >= FILE_NUM_MAX_WITH_OWL_SAVE || !isArchipelago[fileIndex]) {
        return true;
    }
    Archipelago::Client& client = Archipelago::Client::Instance();
    if (client.GetServerAddress() != archipelagoServers[fileIndex] ||
        client.GetSlotName() != archipelagoSlots[fileIndex]) {
        EmitArchipelagoFileSelectError("this save belongs to a different server or slot.");
        return false;
    }
    if (!client.IsReady()) {
        if (client.GetStatus() == Archipelago::ConnectionStatus::Disconnected ||
            client.GetStatus() == Archipelago::ConnectionStatus::Error) {
            client.Connect();
        }
        EmitArchipelagoFileSelectError("connecting; wait until Start Archipelago is ready.");
        return false;
    }
    if (client.GetSeedName() != archipelagoSeeds[fileIndex]) {
        EmitArchipelagoFileSelectError("this slot is connected to a different seed.");
        return false;
    }
    return true;
}

extern "C" void Archipelago_DrawFileSelectInfo() {
    if (gFileSelectState == nullptr ||
        (gFileSelectState->configMode != CM_MAIN_MENU && gFileSelectState->menuMode != FS_MENU_MODE_SELECT)) {
        return;
    }

    int fileIndex = gFileSelectState->selectedFileIndex;
    const bool selectedArchipelago = fileIndex >= 0 && fileIndex < FILE_NUM_MAX_WITH_OWL_SAVE &&
                                     isArchipelago[fileIndex] &&
                                     gFileSelectState->menuMode == FS_MENU_MODE_SELECT;
    if (!selectedArchipelago && !IsArchipelagoCreationMode()) {
        return;
    }

    Archipelago::Client& client = Archipelago::Client::Instance();
    const std::string currentServer = client.GetServerAddress();
    const std::string currentSlot = client.GetSlotName();
    const char* server = selectedArchipelago ? archipelagoServers[fileIndex] : currentServer.c_str();
    const char* slot = selectedArchipelago ? archipelagoSlots[fileIndex] : currentSlot.c_str();
    const bool ready = selectedArchipelago ? CurrentConnectionMatches(fileIndex) : client.IsReady();
    const char* status = ready ? "Start Archipelago" :
                         client.IsConnected() ? "Archipelago: synchronizing" : "Archipelago: not connected";

    OPEN_DISPS(gFileSelectState->state.gfxCtx);
    OPEN_PRINTER(POLY_OPA_DISP);
    GfxPrint_SetColor(&printer, ready ? 120 : 255, ready ? 255 : 120, 120, 255);
    GfxPrint_SetPos(&printer, 8, 3);
    GfxPrint_Printf(&printer, status);
    GfxPrint_SetColor(&printer, 220, 220, 220, 255);
    GfxPrint_SetPos(&printer, 8, 4);
    GfxPrint_Printf(&printer, "Server: %.23s", server);
    GfxPrint_SetPos(&printer, 8, 5);
    GfxPrint_Printf(&printer, "Slot: %.25s", slot);
    CLOSE_PRINTER(printer, POLY_OPA_DISP);
    CLOSE_DISPS(gFileSelectState->state.gfxCtx);
}

// 5 rectangles per save file:
// Rand Left Aligned
// Rand Left Aligned (shadow offset)
// Rand Center Aligned
// Rand Center Aligned (shadow offset)
// Owl
Vtx sRandVtxData[20 * FILE_NUM_MAX];

constexpr s16 RAND_ICON_HEIGHT = 16;
constexpr s16 RAND_ICON_WIDTH = 32;
constexpr s16 ARCHIPELAGO_ICON_WIDTH = 44;

// Initialize all vtx data with dummy/default values
void CreateRandSaveTypeVtxData() {
    for (int vtxId = 0; vtxId < ARRAY_COUNT(sRandVtxData); vtxId += 4) {
        // x-coord (left)
        sRandVtxData[vtxId + 0].v.ob[0] = sRandVtxData[vtxId + 2].v.ob[0] = 0;
        // x-coord (right)
        sRandVtxData[vtxId + 1].v.ob[0] = sRandVtxData[vtxId + 3].v.ob[0] = 0;

        // y-coord (top)
        sRandVtxData[vtxId + 0].v.ob[1] = sRandVtxData[vtxId + 1].v.ob[1] = 0;
        // y-coord (bottom)
        sRandVtxData[vtxId + 2].v.ob[1] = sRandVtxData[vtxId + 3].v.ob[1] = 0;

        // z-coordinate
        sRandVtxData[vtxId + 0].v.ob[2] = sRandVtxData[vtxId + 1].v.ob[2] = sRandVtxData[vtxId + 2].v.ob[2] =
            sRandVtxData[vtxId + 3].v.ob[2] = 0;

        // flag
        sRandVtxData[vtxId + 0].v.flag = sRandVtxData[vtxId + 1].v.flag = sRandVtxData[vtxId + 2].v.flag =
            sRandVtxData[vtxId + 3].v.flag = 0;

        // texture coordinates
        sRandVtxData[vtxId + 0].v.tc[0] = sRandVtxData[vtxId + 0].v.tc[1] = sRandVtxData[vtxId + 1].v.tc[1] =
            sRandVtxData[vtxId + 2].v.tc[0] = 0;
        sRandVtxData[vtxId + 1].v.tc[0] = sRandVtxData[vtxId + 2].v.tc[1] = sRandVtxData[vtxId + 3].v.tc[0] =
            sRandVtxData[vtxId + 3].v.tc[1] = 0;

        // alpha
        sRandVtxData[vtxId + 0].v.cn[0] = sRandVtxData[vtxId + 1].v.cn[0] = sRandVtxData[vtxId + 2].v.cn[0] =
            sRandVtxData[vtxId + 3].v.cn[0] = sRandVtxData[vtxId + 0].v.cn[1] = sRandVtxData[vtxId + 1].v.cn[1] =
                sRandVtxData[vtxId + 2].v.cn[1] = sRandVtxData[vtxId + 3].v.cn[1] = sRandVtxData[vtxId + 0].v.cn[2] =
                    sRandVtxData[vtxId + 1].v.cn[2] = sRandVtxData[vtxId + 2].v.cn[2] =
                        sRandVtxData[vtxId + 3].v.cn[2] = sRandVtxData[vtxId + 0].v.cn[3] =
                            sRandVtxData[vtxId + 1].v.cn[3] = sRandVtxData[vtxId + 2].v.cn[3] =
                                sRandVtxData[vtxId + 3].v.cn[3] = 255;
    }
}

// Updates the vtx values on every draw to account for file info moving up/down
void SetRandSaveTypeVtxData() {
    int startY = 44;
    int vtxId = 0;

    for (int i = 0; i < FILE_NUM_MAX; i++, startY -= 16, vtxId += 4) {
        int posY;
        int posX = gFileSelectState->windowPosX + 163;
        const s16 iconWidth = isArchipelago[i] ? ARCHIPELAGO_ICON_WIDTH : RAND_ICON_WIDTH;
        const s16 centeredOffset = (52 - iconWidth) / 2;

        // Compute real Y position based on current file select state
        if ((gFileSelectState->configMode == 0x10) && (i == gFileSelectState->copyDestFileIndex)) {
            posY = gFileSelectState->fileNamesY[i] + 0x2C;
        } else if (((gFileSelectState->configMode == 0x11) || (gFileSelectState->configMode == 0x12)) &&
                   (i == gFileSelectState->copyDestFileIndex)) {
            posY = gFileSelectState->buttonYOffsets[i] + startY;
        } else {
            posY = startY + gFileSelectState->buttonYOffsets[i] + gFileSelectState->fileNamesY[i];
        }

        /* Rand Icons */
        // 4 sets: Left aligned, Left aligned (shadow), Center aligned, Center aligned (shadow)
        for (int j = 0; j < 4; j++, vtxId += 4) {
            // x-coord (left)
            sRandVtxData[vtxId + 0].v.ob[0] = sRandVtxData[vtxId + 2].v.ob[0] =
                posX + (j % 2 ? 1 : 0) + ((j > 1) ? centeredOffset : 0);
            // x-coord (right)
            sRandVtxData[vtxId + 1].v.ob[0] = sRandVtxData[vtxId + 3].v.ob[0] =
                sRandVtxData[vtxId + 0].v.ob[0] + iconWidth;

            // y-coord (top)
            sRandVtxData[vtxId + 0].v.ob[1] = sRandVtxData[vtxId + 1].v.ob[1] = posY - (j % 2 ? 1 : 0);
            // y-coord (bottom)
            sRandVtxData[vtxId + 2].v.ob[1] = sRandVtxData[vtxId + 3].v.ob[1] =
                sRandVtxData[vtxId + 0].v.ob[1] - RAND_ICON_HEIGHT;

            // texture coordinates
            sRandVtxData[vtxId + 0].v.tc[0] = sRandVtxData[vtxId + 0].v.tc[1] = sRandVtxData[vtxId + 1].v.tc[1] =
                sRandVtxData[vtxId + 2].v.tc[0] = 0;
            sRandVtxData[vtxId + 1].v.tc[0] = sRandVtxData[vtxId + 3].v.tc[0] = iconWidth << 5;
            sRandVtxData[vtxId + 2].v.tc[1] = sRandVtxData[vtxId + 3].v.tc[1] = RAND_ICON_HEIGHT << 5;
        }

        /* Owl Icon */
        // x-coord (left)
        sRandVtxData[vtxId + 0].v.ob[0] = sRandVtxData[vtxId + 2].v.ob[0] = posX + 28;
        // x-coord (right)
        sRandVtxData[vtxId + 1].v.ob[0] = sRandVtxData[vtxId + 3].v.ob[0] = sRandVtxData[vtxId + 0].v.ob[0] + 24;

        // y-coord (top)
        sRandVtxData[vtxId + 0].v.ob[1] = sRandVtxData[vtxId + 1].v.ob[1] = posY - 2;
        // y-coord (bottom)
        sRandVtxData[vtxId + 2].v.ob[1] = sRandVtxData[vtxId + 3].v.ob[1] = sRandVtxData[vtxId + 0].v.ob[1] - 12;

        // texture coordinates
        sRandVtxData[vtxId + 0].v.tc[0] = sRandVtxData[vtxId + 0].v.tc[1] = sRandVtxData[vtxId + 1].v.tc[1] =
            sRandVtxData[vtxId + 2].v.tc[0] = 0;
        sRandVtxData[vtxId + 1].v.tc[0] = sRandVtxData[vtxId + 3].v.tc[0] = 24 << 5;
        sRandVtxData[vtxId + 2].v.tc[1] = sRandVtxData[vtxId + 3].v.tc[1] = 12 << 5;
    }
}

void SpriteLoad(Sprite* sprite) {
    OPEN_DISPS(gFileSelectState->state.gfxCtx);

    /*
     * Due to macro expansion and the token-pasting operator (##), we cannot pass sprite->im_siz in directly.
     * Instead we must call gDPLoadTextureBlock with the raw IM_SIZ define name itself to properly expand the correct
     * defines internally.
     */

    if (sprite->im_siz == G_IM_SIZ_8b) {
        gDPLoadTextureBlock(POLY_OPA_DISP++, sprite->tex, sprite->im_fmt, G_IM_SIZ_8b, sprite->width, sprite->height, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
    } else if (sprite->im_siz == G_IM_SIZ_16b) {
        gDPLoadTextureBlock(POLY_OPA_DISP++, sprite->tex, sprite->im_fmt, G_IM_SIZ_16b, sprite->width, sprite->height,
                            0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPLoadTextureBlock(POLY_OPA_DISP++, sprite->tex, sprite->im_fmt, G_IM_SIZ_32b, sprite->width, sprite->height,
                            0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
    }

    CLOSE_DISPS(gFileSelectState->state.gfxCtx);
}

void SpriteDraw(Sprite* sprite, int left, int top, int width, int height) {
    int width_factor = (1 << 10) * sprite->width / width;
    int height_factor = (1 << 10) * sprite->height / height;

    OPEN_DISPS(gFileSelectState->state.gfxCtx);

    gSPWideTextureRectangle(POLY_OPA_DISP++, left << 2, top << 2, (left + width) << 2, (top + height) << 2,
                            G_TX_RENDERTILE, 0, 0, width_factor, height_factor);

    CLOSE_DISPS(gFileSelectState->state.gfxCtx);
}

void DrawSeedHashSprites() {
    OPEN_DISPS(gFileSelectState->state.gfxCtx);
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

    // Draw icons on the main menu, when a rando file is selected
    if (gFileSelectState->configMode == CM_MAIN_MENU) {
        if (gFileSelectState->fileInfoAlpha[gFileSelectState->selectedFileIndex] > 0 &&
            isRando[gFileSelectState->selectedFileIndex]) {
            // Use file info alpha to match fading
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 0xFF, 0xFF, 0xFF,
                            gFileSelectState->fileInfoAlpha[gFileSelectState->selectedFileIndex]);

            u16 xStart = 64;
            // Draw Seed Icons for specific file
            u32 fileHash = seedHashes[gFileSelectState->selectedFileIndex];
            for (int i = 0; i < 5; i++) {
                u8 hashPart = fileHash % 100;
                fileHash /= 100;
                SpriteLoad(GetSeedTexture(hashPart));
                SpriteDraw(GetSeedTexture(hashPart), xStart + (40 * i), 10, 24, 24);
            }
        }
    }

    gDPPipeSync(POLY_OPA_DISP++);

    CLOSE_DISPS(gFileSelectState->state.gfxCtx);
}

void RegisterShoulds() {

    // Renders the small blank info box next to the file info
    REGISTER_VB_SHOULD(VB_DRAW_FILE_SELECT_SMALL_EXTRA_INFO_BOX, {
        int fileIndex = va_arg(args, int);

        // Bail out if not a rando save, or if the save is also an owl save
        // because owl saves already render the small box and large box
        if (!isRando[fileIndex] || gFileSelectState->isOwlSave[fileIndex + FILE_NUM_OWL_SAVE_OFFSET]) {
            return;
        }

        // We want to force the original small box to render which uses the alpha for the collapsed files
        *should = true;

        OPEN_DISPS(gFileSelectState->state.gfxCtx);

        // But then we also render the same small box again, but using the expanded file info alpha
        gDPSetPrimColorOverride(POLY_OPA_DISP++, 0, 0, sWindowContentColors[0], sWindowContentColors[1],
                                sWindowContentColors[2], gFileSelectState->fileInfoAlpha[fileIndex],
                                COSMETIC_ID("Menus.FilePlates"));
        gDPLoadTextureBlock(POLY_OPA_DISP++, gFileSelBlankButtonTex, G_IM_FMT_IA, G_IM_SIZ_16b, 52, 16, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
        gSP1Quadrangle(POLY_OPA_DISP++, 12, 14, 15, 13, 0);

        CLOSE_DISPS(gFileSelectState->state.gfxCtx);
    });

    REGISTER_VB_SHOULD(VB_DRAW_FILE_SELECT_EXTRA_INFO_DETAILS, {
        int fileIndex = va_arg(args, int);

        if (!isRando[fileIndex]) {
            return;
        }

        DrawSeedHashSprites();

        SetRandSaveTypeVtxData();

        OPEN_DISPS(gFileSelectState->state.gfxCtx);

        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

        gSPVertex(POLY_OPA_DISP++, (uintptr_t)&sRandVtxData[20 * fileIndex], 20, 0);

        if (isArchipelago[fileIndex]) {
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFileSelArchiButtonTex, G_IM_FMT_IA, G_IM_SIZ_16b,
                                ARCHIPELAGO_ICON_WIDTH, RAND_ICON_HEIGHT, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        } else {
            gDPLoadTextureBlock_4b(POLY_OPA_DISP++, gFileSelRandIconTex, G_IM_FMT_I, RAND_ICON_WIDTH, RAND_ICON_HEIGHT,
                                   0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK,
                                   G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        }

        // Rand Icon (shadow)
        gDPSetPrimColor(POLY_OPA_DISP++, 0x00, 0x00, 0, 0, 0, gFileSelectState->nameAlpha[fileIndex]);

        if (gFileSelectState->isOwlSave[fileIndex + FILE_NUM_OWL_SAVE_OFFSET]) {
            gSP1Quadrangle(POLY_OPA_DISP++, 4, 6, 7, 5, 0); // Left aligned
        } else {
            gSP1Quadrangle(POLY_OPA_DISP++, 12, 14, 15, 13, 0); // Centered
        }

        // Rand Icon
        if (isArchipelago[fileIndex]) {
            gDPSetPrimColor(POLY_OPA_DISP++, 0x00, 0x00, 110, 180, 255,
                            gFileSelectState->nameAlpha[fileIndex]);
        } else {
            gDPSetPrimColor(POLY_OPA_DISP++, 0x00, 0x00, 255, 255, 255,
                            gFileSelectState->nameAlpha[fileIndex]);
        }

        if (gFileSelectState->isOwlSave[fileIndex + FILE_NUM_OWL_SAVE_OFFSET]) {
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0); // Left aligned
        } else {
            gSP1Quadrangle(POLY_OPA_DISP++, 8, 10, 11, 9, 0); // Centered
        }

        // Owl Icon
        if (gFileSelectState->isOwlSave[fileIndex + FILE_NUM_OWL_SAVE_OFFSET]) {
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFileSelOwlSaveIconTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 24, 12, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);
            gSP1Quadrangle(POLY_OPA_DISP++, 16, 18, 19, 17, 0);
        }

        CLOSE_DISPS(gFileSelectState->state.gfxCtx);
    });

    // Prevent original owl save icon from rendering
    REGISTER_VB_SHOULD(VB_DRAW_FILE_SELECT_OWL_SAVE_ICON, {
        int fileIndex = va_arg(args, int);

        if (isRando[fileIndex]) {
            *should = false;
        }
    });
}

// Doesn't really look great yet, but the start to how we will augment the file select screen for rando saves
void Rando::MiscBehavior::InitFileSelect() {
    RegisterShoulds();

    CreateRandSaveTypeVtxData();

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnFileSelectSaveLoad>(
        [](s16 fileNum, bool isOwlSave, SaveContext* saveContext) {
            const int index = fileNum + (isOwlSave ? FILE_NUM_OWL_SAVE_OFFSET : 0);
            isArchipelago[index] = saveContext->save.shipSaveInfo.saveType == SAVETYPE_ARCHIPELAGO;
            isRando[index] = saveContext->save.shipSaveInfo.saveType == SAVETYPE_RANDO || isArchipelago[index];
            if (isRando[index]) {
                seedHashes[index] = saveContext->save.shipSaveInfo.rando.finalSeed;
            }
            if (isArchipelago[index]) {
                snprintf(archipelagoServers[index], sizeof(archipelagoServers[index]), "%s",
                         saveContext->save.shipSaveInfo.rando.archipelagoServer);
                snprintf(archipelagoSlots[index], sizeof(archipelagoSlots[index]), "%s",
                         saveContext->save.shipSaveInfo.rando.archipelagoSlot);
                snprintf(archipelagoSeeds[index], sizeof(archipelagoSeeds[index]), "%s",
                         saveContext->save.shipSaveInfo.rando.archipelagoSeed);
            }
        });
}
