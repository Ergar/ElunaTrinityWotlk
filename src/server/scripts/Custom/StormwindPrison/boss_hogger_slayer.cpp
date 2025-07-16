#include "stormwind_prison.h"

#include "ScriptedCreature.h"
#include "ScriptMgr.h"

enum Spells
{
    SPELL_FRENZY = 37023,

    SPELL_HAMSTRING = 9080,
    SPELL_MORTALSTRIKE = 29572,
};

enum Phases
{
    PHASE_ALL = 0,
    PHASE_ONE = 1,
    PHASE_TWO = 2
};

enum GROUPS
{
    GROUP_ONE = 1,
    GROUP_TWO = 2
};

struct boss_hogger_slayer : public BossAI
{
    boss_hogger_slayer(Creature* creature) : BossAI(creature, BOSS_HOGGER)
    {
        _phase = PHASE_ALL;
    }

    void Reset() override
    {
        _Reset();
        _phase = PHASE_ONE;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _phase = PHASE_ONE;
        me->setActive(true);
        DoZoneInCombat();

        // All Phases
        scheduler
            .Schedule(Seconds(5), [this](TaskContext hamstring)
                {
                    me->Say("HAMSTRING!", LANG_UNIVERSAL);
                    DoCastVictim(SPELL_HAMSTRING);
                    hamstring.Repeat(Seconds(12), Seconds(15));
                });

        me->Say("HAHAH", LANG_UNIVERSAL);

        instance->SetBossState(BOSS_HOGGER, IN_PROGRESS);
    }

    void JustReachedHome() override
    {
        _JustReachedHome();

        instance->SetBossState(BOSS_HOGGER, FAIL);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        scheduler.Update(diff, [this]
            {
                DoMeleeAttackIfReady();
            });
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (damage >= me->GetHealth())
            return;

        if (_phase == PHASE_ONE && me->HealthBelowPctDamaged(50, damage))
        {
            _phase = PHASE_TWO;
            scheduler.CancelGroup(GROUP_ONE);

            me->Say("FRENZY", LANG_UNIVERSAL);
            DoCast(me, SPELL_FRENZY);

            // Phase Two
            scheduler
                .Schedule(Seconds(3), GROUP_TWO, [this](TaskContext mortalStrike)
                    {
                        me->Say("MORTAL STRIKE!", LANG_UNIVERSAL);
                        DoCastVictim(SPELL_MORTALSTRIKE);
                        mortalStrike.Repeat(Seconds(7), Seconds(10));
                    });
        }
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        scheduler.CancelAll();
        _DespawnAtEvade();
    }

private:
    uint8 _phase;
};

void AddSC_boss_hogger_slayer()
{
    RegisterStormwindPrisonCreatureAI(boss_hogger_slayer);
}
