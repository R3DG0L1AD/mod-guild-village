// Family LAN patch (ClassScrolls plan 12.4 step 2d, decision D6): a playerbot may enter and see a guild
// village when its group contains a real player of a village-owning guild. Real players outside the guild
// are still bounced. Released under AGPL-3.0 like the module.
#ifndef GV_FAMILYLAN_H
#define GV_FAMILYLAN_H

#include "DatabaseEnv.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"

namespace GuildVillageFamilyLan
{
    inline bool GuildOwnsVillage(uint32 guildId)
    {
        return guildId && WorldDatabase.Query("SELECT 1 FROM customs.gv_guild WHERE guild={}", guildId);
    }

    // Village guild for this player: its own guild if that owns a village; for a playerbot (headless
    // session) the guild of a real group member that owns one (leader first). requireMap: that member
    // must currently be on this map (0 = anywhere). Returns 0 if none.
    inline uint32 VillageGuildFor(Player const* p, uint32 requireMap = 0)
    {
        if (!p)
            return 0;
        if (GuildOwnsVillage(p->GetGuildId()))
            return p->GetGuildId();

        WorldSession* s = p->GetSession();
        if (!s || !s->IsHeadless())
            return 0;
        Group* grp = const_cast<Player*>(p)->GetGroup();
        if (!grp)
            return 0;

        auto check = [&](Player* m) -> uint32
        {
            if (!m || m == p || !m->GetSession() || m->GetSession()->IsHeadless())
                return 0;
            if (requireMap && m->GetMapId() != requireMap)
                return 0;
            return GuildOwnsVillage(m->GetGuildId()) ? m->GetGuildId() : 0;
        };

        if (uint32 g = check(ObjectAccessor::FindConnectedPlayer(grp->GetLeaderGUID())))
            return g;
        for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
            if (uint32 g = check(ref->GetSource()))
                return g;
        return 0;
    }
}

#endif
