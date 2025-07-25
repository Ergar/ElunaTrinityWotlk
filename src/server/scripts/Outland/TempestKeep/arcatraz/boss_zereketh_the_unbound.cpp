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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "arcatraz.h"

enum ZerekethTexts
{
    SAY_AGGRO                       = 0,
    SAY_SLAY                        = 1,
    SAY_SHADOW_NOVA                 = 2,
    SAY_DEATH                       = 3
};

enum ZerekethSpells
{
    SPELL_VOID_ZONE                 = 36119,
    SPELL_SHADOW_NOVA               = 36127,
    SPELL_SEED_OF_CORRUPTION        = 36123
};

enum ZerekethEvents
{
    EVENT_VOID_ZONE                 = 1,
    EVENT_SHADOW_NOVA,
    EVENT_SEED_OF_CORRUPTION
};

// 20870 - Zereketh the Unbound
struct boss_zereketh_the_unbound : public BossAI
{
    boss_zereketh_the_unbound(Creature* creature) : BossAI(creature, DATA_ZEREKETH) { }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        events.ScheduleEvent(EVENT_VOID_ZONE, 10s, 15s);
        events.ScheduleEvent(EVENT_SHADOW_NOVA, 15s, 20s);
        events.ScheduleEvent(EVENT_SEED_OF_CORRUPTION, 5s, 10s);
        Talk(SAY_AGGRO);
    }

    void OnSpellCast(SpellInfo const* spell) override
    {
        if (spell->Id == sSpellMgr->GetSpellIdForDifficulty(SPELL_SHADOW_NOVA, me))
            if (roll_chance_i(50))
                Talk(SAY_SHADOW_NOVA);
    }

    // Do not despawn Void Zone
    void JustSummoned(Creature* /*summon*/) override { }

    void KilledUnit(Unit* /*victim*/) override
    {
        Talk(SAY_SLAY);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        Talk(SAY_DEATH);
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
                case EVENT_VOID_ZONE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                        DoCast(target, SPELL_VOID_ZONE);
                    events.Repeat(15s);
                    break;
                case EVENT_SHADOW_NOVA:
                    DoCastSelf(SPELL_SHADOW_NOVA);
                    events.Repeat(15s, 20s);
                    break;
                case EVENT_SEED_OF_CORRUPTION:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                        DoCast(target, SPELL_SEED_OF_CORRUPTION);
                    events.Repeat(15s, 20s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }
};

void AddSC_boss_zereketh_the_unbound()
{
    RegisterArcatrazCreatureAI(boss_zereketh_the_unbound);
}
