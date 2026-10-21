#pragma once
// Simulation and render callbacks select a player only for their own thread.
static bool g_camera_cooperative;
static thread_local int g_camera_owner=-1;
static ff_camera::FirstPersonView* g_camera_player_views[2]={};
static unsigned CameraPlayerOffset() {return g_camera_owner==1?0xF4:0;}
static ff_camera::FirstPersonView& CameraFirstPersonView() {
    return g_camera_cooperative && g_camera_owner>=0 && g_camera_player_views[g_camera_owner]?
        *g_camera_player_views[g_camera_owner]:g_first_person;
}
struct CameraPlayerScope {
    int previous;
    explicit CameraPlayerScope(int player):previous(g_camera_owner){g_camera_owner=player;}
    explicit CameraPlayerScope(void* actor):previous(g_camera_owner) {
        if(g_camera_cooperative) {
            const auto address=reinterpret_cast<uintptr_t>(actor);
            g_camera_owner=address==g_base+0x86A500?0:address==g_base+0x86A5F4?1:-1;
        }
    }
    ~CameraPlayerScope(){g_camera_owner=previous;}
};
