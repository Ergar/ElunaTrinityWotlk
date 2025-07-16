#include "stormwind_prison.h"

#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"

enum Spells
{
};

enum Phases
{
    PHASE_ALL = 0,
    PHASE_ONE = 1,
    PHASE_TWO = 2
};

enum Events
{
};

Position const FireGauntletSpawnPos = { -77.438904f, 76.152100f, -32.967300f, 3.632950f };

struct boss_bastian_mudde : public BossAI
{
    boss_bastian_mudde(Creature* creature) : BossAI(creature, BOSS_BASTIAN_MUDDE)
    {
        _phase = PHASE_ALL;
    }

    void Reset() override
    {
        _Reset();
        _phase = PHASE_ONE;

        if (!instance->GetData(DATA_FIRE_GAUNTLET_FINISHED))
        {
            me->SetEmoteState(EMOTE_STATE_SPELL_CHANNEL_DIRECTED);
            me->SummonCreature(NPC_GAUNTLET_TRIGGER, FireGauntletSpawnPos);
        }
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _phase = PHASE_ONE;
        me->setActive(true);
        DoZoneInCombat();

        me->Say("AHH ICH KANN NICHT MEHR!!!", LANG_UNIVERSAL);

        //instance->SetData(DATA_FIRE_GAUNTLET_FINISHED, 1);
        instance->SetBossState(BOSS_BASTIAN_MUDDE, IN_PROGRESS);
    }

    void JustReachedHome() override
    {
        _JustReachedHome();

        instance->SetBossState(BOSS_BASTIAN_MUDDE, FAIL);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            }
        }

        DoMeleeAttackIfReady();
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (damage >= me->GetHealth())
            return;
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        events.Reset();
        summons.DespawnAll();
        _DespawnAtEvade(5s);
    }

    void DoAction(int32 param) override
    {
        if (param == ACTION_FINISH_GAUNTLET)
        {
            summons.DespawnAll();
            me->SetEmoteState(EMOTE_ONESHOT_NONE);
            me->Say("PLSS ICH HABE MEHR AUSGEGEBEN ALS EINGENOMMEN", LANG_UNIVERSAL);
        }
    }

private:
    uint8 _phase;
};

class go_gauntlet_lever : public GameObjectScript
{
public:
    go_gauntlet_lever() : GameObjectScript("go_gauntlet_lever") { }

    struct go_gauntlet_leverAI : public GameObjectAI
    {
        go_gauntlet_leverAI(GameObject* go) : GameObjectAI(go), instance(go->GetInstanceScript()) { }

        InstanceScript* instance;

        bool OnGossipHello(Player* /*player*/) override
        {
            instance->SetData(DATA_GAUNTLET_LEVER_ACTIVATED, 1);

            me->SetFlag(GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE);
            me->SetGoState(GO_STATE_ACTIVE);

            return true;
        }
    };
    

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return GetStormwindPrisonAI<go_gauntlet_leverAI>(go);
    }
};

void AddSC_boss_bastian_mudde()
{
    RegisterStormwindPrisonCreatureAI(boss_bastian_mudde);
    new go_gauntlet_lever();
}
