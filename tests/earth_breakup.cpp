#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "systems/EarthBreakupSystem.h"
#include "systems/EarthDamageSystem.h"
#include "systems/MeteorShower.h"

int main()
{
    EarthBreakupSystem breakup;
    assert(!breakup.active() && !breakup.graphicsReady() && !breakup.coreCollider().isValid());
    assert(breakup.fragments().size() == EarthBreakupSystem::FragmentCount);
    Transform earth; earth.position={30,50,0}; earth.rotation={0.2f,0.7f,0.1f}; earth.scale=glm::vec3(10);
    breakup.update(1,EarthBreakupSystem::Threshold-0.01f,earth); assert(!breakup.active());
    breakup.update(0,1,earth); breakup.update(-1,1,earth);
    breakup.update(std::numeric_limits<float>::quiet_NaN(),1,earth); assert(!breakup.active());
    std::vector<EarthFragment> replay;
    for(int pass=0;pass<2;++pass) {
        breakup.reset(); assert(!breakup.active()); breakup.update(0.1f,1,earth); assert(breakup.active());
        assert(breakup.coreLight().position == earth.position && breakup.coreLight().intensity > 100);
        for(const auto& f:breakup.fragments()) {
            const auto reconstructed = glm::vec3(f.transform.getMatrix()*glm::vec4(-f.pivot,1));
            assert(glm::distance(reconstructed,earth.position) < 0.00001f);
            assert(glm::dot(f.velocity,f.transform.position-earth.position) > 0);
        }
        const auto core=breakup.coreCollider();
        assert(core.center==earth.position && std::abs(core.radius-5.5f)<0.00001f);
        // Swept contact absorbs a meteor even if a frame crosses the whole core.
        // A near miss survives: absorption uses the physical radius, not the bloom halo.
        MeteorSystem incoming; Transform meteor;
        meteor.position=earth.position+glm::vec3(-20,0,0); meteor.scale=glm::vec3(0.2f);
        assert(incoming.spawn(meteor,{200,0,0},10));
        meteor.position.y+=8; assert(incoming.spawn(meteor,{200,0,0},10));
        incoming.update(0.25f,core);
        assert(incoming.size()==1 && incoming.impacts().size()==1);
        assert(std::abs(glm::distance(incoming.impacts()[0].position,core.center)-core.radius)<0.00001f);
        const auto birth=breakup.fragments();
        Transform changed=earth; changed.rotation.y+=1; changed.position.x+=100;
        breakup.update(0.5f,0,changed); // Latch and captured parent stay stable.
        for(std::size_t i=0;i<birth.size();++i) {
            const auto& f=breakup.fragments()[i];
            assert(glm::distance(f.transform.position,birth[i].transform.position+f.velocity*0.5f) < 0.00001f);
            assert(f.transform.rotation != birth[i].transform.rotation);
        }
        assert(breakup.coreLight().position == earth.position);
        assert(breakup.coreCollider().center==core.center && breakup.coreCollider().radius==core.radius);
        if(!pass) replay=breakup.fragments();
        else for(std::size_t i=0;i<replay.size();++i) {
            assert(replay[i].transform.position == breakup.fragments()[i].transform.position);
            assert(replay[i].transform.rotation == breakup.fragments()[i].transform.rotation);
            assert(replay[i].velocity == breakup.fragments()[i].velocity);
        }
        LightManager lights; PointLight flash; lights.setTransientPointLights({flash}); breakup.publish(lights);
        assert(lights.transientPointLights().size()==2 && lights.shaderPointLights()[0].intensity==450);
    }
    breakup.reset(); assert(!breakup.active() && breakup.coreLight().intensity==0 && !breakup.coreCollider().isValid());
    // Reference shower: physical threshold can be reached without graphics or Timeline.
    MeteorSystem meteors; MeteorShower shower(meteors); EarthDamageSystem damage;
    MeteorShowerConfig config;
    config.spawnRate=30; config.origin={0,17,0}; config.spawnHalfExtents={17,3,17};
    config.direction={0.15f,-1,0.1f}; config.spreadRadians=glm::radians(12.0f);
    config.minSpeed=3; config.maxSpeed=5; config.minScale=0.4f; config.maxScale=0.9f;
    config.minLifetime=4; config.maxLifetime=7; config.seed=42;
    assert(shower.configure(config)); Transform target; target.scale=glm::vec3(10);
    for(int frame=1;frame<=120;++frame) {
        if(frame==40) shower.start();
        if(frame==80) shower.stop();
        meteors.update(0.25f,{{0,0,0},10}); damage.consume(meteors.impacts(),target); shower.update(0.25f);
    }
    assert(damage.destructionLevel() >= EarthBreakupSystem::Threshold);
    std::cout << "Prepared fragment motion, core light and replay passed; shower destruction=" << damage.destructionLevel() << "\n";
}
