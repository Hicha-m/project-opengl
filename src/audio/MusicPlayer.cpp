#include "audio/MusicPlayer.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <iostream>

MusicPlayer::~MusicPlayer() { release(); }
bool MusicPlayer::load(const std::string& path) {
    release();
    if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    mInitialized=true;
    SDL_AudioSpec spec{}; Uint8* data=nullptr; Uint32 length=0;
    if(!SDL_LoadWAV(path.c_str(),&spec,&data,&length)) { release(); return false; }
    mPCM.assign(data,data+length); SDL_free(data);
    mBytesPerSecond=double(spec.freq)*spec.channels*(SDL_AUDIO_BITSIZE(spec.format)/8);
    if(mBytesPerSecond<=0 || mPCM.empty()) { release(); return false; }
    mDuration=float(mPCM.size()/mBytesPerSecond);
    mStream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
    if(!mStream) { std::cerr<<"Music unavailable: "<<SDL_GetError()<<"\n"; release(); return false; }
    setMuted(mMuted);
    return true;
}
bool MusicPlayer::restart() {
    if(!mStream) return false;
    SDL_PauseAudioStreamDevice(mStream);
    if(!SDL_ClearAudioStream(mStream) || !SDL_PutAudioStreamData(mStream,mPCM.data(),int(mPCM.size()))
        || !SDL_FlushAudioStream(mStream)) { mRunning=false; return false; }
    mPosition=0;
    mRunning=SDL_ResumeAudioStreamDevice(mStream);
    return mRunning;
}
float MusicPlayer::position() {
    if(!mStream || !mRunning) return mPosition;
    const int remaining=SDL_GetAudioStreamQueued(mStream);
    if(remaining<0) { mRunning=false; return mPosition; }
    const float consumed=float((double(mPCM.size())-remaining)/mBytesPerSecond);
    mPosition=std::clamp(std::max(mPosition,consumed),0.0f,mDuration);
    return mPosition;
}
void MusicPlayer::setMuted(bool muted) {
    mMuted=muted;
    if(mStream) SDL_SetAudioStreamGain(mStream,muted?0.0f:0.7f);
}
void MusicPlayer::release() {
    if(mStream) SDL_DestroyAudioStream(mStream);
    mStream=nullptr; mRunning=false; mPCM.clear(); mPosition=mDuration=0; mBytesPerSecond=0;
    if(mInitialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    mInitialized=false;
}
