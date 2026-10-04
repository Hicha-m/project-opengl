#include "systems/SolarSystem.h"
#include <cmath>

const glm::vec3 SolarSystem::SunCenter{-290,50,0};
const std::array<PlanetOrbit,8> SolarSystem::Planets{{
    {"Mercury",120,3.8f,0.008f,0.7f,0.02f},
    {"Venus",220,9.5f,0.004f,2.2f,0.015f},
    {"Earth",320,10,0.001f,0,0},
    {"Mars",440,5.3f,0.00065f,3.4f,0.02f},
    {"Jupiter",600,28,0.0003f,0.9f,0.01f},
    {"Saturn",760,23,0.00017f,4.7f,0.015f},
    {"Uranus",920,17,0.00010f,2.7f,0.02f},
    {"Neptune",1080,16.5f,0.00007f,5.5f,0.015f}
}};
glm::vec3 SolarSystem::planetPosition(const PlanetOrbit& p,float time) {
    const float a=p.phase+p.angularSpeed*time;
    return SunCenter+p.radius*glm::vec3(std::cos(a),std::sin(a)*std::sin(p.tilt),std::sin(a)*std::cos(p.tilt));
}
glm::vec3 SolarSystem::planetVelocity(const PlanetOrbit& p,float time) {
    const float a=p.phase+p.angularSpeed*time;
    return p.radius*p.angularSpeed*glm::vec3(-std::sin(a),std::cos(a)*std::sin(p.tilt),std::cos(a)*std::cos(p.tilt));
}
void SolarSystem::reset(Scene& scene) {
    mMoonReleased=false; mMoonVelocity=glm::vec3(0); mMoonPosition=glm::vec3(0);
    update(scene,0,0,false);
}
void SolarSystem::update(Scene& scene,float time,float dt,bool destroyed) {
    if(!std::isfinite(time) || !std::isfinite(dt) || dt<0) return;
    for(const auto& p:Planets) if(auto* object=scene.findObject(p.name)) {
        if(p.name==std::string("Earth") && destroyed) continue;
        object->transform.position=planetPosition(p,time);
        if(p.name!=std::string("Earth")) object->transform.rotation.y=time*0.025f;
    }
    auto* earth=scene.findObject("Earth");
    if(!earth) return;
    if(auto* clouds=scene.findObject("EarthClouds")) clouds->transform.position=earth->transform.position;
    if(auto* rings=scene.findObject("SaturnRings")) if(auto* saturn=scene.findObject("Saturn")) rings->transform.position=saturn->transform.position;
    const float angle=0.6f+MoonSpeed*time;
    const auto relative=MoonRadius*glm::vec3(std::cos(angle),0.12f*std::sin(angle),std::sin(angle));
    if(auto* moon=scene.findObject("Moon")) {
        if(!destroyed) {
            moon->transform.position=earth->transform.position+relative;
            moon->transform.rotation.y=-angle;
        } else if(!mMoonReleased) {
            // Freeze the last orbital pose and retain its tangent velocity.
            mMoonReleased=true; mMoonPosition=moon->transform.position;
            const float previousTime=time-dt, previousAngle=0.6f+MoonSpeed*previousTime;
            mMoonVelocity=planetVelocity(Planets[2],previousTime)+MoonRadius*MoonSpeed*
                glm::vec3(-std::sin(previousAngle),0.12f*std::cos(previousAngle),std::cos(previousAngle));
            mMoonPosition+=mMoonVelocity*dt;
            moon->transform.position=mMoonPosition;
        } else {
            mMoonPosition+=mMoonVelocity*dt; moon->transform.position=mMoonPosition;
        }
    }
    if(auto* orbit=scene.findObject("OrbitEarth")) orbit->visible=!destroyed;
    if(auto* orbit=scene.findObject("OrbitMoon")) {
        orbit->transform.position=earth->transform.position; orbit->visible=!destroyed;
    }
}
