#include "ScriptMgr.h"
#include "Creature.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "Chat.h"
#include "WorldStatePackets.h"
#include "Log.h"

#include "stormwind_prison.h"

enum SPRWorldStates
{
    WORLDSTATE_SHOW_MONSTER_KILLED  = 50000,
    WORLDSTATE_MONSTER_KILLED       = 50001,
    WORLDSTATE_MONSTER_SPAWNED      = 50002
};

enum SPRSpells
{
    DAVID_BEAM_VISUAL   = 56312
};

Position const BuffSpawnPos[1] =
{
    { -1.083596f, 55.143078f, -27.550673f, 4.701599f }
};

DoorData const doorData[] =
{
    { GO_HOGGER_GATE,       BOSS_HOGGER,            DOOR_TYPE_ROOM },
    { GO_WING_A_DOOR_ENTRY, BOSS_JAN_LANGENBUDDE,   DOOR_TYPE_PASSAGE },
    { GO_WING_A_DOOR_ENTRY, BOSS_BASTIAN_MUDDE,     DOOR_TYPE_PASSAGE }
};

class instance_stormwind_prison : public InstanceMapScript
{
public:
    instance_stormwind_prison() : InstanceMapScript(StormwindPrisonScriptName, 35) { }

    struct instance_stormwind_prison_InstanceMapScript : public InstanceScript
    {
        instance_stormwind_prison_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadDoorData(doorData);

            _currentWorldStates[WORLDSTATE_SHOW_MONSTER_KILLED] = 1;
            _currentWorldStates[WORLDSTATE_MONSTER_KILLED] = 0;
            _currentWorldStates[WORLDSTATE_MONSTER_SPAWNED] = 0;
            _sentWorldStates = _currentWorldStates;

            _fireGauntletFinished = false;
            _gauntletLeverActivated = 0;
        }

        void FillInitialWorldStates(WorldPackets::WorldState::InitWorldStates& packet) override
        {
            for (WorldStateMap::const_iterator itr = _sentWorldStates.begin(); itr != _sentWorldStates.end(); ++itr)
                packet.Worldstates.emplace_back(itr->first, itr->second);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            if (!creature->IsSummon())
                SetData(DATA_MONSTER_SPAWNED, 1);

            switch (creature->GetEntry())
            {
            case NPC_HOGGER:
                HoggerGUID = creature->GetGUID();
                break;
            case NPC_JAN_LANGENBUDDE:
                JanLangenbuddeGUID = creature->GetGUID();
                break;
            case NPC_BASTIAN_MUDDE:
                BastianMuddeGUID = creature->GetGUID();
                break;
            }
        } 

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
            case GO_SPEEDBUFF_ENTRY:
                SpeedbuffGO = go->GetGUID();
                break;
            case GO_WING_A_DOOR_ENTRY:
                WingADoor = go->GetGUID();
                break;
            default:
                break;
            }

            InstanceScript::OnGameObjectCreate(go);
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (unit->GetTypeId() == TYPEID_PLAYER)
                return;

            if (!unit->IsSummon())
                SetData(DATA_MONSTER_KILLED, 1);


            /*if (unit->GetGUID() == HoggerGUID)
            {
                door->SetGoState(GO_STATE_ACTIVE);
            }*/
        }

        void Create() override
        {
            InstanceScript::Create();

            SetupBuffs();
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            switch (type)
            {
            case DATA_MONSTER_KILLED:
                SetWorldState(WORLDSTATE_MONSTER_KILLED, _currentWorldStates[WORLDSTATE_MONSTER_KILLED] + 1);
                if (_currentWorldStates[WORLDSTATE_MONSTER_KILLED] == _currentWorldStates[WORLDSTATE_MONSTER_SPAWNED])
                    SendAllKilledMessage();
                break;
            case DATA_MONSTER_SPAWNED:
                SetWorldState(WORLDSTATE_MONSTER_SPAWNED, _currentWorldStates[WORLDSTATE_MONSTER_SPAWNED] + 1);
                break;
            case DATA_FIRE_GAUNTLET_FINISHED:
                _fireGauntletFinished = true;
                if (Creature* creature = instance->GetCreature(BastianMuddeGUID))
                    creature->GetAI()->DoAction(ACTION_FINISH_GAUNTLET);
                break;
            case DATA_GAUNTLET_LEVER_ACTIVATED:
                if (++_gauntletLeverActivated == 4)
                    SetData(DATA_FIRE_GAUNTLET_FINISHED, 1);
                break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
            case DATA_FIRE_GAUNTLET_FINISHED:
                return _fireGauntletFinished;
            }
            return 0;
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
            case DATA_JAN_LANGENBUDDE:
                return JanLangenbuddeGUID;
            }
            return ObjectGuid::Empty;
        }

        void HandleTriggerBuff(ObjectGuid go_guid) override
        {
            GameObject* go = instance->GetGameObject(go_guid);
            if (!go || go->GetGoType() != GAMEOBJECT_TYPE_TRAP || !go->isSpawned())
                return;

            go->SetLootState(GO_JUST_DEACTIVATED);          
            go->SetRespawnTime(5);
        }

    private:
        typedef std::unordered_map<uint32, uint32> WorldStateMap;

        void SetupBuffs()
        {
            AddObject(SPRGameObjectIds::GO_BERSERKERBUFF_ENTRY, BuffSpawnPos[0]);
        }

        bool AddObject(SPRGameObjectIds goId, Position const& pos)
        {
            QuaternionData rotation = QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.f, 0.f);

            GameObject* go = new GameObject;

            if (!go->Create(instance->GenerateLowGuid<HighGuid::GameObject>(), goId, instance, PHASEMASK_NORMAL, pos, rotation, 255, GO_STATE_READY))
            {
                delete go;
                return false;
            }

            if (!instance->AddToMap(go))
            {
                delete go;
                return false;
            }

            SpeedbuffGO = go->GetGUID();

            return true;
        }

        void SetWorldState(SPRWorldStates state, uint32 value, bool immediate = true)
        {
            TC_LOG_DEBUG("scripts.cos", "instance_stormwind_prison::SetWorldState: {} {}", uint32(state), value);
            _currentWorldStates[state] = value;
            if (immediate)
                PropagateWorldStateUpdate();
        }

        void PropagateWorldStateUpdate()
        {
            TC_LOG_DEBUG("scripts.cos", "instance_stormwind_prison::PropagateWorldStateUpdate: Propagate world states");
            for (WorldStateMap::const_iterator it = _currentWorldStates.begin(); it != _currentWorldStates.end(); ++it)
            {
                uint32& sent = _sentWorldStates[it->first];
                if (sent != it->second)
                {
                    TC_LOG_DEBUG("scripts.cos", "instance_stormwind_prison::PropagateWorldStateUpdate: Sending world state {} ({})", it->first, it->second);
                    DoUpdateWorldState(it->first, it->second);
                    sent = it->second;
                }
            }
        }

        void SendAllKilledMessage()
        {
            for (MapReference const& ref : instance->GetPlayers())
                if (Player* player = ref.GetSource())
                {
                    ChatHandler chatHandle = ChatHandler(player->GetSession());
                    chatHandle.PSendSysMessage("You killed all Monsters!");
                }
        }

        ObjectGuid HoggerGUID;
        ObjectGuid JanLangenbuddeGUID;
        ObjectGuid BastianMuddeGUID;
        ObjectGuid SpeedbuffGO;
        ObjectGuid WingADoor;
        WorldStateMap _currentWorldStates;
        WorldStateMap _sentWorldStates;
        bool _fireGauntletFinished;
        uint8 _gauntletLeverActivated;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_stormwind_prison_InstanceMapScript(map);
    }
};

void AddSC_instance_stormwind_prison()
{
    new instance_stormwind_prison();
}
