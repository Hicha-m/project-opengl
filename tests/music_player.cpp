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
    assert(music.restart()); float previous=0;
    for(int i=0;i<10;++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        const float time=music.position(); assert(time>=previous && time<2); previous=time;
    }
    assert(previous>0);
    music.setMuted(true); assert(music.running());
    assert(music.restart() && music.position()<0.2f);
    music.setMuted(false);
    music.release(); assert(!music.ready() && !music.running() && music.position()==0);
    assert(!music.load("build/music/missing.wav") && !music.ready());
    assert(music.load("build/music/cinematic.wav") && music.restart());
    std::cout<<"Music decoding, playback clock, rewind, mute and resource recovery passed (dummy audio)\n";
}
