/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* Cystals should really be a DB creature summon group, having them in `creature` like this will cause tons of despawn/respawn bugs */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "magisters_terrace.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"

enum SelinTexts
{
    SAY_AGGRO                       = 0,
    SAY_ENERGY                      = 1,
    SAY_EMPOWERED                   = 2,
    SAY_SLAY                        = 3,
    SAY_DEATH                       = 4,
    EMOTE_CRYSTAL                   = 5
};

enum SelinSpells
{
    // Selin
    SPELL_DRAIN_LIFE                = 44294,
    SPELL_FEL_EXPLOSION             = 44314,

    SPELL_DRAIN_MANA                = 46153,               // Heroic only

    // Crystal
    SPELL_FEL_CRYSTAL_DUMMY         = 44329,
    SPELL_MANA_RAGE                 = 44320,
    SPELL_QUIET_SUICIDE             = 3617
};

enum SelinEvents
{
    EVENT_FEL_EXPLOSION             = 1,
    EVENT_DRAIN_CRYSTAL,
    EVENT_DRAIN_MANA,
    EVENT_DRAIN_LIFE,
    EVENT_EMPOWER
};

enum SelinPhases
{
    PHASE_NORMAL                    = 1,
    PHASE_DRAIN                     = 2
};

enum SelinMisc
{
    ACTION_SWITCH_PHASE             = 1
};

// 24723 - Selin Fireheart
struct boss_selin_fireheart : public BossAI
{
    boss_selin_fireheart(Creature* creature) : BossAI(creature, DATA_SELIN_FIREHEART), _scheduledEvents(false) { }

    void Reset() override
    {
        std::list<Creature*> crystals;
        me->GetCreatureListWithEntryInGrid(crystals, NPC_FEL_CRYSTAL, 250.0f);

        for (Creature* creature : crystals)
            creature->Respawn(true);

        _Reset();
        CrystalGUID.Clear();
        _scheduledEvents = false;
    }

    void DoAction(int32 action) override
    {
        switch (action)
        {
            case ACTION_SWITCH_PHASE:
                events.SetPhase(PHASE_NORMAL);
                events.ScheduleEvent(EVENT_FEL_EXPLOSION, 2s, 0, PHASE_NORMAL);
                AttackStart(me->GetVictim());
                me->GetMotionMaster()->MoveChase(me->GetVictim());
                break;
            default:
                break;
        }
    }

    void SelectNearestCrystal()
    {
        if (Creature* crystal = me->FindNearestCreature(NPC_FEL_CRYSTAL, 250.0f))
        {
            Talk(SAY_ENERGY);
            Talk(EMOTE_CRYSTAL);

            DoCast(crystal, SPELL_FEL_CRYSTAL_DUMMY);
            CrystalGUID = crystal->GetGUID();

            float x, y, z;
            crystal->GetClosePoint(x, y, z, me->GetCombatReach(), CONTACT_DISTANCE);

            events.SetPhase(PHASE_DRAIN);
            me->SetWalk(false);
            me->GetMotionMaster()->MovePoint(1, x, y, z);
        }
    }

    void ShatterRemainingCrystals()
    {
        std::list<Creature*> crystals;
        me->GetCreatureListWithEntryInGrid(crystals, NPC_FEL_CRYSTAL, 250.0f);

        for (Creature* crystal : crystals)
            crystal->CastSpell(crystal, SPELL_QUIET_SUICIDE, true);
    }

    void JustEngagedWith(Unit* who) override
    {
        Talk(SAY_AGGRO);
        BossAI::JustEngagedWith(who);

        events.SetPhase(PHASE_NORMAL);
        events.ScheduleEvent(EVENT_FEL_EXPLOSION, 2100ms, 0, PHASE_NORMAL);
     }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_SLAY);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == 1)
        {
            Unit* CrystalChosen = ObjectAccessor::GetUnit(*me, CrystalGUID);
            if (CrystalChosen && CrystalChosen->IsAlive())
            {
                CrystalChosen->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                CrystalChosen->CastSpell(me, SPELL_MANA_RAGE, true);
                events.ScheduleEvent(EVENT_EMPOWER, 10s, PHASE_DRAIN);
            }
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_DEATH);
        _JustDied();

        ShatterRemainingCrystals();
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
                case EVENT_FEL_EXPLOSION:
                    DoCastAOE(SPELL_FEL_EXPLOSION);
                    events.ScheduleEvent(EVENT_FEL_EXPLOSION, 2s, 0, PHASE_NORMAL);
                    break;
                case EVENT_DRAIN_CRYSTAL:
                    SelectNearestCrystal();
                    _scheduledEvents = false;
                    break;
                case EVENT_DRAIN_MANA:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 45.0f, true))
                        DoCast(target, SPELL_DRAIN_MANA);
                    events.ScheduleEvent(EVENT_DRAIN_MANA, 10s, 0, PHASE_NORMAL);
                    break;
                case EVENT_DRAIN_LIFE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 20.0f, true))
                        DoCast(target, SPELL_DRAIN_LIFE);
                    events.ScheduleEvent(EVENT_DRAIN_LIFE, 10s, 0, PHASE_NORMAL);
                    break;
                case EVENT_EMPOWER:
                {
                    Talk(SAY_EMPOWERED);

                    Creature* CrystalChosen = ObjectAccessor::GetCreature(*me, CrystalGUID);
                    if (CrystalChosen && CrystalChosen->IsAlive())
                        CrystalChosen->KillSelf();

                    CrystalGUID.Clear();

                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveChase(me->GetVictim());
                    break;
                }
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        if (me->GetPowerPct(POWER_MANA) < 10.f)
        {
            if (events.IsInPhase(PHASE_NORMAL) && !_scheduledEvents)
            {
                _scheduledEvents = true;
                Milliseconds timer = randtime(3s, 7s);
                events.ScheduleEvent(EVENT_DRAIN_LIFE, timer, 0, PHASE_NORMAL);

                if (IsHeroic())
                {
                    events.ScheduleEvent(EVENT_DRAIN_CRYSTAL, 10s, 15s, 0, PHASE_NORMAL);
                    events.ScheduleEvent(EVENT_DRAIN_MANA, timer + 5s, 0, PHASE_NORMAL);
                }
                else
                    events.ScheduleEvent(EVENT_DRAIN_CRYSTAL, 20s, 25s, 0, PHASE_NORMAL);
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    ObjectGuid CrystalGUID;
    bool _scheduledEvents;
};

// 24722 - Fel Crystal
struct npc_fel_crystal : public ScriptedAI
{
    npc_fel_crystal(Creature* creature) : ScriptedAI(creature) { }

    void JustDied(Unit* /*killer*/) override
    {
        if (InstanceScript* instance = me->GetInstanceScript())
        {
            Creature* selin = instance->GetCreature(DATA_SELIN_FIREHEART);
            if (selin && selin->IsAlive())
                selin->AI()->DoAction(ACTION_SWITCH_PHASE);
        }
    }
};

void AddSC_boss_selin_fireheart()
{
    RegisterMagistersTerraceCreatureAI(boss_selin_fireheart);
    RegisterMagistersTerraceCreatureAI(npc_fel_crystal);
}
