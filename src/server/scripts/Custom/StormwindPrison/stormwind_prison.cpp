#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "PassiveAI.h"
#include "stormwind_prison.h"

enum Spells
{
    SPELL_FLAME_TSUNAMI             = 57494,    // the visual dummy
    SPELL_FLAME_TSUNAMI_DMG_AURA    = 57491,    // periodic damage, npc has this aura
};
enum Events
{
    EVENT_GAUNTLET_FLAME = 1
};

Position const FlameSpawn = { -75.308914f, 71.668556f, -31.716976f, 6.280744f };
Position const FlameEnd = { -11.891099f, 71.668556f, -27.550669f, 6.280744f };

struct npc_fire_gauntlet_trigger : public NullCreatureAI
{
    npc_fire_gauntlet_trigger(Creature* creature) : NullCreatureAI(creature) { }

    void Reset() override
    {
        if (InstanceScript* instance = me->GetInstanceScript())
            if (!instance->GetData(DATA_FIRE_GAUNTLET_FINISHED))
                _events.ScheduleEvent(EVENT_GAUNTLET_FLAME, 500ms);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_GAUNTLET_FLAME:
                if (Creature* wave = me->SummonCreature(NPC_FLAME_TSUNAMI, FlameSpawn, TEMPSUMMON_TIMED_DESPAWN, 5s))
                    wave->GetMotionMaster()->MovePoint(0, FlameEnd);

                _events.ScheduleEvent(EVENT_GAUNTLET_FLAME, 5s);
                break;
            }
        }
    }

private:
    EventMap _events;

};

enum FlameTsunami
{
    EVENT_TSUNAMI_TIMER = 1,
};

struct npc_swp_flame_tsunami : public ScriptedAI
{
    npc_swp_flame_tsunami(Creature* creature) : ScriptedAI(creature)
    {
        me->SetDisplayId(11686);
        me->AddAura(SPELL_FLAME_TSUNAMI, me);
    }

    void Reset() override
    {
        me->SetReactState(REACT_PASSIVE);
        events.ScheduleEvent(EVENT_TSUNAMI_TIMER, 100ms);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_UNINTERACTIBLE);
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_TSUNAMI_TIMER:
                DoCast(me, SPELL_FLAME_TSUNAMI_DMG_AURA);
                events.ScheduleEvent(EVENT_TSUNAMI_TIMER, 500ms);
                break;
            }
        }
    }

private:
    EventMap events;
};


void AddSC_stormwind_prison()
{
    RegisterStormwindPrisonCreatureAI(npc_swp_flame_tsunami);
    RegisterStormwindPrisonCreatureAI(npc_fire_gauntlet_trigger);
}
