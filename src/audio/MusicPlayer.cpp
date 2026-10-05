#include "audio/MusicPlayer.h"
#include "platform/ResourcePaths.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#define DR_MP3_NO_STDIO
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

MusicPlayer::~MusicPlayer()
{
    release();
}

bool MusicPlayer::load(const std::string& path)
{
    release();
    if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    mInitialized=true;
    SDL_AudioSpec spec{};
    const auto filename = ResourcePaths::resolve(path).u8string();
    // SDL handles UTF-8 paths on every desktop target; decode the same compressed
    // source in memory without an external player or a generated WAV on disk.
    size_t encodedSize=0;
    std::unique_ptr<void,decltype(&SDL_free)> encoded(SDL_LoadFile(filename.c_str(),&encodedSize),SDL_free);
    if(!encoded || encodedSize==0) { release(); return false; }
    drmp3_config config{};
    drmp3_uint64 frameCount=0;
    const auto freePCM=[](drmp3_int16* pcm) { drmp3_free(pcm,nullptr); };
    std::unique_ptr<drmp3_int16,decltype(freePCM)> decoded(
        drmp3_open_memory_and_read_pcm_frames_s16(encoded.get(),encodedSize,&config,&frameCount,nullptr),freePCM);
    if(!decoded || !config.channels || !config.sampleRate || !frameCount
        || frameCount>std::numeric_limits<int>::max()/(sizeof(drmp3_int16)*config.channels)) {
        release(); return false;
    }
    const auto bytes=static_cast<size_t>(frameCount)*config.channels*sizeof(drmp3_int16);
    const auto* data=reinterpret_cast<const unsigned char*>(decoded.get());
    mPCM.assign(data,data+bytes);
    spec={SDL_AUDIO_S16,static_cast<int>(config.channels),static_cast<int>(config.sampleRate)};
    mBytesPerFrame=spec.channels*(SDL_AUDIO_BITSIZE(spec.format)/8);
    mBytesPerSecond=double(spec.freq)*mBytesPerFrame;
    if(mBytesPerSecond<=0 || mPCM.empty()) { release(); return false; }
    mDuration=float(mPCM.size()/mBytesPerSecond);
    mDevice=SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec);
    SDL_AudioSpec output{};
    if (!mDevice || !SDL_GetAudioDeviceFormat(mDevice, &output, nullptr))
    {
        release();
        return false;
    }
    SDL_PauseAudioDevice(mDevice);
    mStream = SDL_CreateAudioStream(&spec, &output);
    if (mStream && !SDL_BindAudioStream(mDevice, mStream))
    {
        release();
        return false;
    }
    if (!mStream)
    {
        std::cerr << "Music unavailable: " << SDL_GetError() << "\n";
        release();
        return false;
    }
    setPlaybackRate(mRate);
    setMuted(mMuted);
    return true;
}

bool MusicPlayer::restart()
{
    return seek(0);
}

bool MusicPlayer::seek(float seconds)
{
    if (!mStream || !std::isfinite(seconds))
    {
        return false;
    }
    seconds = std::clamp(seconds, 0.0f, mDuration);
    SDL_PauseAudioStreamDevice(mStream);
    for (auto& voice : mVoices)
    {
        if (voice.stream)
        {
            SDL_ClearAudioStream(voice.stream);
        }
    }
    mLastImpactTime = -1;
    mNextVoice = mImpactPlayCount = 0;
    const std::size_t offset = std::min(mPCM.size(), std::size_t(double(seconds) * mBytesPerSecond) /
                                                         mBytesPerFrame * mBytesPerFrame);
    if (!SDL_ClearAudioStream(mStream) ||
        (offset < mPCM.size() &&
         !SDL_PutAudioStreamData(mStream, mPCM.data() + offset, int(mPCM.size() - offset))) ||
        !SDL_FlushAudioStream(mStream))
    {
        mRunning = false;
        return false;
    }
    mPosition = float(offset / mBytesPerSecond);
    mRunning = true;
    if (!mPaused)
    {
        mRunning = SDL_ResumeAudioStreamDevice(mStream);
    }
    return mRunning;
}

void MusicPlayer::setPaused(bool paused)
{
    if (mStream && paused && !mPaused)
    {
        position();
    }
    mPaused = paused;
    if (mStream)
    {
        if (paused)
        {
            SDL_PauseAudioStreamDevice(mStream);
        }
        else if (mRunning)
        {
            SDL_ResumeAudioStreamDevice(mStream);
        }
    }
}

bool MusicPlayer::setPlaybackRate(float rate)
{
    if (!std::isfinite(rate) || rate < 0.25f || rate > 8)
    {
        return false;
    }
    if (mStream && !SDL_SetAudioStreamFrequencyRatio(mStream, rate))
    {
        return false;
    }
    for (auto& voice : mVoices)
    {
        if (voice.stream)
        {
            SDL_SetAudioStreamFrequencyRatio(voice.stream, rate);
        }
    }
    mRate = rate;
    return true;
}

float MusicPlayer::position()
{
    if (!mStream || !mRunning || mPaused)
    {
        return mPosition;
    }
    const int remaining = SDL_GetAudioStreamQueued(mStream);
    if (remaining < 0)
    {
        mRunning = false;
        return mPosition;
    }
    const float consumed = float((double(mPCM.size()) - remaining) / mBytesPerSecond);
    mPosition = std::clamp(std::max(mPosition, consumed), 0.0f, mDuration);
    return mPosition;
}

void MusicPlayer::setMuted(bool muted)
{
    mMuted = muted;
    if (mStream)
    {
        SDL_SetAudioStreamGain(mStream, muted ? 0.0f : 0.7f);
    }
    for (auto& voice : mVoices)
    {
        if (voice.stream)
        {
            SDL_SetAudioStreamGain(voice.stream, muted ? 0.0f : voice.gain);
        }
    }
}

void MusicPlayer::release()
{
    for (auto& voice : mVoices)
    {
        if (voice.stream)
        {
            SDL_DestroyAudioStream(voice.stream);
        }
        voice = {};
    }
    mImpactPCM.clear();
    mLastImpactTime = -1;
    mNextVoice = mImpactPlayCount = 0;
    if (mStream)
    {
        SDL_DestroyAudioStream(mStream);
    }
    if (mDevice)
    {
        SDL_CloseAudioDevice(mDevice);
    }
    mDevice = 0;
    mPaused = false;
    mStream = nullptr;
    mRunning = false;
    mPCM.clear();
    mPosition = mDuration = 0;
    mBytesPerSecond = 0;
    if (mInitialized)
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    mInitialized = false;
}

bool MusicPlayer::loadImpact(const std::string& path)
{
    for (auto& voice : mVoices)
    {
        if (voice.stream)
        {
            SDL_DestroyAudioStream(voice.stream);
        }
        voice = {};
    }
    mImpactPCM.clear();
    mLastImpactTime = -1;
    mNextVoice = mImpactPlayCount = 0;
    if (!mStream)
    {
        return false;
    }
    SDL_AudioSpec source{}, output{};
    Uint8* data = nullptr;
    Uint32 length = 0;
    const auto filename = ResourcePaths::resolve(path).u8string();
    if (!SDL_LoadWAV(filename.c_str(), &source, &data, &length))
    {
        return false;
    }
    mImpactPCM.assign(data, data + length);
    SDL_free(data);
    if (mImpactPCM.empty() || !SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(mStream), &output, nullptr))
    {
        mImpactPCM.clear();
        return false;
    }
    for (auto& voice : mVoices)
    {
        voice.stream = SDL_CreateAudioStream(&source, &output);
        if (!voice.stream || !SDL_BindAudioStream(SDL_GetAudioStreamDevice(mStream), voice.stream))
        {
            std::cerr << "Impact audio unavailable: " << SDL_GetError() << "\n";
            for (auto& cleanup : mVoices)
            {
                if (cleanup.stream)
                {
                    SDL_DestroyAudioStream(cleanup.stream);
                }
                cleanup = {};
            }
            mImpactPCM.clear();
            return false;
        }
    }
    setPlaybackRate(mRate);
    return true;
}

void MusicPlayer::playImpacts(const std::vector<MeteorImpact>& impacts, const glm::vec3& listener, float time)
{
    if (!mRunning || mPaused || mImpactPCM.empty() || impacts.empty() || time - mLastImpactTime < 0.12f)
    {
        return;
    }
    const float gain = impactGain(impacts, listener);
    if (gain < 0.01f)
    {
        return;
    }
    // Bounded overlapping voices; mix the strongest contact of each frame.
    unsigned selected = mNextVoice;
    for (unsigned i = 0; i < mVoices.size(); ++i)
    {
        if (SDL_GetAudioStreamQueued(mVoices[i].stream) == 0)
        {
            selected = i;
            break;
        }
    }
    auto& voice = mVoices[selected];
    voice.gain = gain;
    SDL_SetAudioStreamGain(voice.stream, mMuted ? 0 : gain);
    if (SDL_ClearAudioStream(voice.stream) &&
        SDL_PutAudioStreamData(voice.stream, mImpactPCM.data(), int(mImpactPCM.size())) &&
        SDL_FlushAudioStream(voice.stream))
    {
        mLastImpactTime = time;
        ++mImpactPlayCount;
        mNextVoice = (selected + 1) % mVoices.size();
    }
}

float MusicPlayer::impactGain(const std::vector<MeteorImpact>& impacts, const glm::vec3& listener)
{
    float gain = 0;
    for (const auto& impact : impacts)
    {
        const float distance = glm::distance(listener, impact.position);
        const float strength =
            std::clamp(impact.meteorScale * glm::length(impact.velocity) / 14.0f, 0.2f, 1.0f);
        gain = std::max(gain, 0.25f * (0.25f + 0.4f * strength) / (1 + distance * distance / (180 * 180)));
    }
    return gain;
}
