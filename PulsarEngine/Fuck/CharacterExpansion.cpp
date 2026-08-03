#include <kamek.hpp>
#include <MarioKartWii/System/identifiers.hpp>
#include <MarioKartWii/Archive/ArchiveMgr.hpp>
#include <MarioKartWii/Kart/KartFunctions.hpp>
#include <MarioKartWii/Race/RaceData.hpp>
#include <MarioKartWii/Kart/KartMovement.hpp>
#include <MarioKartWii/Scene/RaceScene.hpp>
#include <MarioKartWii/3D/Model/Awards.hpp>
#include <MarioKartWii/Driver/DriverController.hpp>
#include <MarioKartWii/System/Identifiers.hpp>
#include <MarioKartWii/Kart/KartPointers.hpp>

//807e7bac stuff for menu bars ignore for now
//disable load for now
kmWrite32(0x807E7E9C, 0x38000000);
kmWrite32(0x807E7EA4, 0x48000018);
kmWrite32(0x80541fb8, 0x60000000); //disable menu models (it crashes rn cause it tries to load into all ArchiveFiles which runs out of Heap space)
kmWrite32(0x8082fab4, 0x60000000); //disable here for now (related to menu models which I want to deal with later)

static StatsParamFile::Entry* KartParamEntries[12] = {0};
static StatsParamFile::Entry* DriverParamEntries[12] = {0};
static void* KartPartsDispParam[12] = {0}; //this array is mixed between Kart and Bike Disp Entries, which have a different structure but I can determine at runtime which it is
static DriverDispParam::Entry* DriverDispParam[12] = {0};
static KartDriverDispParam::Entry* KartDriverDispParam[12] = {0};



//-- in RACE changes

//8081cd3c get character weight class
//set default to lightweight instead of -1 so the game doesn't give an invalid kart select screen
kmWrite32(0x8081cea8, 0x38600000);

//UVC
kmWrite32(0x80847508, 0x38800024);
kmWrite32(0x80847528, 0x38600001);

kmWrite32(0x8053ff14, 0x38800003); //add 1 more archive to kartModelHolders and kartModelHolders2

//80540e3c loadKartNormal //loadKartBackup 80540f90 (same function? no! (other than the inline))
ArchivesHolder* loadFuckFiles(ArchivesHolder *archivesHolder, u32 playerId, u32 VehicleId, u32 CharacterId, u32 team, u32 lod, EGG::Heap *archiveHeap, EGG::Heap *fileHeap){
    if(!archivesHolder->HasArchives()){
        char path [0x128];      
    
        //try load combo first
        snprintf(path, 0x80, "Race/Combos/%u-%u.szs", VehicleId, CharacterId);
        archivesHolder->archives[0].Load(path, archiveHeap, 1, 8, fileHeap, 0);

        //if combo didn't load, load fallback
        if(archivesHolder->archives[0].status == ARCHIVE_STATUS_NONE) {
            snprintf(path, 0x80, "Race/Chars/%u.szs", CharacterId);
            archivesHolder->archives[1].Load(path, archiveHeap, 1, 8, fileHeap, 0);

            snprintf(path, 0x80, "Race/Karts/%u.szs", VehicleId);
            archivesHolder->archives[2].Load(path, archiveHeap, 1, 8, fileHeap, 0);
        }
        
    }
    return archivesHolder;
}

ArchivesHolder* loadKart(ArchiveMgr* _this, u32 playerId, u32 VehicleId, u32 CharacterId, u32 team, u32 lod, EGG::Heap *archiveHeap, EGG::Heap *fileHeap){

    ArchivesHolder* archiveHolder = &_this->kartModelsHolders[playerId];
    loadFuckFiles(archiveHolder, playerId, VehicleId, CharacterId, team, lod, archiveHeap, fileHeap);
    return archiveHolder;
}

ArchivesHolder* loadKart2(ArchiveMgr* _this, u32 playerId, u32 VehicleId, u32 CharacterId, u32 team, u32 lod, EGG::Heap *archiveHeap, EGG::Heap *fileHeap){

    ArchivesHolder* archiveHolder = &_this->kartModelsHolders2[playerId];
    loadFuckFiles(archiveHolder, playerId, VehicleId, CharacterId, team, lod, archiveHeap, fileHeap);
    return archiveHolder;
}

kmBranch(0x80540e3c, loadKart); //change loadKart and loadKart2 to point to my versions
kmBranch(0x80540f90, loadKart2);


//805919f4 InitParams, change the way stats are loaded, ideally not from the .bin files
void InitCustomBins(){
    //for each player load the required bin entries not from common but from the character/vehicle combo
    for(u32 playerId = 0; playerId < Racedata::sInstance->racesScenario.playerCount; playerId++) {
        KartParamEntries[playerId] = (StatsParamFile::Entry*)ArchiveMgr::sInstance->GetKartArchiveFile(playerId, "kartParam.bin", 0);
        DriverParamEntries[playerId] = (StatsParamFile::Entry*)ArchiveMgr::sInstance->GetKartArchiveFile(playerId, "driverParam.bin", 0);
        KartPartsDispParam[playerId] = ArchiveMgr::sInstance->GetKartArchiveFile(playerId, "kartPartsDispParam.bin", 0);
        DriverDispParam[playerId] = (DriverDispParam::Entry*)ArchiveMgr::sInstance->GetKartArchiveFile(playerId, "driverDispParam.bin", 0);
        KartDriverDispParam[playerId] = (KartDriverDispParam::Entry*)ArchiveMgr::sInstance->GetKartArchiveFile(playerId, "kartDriverDispParam.bin", 0);
    }
    return;
}
kmBranch(0x80591b70, InitCustomBins);

//kartParam.bin and driverParam.bin replacement
kmWrite32(0x8058f668, 0x7F83E378); //send playerId to Compute Stats
//redirect for kartParam.bin and driverParam.bin
asmFunc kartParamRedirect() {
    ASM(
        nofralloc;
        lis r5, KartParamEntries@ha;
        addi r5, r5, KartParamEntries@l;
        mulli r4, r30, 4; //index into pointer array with playerid
        lwzx r4, r5, r4;
        lwz r5, 0x0(r4);
        blr;
    )
}
kmCall(0x80591fdc, kartParamRedirect);
asmFunc driverParamRedirect() {
    ASM(
        nofralloc;
        lis r5, DriverParamEntries@ha;
        addi r5, r5, DriverParamEntries@l;
        mulli r4, r30, 4; //index into pointer array with playerid
        lwzx r6, r4, r5;
        blr;
    )
}
kmCall(0x8059217c, driverParamRedirect);

//redirect for bikeDispParam and kartDispParam.bin (I'm combining them)
KartPartsDispParam::Entry* GetKartPartsDisplayStats(bool isbike, u32 playerid){
    if(isbike) return 0;
    return (KartPartsDispParam::Entry*)KartPartsDispParam[playerid];
}
kmBranch(0x80592558, GetKartPartsDisplayStats);

BikePartsDispParam::Entry* GetBikePartsDisplayStats(bool isbike, u32 playerid){
    if(!isbike) return 0;
    return (BikePartsDispParam::Entry*)KartPartsDispParam[playerid];
}
kmBranch(0x80592620, GetBikePartsDisplayStats);

//pass playerid instead of vehicle id
kmWrite32(0x8058f6a4, 0x7F84E378);
kmWrite32(0x8058f6b4, 0x7F84E378);
kmWrite32(0x8058f71c, 0x7F84E378);
kmWrite32(0x8058f72c, 0x7F84E378);

DriverDispParam::Entry* GetDriverDisplayStats(u32 playerid){
    return (DriverDispParam::Entry*)DriverDispParam[playerid];
}
kmBranch(0x805927c0, GetDriverDisplayStats);

//pass playerid instead of character id
kmWrite32(0x8058f694, 0x7F83E378);
kmWrite32(0x8058f70c, 0x7F83E378);

KartDriverDispParam::Entry* GetKartDriverDisplayStats(u32 playerid){
    return KartDriverDispParam[playerid];
}
kmBranch(0x80592498, GetKartDriverDisplayStats);

//pass playerid instead of vehicle id
kmWrite32(0x8058f64c, 0x7F83E378);

kmWrite32(0x807c8c60, 0x8B440010);
kmWrite32(0x807c8c68, 0x7F43D378);

kmWrite32(0x807c8cbc, 0x8B440010);
kmWrite32(0x807c8cc4, 0x7F43D378);


//-- Award fixes
//80789340 initPlayers needs complete rewrite because yes (luckily only need to modify this function because of nintendos amazing coding decisions)
void Award_initFuckPlayers(AwardsMgr *awardMgr, RacedataScenario *racedataScenario, u32 *CharacterIds){
    for (u32 playerId = 0; playerId < 12; playerId++) {
        if(racedataScenario->players[playerId].playerType == PLAYER_NONE){
            continue;
        }

        CharacterIds[playerId] = playerId;

        //this only needs to be assigned if you want the mii head 
        awardMgr->miiHeadModels[playerId][0] = 0;
        awardMgr->characterModels[playerId][0] = new ModelDirector(10,0);
        nw4r::g3d::ResFile resfile;
        awardMgr->characterModels[playerId][0]->BindBRRES(resfile,ARCHIVE_HOLDER_AWARD,"mr.brres");
        awardMgr->characterModels[playerId][0]->LoadWithAnm("model", resfile, 0);
        awardMgr->characterModels[playerId][0]->ToggleVisible(false);

        for(int i = 0; i < 6; i++) {
            awardMgr->characterModels[playerId][0]->LinkEmptyAnm(i); //t pose for now, I need to change this later anyways
        }

        awardMgr->shadowCharModels[playerId][0] = new ShadowModelDirector(0);
        //shadow model (literally no reason this array even has an alt version we love nintendo)
        nw4r::g3d::ResFile resfile2;
        awardMgr->shadowCharModels[playerId][0]->BindBRRES(resfile2,ARCHIVE_HOLDER_AWARD,"shadow.brres");
        awardMgr->shadowCharModels[playerId][0]->LoadScnMdl1Mat1SHp("shadow", resfile2, 0);
        awardMgr->shadowCharModels[playerId][0]->ToggleVisible(false);
    }
    return;
}
kmBranch(0x80789340, Award_initFuckPlayers);

//kill charactersound (it freezes with invalid ids and I want to change that later so disable for now)
//kmWrite32(0x807cb0f4, 0x60000000);
//kmWrite32(0x807cb15c, 0x60000000);


void CharacterActor_customLink(Audio::CharacterActor* self, void *KartObject, u16 objectId){
    self->LinkedRaceActor::Link(KartObject, objectId);
    DriverController *playerModel = (DriverController*)self->pointer;
    self->model = (Audio::DriverController*)playerModel;
    self->isLocal = playerModel->IsLocal();
    self->isMii = (playerModel->miiHeads != 0);
    self->isGhost = playerModel->IsGhost();
    self->isReplay = (Racedata::sInstance->racesScenario.settings.gametype == GAMETYPE_REPLAY);
    
    if(self->isGhost) return;

    self->objectId = (s16)playerModel->pointers->values->character;
    self->playerId = playerModel->GetPlayerIdx();
    self->hudSlotId = playerModel->GetScreenIdx();
    if(self->hudSlotId == -1 && self->isLocal) {
        self->isLocal = false;
    }

    self->hasSound = true;

    if(!self->isLocal) {
        self->hasSound = false;
        return;
    }
}
kmBranch(0x80863a9c, CharacterActor_customLink);
