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

// This is where scripts' loading functions should be declared:

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()

void AddSC_AutoBalance();

void AddSC_boss_hogger_slayer();
void AddSC_boss_jan_langenbudde();
void AddSC_boss_bastian_mudde();
void AddSC_stormwind_prison();
void AddSC_instance_stormwind_prison();

void AddCustomScripts()
{
    // VAS AutoBalance
    AddSC_AutoBalance();

    AddSC_boss_hogger_slayer();
    AddSC_boss_jan_langenbudde();
    AddSC_boss_bastian_mudde();
    AddSC_stormwind_prison();
    AddSC_instance_stormwind_prison();
}
