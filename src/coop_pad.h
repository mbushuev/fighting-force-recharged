#pragma once
#include "modern_input.h"

namespace ff_input {
struct CooperativePads {
    uint64_t search_at=0;
    unsigned connected=0,claim_ready=0;
    bool awaiting_owner=false,pause_requested=false;

    template<typename Reader>
    void Poll(PadSession& first,PadSession& second,const int (&fixed)[2],uint64_t now,int tag,
              bool focus,bool playing,int left_deadzone,int right_deadzone,Reader reader) {
        PadSession* players[]={&first,&second};
        for(auto player:players) {
            if(!focus || player->context!=tag) {
                player->ready=false;player->last_poll=0;player->context=tag;
            }
        }
        if(!focus){claim_ready=0;return;}
        if(first.last_poll && second.last_poll && now-first.last_poll<4 && now-second.last_poll<4)return;
        first.last_poll=second.last_poll=now;
        // Poll active slots together. Probe empty slots less often, so a
        // disconnected device does not slow every gameplay input poll.
        unsigned probe=connected;
        if(now>=search_at) {
            probe=15;search_at=now+250;
        }
        for(auto player:players)if(player->selected>=0)probe|=1u<<player->selected;
        Pad samples[4]={};
        connected=0;
        for(int slot=0;slot<4;++slot)if((probe&(1u<<slot)) && reader(slot,samples[slot]))connected|=1u<<slot;
        const auto online=[&](int slot){return slot>=0 && (connected&(1u<<slot));};
        const auto assign=[&](int player,int slot) {
            auto& session=*players[player];
            if(session.selected!=slot)session.ready=false;
            if(slot>=0)session.last_slot=slot;
            session.selected=slot;session.raw=slot<0?Pad{}:samples[slot];
        };
        const bool previously_assigned=first.selected>=0 || second.selected>=0;
        for(int i=0;i<2;++i) {
            const int slot=players[i]->selected;
            assign(i,online(slot) && slot!=fixed[1-i] && (fixed[i]<0 || slot==fixed[i])?slot:-1);
        }
        if(playing && previously_assigned && first.selected<0 && second.selected<0 &&
           first.last_slot>=0 && second.last_slot>=0) {
            pause_requested=true;
            // XInput has slot numbers but no persistent identity. After both
            // automatic assignments disappear, let one pad identify its owner.
            if(fixed[0]<0 && fixed[1]<0){awaiting_owner=true;claim_ready=0;}
        }
        if(awaiting_owner) {
            int candidate=-1,owner=-1;
            claim_ready&=connected;
            for(int slot=0;slot<4;++slot)if(online(slot)) {
                const auto& pad=samples[slot];
                const unsigned bit=1u<<slot;
                if(PadNeutral(pad,left_deadzone,right_deadzone))claim_ready|=bit;
                else if(claim_ready&bit) {
                    claim_ready&=~bit;
                    if(pad.buttons!=A && pad.buttons!=X)continue;
                    if(candidate>=0)owner=-1; // Simultaneous claims require a new press.
                    else {candidate=slot;owner=pad.buttons==A?0:1;}
                }
            }
            if(owner<0)return;
            assign(owner,candidate);awaiting_owner=false;claim_ready=0;
        }
        // Keep connected assignments first, then reserve every explicit slot.
        // Give each vacant player their previous slot before taking a spare.
        for(int i=0;i<2;++i)if(players[i]->selected<0) {
            const int wanted=fixed[i]>=0?fixed[i]:players[i]->last_slot;
            if(online(wanted) && wanted!=fixed[1-i] && wanted!=players[1-i]->selected)assign(i,wanted);
        }
        for(int i=0;i<2;++i)if(players[i]->selected<0 && fixed[i]<0)
            for(int slot=0;slot<4;++slot)if(online(slot) && slot!=fixed[1-i] && slot!=players[1-i]->selected) {
                assign(i,slot);break;
            }
    }
};
} // namespace ff_input
