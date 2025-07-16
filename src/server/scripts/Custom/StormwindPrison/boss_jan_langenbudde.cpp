#include "stormwind_prison.h"

#include "ScriptedCreature.h"
#include "ScriptMgr.h"


enum Phases
{
    PHASE_INTRO         = 0,
    PHASE_ONE           = 1,
    PHASE_INTERMISSION  = 2,
    PHASE_TWO           = 3
};

enum Spells
{
    SPELL_REFLECTION            = 9906,
    SPELL_VOID_ZONE             = 37063,
    SPELL_SHADOW_BOLT           = 71936,
    SPELL_SHADOW_BOLT_VOLLEY    = 41282,
    SPELL_SHADOW_BLAST          = 70866,

    // David
    SPELL_BEAM_VISUAL           = 60342
};

enum Events
{
    EVENT_REFLECTION            = 1,
    EVENT_VOID_ZONE             = 2,
    EVENT_SHADOW_BOLT           = 3,
    EVENT_SHADOW_BOLT_VOLLEY    = 4,
    EVENT_SHADOW_BLAST          = 5
};

Position const DavidSpawnPos[3] =
{
    { -1.04316f, 191.878f, -27.5507f, 4.74281f },
    { -43.8009f, 150.2f, -27.5507f, 6.27319f   },
    { 41.0569f, 149.633f, -27.5507f, 3.11982f  }
};

struct boss_jan_langenbudde : public BossAI
{
    boss_jan_langenbudde(Creature* creature) : BossAI(creature, BOSS_JAN_LANGENBUDDE)
    {
        Initialize();
    }

    void Reset() override
    {
        _Reset();
        Initialize();

        SpawnDavids();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _phase = PHASE_ONE;
        me->setActive(true);
        DoZoneInCombat();

        me->Say("DEM DAVID NOOBS", LANG_UNIVERSAL);

        instance->SetBossState(BOSS_JAN_LANGENBUDDE, IN_PROGRESS);
        
        events.ScheduleEvent(EVENT_VOID_ZONE, 5s);
        events.ScheduleEvent(EVENT_REFLECTION, 10s);
        events.ScheduleEvent(EVENT_SHADOW_BOLT, 1s);
        events.ScheduleEvent(EVENT_SHADOW_BLAST, 12s);
    }

    void JustReachedHome() override
    {
        _JustReachedHome();
        
        instance->SetBossState(BOSS_JAN_LANGENBUDDE, FAIL);
    }

    void AttackStart(Unit* target) override
    {
        if (!target)
            return;

        if (me->Attack(target, true))
            DoStartNoMovement(target);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_SHADOW_BOLT:
                DoCast(SPELL_SHADOW_BOLT);
                events.ScheduleEvent(EVENT_SHADOW_BOLT, 3500ms);
                break;
            case EVENT_VOID_ZONE:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random))
                    DoCast(target, SPELL_VOID_ZONE);
                events.ScheduleEvent(EVENT_VOID_ZONE, 5s, 10s);
                break;
            case EVENT_REFLECTION:
                DoCastSelf(SPELL_REFLECTION, true);
                events.ScheduleEvent(EVENT_REFLECTION, 15s, 20s, 0);
                break;
            case EVENT_SHADOW_BLAST:
                DoCastAOE(SPELL_SHADOW_BLAST);
                events.ScheduleEvent(EVENT_SHADOW_BLAST, 12s);
                events.DelayEvents(1s);
                events.ScheduleEvent(EVENT_SHADOW_BOLT_VOLLEY, 1s);
                break;
            case EVENT_SHADOW_BOLT_VOLLEY:
                DoCastAOE(SPELL_SHADOW_BOLT_VOLLEY);
                break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
    }

    void JustSummoned(Creature* summon) override
    {
        if (summon->GetEntry() == NPC_DAVID)
        {
            summon->CastSpell(me, SPELL_BEAM_VISUAL, true);
            summon->SetEmoteState(Emote::EMOTE_STATE_SPELL_CHANNEL_DIRECTED);
        }
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        events.Reset();
        summons.DespawnAll();
        _DespawnAtEvade(5s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (damage >= me->GetHealth())
            return;

        if (_phase == PHASE_ONE && me->HealthBelowPctDamaged(50, damage))
        {
            me->Say("PLS DONT KILL ME", LANG_UNIVERSAL);

            me->CastStop();
            _phase = PHASE_INTERMISSION;
            _davidsDied = 0;

            events.Reset();
            events.ScheduleEvent(EVENT_VOID_ZONE, 5s);
            events.ScheduleEvent(EVENT_SHADOW_BOLT, 1s);

            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);

            SpawnDavids();
        }
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
    {
        if (summon->GetEntry() == NPC_DAVID)
        {
            _davidsDied++;

            switch (_phase)
            {
            case PHASE_INTRO:
                if (_davidsDied == 3)
                    me->Say("SHIEET PLS DONT KILL ME", LANG_UNIVERSAL);
                else
                    me->Say("ONE DAVID DOWN HAHAHA", LANG_UNIVERSAL);
                break;
            case PHASE_INTERMISSION:
                if (_davidsDied == 3)
                {
                    me->Say("SHIEET NOT AGAIN", LANG_UNIVERSAL);                    

                    _phase = PHASE_TWO;
                    events.SetPhase(PHASE_TWO);
                    events.ScheduleEvent(EVENT_VOID_ZONE, 5s);
                    events.ScheduleEvent(EVENT_REFLECTION, 10s);
                    events.ScheduleEvent(EVENT_SHADOW_BOLT, 1s);
                    events.ScheduleEvent(EVENT_SHADOW_BLAST, 6s);

                }
                else
                    me->Say("LEL WAT", LANG_UNIVERSAL);
                break;
            }

            if (_davidsDied == 3)
            {
                me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_UNINTERACTIBLE);
                me->SetReactState(REACT_AGGRESSIVE);
            }
        }
    }

    void Initialize()
    {
        _phase = PHASE_INTRO;
        _davidsDied = 0;

        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_UNINTERACTIBLE);
        me->SetReactState(REACT_PASSIVE);
    }    

private:
    uint8 _phase;
    uint8 _davidsDied;

    void SpawnDavids()
    {
        for (Position const &pos : DavidSpawnPos)
            me->SummonCreature(NPC_DAVID, pos, TEMPSUMMON_CORPSE_DESPAWN);
    }
};

struct npc_david : public ScriptedAI
{
    npc_david(Creature* creature) : ScriptedAI(creature)
    {}

    /*void Reset() override
    {

    }*/

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->CastStop();
    }

    void JustReachedHome() override
    {
        if (InstanceScript* instance = me->GetInstanceScript())
            if (ObjectGuid janGuid = instance->GetGuidData(DATA_JAN_LANGENBUDDE))
                if (Creature* creature = ObjectAccessor::GetCreature(*me, janGuid))
                {
                    me->CastSpell(creature, SPELL_BEAM_VISUAL, true);
                    me->SetEmoteState(Emote::EMOTE_STATE_SPELL_CHANNEL_DIRECTED);
                }
    }
};

void AddSC_boss_jan_langenbudde()
{
    RegisterStormwindPrisonCreatureAI(boss_jan_langenbudde);
    RegisterStormwindPrisonCreatureAI(npc_david);
}
