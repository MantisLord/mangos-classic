/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Npcs_Special
SD%Complete: 100
SDComment: To be used for special NPCs that are located globally.
SDCategory: NPCs
EndScriptData

*/

#include "AI/ScriptDevAI/include/sc_common.h"
#include "AI/ScriptDevAI/base/escort_ai.h"
#include "Globals/ObjectMgr.h"
#include "GameEvents/GameEventMgr.h"
#include "AI/ScriptDevAI/base/TimerAI.h"

/* ContentData
npc_chicken_cluck       100%    support for quest 3861 (Cluck!)
npc_guardian            100%    guardianAI used to prevent players from accessing off-limits areas. Not in use by SD2
npc_garments_of_quests   80%    NPC's related to all Garments of-quests 5621, 5624, 5625, 5648, 5650
npc_injured_patient     100%    patients for triage-quests (6622 and 6624)
npc_doctor              100%    Gustaf Vanhowzen and Gregory Victor, quest 6622 and 6624 (Triage)
npc_innkeeper            25%    ScriptName not assigned. Innkeepers in general.
npc_redemption_target   100%    Used for the paladin quests: 1779,1781,9600,9685
EndContentData */

/*########
# npc_chicken_cluck
#########*/

enum
{
    EMOTE_CLUCK_TEXT1       = -1000204,
    EMOTE_CLUCK_TEXT2       = -1000205,

    QUEST_CLUCK             = 3861,
    FACTION_FRIENDLY        = 35,
    FACTION_CHICKEN         = 31
};

struct npc_chicken_cluckAI : public ScriptedAI
{
    npc_chicken_cluckAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    uint32 m_uiResetFlagTimer;

    void Reset() override
    {
        m_uiResetFlagTimer = 20000;

        m_creature->setFaction(FACTION_CHICKEN);
        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
    }

    void ReceiveEmote(Player* pPlayer, uint32 uiEmote) override
    {
        if (uiEmote == TEXTEMOTE_CHICKEN && !urand(0, 49))
        {
            m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            m_creature->setFaction(FACTION_FRIENDLY);
            DoScriptText(EMOTE_CLUCK_TEXT1, m_creature);
        }
        else if (uiEmote == TEXTEMOTE_CHEER && pPlayer->GetQuestStatus(QUEST_CLUCK) == QUEST_STATUS_COMPLETE)
        {
            m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            m_creature->setFaction(FACTION_FRIENDLY);
            DoScriptText(EMOTE_CLUCK_TEXT2, m_creature);
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        // Reset flags after a certain time has passed so that the next player has to start the 'event' again
        if (m_creature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
        {
            if (m_uiResetFlagTimer < uiDiff)
                EnterEvadeMode();
            else
                m_uiResetFlagTimer -= uiDiff;
        }

        if (m_creature->SelectHostileTarget() && m_creature->GetVictim())
            DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_npc_chicken_cluck(Creature* pCreature)
{
    return new npc_chicken_cluckAI(pCreature);
}

/*######
## Triage quest
######*/

enum
{
    SAY_DOC1                    = -1000201,
    SAY_DOC2                    = -1000202,
    SAY_DOC3                    = -1000203,

    QUEST_TRIAGE_H              = 6622,
    QUEST_TRIAGE_A              = 6624,

    DOCTOR_ALLIANCE             = 12939,
    DOCTOR_HORDE                = 12920,
    ALLIANCE_COORDS             = 7,
    HORDE_COORDS                = 6
};

struct Location
{
    float x, y, z, o;
};

static Location AllianceCoords[] =
{
    { -3757.38f, -4533.05f, 14.16f, 3.62f},                 // Top-far-right bunk as seen from entrance
    { -3754.36f, -4539.13f, 14.16f, 5.13f},                 // Top-far-left bunk
    { -3749.54f, -4540.25f, 14.28f, 3.34f},                 // Far-right bunk
    { -3742.10f, -4536.85f, 14.28f, 3.64f},                 // Right bunk near entrance
    { -3755.89f, -4529.07f, 14.05f, 0.57f},                 // Far-left bunk
    { -3749.51f, -4527.08f, 14.07f, 5.26f},                 // Mid-left bunk
    { -3746.37f, -4525.35f, 14.16f, 5.22f},                 // Left bunk near entrance
};

// alliance run to where
#define A_RUNTOX -3742.96f
#define A_RUNTOY -4531.52f
#define A_RUNTOZ 11.91f

static Location HordeCoords[] =
{
    { -1013.75f, -3492.59f, 62.62f, 4.34f},                 // Left, Behind
    { -1017.72f, -3490.92f, 62.62f, 4.34f},                 // Right, Behind
    { -1015.77f, -3497.15f, 62.82f, 4.34f},                 // Left, Mid
    { -1019.51f, -3495.49f, 62.82f, 4.34f},                 // Right, Mid
    { -1017.25f, -3500.85f, 62.98f, 4.34f},                 // Left, front
    { -1020.95f, -3499.21f, 62.98f, 4.34f}                  // Right, Front
};

// horde run to where
#define H_RUNTOX -1016.44f
#define H_RUNTOY -3508.48f
#define H_RUNTOZ 62.96f

const uint32 AllianceSoldierId[3] =
{
    12938,                                                  // 12938 Injured Alliance Soldier
    12936,                                                  // 12936 Badly injured Alliance Soldier
    12937                                                   // 12937 Critically injured Alliance Soldier
};

const uint32 HordeSoldierId[3] =
{
    12923,                                                  // 12923 Injured Soldier
    12924,                                                  // 12924 Badly injured Soldier
    12925                                                   // 12925 Critically injured Soldier
};

/*######
## npc_doctor (handles both Gustaf Vanhowzen and Gregory Victor)
######*/

struct npc_doctorAI : public ScriptedAI
{
    npc_doctorAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    ObjectGuid m_playerGuid;

    uint32 m_uiSummonPatientTimer;
    uint32 m_uiSummonPatientCount;
    uint32 m_uiPatientDiedCount;
    uint32 m_uiPatientSavedCount;

    bool m_bIsEventInProgress;

    GuidList m_lPatientGuids;
    std::vector<Location*> m_vPatientSummonCoordinates;

    void Reset() override
    {
        m_playerGuid.Clear();

        m_uiSummonPatientTimer = 10000;
        m_uiSummonPatientCount = 0;
        m_uiPatientDiedCount = 0;
        m_uiPatientSavedCount = 0;

        m_lPatientGuids.clear();
        m_vPatientSummonCoordinates.clear();

        m_bIsEventInProgress = false;

        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }

    void BeginEvent(Player* pPlayer);
    void PatientDied(Location* pPoint);
    void PatientSaved(Creature* pSoldier, Player* pPlayer, Location* pPoint);
    void UpdateAI(const uint32 uiDiff) override;
};

/*#####
## npc_injured_patient (handles all the patients, no matter Horde or Alliance)
#####*/

struct npc_injured_patientAI : public ScriptedAI
{
    npc_injured_patientAI(Creature* pCreature) : ScriptedAI(pCreature), isSaved(false) {Reset();}

    ObjectGuid m_doctorGuid;
    Location* m_pCoord;
    bool isSaved;

    void EnterEvadeMode() override
    {
        if (isSaved)
            ScriptedAI::EnterEvadeMode();
    }

    void Reset() override
    {
        m_doctorGuid.Clear();
        m_pCoord = nullptr;

        // no select
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        // no regen health
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
        // to make them lay with face down
        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);

        switch (m_creature->GetEntry())
        {
            // lower max health
            case 12923:
            case 12938:                                     // Injured Soldier
                m_creature->SetHealth(uint32(m_creature->GetMaxHealth()*.75));
                break;
            case 12924:
            case 12936:                                     // Badly injured Soldier
                m_creature->SetHealth(uint32(m_creature->GetMaxHealth()*.50));
                break;
            case 12925:
            case 12937:                                     // Critically injured Soldier
                m_creature->SetHealth(uint32(m_creature->GetMaxHealth()*.25));
                break;
        }
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
    {
        if (pCaster->GetTypeId() == TYPEID_PLAYER && m_creature->IsAlive() && pSpell->Id == 20804)
        {
            Player* pPlayer = static_cast<Player*>(pCaster);
            if (pPlayer->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE)
            {
                if (Creature* pDoctor = m_creature->GetMap()->GetCreature(m_doctorGuid))
                {
                    if (npc_doctorAI* pDocAI = dynamic_cast<npc_doctorAI*>(pDoctor->AI()))
                        pDocAI->PatientSaved(m_creature, pPlayer, m_pCoord);
                }
            }
            // make not selectable
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            // regen health
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
            // stand up
            m_creature->SetStandState(UNIT_STAND_STATE_STAND);

            switch (urand(0, 2))
            {
                case 0: DoScriptText(SAY_DOC1, m_creature); break;
                case 1: DoScriptText(SAY_DOC2, m_creature); break;
                case 2: DoScriptText(SAY_DOC3, m_creature); break;
            }

            m_creature->SetWalk(false);
            isSaved = true;

            switch (m_creature->GetEntry())
            {
                case 12923:
                case 12924:
                case 12925:
                    m_creature->GetMotionMaster()->MovePoint(0, H_RUNTOX, H_RUNTOY, H_RUNTOZ);
                    break;
                case 12936:
                case 12937:
                case 12938:
                    m_creature->GetMotionMaster()->MovePoint(0, A_RUNTOX, A_RUNTOY, A_RUNTOZ);
                    break;
            }
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        // Don't reduce health if already healed
        if (isSaved)
            return;

        // lower HP on every world tick makes it a useful counter, not officlone though
        uint32 uiHPLose = uint32(0.05f * uiDiff);
        if (m_creature->IsAlive() && m_creature->GetHealth() > 1 + uiHPLose)
        {
            m_creature->SetHealth(m_creature->GetHealth() - uiHPLose);
        }

        if (m_creature->IsAlive() && m_creature->GetHealth() <= 1 + uiHPLose)
        {
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_creature->SetDeathState(JUST_DIED);
            m_creature->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

            if (Creature* pDoctor = m_creature->GetMap()->GetCreature(m_doctorGuid))
            {
                if (npc_doctorAI* pDocAI = dynamic_cast<npc_doctorAI*>(pDoctor->AI()))
                    pDocAI->PatientDied(m_pCoord);
            }
        }
    }
};

UnitAI* GetAI_npc_injured_patient(Creature* pCreature)
{
    return new npc_injured_patientAI(pCreature);
}

/*
npc_doctor (continue)
*/

void npc_doctorAI::BeginEvent(Player* pPlayer)
{
    m_playerGuid = pPlayer->GetObjectGuid();

    m_uiSummonPatientTimer = 10000;
    m_uiSummonPatientCount = 0;
    m_uiPatientDiedCount = 0;
    m_uiPatientSavedCount = 0;

    switch (m_creature->GetEntry())
    {
        case DOCTOR_ALLIANCE:
            for (auto& AllianceCoord : AllianceCoords)
                m_vPatientSummonCoordinates.push_back(&AllianceCoord);
            break;
        case DOCTOR_HORDE:
            for (auto& HordeCoord : HordeCoords)
                m_vPatientSummonCoordinates.push_back(&HordeCoord);
            break;
    }

    m_bIsEventInProgress = true;
    m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
}

void npc_doctorAI::PatientDied(Location* pPoint)
{
    Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid);

    if (pPlayer && (pPlayer->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE))
    {
        ++m_uiPatientDiedCount;

        if (m_uiPatientDiedCount > 5)
        {
            if (pPlayer->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE)
                pPlayer->FailQuest(QUEST_TRIAGE_A);
            else if (pPlayer->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE)
                pPlayer->FailQuest(QUEST_TRIAGE_H);

            Reset();
            return;
        }

        m_vPatientSummonCoordinates.push_back(pPoint);
    }
    else
        // If no player or player abandon quest in progress
        Reset();
}

void npc_doctorAI::PatientSaved(Creature* /*soldier*/, Player* pPlayer, Location* pPoint)
{
    if (pPlayer && m_playerGuid == pPlayer->GetObjectGuid())
    {
        if (pPlayer->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE)
        {
            ++m_uiPatientSavedCount;

            if (m_uiPatientSavedCount == 15)
            {
                for (GuidList::const_iterator itr = m_lPatientGuids.begin(); itr != m_lPatientGuids.end(); ++itr)
                {
                    if (Creature* Patient = m_creature->GetMap()->GetCreature(*itr))
                        Patient->SetDeathState(JUST_DIED);
                }

                switch (m_creature->GetEntry())
                {
                    case DOCTOR_ALLIANCE: pPlayer->RewardPlayerAndGroupAtEventExplored(QUEST_TRIAGE_A, m_creature); break;
                    case DOCTOR_HORDE:    pPlayer->RewardPlayerAndGroupAtEventExplored(QUEST_TRIAGE_H, m_creature); break;
                    default:
                        script_error_log("Invalid entry for Triage doctor. Please check your database");
                        return;
                }

                Reset();
                return;
            }

            m_vPatientSummonCoordinates.push_back(pPoint);
        }
    }
}

void npc_doctorAI::UpdateAI(const uint32 uiDiff)
{
    if (m_bIsEventInProgress && m_uiSummonPatientCount >= 21)	// worst case scenario : 5 deads + 15 saved
    {
        Reset();
        return;
    }

    if (m_bIsEventInProgress && !m_vPatientSummonCoordinates.empty())
    {
        if (m_uiSummonPatientTimer < uiDiff)
        {
            std::vector<Location*>::iterator itr = m_vPatientSummonCoordinates.begin() + urand(0, m_vPatientSummonCoordinates.size() - 1);
            uint32 patientEntry = 0;

            switch (m_creature->GetEntry())
            {
                case DOCTOR_ALLIANCE: patientEntry = AllianceSoldierId[urand(0, 2)]; break;
                case DOCTOR_HORDE:    patientEntry = HordeSoldierId[urand(0, 2)];    break;
                default:
                    script_error_log("Invalid entry for Triage doctor. Please check your database");
                    return;
            }

            if (Creature* Patient = m_creature->SummonCreature(patientEntry, (*itr)->x, (*itr)->y, (*itr)->z, (*itr)->o, TEMPSPAWN_TIMED_OOC_OR_CORPSE_DESPAWN, 5000))
            {
                // 2.4.3, this flag appear to be required for client side item->spell to work (TARGET_UNIT_FRIEND)
                Patient->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP);

                m_lPatientGuids.push_back(Patient->GetObjectGuid());

                if (npc_injured_patientAI* pPatientAI = dynamic_cast<npc_injured_patientAI*>(Patient->AI()))
                {
                    pPatientAI->m_doctorGuid = m_creature->GetObjectGuid();
                    pPatientAI->m_pCoord = *itr;
                    m_vPatientSummonCoordinates.erase(itr);
                }
            }
            m_uiSummonPatientTimer = 10000;
            ++m_uiSummonPatientCount;
        }
        else
            m_uiSummonPatientTimer -= uiDiff;
    }
}

bool QuestAccept_npc_doctor(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if ((pQuest->GetQuestId() == QUEST_TRIAGE_A) || (pQuest->GetQuestId() == QUEST_TRIAGE_H))
    {
        if (npc_doctorAI* pDocAI = dynamic_cast<npc_doctorAI*>(pCreature->AI()))
            pDocAI->BeginEvent(pPlayer);
    }

    return true;
}

UnitAI* GetAI_npc_doctor(Creature* pCreature)
{
    return new npc_doctorAI(pCreature);
}

/*######
## npc_garments_of_quests
######*/

enum
{
    SPELL_LESSER_HEAL_R2    = 2052,
    SPELL_FORTITUDE_R1      = 1243,

    QUEST_MOON              = 5621,
    QUEST_LIGHT_1           = 5624,
    QUEST_LIGHT_2           = 5625,
    QUEST_SPIRIT            = 5648,
    QUEST_DARKNESS          = 5650,

    ENTRY_SHAYA             = 12429,
    ENTRY_ROBERTS           = 12423,
    ENTRY_DOLF              = 12427,
    ENTRY_KORJA             = 12430,
    ENTRY_DG_KEL            = 12428,

    SAY_COMMON_HEALED       = -1000231,
    SAY_DG_KEL_THANKS       = -1000232,
    SAY_DG_KEL_GOODBYE      = -1000233,
    SAY_ROBERTS_THANKS      = -1000256,
    SAY_ROBERTS_GOODBYE     = -1000257,
    SAY_KORJA_THANKS        = -1000258,
    SAY_KORJA_GOODBYE       = -1000259,
    SAY_DOLF_THANKS         = -1000260,
    SAY_DOLF_GOODBYE        = -1000261,
    SAY_SHAYA_THANKS        = -1000262,
    SAY_SHAYA_GOODBYE       = -1000263,
};

struct npc_garments_of_questsAI : public npc_escortAI
{
    npc_garments_of_questsAI(Creature* pCreature) : npc_escortAI(pCreature) { Reset(); }

    ObjectGuid m_playerGuid;

    bool m_bIsHealed;
    bool m_bCanRun;

    uint32 m_uiRunAwayTimer;

    void Reset() override
    {
        m_playerGuid.Clear();

        m_bIsHealed = false;
        m_bCanRun = false;

        m_uiRunAwayTimer = 5000;

        m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);
        // special pvp flag applies to Classic only
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP);
        // expect database to have RegenHealth=0
        m_creature->SetHealth(int(m_creature->GetMaxHealth() * 0.7));
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
    {
        if (pSpell->Id == SPELL_LESSER_HEAL_R2 || pSpell->Id == SPELL_FORTITUDE_R1)
        {
            // not while in combat
            if (m_creature->IsInCombat())
                return;

            // nothing to be done now
            if (m_bIsHealed && m_bCanRun)
                return;

            if (pCaster->GetTypeId() == TYPEID_PLAYER)
            {
                switch (m_creature->GetEntry())
                {
                    case ENTRY_SHAYA:
                        if (((Player*)pCaster)->GetQuestStatus(QUEST_MOON) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (m_bIsHealed && !m_bCanRun && pSpell->Id == SPELL_FORTITUDE_R1)
                            {
                                DoScriptText(SAY_SHAYA_THANKS, m_creature, pCaster);
                                m_bCanRun = true;
                            }
                            else if (!m_bIsHealed && pSpell->Id == SPELL_LESSER_HEAL_R2)
                            {
                                m_playerGuid = pCaster->GetObjectGuid();
                                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                                DoScriptText(SAY_COMMON_HEALED, m_creature, pCaster);
                                m_bIsHealed = true;
                            }
                        }
                        break;
                    case ENTRY_ROBERTS:
                        if (((Player*)pCaster)->GetQuestStatus(QUEST_LIGHT_1) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (m_bIsHealed && !m_bCanRun && pSpell->Id == SPELL_FORTITUDE_R1)
                            {
                                DoScriptText(SAY_ROBERTS_THANKS, m_creature, pCaster);
                                m_bCanRun = true;
                            }
                            else if (!m_bIsHealed && pSpell->Id == SPELL_LESSER_HEAL_R2)
                            {
                                m_playerGuid = pCaster->GetObjectGuid();
                                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                                DoScriptText(SAY_COMMON_HEALED, m_creature, pCaster);
                                m_bIsHealed = true;
                            }
                        }
                        break;
                    case ENTRY_DOLF:
                        if (((Player*)pCaster)->GetQuestStatus(QUEST_LIGHT_2) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (m_bIsHealed && !m_bCanRun && pSpell->Id == SPELL_FORTITUDE_R1)
                            {
                                DoScriptText(SAY_DOLF_THANKS, m_creature, pCaster);
                                m_bCanRun = true;
                            }
                            else if (!m_bIsHealed && pSpell->Id == SPELL_LESSER_HEAL_R2)
                            {
                                m_playerGuid = pCaster->GetObjectGuid();
                                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                                DoScriptText(SAY_COMMON_HEALED, m_creature, pCaster);
                                m_bIsHealed = true;
                            }
                        }
                        break;
                    case ENTRY_KORJA:
                        if (((Player*)pCaster)->GetQuestStatus(QUEST_SPIRIT) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (m_bIsHealed && !m_bCanRun && pSpell->Id == SPELL_FORTITUDE_R1)
                            {
                                DoScriptText(SAY_KORJA_THANKS, m_creature, pCaster);
                                m_bCanRun = true;
                            }
                            else if (!m_bIsHealed && pSpell->Id == SPELL_LESSER_HEAL_R2)
                            {
                                m_playerGuid = pCaster->GetObjectGuid();
                                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                                DoScriptText(SAY_COMMON_HEALED, m_creature, pCaster);
                                m_bIsHealed = true;
                            }
                        }
                        break;
                    case ENTRY_DG_KEL:
                        if (((Player*)pCaster)->GetQuestStatus(QUEST_DARKNESS) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (m_bIsHealed && !m_bCanRun && pSpell->Id == SPELL_FORTITUDE_R1)
                            {
                                DoScriptText(SAY_DG_KEL_THANKS, m_creature, pCaster);
                                m_bCanRun = true;
                            }
                            else if (!m_bIsHealed && pSpell->Id == SPELL_LESSER_HEAL_R2)
                            {
                                m_playerGuid = pCaster->GetObjectGuid();
                                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                                DoScriptText(SAY_COMMON_HEALED, m_creature, pCaster);
                                m_bIsHealed = true;
                            }
                        }
                        break;
                }

                // give quest credit, not expect any special quest objectives
                if (m_bCanRun)
                    ((Player*)pCaster)->TalkedToCreature(m_creature->GetEntry(), m_creature->GetObjectGuid());
            }
        }
    }

    void WaypointReached(uint32 /*uiPointId*/) override {}

    void UpdateEscortAI(const uint32 uiDiff) override
    {
        if (m_bCanRun && !m_creature->IsInCombat())
        {
            if (m_uiRunAwayTimer <= uiDiff)
            {
                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                {
                    switch (m_creature->GetEntry())
                    {
                        case ENTRY_SHAYA:   DoScriptText(SAY_SHAYA_GOODBYE, m_creature, pPlayer);   break;
                        case ENTRY_ROBERTS: DoScriptText(SAY_ROBERTS_GOODBYE, m_creature, pPlayer); break;
                        case ENTRY_DOLF:    DoScriptText(SAY_DOLF_GOODBYE, m_creature, pPlayer);    break;
                        case ENTRY_KORJA:   DoScriptText(SAY_KORJA_GOODBYE, m_creature, pPlayer);   break;
                        case ENTRY_DG_KEL:  DoScriptText(SAY_DG_KEL_GOODBYE, m_creature, pPlayer);  break;
                    }

                    Start(true);
                }
                else
                    EnterEvadeMode();                       // something went wrong

                m_uiRunAwayTimer = 30000;
            }
            else
                m_uiRunAwayTimer -= uiDiff;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_npc_garments_of_quests(Creature* pCreature)
{
    return new npc_garments_of_questsAI(pCreature);
}

/*######
## npc_guardian
######*/

#define SPELL_DEATHTOUCH                5

struct npc_guardianAI : public ScriptedAI
{
    npc_guardianAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    void Reset() override
    {
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    }

    void UpdateAI(const uint32 /*diff*/) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_creature->isAttackReady())
        {
            m_creature->CastSpell(m_creature->GetVictim(), SPELL_DEATHTOUCH, TRIGGERED_OLD_TRIGGERED);
            m_creature->resetAttackTimer();
        }
    }
};

UnitAI* GetAI_npc_guardian(Creature* pCreature)
{
    return new npc_guardianAI(pCreature);
}

/*########
# npc_innkeeper
#########*/

// Script applied to all innkeepers by npcflag.
// Are there any known innkeepers that does not hape the options in the below?
// (remember gossipHello is not called unless npcflag|1 is present)

enum
{
    TEXT_ID_WHAT_TO_DO              = 1853,

    SPELL_TRICK_OR_TREAT            = 24751,                // create item or random buff
    SPELL_TRICK_OR_TREATED          = 24755,                // buff player get when tricked or treated
};

#define GOSSIP_ITEM_TRICK_OR_TREAT  "Trick or Treat!"
#define GOSSIP_ITEM_WHAT_TO_DO      "What can I do at an Inn?"

bool GossipHello_npc_innkeeper(Player* pPlayer, Creature* pCreature)
{
    pPlayer->PrepareGossipMenu(pCreature, pPlayer->GetDefaultGossipMenuForSource(pCreature));

    if (IsHolidayActive(HOLIDAY_HALLOWS_END) && !pPlayer->HasAura(SPELL_TRICK_OR_TREATED, EFFECT_INDEX_0))
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_TRICK_OR_TREAT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

    // Should only apply to innkeeper close to start areas.
    if (AreaTableEntry const* pAreaEntry = GetAreaEntryByAreaID(pCreature->GetAreaId()))
    {
        // Note: this area flag doesn't exist in 1.12.1. The behavior of this gossip require additional research
        //if (pAreaEntry->flags & AREA_FLAG_LOWLEVEL)
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_WHAT_TO_DO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
    }

    pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());
    pPlayer->SendPreparedGossip(pCreature);
    return true;
}

bool GossipSelect_npc_innkeeper(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
    switch (uiAction)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            pPlayer->SEND_GOSSIP_MENU(TEXT_ID_WHAT_TO_DO, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            pPlayer->CLOSE_GOSSIP_MENU();
            pCreature->CastSpell(pPlayer, SPELL_TRICK_OR_TREAT, TRIGGERED_OLD_TRIGGERED);
            break;
        case GOSSIP_OPTION_VENDOR:
            pPlayer->SEND_VENDORLIST(pCreature->GetObjectGuid());
            break;
        case GOSSIP_OPTION_INNKEEPER:
            pPlayer->CLOSE_GOSSIP_MENU();
            pPlayer->SetBindPoint(pCreature->GetObjectGuid());
            break;
    }

    return true;
}

/*######
## npc_redemption_target
######*/

enum
{
    SAY_HEAL                    = -1000187,

    SPELL_SYMBOL_OF_LIFE        = 8593,
    SPELL_SHIMMERING_VESSEL     = 31225,
    SPELL_REVIVE_SELF           = 32343,

    NPC_FURBOLG_SHAMAN          = 17542,        // draenei side
    NPC_BLOOD_KNIGHT            = 17768,        // blood elf side
};

struct npc_redemption_targetAI : public ScriptedAI
{
    npc_redemption_targetAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiEvadeTimer;
    uint32 m_uiHealTimer;

    ObjectGuid m_playerGuid;

    void Reset() override
    {
        m_uiEvadeTimer = 0;
        m_uiHealTimer  = 0;

        m_creature->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
    }

    void DoReviveSelf(ObjectGuid m_guid)
    {
        // Wait until he resets again
        if (m_uiEvadeTimer)
            return;

        DoCastSpellIfCan(m_creature, SPELL_REVIVE_SELF);
        m_creature->SetDeathState(JUST_ALIVED);
        m_playerGuid = m_guid;
        m_uiHealTimer = 2000;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_uiHealTimer)
        {
            if (m_uiHealTimer <= uiDiff)
            {
                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                {
                    DoScriptText(SAY_HEAL, m_creature, pPlayer);

                    // Quests 9600 and 9685 requires kill credit
                    if (m_creature->GetEntry() == NPC_FURBOLG_SHAMAN || m_creature->GetEntry() == NPC_BLOOD_KNIGHT)
                        pPlayer->KilledMonsterCredit(m_creature->GetEntry(), m_creature->GetObjectGuid());
                }

                m_creature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                m_uiHealTimer = 0;
                m_uiEvadeTimer = 2 * MINUTE * IN_MILLISECONDS;
            }
            else
                m_uiHealTimer -= uiDiff;
        }

        if (m_uiEvadeTimer)
        {
            if (m_uiEvadeTimer <= uiDiff)
            {
                EnterEvadeMode();
                m_uiEvadeTimer = 0;
            }
            else
                m_uiEvadeTimer -= uiDiff;
        }
    }
};

UnitAI* GetAI_npc_redemption_target(Creature* pCreature)
{
    return new npc_redemption_targetAI(pCreature);
}

bool EffectDummyCreature_npc_redemption_target(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Creature* pCreatureTarget, ObjectGuid /*originalCasterGuid*/)
{
    // always check spellid and effectindex
    if ((uiSpellId == SPELL_SYMBOL_OF_LIFE || uiSpellId == SPELL_SHIMMERING_VESSEL) && uiEffIndex == EFFECT_INDEX_0)
    {
        if (npc_redemption_targetAI* pTargetAI = dynamic_cast<npc_redemption_targetAI*>(pCreatureTarget->AI()))
            pTargetAI->DoReviveSelf(pCaster->GetObjectGuid());

        // always return true when we are handling this spell and effect
        return true;
    }

    return false;
}

/*######
## npc_the_cleaner
######*/
enum
{
    SPELL_IMMUNITY      = 29230,
    SAY_CLEANER_AGGRO   = -1001253
};

struct npc_the_cleanerAI : public ScriptedAI
{
    npc_the_cleanerAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiDespawnTimer;

    void Reset() override
    {
        DoCastSpellIfCan(m_creature, SPELL_IMMUNITY, CAST_TRIGGERED);
        m_uiDespawnTimer = 3000;
    }

    void Aggro(Unit* pWho) override
    {
        DoScriptText(SAY_CLEANER_AGGRO, m_creature);
    }

    void EnterEvadeMode() override
    {
        ScriptedAI::EnterEvadeMode();

        m_creature->ForcedDespawn();
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_uiDespawnTimer < uiDiff)
        {
            if (m_creature->getThreatManager().getThreatList().empty())
                m_creature->ForcedDespawn();
        }
        else
            m_uiDespawnTimer -= uiDiff;

        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_npc_the_cleaner(Creature* pCreature)
{
    return new npc_the_cleanerAI(pCreature);
}

/*######
## npc_aoe_damage_trigger
######*/

enum npc_aoe_damage_trigger
{
    // trigger npcs
    NPC_VOID_ZONE = 16697,

    // m_uiAuraPassive
    SPELL_CONSUMPTION_NPC_16697 = 28874,
};

struct npc_aoe_damage_triggerAI : public ScriptedAI, public TimerManager
{
    npc_aoe_damage_triggerAI(Creature* pCreature) : ScriptedAI(pCreature), m_uiAuraPassive(SetAuraPassive())
    {
        AddCustomAction(1, GetTimer(), [&]() {CastConsumption(); });
        SetReactState(REACT_PASSIVE);
    }

    uint32 m_uiAuraPassive;

    inline uint32 GetTimer()
    {
        return 2500; // adjust in future if other void zones are different this is for NPC_VOID_ZONE
    }

    inline uint32 SetAuraPassive()
    {
        switch (m_creature->GetCreatureInfo()->Entry)
        {
            case NPC_VOID_ZONE:
                return SPELL_CONSUMPTION_NPC_16697;
            default:
                return SPELL_CONSUMPTION_NPC_16697;
        }
    }

    void CastConsumption()
    {
        DoCastSpellIfCan(m_creature, m_uiAuraPassive, CAST_TRIGGERED | CAST_AURA_NOT_PRESENT);
    }

    void Reset() override {}

    void UpdateAI(const uint32 diff) override
    {
        UpdateTimers(diff);
    }
};

enum
{
    GOSSIP_TEXT_CRYSTAL = 50400,
    GOSSIP_TEXT_CRYSTAL_2 = 50401,
};

bool GossipHello_npc_enchantment_crystal(Player* pPlayer, Creature* pCreature)
{
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Head", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Shoulder", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Cloak", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Chest", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Wrist", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Hands", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Legs", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Feet", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "2H Weapon", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 11);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Mainhand Weapon", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 12);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Offhand", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Ranged", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 16);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Shield", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 17);

    pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL, pCreature->GetObjectGuid());
    return true;
}

bool GossipSelect_npc_enchantment_crystal(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
    {
        switch (pPlayer->getClass())
        {
            case CLASS_PALADIN:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/7 defense/24 healing", EQUIPMENT_SLOT_HEAD, 24160);
                break;
            case CLASS_MAGE:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "18 spell damage and healing/1% spell hit", EQUIPMENT_SLOT_HEAD, 24164);
                break;
            case CLASS_PRIEST:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/4 mp5/24 healing", EQUIPMENT_SLOT_HEAD, 24167);
                break;
            case CLASS_WARLOCK:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/18 spell damage and healing", EQUIPMENT_SLOT_HEAD, 24165);
                break;
            case CLASS_DRUID:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/10 intellect/24 healing", EQUIPMENT_SLOT_HEAD, 24168);
                break;
            case CLASS_HUNTER:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "24 ranged attack power/10 stamina/1% crit", EQUIPMENT_SLOT_HEAD, 24162);
                break;
            case CLASS_SHAMAN:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 intellect/13 spell damage and healing", EQUIPMENT_SLOT_HEAD, 24163);
                break;
            case CLASS_WARRIOR:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/7 defense/15 shield block", EQUIPMENT_SLOT_HEAD, 24149);
                break;
        }
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "100 hp", EQUIPMENT_SLOT_HEAD, 15389);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "150 mana", EQUIPMENT_SLOT_HEAD, 15340);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "125 armor", EQUIPMENT_SLOT_HEAD, 15391);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "8 strength", EQUIPMENT_SLOT_HEAD, 15397);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "8 healing", EQUIPMENT_SLOT_HEAD, 22844);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 nature resistance", EQUIPMENT_SLOT_HEAD, 28161);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "20 fire resistance", EQUIPMENT_SLOT_HEAD, 15394);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "1% dodge", EQUIPMENT_SLOT_HEAD, 22846);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "1% haste", EQUIPMENT_SLOT_HEAD, 22840);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 2)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "33 healing", EQUIPMENT_SLOT_SHOULDERS, 24420);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "18 spell damage and healing", EQUIPMENT_SLOT_SHOULDERS, 24421);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "30 attack power", EQUIPMENT_SLOT_SHOULDERS, 24422);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "5 all resistance", EQUIPMENT_SLOT_SHOULDERS, 22599);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 3)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 fire resistance", EQUIPMENT_SLOT_BACK, 25081);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 nature resistance", EQUIPMENT_SLOT_BACK, 25082);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 shadow resistance", EQUIPMENT_SLOT_BACK, 13522);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "1% dodge", EQUIPMENT_SLOT_BACK, 25086);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "increase stealth", EQUIPMENT_SLOT_BACK, 25083);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "decrease threat 2%", EQUIPMENT_SLOT_BACK, 25084);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "3 agility", EQUIPMENT_SLOT_BACK, 13882);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "70 armor", EQUIPMENT_SLOT_BACK, 20015);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "3 all resistances", EQUIPMENT_SLOT_BACK, 13794);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 4)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "4 to all stats", EQUIPMENT_SLOT_CHEST, 20025);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "100 hp", EQUIPMENT_SLOT_CHEST, 20073);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "100 mana", EQUIPMENT_SLOT_CHEST, 20028);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "lesser absorption", EQUIPMENT_SLOT_CHEST, 13538);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 5)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "9 stamina", EQUIPMENT_SLOT_WRISTS, 20011);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "9 strength", EQUIPMENT_SLOT_WRISTS, 20010);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "7 intellect", EQUIPMENT_SLOT_WRISTS, 20008);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "4 mp5", EQUIPMENT_SLOT_WRISTS, 23801);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "24 healing", EQUIPMENT_SLOT_WRISTS, 25079);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "1 agility", EQUIPMENT_SLOT_WRISTS, 7779);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "3 defense", EQUIPMENT_SLOT_WRISTS, 13931);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "9 spirit", EQUIPMENT_SLOT_WRISTS, 20009);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 6)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "30 healing", EQUIPMENT_SLOT_HANDS, 25079);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "20 shadow spell damage", EQUIPMENT_SLOT_HANDS, 25073);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "20 frost spell damage", EQUIPMENT_SLOT_HANDS, 25074);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "20 fire spell damage", EQUIPMENT_SLOT_HANDS, 25078);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 agility", EQUIPMENT_SLOT_HANDS, 25080);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "2% threat", EQUIPMENT_SLOT_HANDS, 25072);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "1% attack speed", EQUIPMENT_SLOT_HANDS, 13948);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "7 strength", EQUIPMENT_SLOT_HANDS, 20013);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "5 herbalism", EQUIPMENT_SLOT_HANDS, 13868);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "5 mining", EQUIPMENT_SLOT_HANDS, 13841);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "5 skinning", EQUIPMENT_SLOT_HANDS, 13698);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "minor mount speed", EQUIPMENT_SLOT_HANDS, 13947);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 7)
    {
        switch (pPlayer->getClass())
        {
            case CLASS_PALADIN:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/7 defense/24 healing", EQUIPMENT_SLOT_LEGS, 24160);
                break;
            case CLASS_MAGE:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "18 spell damage and healing/1% spell hit", EQUIPMENT_SLOT_LEGS, 24164);
                break;
            case CLASS_PRIEST:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/4 mp5/24 healing", EQUIPMENT_SLOT_LEGS, 24167);
                break;
            case CLASS_WARLOCK:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/18 spell damage and healing", EQUIPMENT_SLOT_LEGS, 24165);
                break;
            case CLASS_DRUID:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/10 intellect/24 healing", EQUIPMENT_SLOT_LEGS, 24168);
                break;
            case CLASS_HUNTER:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "24 ranged attack power/10 stamina/1% crit", EQUIPMENT_SLOT_LEGS, 24162);
                break;
            case CLASS_SHAMAN:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 intellect/13 spell damage and healing", EQUIPMENT_SLOT_LEGS, 24163);
                break;
            case CLASS_WARRIOR:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 stamina/7 defense/15 shield block", EQUIPMENT_SLOT_LEGS, 24149);
                break;
        }
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "100 hp", EQUIPMENT_SLOT_LEGS, 15389);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "150 mana", EQUIPMENT_SLOT_LEGS, 15340);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "125 armor", EQUIPMENT_SLOT_LEGS, 15391);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "8 strength", EQUIPMENT_SLOT_LEGS, 15397);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "8 healing", EQUIPMENT_SLOT_LEGS, 22844);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "10 nature resistance", EQUIPMENT_SLOT_LEGS, 28161);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "20 fire resistance", EQUIPMENT_SLOT_LEGS, 15394);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "1% dodge", EQUIPMENT_SLOT_LEGS, 22846);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "1% haste", EQUIPMENT_SLOT_LEGS, 22840);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 8)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "minor speed", EQUIPMENT_SLOT_FEET, 13890);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "7 agility", EQUIPMENT_SLOT_FEET, 20023);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "7 stamina", EQUIPMENT_SLOT_FEET, 20020);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "5 spirit", EQUIPMENT_SLOT_FEET, 20024);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 11)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "25 agility", EQUIPMENT_SLOT_MAINHAND, 27837);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "9 physical damage", EQUIPMENT_SLOT_MAINHAND, 20030);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "9 intellect", EQUIPMENT_SLOT_MAINHAND, 20036);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "9 spirit", EQUIPMENT_SLOT_MAINHAND, 20035);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 12)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Crusader", EQUIPMENT_SLOT_MAINHAND, 20034);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Fiery Weapon", EQUIPMENT_SLOT_MAINHAND, 13898);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "30 spell damage", EQUIPMENT_SLOT_MAINHAND, 22749);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "55 healing", EQUIPMENT_SLOT_MAINHAND, 22750);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 agility", EQUIPMENT_SLOT_MAINHAND, 23800);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Next Page ->", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 13);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 13)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 strength", EQUIPMENT_SLOT_MAINHAND, 23799);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Icy Chill", EQUIPMENT_SLOT_MAINHAND, 20029);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Lifestealing", EQUIPMENT_SLOT_MAINHAND, 20032);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "20 spirit", EQUIPMENT_SLOT_MAINHAND, 23803);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "22 intellect", EQUIPMENT_SLOT_MAINHAND, 23804);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Previous Page", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 12);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 14)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Crusader", EQUIPMENT_SLOT_OFFHAND, 20034);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Fiery Weapon", EQUIPMENT_SLOT_OFFHAND, 13898);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "30 spell damage", EQUIPMENT_SLOT_OFFHAND, 22749);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "55 healing", EQUIPMENT_SLOT_OFFHAND, 22750);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 agility", EQUIPMENT_SLOT_OFFHAND, 23800);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Next Page ->", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 15);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 15)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "15 strength", EQUIPMENT_SLOT_OFFHAND, 23799);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Icy Chill", EQUIPMENT_SLOT_OFFHAND, 20029);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Lifestealing", EQUIPMENT_SLOT_OFFHAND, 20032);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "20 spirit", EQUIPMENT_SLOT_OFFHAND, 23803);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "22 intellect", EQUIPMENT_SLOT_OFFHAND, 23804);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Previous Page", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu ", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 16)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "7 damage", EQUIPMENT_SLOT_RANGED, 12460);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "3% hit", EQUIPMENT_SLOT_RANGED, 22779);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu ", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 17)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "Thorium Shield Spike", EQUIPMENT_SLOT_OFFHAND, 16623);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "7 stamina", EQUIPMENT_SLOT_OFFHAND, 20017);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "8 frost resistance", EQUIPMENT_SLOT_OFFHAND, 13933);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Main Menu", 0, 0);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_CRYSTAL_2, pCreature->GetObjectGuid());
    }
    else if (uiAction == GOSSIP_ACTION_INFO_DEF + 0)
        GossipHello_npc_enchantment_crystal(pPlayer, pCreature);
    else
    {
        pPlayer->EnchantItem(uiAction, uiSender);
        GossipHello_npc_enchantment_crystal(pPlayer, pCreature);
    }
    return true;
}


enum
{
    SPELL_JOURNEYMAN_RIDING = 33392,
    SPELL_ARTISAN_FIRST_AID = 10847,
    SPELL_HEAVY_RUNECLOTH_BAND = 18632,

    GOSSIP_MENU_OVERLORD_MAIN = 50414,
};

const std::vector<uint32> Level1StartingGear = { 25,35,36,37,39,40,43,44,47,48,51,52,55,56,57,59,117,120,121,129,139,140,147,153,159,1395,1396,2070,2092,2101,2102,2361,2362,2504,2508,2512,2516,3661,4536,4540,4604,6098,6116,6118,6119,6121,6122,6123,6124,6126,6127,6129,6135,6137,6138,6139,6140,6144,12282,20857,20891,20892,20893,20894,20895,20896,20898,20899,20900,20980,20981,20982,20983,23322,23344,23346,23347,23348,23474,23475,23477,23478,23479,24145,24146,25861,28979 };

struct CustomTeleportLocation
{
    uint32 map;
    float x, y, z, o;
    uint32 area;
};
static const CustomTeleportLocation teleLocs[] =
{
    {0, -11792.108398f, -3226.608154f, -29.721224f, 2.613495f, 0},  // Dark Portal - Alliance
    {0, -11774.694336f, -3184.537598f, -28.923182f, 2.749808f, 0},  // Dark Portal - Horde

    {0, -8931.93f, -132.83f, 82.88f, 3.26f, 0},                     // Northshire (Human)
    {0, -6213.47f, 330.64f, 383.68f, 3.15f, 0},                     // Coldridge Valley (Dwarf and Gnome)
    {1, 10317.40f, 821.00f, 1326.37f, 2.15f, 0},                    // Shadowglen (Night Elf)

    {1, -607.47f, -4248.13f, 38.95f, 3.27f, 0},                     // Valley of Trials (Orc and Troll)
    {0, 1656.367f, 1682.592f, 120.78681f, 0.06632f, 0},             // Deathknell (Undead)
    {1, -2913.71f, -254.67f, 52.94f, 3.70f, 0},                     // Camp Narache (Tauren)
};

struct npc_test_realm_overlord : public ScriptedAI
{
    npc_test_realm_overlord(Creature* creature) : ScriptedAI(creature) { Reset(); }

    void Reset() override
    {
    }

    void UpdateAI(const uint32 diff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        DoMeleeAttackIfReady();
    }

    uint32 GetStarterMountForRace(Player* player)
    {
        uint32 mountEntry = 0;
        switch (player->getRace())
        {
        case RACE_HUMAN: mountEntry = 2411; break;      // Black Stallion Bridle
        case RACE_DWARF: mountEntry = 5872; break;      // Brown Ram
        case RACE_NIGHTELF: mountEntry = 8632; break;   // Reins of the Spotted Frostsaber
        case RACE_GNOME: mountEntry = 8595; break;      // Blue Mechanostrider
        case RACE_ORC: mountEntry = 5665; break;        // Horn of the Dire Wolf
        case RACE_UNDEAD: mountEntry = 13331; break;    // Red Skeletal Horse
        case RACE_TAUREN: mountEntry = 15277; break;    // Gray Kodo
        case RACE_TROLL: mountEntry = 8588; break;      // Whistle of the Emerald Raptor
        }
        return mountEntry;
    }

    uint32 GetStarterEpicMountForRace(Player* player)
    {
        uint32 mountEntry = 0;
        switch (player->getRace())
        {
        case RACE_HUMAN: mountEntry = 18776; break;     // Swift Palomino
        case RACE_DWARF: mountEntry = 18787; break;     // Swift Gray Ram
        case RACE_NIGHTELF: mountEntry = 18767; break;  // Reins of the Swift Mistsaber
        case RACE_GNOME: mountEntry = 18772; break;     // Swift Green Mechanostrider
        case RACE_ORC: mountEntry = 18797; break;       // Horn of the Swift Timber Wolf 
        case RACE_UNDEAD: mountEntry = 18791; break;    // Purple Skeletal Warhorse
        case RACE_TAUREN: mountEntry = 18795; break;    // Great Gray Kodo
        case RACE_TROLL: mountEntry = 18790; break;     // Swift Orange Raptor
        }
        return mountEntry;
    }

    void BoostPlayer(Player* player, uint32 targetLevel)
    {
        if (player->getLevel() < targetLevel)
        {
            player->GiveLevel(targetLevel);
            player->SetUInt32Value(PLAYER_XP, 0);
            player->learnClassLevelSpells(true);
            player->UpdateSkillsForLevel(true);

            if (player->getClass() == CLASS_HUNTER)
            {
                if (Pet* pet = player->GetPet())
                {
                    if (pet->getLevel() < targetLevel)
                    {
                        pet->GivePetLevel(targetLevel);
                        pet->SavePetToDB(PET_SAVE_AS_CURRENT, player);
                    }
                }
            }
        }

        // Learn skills
        player->CastSpell(player, SPELL_JOURNEYMAN_RIDING, TRIGGERED_OLD_TRIGGERED);
        player->CastSpell(player, SPELL_ARTISAN_FIRST_AID, TRIGGERED_OLD_TRIGGERED);
        player->CastSpell(player, SPELL_HEAVY_RUNECLOTH_BAND, TRIGGERED_OLD_TRIGGERED);

        SkillLineEntry const* sl = sSkillLineStore.LookupEntry(SKILL_FIRST_AID);
        if (sl)
        {
            uint32 maxSkill = player->GetSkillMaxPure(SKILL_FIRST_AID);

            if (player->GetSkillValue(SKILL_FIRST_AID) < maxSkill)
                player->SetSkill(SKILL_FIRST_AID, maxSkill, maxSkill);
        }

        // Remove any gear the character still has from initial creation (now useless)
        for (uint32 itemEntry : Level1StartingGear)
            player->DestroyItemCount(itemEntry, 200, true, false);

        // Onyxia Hide Backpack x4
        if (!player->HasItemCount(17966, 4)) player->StoreNewItemInBestSlots(17966, 4);

        // Epic Ground Mount
        uint32 groundMount = GetStarterEpicMountForRace(player);
        if (!player->HasItemCount(groundMount, 1)) player->StoreNewItemInBestSlots(groundMount, 1);

        player->SaveToDB();
    }
};

bool GossipHello_npc_test_realm_overlord(Player* player, Creature* creature)
{
    player->PrepareGossipMenu(creature, GOSSIP_MENU_OVERLORD_MAIN);
    if (player->getLevel() < 60)
        player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_INTERACT_1, "|cFF00008BBoost to level 60|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3, "Are you sure?", 0, false);
    else {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, "Teleport to Blasted Lands (Dark Portal)", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
    }

    player->SendPreparedGossip(creature);
    return true;
}

bool GossipSelect_npc_test_realm_overlord(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
{
    if (npc_test_realm_overlord* testRealmOverlordAI = dynamic_cast<npc_test_realm_overlord*>(creature->AI()))
    {
        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF + 3:
        {
            player->CLOSE_GOSSIP_MENU();
            testRealmOverlordAI->BoostPlayer(player, 60);
            // Teleport Player to Dark Portal
            if (player->GetTeam() == ALLIANCE)
                player->TeleportTo(teleLocs[0].map, teleLocs[0].x, teleLocs[0].y, teleLocs[0].z, teleLocs[0].o);
            else
                player->TeleportTo(teleLocs[1].map, teleLocs[1].x, teleLocs[1].y, teleLocs[1].z, teleLocs[1].o);
            break;
        }
        // Teleport Only - Dark Portal
        case GOSSIP_ACTION_INFO_DEF + 6:
        {
            player->CLOSE_GOSSIP_MENU();
            // Teleport Player to Dark Portal
            if (player->GetTeam() == ALLIANCE)
                player->TeleportTo(teleLocs[0].map, teleLocs[0].x, teleLocs[0].y, teleLocs[0].z, teleLocs[0].o);
            else
                player->TeleportTo(teleLocs[1].map, teleLocs[1].x, teleLocs[1].y, teleLocs[1].z, teleLocs[1].o);
            break;
        }
        }
    }

    return false;
}

enum
{
    SPELL_CYCLONE_VISUAL_SPAWN  = 8609,

    GOSSIP_TEXT_PET_MENU        = 50406,
    GOSSIP_TEXT_GREET           = 50407,
    GOSSIP_TEXT_REFUSE_LOW      = 50408,
    GOSSIP_TEXT_PICK_SPEC       = 50409,

    NPC_PET_BAT     = 8600, // Plaguebat
    NPC_PET_BEAR    = 7443, // Shardtooth Mauler
    NPC_PET_BOAR    = 5992, // Ashmane Boar
    NPC_PET_CARRION = 1809, // Carrion Vulture
    NPC_PET_CAT     = 1713, // Elder Shadowmaw Panther
    NPC_PET_CRAB    = 1088, // Monstrous Crawler
    NPC_PET_CROC    = 1087, // Sawtooth Snapper
    NPC_PET_GORILLA = 6516, // Un'Goro Thunderer
    NPC_PET_HYENA   = 5986, // Rabid Snickerfang
    NPC_PET_OWL     = 7455, // Winterspring Owl
    NPC_PET_RAPTOR  = 687,  // Jungle Stalker
    NPC_PET_STRIDER = 4725, // Crazed Sandstrider
    NPC_PET_SCORPID = 9695, // Deathlash Scorpid
    NPC_PET_SPIDER  = 8933, // Cave Creeper
    NPC_PET_SERPENT = 5308, // Rogue Vale Screecher
    NPC_PET_TURTLE  = 6369, // Coralshell Tortoise
    NPC_PET_WOLF    = 9697  // Giant Ember Worg
};

// "Best In Slot"
const std::vector<uint32> Lvl60BiS_ShamanResto = { 22466, 21712, 22467, 21583, 22464, 22471, 22465, 22469, 22470, 22468, 23065, 21620, 23047, 19395, 23056, 22819, 22396 };
const std::vector<uint32> Lvl60BiS_ShamanEnhancement = { 18817, 18404, 21684, 23045, 21374, 21602, 21624, 21586, 21651, 21705, 17063, 23038, 22321, 19289, 22798, 22395 };
const std::vector<uint32> Lvl60BiS_ShamanElemental = { 19375, 22943, 21376, 23050, 21671, 21464, 21585, 22730, 21375, 21373, 21707, 21709, 19379, 23046, 22988, 23049, 23199 };
const std::vector<uint32> Lvl60BiS_PriestShadow = { 23035, 18814, 22983, 22731, 23220, 21611, 19133, 21585, 19400, 19131, 21709, 19434, 19379, 23046, 22988, 23049, 22820 };
const std::vector<uint32> Lvl60BiS_PriestDiscHoly = { 21615, 21712, 22515, 22960, 22512, 21604, 22513, 22517, 21582, 22516, 23061, 22939, 23027, 23047, 23056, 23048, 23009 };
const std::vector<uint32> Lvl60BiS_PaladinHoly = { 19375, 23057, 22429, 23050, 22425, 21604, 22427, 20264, 21582, 22430, 23066, 19382, 19395, 23047, 23056, 23075, 23006 };
const std::vector<uint32> Lvl60BiS_PaladinRetribution = { 21387, 18404, 21391, 23045, 21389, 22936, 21623, 23219, 21390, 21388, 23038, 17063, 22321, 19289, 22691, 23203, 17182 };
const std::vector<uint32> Lvl60BiS_PaladinProtection = { 21387, 22732, 21639, 22938, 21389, 20616, 21674, 21598, 19855, 21706, 19376, 21601, 19431, 18406, 22988, 23043, 22401, 19019 };
const std::vector<uint32> Lvl60BiS_WarriorFuryArms = { 12640, 23053, 21330, 23045, 23000, 22936, 21581, 23219, 23068, 19387, 18821, 23038, 22954, 23041, 23054, 23577, 17076, 22812 };
const std::vector<uint32> Lvl60BiS_WarriorProtection = { 22418, 22732, 22419, 22938, 22416, 22423, 22421, 22422, 22417, 22420, 23059, 21601, 19431, 19406, 19019, 23043, 19368 };
const std::vector<uint32> Lvl60BiS_DruidFeralCat = { 8345, 19377, 21665, 21710, 23226, 21602, 21672, 21586, 23071, 21493, 23038, 19432, 19406, 23041, 22988, 22632, 22397, 13385 };
const std::vector<uint32> Lvl60BiS_DruidFeralBear = { 21693, 22732, 19389, 22938, 23226, 21602, 21605, 21675, 20627, 19381, 21601, 23018, 13966, 11811, 943, 23198 };
const std::vector<uint32> Lvl60BiS_DruidBalance = { 19375, 23057, 22983, 23050, 19682, 23021, 21585, 22730, 19683, 19684, 23025, 21709, 19379, 23046, 22988, 23049, 23197 };
const std::vector<uint32> Lvl60BiS_DruidResto = { 20628, 21712, 22491, 22960, 22488, 21604, 22493, 21582, 22489, 22492, 22939, 21620, 23047, 23027, 23056, 22632, 22399, 23048 };
const std::vector<uint32> Lvl60BiS_Rogue = { 22478, 19377, 22479, 23045, 22476, 22483, 22477, 22481, 22482, 22480, 23060, 23038, 23041, 22954, 23054, 22802, 21126, 23577, 22812, 19019 };
const std::vector<uint32> Lvl60BiS_Mage = { 22498, 23057, 22983, 23050, 22496, 23021, 23070, 21585, 22730, 22500, 23062, 23237, 19379, 23046, 19339, 22807, 23049, 22821 };
const std::vector<uint32> Lvl60BiS_Hunter = { 22438, 23053, 22439, 23045, 22436, 22443, 22437, 22441, 22442, 22440, 23067, 22961, 23041, 19406, 22816, 22802, 22812 };
const std::vector<uint32> Lvl60BiS_Warlock = { 22506, 23057, 22507, 23050, 22504, 21186, 23070, 21585, 22730, 22508, 21709, 23025, 19379, 23046, 22807, 23049, 22820 };

const std::vector<uint32> TaxiNodesAlliance =
{
    // Eastern Kingdoms:
    2,     // Stormwind, Elwynn
    4,     // Sentinel Hill, Westfall
    5,     // Lakeshire, Redridge
    6,     // Ironforge, Dun Morogh
    7,     // Menethil Harbor, Wetlands
    8,     // Thelsamar, Loch Modan
    12,    // Darkshire, Duskwood
    14,    // Southshore, Hillsbrad
    16,    // Refuge Pointe, Arathi
    19,    // Booty Bay, Stranglethorn
    43,    // Aerie Peak, The Hinterlands
    45,    // Nethergarde Keep, Blasted Lands
    66,    // Chillwind Camp, Western Plaguelands
    67,    // Light's Hope Chapel, Eastern Plaguelands
    71,    // Morgan's Vigil, Burning Steppes
    74,    // Thorium Point, Searing Gorge
    // Kalimdor:
    26,    // Auberdine, Darkshore
    27,    // Rut'theran Village, Teldrassil
    28,    // Astranaar, Ashenvale
    31,    // Thalanaar, Feralas
    32,    // Theramore, Dustwallow Marsh
    33,    // Stonetalon Peak, Stonetalon Mountains
    37,    // Nijel's Point, Desolace
    39,    // Gadgetzan, Tanaris
    41,    // Feathermoon, Feralas
    49,    // Moonglade
    52,    // Everlook, Winterspring
    64,    // Talrendis Point, Azshara
    65,    // Talonbranch Glade, Felwood
    73,    // Cenarion Hold, Silithus
    79,    // Marshal's Refuge, Un'Goro Crater
    80     // Ratchet, The Barrens
};
const std::vector<uint32> TaxiNodesHorde =
{
    // Eastern kingdoms:
    10,    // The Sepulcher, Silverpine Forest
    11,    // Undercity, Tirisfal
    13,    // Tarren Mill, Hillsbrad
    17,    // Hammerfall, Arathi
    18,    // Booty Bay, Stranglethorn
    20,    // Grom'gol, Stranglethorn
    21,    // Kargath, Badlands
    56,    // Stonard, Swamp of Sorrows
    68,    // Light's Hope Chapel, Eastern Plaguelands
    70,    // Flame Crest, Burning Steppes
    75,    // Thorium Point, Searing Gorge
    76,    // Revantusk Village, The Hinterlands
    // Kalimdor:
    22,    // Thunder Bluff, Mulgore
    23,    // Orgrimmar, Durotar
    25,    // Crossroads, The Barrens
    29,    // Sun Rock Retreat, Stonetalon Mountains
    30,    // Freewind Post, Thousand Needles
    38,    // Shadowprey Village, Desolace
    40,    // Gadgetzan, Tanaris
    42,    // Camp Mojache, Feralas
    44,    // Valormok, Azshara
    48,    // Bloodvenom Post, Felwood
    53,    // Everlook, Winterspring
    55,    // Brackenwall Village, Dustwallow Marsh
    58,    // Zoram'gar Outpost, Ashenvale
    61,    // Splintertree Post, Ashenvale
    69,    // Moonglade
    72,    // Cenarion Hold, Silithus
    77,    // Camp Taurajo, The Barrens
    79,    // Marshal's Refuge, Un'Goro Crater
    80     // Ratchet, The Barrens
};

struct npc_enlistment_officerAI : public ScriptedAI
{
    npc_enlistment_officerAI(Creature* creature) : ScriptedAI(creature)
    {
        Reset();
        FullGearListBiS60.clear();
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_ShamanResto), std::end(Lvl60BiS_ShamanResto));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_ShamanEnhancement), std::end(Lvl60BiS_ShamanEnhancement));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_ShamanElemental), std::end(Lvl60BiS_ShamanElemental));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_PriestShadow), std::end(Lvl60BiS_PriestShadow));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_PriestDiscHoly), std::end(Lvl60BiS_PriestDiscHoly));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_PaladinHoly), std::end(Lvl60BiS_PaladinHoly));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_PaladinRetribution), std::end(Lvl60BiS_PaladinRetribution));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_PaladinProtection), std::end(Lvl60BiS_PaladinProtection));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_WarriorFuryArms), std::end(Lvl60BiS_WarriorFuryArms));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_WarriorProtection), std::end(Lvl60BiS_WarriorProtection));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_DruidFeralCat), std::end(Lvl60BiS_DruidFeralCat));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_DruidFeralBear), std::end(Lvl60BiS_DruidFeralBear));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_DruidBalance), std::end(Lvl60BiS_DruidBalance));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_DruidResto), std::end(Lvl60BiS_DruidResto));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_Rogue), std::end(Lvl60BiS_Rogue));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_Mage), std::end(Lvl60BiS_Mage));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_Hunter), std::end(Lvl60BiS_Hunter));
        FullGearListBiS60.insert(std::end(FullGearListBiS60), std::begin(Lvl60BiS_Warlock), std::end(Lvl60BiS_Warlock));
    }
    std::vector<uint32> FullGearListBiS60;

    void Reset() override {}

    uint32 GetStarterMountForRace(Player* player)
    {
        uint32 mountEntry = 0;
        switch (player->getRace())
        {
        case RACE_HUMAN: mountEntry = 2411; break;      // Black Stallion Bridle
        case RACE_DWARF: mountEntry = 5872; break;      // Brown Ram
        case RACE_NIGHTELF: mountEntry = 8632; break;   // Reins of the Spotted Frostsaber
        case RACE_GNOME: mountEntry = 8595; break;      // Blue Mechanostrider
        case RACE_ORC: mountEntry = 5665; break;        // Horn of the Dire Wolf
        case RACE_UNDEAD: mountEntry = 13331; break;    // Red Skeletal Horse
        case RACE_TAUREN: mountEntry = 15277; break;    // Gray Kodo
        case RACE_TROLL: mountEntry = 8588; break;      // Whistle of the Emerald Raptor
        }
        return mountEntry;
    }

    uint32 GetStarterEpicMountForRace(Player* player)
    {
        uint32 mountEntry = 0;
        switch (player->getRace())
        {
        case RACE_HUMAN: mountEntry = 18776; break;     // Swift Palomino
        case RACE_DWARF: mountEntry = 18787; break;     // Swift Gray Ram
        case RACE_NIGHTELF: mountEntry = 18767; break;  // Reins of the Swift Mistsaber
        case RACE_GNOME: mountEntry = 18772; break;     // Swift Green Mechanostrider
        case RACE_ORC: mountEntry = 18797; break;       // Horn of the Swift Timber Wolf 
        case RACE_UNDEAD: mountEntry = 18791; break;    // Purple Skeletal Warhorse
        case RACE_TAUREN: mountEntry = 18795; break;    // Great Gray Kodo
        case RACE_TROLL: mountEntry = 18790; break;     // Swift Orange Raptor
        }
        return mountEntry;
    }

    void CreatePet(Player* player, Creature* creature, uint32 entry)
    {
        if (Creature* creatureTarget = m_creature->SummonCreature(entry, player->GetPositionX(), player->GetPositionY() + 2, player->GetPositionZ(), player->GetOrientation(), TEMPSPAWN_CORPSE_TIMED_DESPAWN, 500))
        {
            creatureTarget->SetLevel(player->getLevel() - 1);

            Pet* pet = new Pet(HUNTER_PET);

            if (!pet->CreateBaseAtCreature(creatureTarget))
            {
                delete pet;
                return;
            }

            pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, 13481); // tame beast

            pet->SetOwnerGuid(player->GetObjectGuid());
            pet->setFaction(player->getFaction());

            if (player->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
            {
                pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                //pet->SetByteValue(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_PLAYER_CONTROLLED_DEBUFF_LIMIT);
            }
            else
                //pet->SetByteValue(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_CREATURE_DEBUFF_LIMIT);

            if (player->IsImmuneToNPC())
                pet->SetImmuneToNPC(true);

            if (player->IsImmuneToPlayer())
                pet->SetImmuneToPlayer(true);

            if (player->IsPvP())
                pet->SetPvP(true);

            pet->GetCharmInfo()->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);

            uint32 level = creatureTarget->getLevel();
            pet->SetCanModifyStats(true);
            pet->InitStatsForLevel(level);

            pet->SetHealthPercent(creatureTarget->GetHealthPercent());

            // destroy creature object
            creatureTarget->ForcedDespawn();

            // prepare visual effect for levelup
            pet->SetUInt32Value(UNIT_FIELD_LEVEL, level - 1);

            // add pet object to the world
            pet->GetMap()->Add((Creature*)pet);
            pet->AIM_Initialize();

            pet->AI()->SetReactState(REACT_DEFENSIVE);

            // visual effect for levelup
            pet->SetUInt32Value(UNIT_FIELD_LEVEL, level);

            // enable rename and abandon prompt
            //pet->SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);
            //pet->SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_ABANDONED);

            // this enables pet details window (Shift+P)
            pet->InitPetCreateSpells();

            pet->LearnPetPassives();
            pet->CastPetAuras(true);
            pet->CastOwnerTalentAuras();
            pet->UpdateAllStats();

            // start with maximum loyalty and training points
            pet->SetLoyaltyLevel(BEST_FRIEND);
            pet->ModifyLoyalty(26500);
            pet->SetTP(level * (BEST_FRIEND - 1));
            pet->SetPower(POWER_HAPPINESS, HAPPINESS_LEVEL_SIZE * 2); // Content

            pet->SetRequiredXpForNextLoyaltyLevel();

            // caster have pet now
            player->SetPet(pet);

            player->PetSpellInitialize();

            pet->SavePetToDB(PET_SAVE_AS_CURRENT, player);
        }

        player->PlayerTalkClass->CloseGossip();
        creature->MonsterWhisper("An excellent choice! May it serve you well on the battlefield.", player);
    }

    bool HasStarterSet(Player* player, std::vector<uint32> gearList)
    {
        for (auto item : gearList)
        {
            ItemPrototype const* proto = ObjectMgr::GetItemPrototype(item);
            if (!proto)
                continue;

            // don't check for quiver/ammo pouch
            if (proto->InventoryType == INVTYPE_BAG || item == 23197)
                continue;

            if (player->HasItemCount(item, 1))
                return true;
        }

        return false;
    }

    void RemoveStarterSet(Player* player, std::vector<uint32> gearList)
    {
        for (auto item : gearList)
        {
            ItemPrototype const* proto = ObjectMgr::GetItemPrototype(item);
            if (!proto)
                continue;

            // don't check for quiver/ammo pouch
            if (proto->InventoryType == INVTYPE_BAG)
                continue;

            player->DestroyItemCount(item, 2, true, false);
        }
    }

    void GivePlayerItems(Player* recipient, std::vector<uint32> gearList)
    {
        bool allSuccess = true;
        bool alreadyHave = false;
        for (auto item : gearList)
        {
            ItemPrototype const* proto = ObjectMgr::GetItemPrototype(item);
            if (!proto)
                continue;

            if (recipient->HasItemCount(item, (proto->InventoryType != INVTYPE_FINGER && proto->InventoryType != INVTYPE_WEAPON) ? 1 : 2))
            {
                alreadyHave = true;
                continue;
            }

            if (!recipient->StoreNewItemInBestSlots(item, 1))
                allSuccess = false;
        }

        if (alreadyHave)
            m_creature->MonsterWhisper("There are already some pieces of starter gear in your possession. You must destroy it first before I can give you more!", recipient);

        if (!allSuccess)
            m_creature->MonsterWhisper("You can't hold some of the gear I'm trying to give you! Make room for it first and speak to me again.", recipient);
        else
            m_creature->MonsterWhisper("This equipment will serve you well in battle.", recipient);
    }

    bool TaxiNodesKnown(Player& player) const
    {
        switch (uint32(player.GetTeam()))
        {
        case ALLIANCE:
            for (uint32 node : TaxiNodesAlliance)
            {
                if (!player.m_taxi.IsTaximaskNodeKnown(node))
                    return false;
            }
            break;
        case HORDE:
            for (uint32 node : TaxiNodesHorde)
            {
                if (!player.m_taxi.IsTaximaskNodeKnown(node))
                    return false;
            }
            break;
        }
        return true;
    }

    void TaxiNodesTeach(Player& player) const
    {
        // Vanilla taxi nodes

        switch (uint32(player.GetTeam()))
        {
        case ALLIANCE:
            for (uint32 node : TaxiNodesAlliance)
                player.m_taxi.SetTaximaskNode(node);
            break;
        case HORDE:
            for (uint32 node : TaxiNodesHorde)
                player.m_taxi.SetTaximaskNode(node);
            break;
        }
    }
};

bool GossipHello_npc_enlistment_officer(Player* player, Creature* creature)
{
    if (npc_enlistment_officerAI* enlistmentOfficerAI = dynamic_cast<npc_enlistment_officerAI*>(creature->AI()))
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Train talent spell ranks", GOSSIP_SENDER_MAIN, GOSSIP_OPTION_TRAINER);

        if (player->getLevel() >= 60)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "|cFF0008E8Full \"best in slot\" gear - Classic|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2000);

        if (enlistmentOfficerAI->HasStarterSet(player, enlistmentOfficerAI->FullGearListBiS60))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, "|cFF8B0000Remove all level 60 BiS gear.|r", GOSSIP_SENDER_MAIN, 93);

        uint32 groundMount = enlistmentOfficerAI->GetStarterEpicMountForRace(player);
        if ((!player->HasItemCount(groundMount, 1)) && player->getLevel() == 60)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "I seem to be missing my mount. Can you give me one?", GOSSIP_SENDER_MAIN, 96);

        if (player->GetAreaId() != 72)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, "Teleport to Blasted Lands (Dark Portal)", GOSSIP_SENDER_MAIN, 90);

        if (!enlistmentOfficerAI->TaxiNodesKnown(*player))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, "Learn Azeroth flight paths", GOSSIP_SENDER_MAIN, 97);

        if (player->getClass() == CLASS_HUNTER)
        {
            if (!player->GetPet())
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Pet Menu", GOSSIP_SENDER_MAIN, 30);
            else if (player->GetPet()->m_spells.size() >= 1)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Untrain Pet", GOSSIP_SENDER_MAIN, 499);
        }

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "Basic Supplies", GOSSIP_SENDER_MAIN, GOSSIP_OPTION_VENDOR);

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Unlearn Talents", GOSSIP_SENDER_MAIN, 500);

        if (player->GetMoney() < 100000000)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "I need money!", GOSSIP_SENDER_MAIN, 600);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Reset my dungeon/raid lockouts.", GOSSIP_SENDER_MAIN, 700);

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, "Teleport to Test Realm Overlord", GOSSIP_SENDER_MAIN, 89);
        if (player->getLevel() < 60)
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_REFUSE_LOW, creature->GetObjectGuid());
        else
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_GREET, creature->GetObjectGuid());
    }
    return true;
}

bool GossipSelect_npc_enlistment_officer(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 action)
{
    player->PlayerTalkClass->ClearMenus();
    Pet* pPet = player->GetPet();

    if (npc_enlistment_officerAI* enlistmentOfficerAI = dynamic_cast<npc_enlistment_officerAI*>(creature->AI()))
    {
        // Main Menu
        if (action == 29)
            GossipHello_npc_enlistment_officer(player, creature);

        // Pets - Page 1
        else if (action == 30)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Back to Main Menu", GOSSIP_SENDER_MAIN, 29);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Next Page ->", GOSSIP_SENDER_MAIN, 31);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Bat", GOSSIP_SENDER_MAIN, 501);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Bear", GOSSIP_SENDER_MAIN, 502);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Boar", GOSSIP_SENDER_MAIN, 503);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Carrion Bird", GOSSIP_SENDER_MAIN, 504);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Cat", GOSSIP_SENDER_MAIN, 505);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Crab", GOSSIP_SENDER_MAIN, 506);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Crocolisk", GOSSIP_SENDER_MAIN, 507);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Gorilla", GOSSIP_SENDER_MAIN, 508);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Hyena", GOSSIP_SENDER_MAIN, 509);
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_PET_MENU, creature->GetObjectGuid());
        }

        // Pets - Page 2
        else if (action == 31)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Back to Main Menu", GOSSIP_SENDER_MAIN, 29);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "<- Previous Page", GOSSIP_SENDER_MAIN, 30);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Owl", GOSSIP_SENDER_MAIN, 510);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Raptor", GOSSIP_SENDER_MAIN, 511);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Strider", GOSSIP_SENDER_MAIN, 512);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Scorpid", GOSSIP_SENDER_MAIN, 513);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Serpent", GOSSIP_SENDER_MAIN, 514);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Spider", GOSSIP_SENDER_MAIN, 515);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Turtle", GOSSIP_SENDER_MAIN, 516);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Wolf", GOSSIP_SENDER_MAIN, 517);
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_PET_MENU, creature->GetObjectGuid());
        }

        else if (action == GOSSIP_OPTION_VENDOR)
            player->GetSession()->SendListInventory(creature->GetObjectGuid());

        else if (action == GOSSIP_OPTION_TRAINER)
            player->GetSession()->SendTrainerList(creature->GetObjectGuid());

        // Teleport - Test Realm Overlord
        else if (action == 89)
        {
            player->CLOSE_GOSSIP_MENU();
            switch (player->getRace())
            {
            case RACE_HUMAN:
                player->TeleportTo(teleLocs[2].map, teleLocs[2].x, teleLocs[2].y, teleLocs[2].z, teleLocs[2].o);
                break;
            case RACE_DWARF:
            case RACE_GNOME:
                player->TeleportTo(teleLocs[3].map, teleLocs[3].x, teleLocs[3].y, teleLocs[3].z, teleLocs[3].o);
                break;
            case RACE_NIGHTELF:
                player->TeleportTo(teleLocs[4].map, teleLocs[4].x, teleLocs[4].y, teleLocs[4].z, teleLocs[4].o);
                break;
            case RACE_TROLL:
            case RACE_ORC:
                player->TeleportTo(teleLocs[5].map, teleLocs[5].x, teleLocs[5].y, teleLocs[5].z, teleLocs[5].o);
                break;
            case RACE_UNDEAD:
                player->TeleportTo(teleLocs[6].map, teleLocs[6].x, teleLocs[6].y, teleLocs[6].z, teleLocs[6].o);
                break;
            case RACE_TAUREN:
                player->TeleportTo(teleLocs[7].map, teleLocs[7].x, teleLocs[7].y, teleLocs[7].z, teleLocs[7].o);
                break;
            }
        }
        // Teleport - Blasted Lands (Dark Portal)
        else if (action == 90)
        {
            player->CLOSE_GOSSIP_MENU();
            if (player->GetTeam() == ALLIANCE)
                player->TeleportTo(teleLocs[0].map, teleLocs[0].x, teleLocs[0].y, teleLocs[0].z, teleLocs[0].o);
            else
                player->TeleportTo(teleLocs[1].map, teleLocs[1].x, teleLocs[1].y, teleLocs[1].z, teleLocs[1].o);
        }

        // Remove Starter Set - BiS 60
        else if (action == 93)
        {
            enlistmentOfficerAI->RemoveStarterSet(player, enlistmentOfficerAI->FullGearListBiS60);
            player->CLOSE_GOSSIP_MENU();
        }

        // Add Starter Mount
        else if (action == 96)
        {
            uint32 groundMount = enlistmentOfficerAI->GetStarterEpicMountForRace(player);
            if (!player->HasItemCount(groundMount, 1)) player->StoreNewItemInBestSlots(groundMount, 1);
            player->PlayerTalkClass->CloseGossip();
        }

        // Learn Flight Paths
        else if (action == 97)
        {
            enlistmentOfficerAI->TaxiNodesTeach(*player);
            player->SetFacingToObject(creature);
            player->HandleEmote(EMOTE_ONESHOT_SALUTE);
            player->CastSpell(player, SPELL_CYCLONE_VISUAL_SPAWN, TRIGGERED_OLD_TRIGGERED);
            player->PlayerTalkClass->CloseGossip();
        }

        // Untrain Pet
        else if (action == 499)
        {
            player->PlayerTalkClass->CloseGossip();

            if (pPet && pPet->m_spells.size() >= 1)
            {
                CharmInfo* charmInfo = pPet->GetCharmInfo();
                if (charmInfo)
                {
                    for (PetSpellMap::iterator itr = pPet->m_spells.begin(); itr != pPet->m_spells.end();)
                    {
                        uint32 spell_id = itr->first;
                        ++itr;
                        pPet->unlearnSpell(spell_id, false);
                    }

                    uint32 cost = pPet->resetTalentsCost();

                    pPet->SetTP(pPet->getLevel() * (pPet->GetLoyaltyLevel() - 1));

                    for (int i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
                        if (UnitActionBarEntry const* ab = charmInfo->GetActionBarEntry(i))
                            if (ab->GetAction() && ab->IsActionBarForSpell())
                                charmInfo->SetActionBar(i, 0, ACT_DISABLED);

                    // relearn pet passives
                    pPet->LearnPetPassives();

                    pPet->m_resetTalentsTime = time(nullptr);
                    pPet->m_resetTalentsCost = cost;

                    player->PetSpellInitialize();
                }
            }
        }

        // Unlearn Talents
        else if (action == 500)
        {
            player->PlayerTalkClass->CloseGossip();
            player->resetTalents(true);
        }
        // Money!
        else if (action == 600)
        {
            player->PlayerTalkClass->CloseGossip();
            player->ModifyMoney(INT_MAX);
        }
        // Unbind Instances
        else if (action == 700)
        {
            player->PlayerTalkClass->CloseGossip();
            Player::BoundInstancesMap& binds = player->GetBoundInstances();
            for (Player::BoundInstancesMap::iterator itr = binds.begin(); itr != binds.end();)
            {
                if (itr->first != player->GetMapId())
                {
                    DungeonPersistentState* save = itr->second.state;
                    std::string timeleft = secsToTimeString(save->GetResetTime() - time(nullptr), true);
                    player->UnbindInstance(itr);
                }
                else
                    ++itr;
            }
        }

        else if (action == 501) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_BAT);
        else if (action == 502) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_BEAR);
        else if (action == 503) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_BOAR);
        else if (action == 504) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_CARRION);
        else if (action == 505) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_CAT);
        else if (action == 506) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_CRAB);
        else if (action == 507) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_CROC);
        else if (action == 508) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_GORILLA);
        else if (action == 509) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_HYENA);
        else if (action == 510) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_OWL);
        else if (action == 511) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_RAPTOR);
        else if (action == 512) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_STRIDER);
        else if (action == 513) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_SCORPID);
        else if (action == 514) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_SERPENT);
        else if (action == 515) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_SPIDER);
        else if (action == 516) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_TURTLE);
        else if (action == 517) enlistmentOfficerAI->CreatePet(player, creature, NPC_PET_WOLF);

        // Classic Full Best in Slot
        // Pure DPS classes Hunter, Rogue, Mage, and Warlock don't require a secondary selection
        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF + 2000:
        {
            switch (player->getClass())
            {
            case CLASS_ROGUE:
                enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_Rogue);
                player->CLOSE_GOSSIP_MENU();
                break;
            case CLASS_MAGE:
                enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_Mage);
                player->CLOSE_GOSSIP_MENU();
                break;
            case CLASS_HUNTER:
                enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_Hunter);
                player->CLOSE_GOSSIP_MENU();
                break;
            case CLASS_WARLOCK:
                enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_Warlock);
                player->CLOSE_GOSSIP_MENU();
                break;
            case CLASS_SHAMAN:
                player->PrepareGossipMenu(creature, GOSSIP_TEXT_PICK_SPEC);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Restoration", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2001);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Enhancement", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2002);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Elemental", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2003);
                player->SendPreparedGossip(creature);
                break;
            case CLASS_PRIEST:
                player->PrepareGossipMenu(creature, GOSSIP_TEXT_PICK_SPEC);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Shadow", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2004);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Discipline/Holy", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2005);
                player->SendPreparedGossip(creature);
                break;
            case CLASS_PALADIN:
                player->PrepareGossipMenu(creature, GOSSIP_TEXT_PICK_SPEC);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Holy", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2006);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Retribution", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2007);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Protection", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2008);
                player->SendPreparedGossip(creature);
                break;
            case CLASS_WARRIOR:
                player->PrepareGossipMenu(creature, GOSSIP_TEXT_PICK_SPEC);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Fury/Arms", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2009);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Protection", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2010);
                player->SendPreparedGossip(creature);
                break;
            case CLASS_DRUID:
                player->PrepareGossipMenu(creature, GOSSIP_TEXT_PICK_SPEC);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Feral (Cat/DPS)", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2011);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Feral (Bear/Tank)", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2012);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Balance", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2013);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Restoration", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2014);
                player->SendPreparedGossip(creature);
                break;
            }
            break;
        }
        // Classic Full Best in Slot (Spec Selected)
        // Shaman - Restoration
        case GOSSIP_ACTION_INFO_DEF + 2001: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_ShamanResto); player->PlayerTalkClass->CloseGossip(); break;
            // Shaman - Enhancement
        case GOSSIP_ACTION_INFO_DEF + 2002: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_ShamanEnhancement); player->PlayerTalkClass->CloseGossip(); break;
            // Shaman - Elemental
        case GOSSIP_ACTION_INFO_DEF + 2003: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_ShamanElemental); player->PlayerTalkClass->CloseGossip(); break;
            // Priest - Shadow
        case GOSSIP_ACTION_INFO_DEF + 2004: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_PriestShadow); player->PlayerTalkClass->CloseGossip(); break;
            // Priest - Discipline/Holy
        case GOSSIP_ACTION_INFO_DEF + 2005: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_PriestDiscHoly); player->PlayerTalkClass->CloseGossip(); break;
            // Paladin - Holy
        case GOSSIP_ACTION_INFO_DEF + 2006: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_PaladinHoly); player->PlayerTalkClass->CloseGossip(); break;
            // Paladin - Retribution
        case GOSSIP_ACTION_INFO_DEF + 2007: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_PaladinRetribution); player->PlayerTalkClass->CloseGossip(); break;
            // Paladin - Protection
        case GOSSIP_ACTION_INFO_DEF + 2008: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_PaladinProtection); player->PlayerTalkClass->CloseGossip(); break;
            // Warrior - Fury/Arms
        case GOSSIP_ACTION_INFO_DEF + 2009: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_WarriorFuryArms); player->PlayerTalkClass->CloseGossip(); break;
            // Warrior - Protection
        case GOSSIP_ACTION_INFO_DEF + 2010: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_WarriorProtection); player->PlayerTalkClass->CloseGossip(); break;
            // Druid - Feral (Cat/DPS)
        case GOSSIP_ACTION_INFO_DEF + 2011: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_DruidFeralCat); player->PlayerTalkClass->CloseGossip(); break;
            // Druid - Feral (Bear/Tank)
        case GOSSIP_ACTION_INFO_DEF + 2012: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_DruidFeralBear); player->PlayerTalkClass->CloseGossip(); break;
            // Druid - Balance
        case GOSSIP_ACTION_INFO_DEF + 2013: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_DruidBalance); player->PlayerTalkClass->CloseGossip(); break;
            // Druid - Restoration
        case GOSSIP_ACTION_INFO_DEF + 2014: enlistmentOfficerAI->GivePlayerItems(player, Lvl60BiS_DruidResto); player->PlayerTalkClass->CloseGossip(); break;
        }
    }

    return true;
}

UnitAI* GetAI_npc_enlistment_officer(Creature* creature)
{
    return new npc_enlistment_officerAI(creature);
}

void AddSC_npcs_special()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_chicken_cluck";
    pNewScript->GetAI = &GetAI_npc_chicken_cluck;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_injured_patient";
    pNewScript->GetAI = &GetAI_npc_injured_patient;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_doctor";
    pNewScript->GetAI = &GetAI_npc_doctor;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_doctor;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_garments_of_quests";
    pNewScript->GetAI = &GetAI_npc_garments_of_quests;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_guardian";
    pNewScript->GetAI = &GetAI_npc_guardian;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_innkeeper";
    pNewScript->pGossipHello = &GossipHello_npc_innkeeper;
    pNewScript->pGossipSelect = &GossipSelect_npc_innkeeper;
    pNewScript->RegisterSelf(false);                        // script and error report disabled, but script can be used for custom needs, adding ScriptName

    pNewScript = new Script;
    pNewScript->Name = "npc_redemption_target";
    pNewScript->GetAI = &GetAI_npc_redemption_target;
    pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_redemption_target;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_the_cleaner";
    pNewScript->GetAI = &GetAI_npc_the_cleaner;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_aoe_damage_trigger";
    pNewScript->GetAI = &GetNewAIInstance<npc_aoe_damage_triggerAI>;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_enchantment_crystal";
    pNewScript->pGossipHello = &GossipHello_npc_enchantment_crystal;
    pNewScript->pGossipSelect = &GossipSelect_npc_enchantment_crystal;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_enlistment_officer";
    pNewScript->GetAI = &GetAI_npc_enlistment_officer;
    pNewScript->pGossipHello = &GossipHello_npc_enlistment_officer;
    pNewScript->pGossipSelect = &GossipSelect_npc_enlistment_officer;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_test_realm_overlord";
    pNewScript->GetAI = &GetNewAIInstance<npc_test_realm_overlord>;
    pNewScript->pGossipHello = &GossipHello_npc_test_realm_overlord;
    pNewScript->pGossipSelect = &GossipSelect_npc_test_realm_overlord;
    pNewScript->RegisterSelf();
}
