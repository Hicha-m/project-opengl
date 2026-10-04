#include <cassert>
#include <chrono>
#include <thread>
#include <iostream>
#include "audio/MusicPlayer.h"
#include "cinematic/MainSequence.h"
int main() {
    MusicPlayer music;
    assert(music.load("build/music/cinematic.wav"));
    assert(music.ready() && !music.running() && music.position()==0);
    assert(std::abs(music.duration()-MainSequence::Duration)<0.001f);
    assert(music.loadImpact("build/music/impact.wav"));
    assert(music.restart()); float previous=0;
    for(int i=0;i<10;++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        const float time=music.position(); assert(time>=previous && time<2); previous=time;
    }
    assert(previous>0);
    std::vector<MeteorImpact> impacts={{{0,0,0},{0,1,0},{0,-16,0},0.8f}};
    music.playImpacts(impacts,{0,0,30},1);
    assert(music.impactPlayCount()==1 && music.position()>=previous);
    music.playImpacts(impacts,{0,0,30},1.05f); assert(music.impactPlayCount()==1);
    music.playImpacts(impacts,{0,0,30},1.2f); assert(music.impactPlayCount()==2);
    music.setMuted(true); assert(music.running());
    assert(music.restart() && music.position()<0.2f);
    assert(music.impactPlayCount()==0);
    music.setMuted(false);
    music.release(); assert(!music.ready() && !music.running() && music.position()==0);
    assert(!music.load("build/music/missing.wav") && !music.ready());
    assert(music.load("build/music/cinematic.wav") && music.restart());
    std::cout<<"Music decoding, playback clock, rewind, mute and resource recovery passed (dummy audio)\n";
}
