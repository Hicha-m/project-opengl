#include <cassert>
#include <chrono>
#include <thread>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <limits>
#include "audio/MusicPlayer.h"
#include "cinematic/MainSequence.h"
int main() {
    MusicPlayer music;
    assert(music.load("build/music/cinematic.mp3"));
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
    music.setPaused(true);
    assert(music.seek(30)); const float pausedAt=music.position();
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    assert(music.position()==pausedAt && std::abs(pausedAt-30)<0.001f);
    assert(music.impactPlayCount()==0);
    assert(music.setPlaybackRate(4) && !music.setPlaybackRate(0) && !music.setPlaybackRate(1000));
    music.setPaused(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    assert(music.position()>pausedAt+0.3f && music.position()<pausedAt+1.5f);
    music.setPaused(true); assert(music.seek(10) && std::abs(music.position()-10)<0.001f);
    assert(music.seek(music.duration()) && std::abs(music.position()-music.duration())<0.001f);
    assert(music.seek(-20) && music.position()==0);
    music.setPlaybackRate(1); music.setPaused(false);
    music.release(); assert(!music.ready() && !music.running() && music.position()==0);
    assert(!music.load("build/music/missing.mp3") && !music.ready());
    const auto invalid=std::filesystem::temp_directory_path()/
        ("invalid-music-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mp3");
    std::ofstream(invalid,std::ios::binary)<<"This is not an MP3 stream.";
    assert(!music.load(invalid.u8string()) && !music.ready() && !music.running());
    std::ofstream(invalid,std::ios::binary|std::ios::trunc).close();
    assert(!music.load(invalid.u8string()) && !music.ready());
    std::filesystem::remove(invalid);
    assert(music.load("build/music/cinematic.mp3") && music.restart());
    assert(music.loadImpact("build/music/impact.wav"));
    assert(!music.seek(std::numeric_limits<float>::quiet_NaN()));
    music.setPaused(true);
    assert(music.seek(music.duration()+10));
    assert(std::abs(music.position()-MainSequence::Duration)<0.001f);
    std::cout<<"Music decoding, playback clock, rewind, mute and resource recovery passed (dummy audio)\n";
}
