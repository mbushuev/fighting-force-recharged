#pragma once

enum class TransitionSource { Game, Movie, Splash };

// The movie and D3D renderer have independent lifetimes. A frame from the old
// renderer must never uncover a new movie (or the reverse).
struct TransitionGate {
    TransitionSource source=TransitionSource::Game;
    bool pending=false;
    unsigned scenes=0;
    unsigned generation=0;
    bool Begin(TransitionSource next) {
        if(pending && source==next)return false;
        source=next;pending=true;scenes=0;++generation;return true;
    }
    void SceneComplete(){if(pending && source==TransitionSource::Game && scenes<2)++scenes;}
    bool PollGame() {
        if(!pending || source!=TransitionSource::Game || scenes<2)return false;
        pending=false;return true;
    }
    bool MovieFrame(unsigned before,unsigned after) {
        if(!pending || source!=TransitionSource::Movie || after<2 || after<=before)return false;
        pending=false;return true;
    }
    bool SplashFrame() {
        if(!pending || source!=TransitionSource::Splash)return false;
        pending=false;return true;
    }
};
