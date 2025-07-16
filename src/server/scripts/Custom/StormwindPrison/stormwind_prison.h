#ifndef DEF_STORMWIND_PRISON_H
#define DEF_STORMWIND_PRISON_H

#include "CreatureAIImpl.h"

#define StormwindPrisonScriptName "instance_stormwind_prison"
#define DataHeader "SPR"

uint32 const EncounterCount = 3;

enum SPREncounter
{
    BOSS_HOGGER             = 0,
    BOSS_JAN_LANGENBUDDE    = 1,
    BOSS_BASTIAN_MUDDE      = 2,
    NUM_BOSS_ENCOUNTER,
};

enum SPRDataTypes
{
    DATA_JAN_LANGENBUDDE,

    DATA_MONSTER_KILLED,
    DATA_MONSTER_SPAWNED,
    DATA_FIRE_GAUNTLET_FINISHED,
    DATA_GAUNTLET_LEVER_ACTIVATED
};

enum SPRCreatureIds
{   
    NPC_HOGGER              = 300000,
    NPC_JAN_LANGENBUDDE     = 300001,
    NPC_DAVID               = 300002,
    NPC_BASTIAN_MUDDE       = 300003,

    NPC_GAUNTLET_TRIGGER    = 300004,
    NPC_FLAME_TSUNAMI       = 300005
};

enum SPRGameObjectIds
{
    GO_SPEEDBUFF_ENTRY      = 179871,
    GO_REGENBUFF_ENTRY      = 179904,
    GO_BERSERKERBUFF_ENTRY  = 179905,

    GO_WING_A_DOOR_ENTRY    = 300000,

    //Hogger
    GO_HOGGER_GATE          = 300001,

    // Gaunlet
    GO_GAUNTLET_LEVER       = 300002
};

enum SPRActions
{
    ACTION_FINISH_GAUNTLET
};

template <class AI, class T>
inline AI* GetStormwindPrisonAI(T* obj)
{
    return GetInstanceAI<AI>(obj, StormwindPrisonScriptName);
}

#define RegisterStormwindPrisonCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetStormwindPrisonAI)

#endif
