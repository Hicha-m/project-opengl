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
    mDevice=SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec);
    SDL_AudioSpec output{};
    if(!mDevice || !SDL_GetAudioDeviceFormat(mDevice,&output,nullptr)) { release(); return false; }
    SDL_PauseAudioDevice(mDevice);
    mStream=SDL_CreateAudioStream(&spec,&output);
    if(mStream && !SDL_BindAudioStream(mDevice,mStream)) { release(); return false; }
    if(!mStream) { std::cerr<<"Music unavailable: "<<SDL_GetError()<<"\n"; release(); return false; }
    setMuted(mMuted);
    return true;
}
bool MusicPlayer::restart() {
    if(!mStream) return false;
    SDL_PauseAudioStreamDevice(mStream);
    for(auto& voice:mVoices) if(voice.stream) SDL_ClearAudioStream(voice.stream);
    mLastImpactTime=-1; mNextVoice=mImpactPlayCount=0;
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
    for(auto& voice:mVoices) if(voice.stream) SDL_SetAudioStreamGain(voice.stream,muted?0.0f:voice.gain);
}
void MusicPlayer::release() {
    for(auto& voice:mVoices) { if(voice.stream) SDL_DestroyAudioStream(voice.stream); voice={}; }
    mImpactPCM.clear(); mLastImpactTime=-1; mNextVoice=mImpactPlayCount=0;
    if(mStream) SDL_DestroyAudioStream(mStream);
    if(mDevice) SDL_CloseAudioDevice(mDevice);
    mDevice=0;
    mStream=nullptr; mRunning=false; mPCM.clear(); mPosition=mDuration=0; mBytesPerSecond=0;
    if(mInitialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    mInitialized=false;
}

bool MusicPlayer::loadImpact(const std::string& path) {
    for(auto& voice:mVoices) { if(voice.stream) SDL_DestroyAudioStream(voice.stream); voice={}; }
    mImpactPCM.clear(); mLastImpactTime=-1; mNextVoice=mImpactPlayCount=0;
    if(!mStream) return false;
    SDL_AudioSpec source{},output{}; Uint8* data=nullptr; Uint32 length=0;
    if(!SDL_LoadWAV(path.c_str(),&source,&data,&length)) return false;
    mImpactPCM.assign(data,data+length); SDL_free(data);
    if(mImpactPCM.empty() || !SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(mStream),&output,nullptr)) { mImpactPCM.clear(); return false; }
    for(auto& voice:mVoices) {
        voice.stream=SDL_CreateAudioStream(&source,&output);
        if(!voice.stream || !SDL_BindAudioStream(SDL_GetAudioStreamDevice(mStream),voice.stream)) {
            std::cerr<<"Impact audio unavailable: "<<SDL_GetError()<<"\n";
            for(auto& cleanup:mVoices) { if(cleanup.stream) SDL_DestroyAudioStream(cleanup.stream); cleanup={}; }
            mImpactPCM.clear(); return false;
        }
    }
    return true;
}
void MusicPlayer::playImpacts(const std::vector<MeteorImpact>& impacts,const glm::vec3& listener,float time) {
    if(!mRunning || mImpactPCM.empty() || impacts.empty() || time-mLastImpactTime<0.12f) return;
    float gain=0;
    for(const auto& impact:impacts) {
        const float distance=glm::distance(listener,impact.position);
        const float strength=std::clamp(impact.meteorScale*glm::length(impact.velocity)/14.0f,0.2f,1.0f);
        gain=std::max(gain,0.25f*(0.25f+0.4f*strength)/(1+distance*distance/(180*180)));
    }
    if(gain<0.01f) return;
    // Bounded overlapping voices; mix the strongest contact of each frame.
    unsigned selected=mNextVoice;
    for(unsigned i=0;i<mVoices.size();++i) if(SDL_GetAudioStreamQueued(mVoices[i].stream)==0) { selected=i; break; }
    auto& voice=mVoices[selected]; voice.gain=gain;
    SDL_SetAudioStreamGain(voice.stream,mMuted?0:gain);
    if(SDL_ClearAudioStream(voice.stream) && SDL_PutAudioStreamData(voice.stream,mImpactPCM.data(),int(mImpactPCM.size()))
        && SDL_FlushAudioStream(voice.stream)) {
        mLastImpactTime=time; ++mImpactPlayCount; mNextVoice=(selected+1)%mVoices.size();
    }
}
