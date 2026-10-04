#include <cassert>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "cinematic/MainSequence.h"
#include "systems/EarthDamageSystem.h"
#include "systems/EarthBreakupSystem.h"

struct Frame {
    std::vector<Meteor> meteors;
    std::vector<MeteorImpact> impacts;
    std::vector<EarthFragment> fragments;
    float destruction;
    glm::vec3 camera;
};
int main()
{
    Scene scene; Timeline timeline; CinematicCamera camera;
    MeteorSystem system; MeteorShower shower(system);
    EarthDamageSystem damage; EarthBreakupSystem breakup;
    assert(!MainSequence::build(timeline,camera,scene,shower));
    SceneObject earth; earth.name="Earth"; earth.transform.position={30,50,0}; earth.transform.scale=glm::vec3(10);
    scene.addObject(earth); SceneObject clouds; clouds.name="EarthClouds"; scene.addObject(clouds);
    assert(MainSequence::build(timeline,camera,scene,shower));
    assert(timeline.getDuration()==MainSequence::Duration);
    // Tracks keep working after scene storage reallocates.
    for(int i=0;i<100;++i) scene.addObject(SceneObject{});
    const SphereCollider collider{earth.transform.position,10};
    std::vector<Frame> replay;
    float firstBreak=0;
    for(int pass=0;pass<2;++pass) {
        MainSequence::reset(timeline,shower,system); damage.clear(); breakup.reset();
        assert(!timeline.isPlaying() && timeline.getTime()==0 && system.size()==0);
        assert(scene.findObject("Earth")->transform.rotation==glm::vec3(0));
        assert(std::abs(glm::distance(camera.getPosition(),earth.transform.position)-34)<0.001f);
        timeline.play(); MeteorId lastId=0; std::size_t impacts=0, births=0, coreContacts=0;
        float minScale=10,maxScale=0,minX=100,maxX=-100,breakTime=0;
        for(int frame=0;frame<360;++frame) {
            timeline.update(0.25f);
            const bool hittingCore=breakup.active();
            if(hittingCore) system.update(0.25f,breakup.coreCollider()); else system.update(0.25f,collider);
            damage.update(0.25f);
            if(!hittingCore) damage.consume(system.impacts(),scene.findObject("Earth")->transform);
            breakup.update(0.25f,damage.destructionLevel(),scene.findObject("Earth")->transform);
            if(breakup.active()) { shower.stop(); if(!breakTime) breakTime=timeline.getTime(); }
            shower.update(0.25f);
            if(hittingCore) coreContacts+=system.impacts().size();
            else impacts+=system.impacts().size();
            const auto vp=glm::perspective(glm::radians(camera.getFOV()),640.0f/480,0.1f,10000.0f)*camera.getViewMatrix();
            for(const auto& meteor:system.meteors()) if(meteor.id>lastId) {
                lastId=meteor.id; ++births;
                const auto clip=vp*glm::vec4(meteor.transform.position,1);
                // Births outside the frame, including a margin for meteor radius.
                assert(clip.w<=0 || std::abs(clip.x)>clip.w*1.05f || std::abs(clip.y)>clip.w*1.05f);
                assert(meteor.velocity.y<0 && meteor.transform.position.y>earth.transform.position.y+140);
                minScale=std::min(minScale,meteor.transform.scale.x); maxScale=std::max(maxScale,meteor.transform.scale.x);
                minX=std::min(minX,meteor.velocity.x); maxX=std::max(maxX,meteor.velocity.x);
            }
            if(timeline.getTime()<8) assert(!shower.isRunning() && births==0 && damage.destructionLevel()==0);
            if(timeline.getTime()==24 || timeline.getTime()==40 || timeline.getTime()==52)
                if(!pass) std::cout<<"t="<<timeline.getTime()<<" destruction="<<damage.destructionLevel()<<" impacts="<<impacts<<"\n";
            Frame state{system.meteors(),system.impacts(),breakup.fragments(),damage.destructionLevel(),camera.getPosition()};
            if(!pass) replay.push_back(state); else {
                const auto& previous=replay[frame];
                assert(state.destruction==previous.destruction && state.camera==previous.camera);
                assert(state.meteors.size()==previous.meteors.size() && state.impacts.size()==previous.impacts.size());
                for(std::size_t i=0;i<state.meteors.size();++i) {
                    const auto& a=state.meteors[i]; const auto& b=previous.meteors[i];
                    assert(a.id==b.id && a.transform.position==b.transform.position && a.transform.scale==b.transform.scale);
                    assert(a.velocity==b.velocity && a.lifetime==b.lifetime);
                }
                for(std::size_t i=0;i<state.impacts.size();++i) {
                    const auto& a=state.impacts[i]; const auto& b=previous.impacts[i];
                    assert(a.position==b.position && a.normal==b.normal && a.velocity==b.velocity && a.meteorScale==b.meteorScale);
                }
                for(std::size_t i=0;i<state.fragments.size();++i) {
                    assert(state.fragments[i].transform.position==previous.fragments[i].transform.position);
                    assert(state.fragments[i].transform.rotation==previous.fragments[i].transform.rotation);
                }
            }
        }
        assert(coreContacts>0);
        assert(impacts>20 && minScale<0.2f && maxScale>0.65f && minX<-2 && maxX>2);
        assert(breakTime>40 && breakTime<65); // Long buildup and at least 25 s of aftermath.
        assert(!timeline.isPlaying() && timeline.getTime()==MainSequence::Duration && !shower.isRunning());
        assert(system.size()==0 && breakup.active() && breakup.fragments().size()==32);
        if(!pass) firstBreak=breakTime; else assert(breakTime==firstBreak);
        std::cout<<"Rupture at "<<breakTime<<" s; "<<impacts<<" impacts, "<<births<<" meteors\n";
    }
    MainSequence::reset(timeline,shower,system); timeline.play(); timeline.update(MainSequence::Duration);
    assert(!shower.isRunning()); // Large step crosses all events in chronological order.
    // Also restart halfway through an active wave.
    MainSequence::reset(timeline,shower,system); timeline.play(); timeline.update(42); shower.update(1);
    assert(system.size()>0); MainSequence::reset(timeline,shower,system);
    assert(system.size()==0 && system.impacts().empty() && !shower.isRunning());
    std::cout<<"Final cinematic progression, offscreen births, varied bombardment and exact replay passed\n";
}
