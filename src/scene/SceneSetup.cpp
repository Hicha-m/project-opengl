#include "scene/SceneSetup.h"
#include "platform/ResourcePaths.h"
#include <stdexcept>
#include "systems/EarthDamageSystem.h"
#include "systems/SolarSystem.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{
    bool loadShader(ShaderProgram& shader, const char* vertex, const char* fragment)
    {
        if (!shader.loadShaders(vertex, fragment)) return false;
        // The legacy shader loader logs compile errors but returns true after linking.
        GLint linked = GL_FALSE;
        glGetProgramiv(shader.getProgram(), GL_LINK_STATUS, &linked);
        return linked == GL_TRUE;
    }

    bool loadShuttle(Scene& scene, SceneResources& r)
    {
        if (!loadShader(r.shuttleShader,"shaders/shuttle.vert","shaders/shuttle.frag")
            || !loadShader(r.exhaustShader,"shaders/shuttle.vert","shaders/exhaust.frag")
            || !r.shuttleSource.loadOBJ("models/shuttle/shuttle.obj")) return false;
        struct Surface { glm::vec3 color{0.8f}; float metal=0, rough=0.7f; std::string diffuse,normal; };
        std::unordered_map<std::string,Surface> surfaces;
        std::ifstream file(ResourcePaths::resolve("models/shuttle/shuttle.mtl"));
        if(!file) return false;
        std::string line,name;
        while(std::getline(file,line)) {
            std::istringstream row(line); std::string key; row>>key;
            if(key=="newmtl") row>>name;
            else if(!name.empty()) {
                auto& material=surfaces[name];
                if(key=="Kd") row>>material.color.x>>material.color.y>>material.color.z;
                else if(key=="Pm") row>>material.metal;
                else if(key=="Pr") row>>material.rough;
                else if(key=="map_Kd") row>>material.diffuse;
                else if(key=="norm") row>>material.normal;
            }
        }
        const auto& vertices=r.shuttleSource.vertices();
        glm::vec3 low(vertices.front().position), high(low);
        for(const auto& v:vertices) { low=glm::min(low,v.position); high=glm::max(high,v.position); }
        const auto center=(low+high)*0.5f, extent=high-low;
        const float size=std::max({extent.x,extent.y,extent.z});
        for(const auto& section:r.shuttleSource.sections()) {
            auto part=std::make_unique<ShuttlePart>();
            part->name=r.shuttleParts.empty()?"Shuttle":"Shuttle/"+section.material;
            std::vector<Vertex> subset(vertices.begin()+section.first,vertices.begin()+section.first+section.count);
            for(auto& v:subset) v.position=(v.position-center)/size;
            part->mesh.setVertices(subset);
            const auto surface=surfaces[section.material];
            const bool exhaust=surface.diffuse=="Untitled101_20260818133021.png";
            auto* shader=exhaust?&r.exhaustShader:&r.shuttleShader;
            part->material=Material(shader);
            auto& material=part->material;
            material.setInt("isExhaust",exhaust);
            if(exhaust) {
                float anchor=subset.front().position.z;
                for(const auto& v:subset) anchor=std::min(anchor,v.position.z);
                material.setFloat("exhaustAnchor",anchor);
                material.setFloat("exhaustTime",0);
                // Four crossing planes per engine share the same animation phase.
                material.setFloat("exhaustPhase",section.material=="Untitled101_20260818133021"
                    || section.material=="Untitled101_20260818133021.002"
                    || section.material=="Untitled101_20260818133021.003"
                    || section.material=="Untitled101_20260818133021.004"?0.0f:1.7f);
                material.blending=true; material.additiveBlending=true; material.depthWrite=false;
            }
            if(!surface.diffuse.empty()) {
                if(!part->diffuse.loadTexture("models/shuttle/"+surface.diffuse,true)) return false;
                material.addTexture("surfaceMap",&part->diffuse,0);
            }
            if(!surface.normal.empty()) {
                if(!part->normal.loadTexture("models/shuttle/"+surface.normal,true)) return false;
                material.addTexture("normalMap",&part->normal,1);
            }
            material.setInt("hasSurfaceMap",!surface.diffuse.empty());
            material.setInt("hasNormalMap",!surface.normal.empty());
            material.setVec3("baseColor",surface.color);
            material.setVec3("sunPosition",SolarSystem::SunCenter);
            material.setFloat("metallic",surface.metal); material.setFloat("roughness",surface.rough);
            SceneObject object(part->name,&part->mesh,shader);
            object.material=material; object.visible=false;
            scene.addObject(object); r.shuttleParts.push_back(std::move(part));
        }
        return !r.shuttleParts.empty();
    }

    bool loadResources(SceneResources& r)
    {
        return loadShader(r.sunShader, "shaders/sun.vert", "shaders/sun.frag")
            && loadShader(r.earthShader, "shaders/earth.vert", "shaders/earth.frag")
            && loadShader(r.cloudShader, "shaders/clouds.vert", "shaders/clouds.frag")
            && loadShader(r.starShader, "shaders/stars.vert", "shaders/stars.frag")
            && r.earthDayTexture.loadTexture("textures/earth/2k_earth_daymap.jpg", true)
            && r.earthNightTexture.loadTexture("textures/earth/2k_earth_nightmap.jpg", true)
            && r.earthCloudsTexture.loadTexture("textures/earth/2k_earth_clouds.jpg", true)
            && r.earthSpecularTexture.loadTexture("textures/earth/2k_earth_specular_map.png", true)
            && r.earthNormalTexture.loadTexture("textures/earth/2k_earth_normal_map.png", true)
            && r.sunTexture.loadTexture("textures/sun/2k_sun.jpg", true)
            && r.starTexture.loadTexture("textures/space/2k_stars.jpg", true)
            && r.milkyWayTexture.loadTexture("textures/space/2k_stars_milky_way.jpg",true)
            && r.galaxyTexture.loadTexture("textures/space/2k_milky_way.jpg",true)
            && r.moonTexture.loadTexture("textures/moon/2k_moon.jpg",true)
            && r.saturnRingTexture.loadTexture("textures/planetes/2k_saturn_ring_alpha.png",true)
            && loadShader(r.planetShader,"shaders/planet.vert","shaders/planet.frag")
            && loadShader(r.ringShader,"shaders/planet.vert","shaders/ring.frag")
            && loadShader(r.orbitShader,"shaders/planet.vert","shaders/orbit.frag");
    }
}

bool SceneSetup::build(Scene& scene, LightManager& lightManager, SceneResources& resources)
{
    if (!loadResources(resources)) return false;
    const char* files[]={"2k_mercury.jpg","2k_venus_atmosphere.jpg","2k_mars.jpg","2k_jupiter.jpg",
        "2k_saturn.jpg","2k_uranus.jpg","2k_neptune.jpg"};
    for(unsigned i=0;i<7;++i)
        if(!resources.planetTextures[i].loadTexture(std::string("textures/planetes/")+files[i],true)) return false;
    auto annulus=[](Mesh& mesh,float inner,float outer) {
        std::vector<Vertex> vertices;
        for(int i=0;i<256;++i) {
            const float a=i*6.2831853f/256, b=(i+1)*6.2831853f/256;
            auto vertex=[](float angle,float radius,float u) {
                Vertex v{}; v.position={radius*std::cos(angle),0,radius*std::sin(angle)};
                v.normal={0,1,0}; v.texCoords={u,0.5f}; return v;
            };
            const Vertex points[]={vertex(a,inner,0),vertex(b,inner,0),vertex(b,outer,1),
                vertex(a,inner,0),vertex(b,outer,1),vertex(a,outer,1)};
            vertices.insert(vertices.end(),std::begin(points),std::end(points));
        }
        mesh.setVertices(vertices);
    };
    annulus(resources.orbitMesh,0.98f,1.02f);
    annulus(resources.ringMesh,1.35f,2.3f);
    const EarthDamageSystem emptyDamage;
    if (!resources.earthDamageTexture.createRed(EarthDamageSystem::Width, EarthDamageSystem::Height,
        emptyDamage.pixels().data())) return false;
    if (!resources.earthHeatTexture.createRed(EarthDamageSystem::Width, EarthDamageSystem::Height,
        emptyDamage.heatPixels().data(), true)) return false;
    SceneObject earthObject("Earth",&resources.earthSphere.getMesh(),&resources.earthShader);
    earthObject.transform.position = glm::vec3(30.0f, 50.0f, 0.0f);
    earthObject.transform.scale = glm::vec3(10.0f);
    earthObject.material.addTexture("dayMap",&resources.earthDayTexture,0);
    earthObject.material.addTexture("nightMap",&resources.earthNightTexture,1);
    earthObject.material.addTexture("specularMap",&resources.earthSpecularTexture,2);
    earthObject.material.addTexture("normalMap",&resources.earthNormalTexture,3);
    earthObject.material.addTexture("damageMap", &resources.earthDamageTexture, 4);
    earthObject.material.addTexture("heatMap", &resources.earthHeatTexture, 5);
    earthObject.material.setFloat("destructionLevel", 0.0f);
    earthObject.material.receivesLighting = true;
    scene.addObject(earthObject);

    SceneObject cloudObject("EarthClouds",&resources.earthSphere.getMesh(),&resources.cloudShader);
    cloudObject.transform.position = glm::vec3(30.0f, 50.0f, 0.0f);
    cloudObject.transform.scale = glm::vec3(10.1f);
    cloudObject.material.addTexture("cloudMap",&resources.earthCloudsTexture,0);
    cloudObject.material.blending = true;
    cloudObject.material.receivesLighting = true;
    scene.addObject(cloudObject);

    SceneObject sunObject("Sun",&resources.sunSphere.getMesh(),&resources.sunShader);

    sunObject.transform.position = SolarSystem::SunCenter;
    sunObject.transform.scale = glm::vec3(50.0f);
    sunObject.material.addTexture("sunMap",&resources.sunTexture,0);
    sunObject.material.setFloat("emission", 1.0f);
    sunObject.material.setFloat("bloomEmission", 3.0f);
    sunObject.material.receivesLighting = false;
    scene.addObject(sunObject);

    unsigned textureIndex=0;
    for(const auto& p:SolarSystem::Planets) {
        if(std::string(p.name)=="Earth") continue;
        SceneObject planet(p.name,&resources.earthSphere.getMesh(),&resources.planetShader);
        planet.transform.position=SolarSystem::planetPosition(p,0);
        planet.transform.scale=glm::vec3(p.size);
        if(std::string(p.name)=="Saturn") planet.transform.rotation.z=glm::radians(26.7f);
        if(std::string(p.name)=="Uranus") planet.transform.rotation.z=glm::radians(98.0f);
        planet.material.addTexture("surfaceMap",&resources.planetTextures[textureIndex++],0);
        planet.material.setVec3("sunPosition",SolarSystem::SunCenter);
        scene.addObject(planet);
    }
    SceneObject moon("Moon",&resources.earthSphere.getMesh(),&resources.planetShader);
    moon.transform.scale=glm::vec3(2.7f);
    moon.material.addTexture("surfaceMap",&resources.moonTexture,0);
    moon.material.setVec3("sunPosition",SolarSystem::SunCenter);
    scene.addObject(moon);
    if(!loadShuttle(scene,resources)) return false;
    SceneObject rings("SaturnRings",&resources.ringMesh,&resources.ringShader);
    rings.transform.scale=glm::vec3(23);
    rings.transform.rotation.z=glm::radians(26.7f);
    rings.material.addTexture("ringMap",&resources.saturnRingTexture,0);
    rings.material.setVec3("sunPosition",SolarSystem::SunCenter);
    rings.material.blending=true; rings.material.depthWrite=false;
    scene.addObject(rings);
    for(const auto& p:SolarSystem::Planets) {
        SceneObject orbit(std::string("Orbit")+p.name,&resources.orbitMesh,&resources.orbitShader);
        orbit.transform.position=SolarSystem::SunCenter;
        orbit.transform.scale=glm::vec3(p.radius);
        orbit.transform.rotation.x=-p.tilt;
        orbit.material.blending=true; orbit.material.depthWrite=false;
        scene.addObject(orbit);
    }
    SceneObject moonOrbit("OrbitMoon",&resources.orbitMesh,&resources.orbitShader);
    moonOrbit.transform.scale={SolarSystem::MoonRadius,SolarSystem::MoonRadius,SolarSystem::MoonRadius};
    moonOrbit.transform.rotation.x=-std::atan(0.12f);
    moonOrbit.material.blending=true; moonOrbit.material.depthWrite=false;
    scene.addObject(moonOrbit);
    SolarSystem initialSolar; initialSolar.reset(scene);

    SceneObject starObject("Stars",&resources.starSphere.getMesh(),&resources.starShader);

    starObject.transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    starObject.transform.scale = glm::vec3(500.0f);
    starObject.material.addTexture("starMap",&resources.starTexture,0);
    starObject.material.addTexture("milkyWayMap",&resources.milkyWayTexture,1);
    starObject.material.addTexture("galaxyMap",&resources.galaxyTexture,2);
    starObject.material.setFloat("milkyWayBlend",0);
    starObject.material.setFloat("galaxyBlend",0);
    starObject.material.setFloat("galaxyScale",1);
    starObject.material.setFloat("galaxyOpacity",1);
    starObject.material.depthLEqual = true;
    starObject.material.depthWrite = false;
    // Draw the sky before transparent rings/orbit guides (which do not write depth).
    scene.objects.insert(scene.objects.begin(),starObject);

    DirectionalLight sunLight;

    sunLight.direction = glm::normalize(earthObject.transform.position - sunObject.transform.position);
    sunLight.color = glm::vec3(1.0f,0.95f,0.8f);
    sunLight.intensity =1.0f;

    // Set the directional light
    lightManager.setDirectionalLight(sunLight);
    return true;

}

void SceneSetup::update(Scene& scene, const glm::vec3& cameraPosition)
{
    if (auto* stars = scene.findObject("Stars")) {
        stars->transform.position = cameraPosition;
        const float distance=glm::distance(cameraPosition,SolarSystem::SunCenter);
        stars->material.setFloat("milkyWayBlend",glm::smoothstep(3000.0f,5000.0f,distance));
        stars->material.setFloat("galaxyBlend",glm::smoothstep(5000.0f,7000.0f,distance));
        stars->material.setFloat("galaxyScale",5000.0f/std::max(distance,5000.0f));
        stars->material.setFloat("galaxyOpacity",1.0f-glm::smoothstep(18000.0f,32000.0f,distance));
    }
    const float solarDistance=glm::distance(cameraPosition,SolarSystem::SunCenter);
    const float orbitOpacity=glm::smoothstep(450.0f,900.0f,solarDistance)
        *(1.0f-glm::smoothstep(12000.0f,24000.0f,solarDistance));
    for(auto& object:scene.objects) if(object.name.rfind("Orbit",0)==0)
        object.material.setFloat("opacity",orbitOpacity);
}

SphereCollider SceneSetup::earthCollider(const Scene& scene, const SceneResources& resources)
{
    const auto* earth = scene.findObject("Earth");
    if (!earth) throw std::invalid_argument("Earth collider requires Earth");
    const auto& scale = earth->transform.scale;
    SphereCollider collider{earth->transform.position, resources.earthSphere.getRadius() * scale.x};
    if (!collider.isValid() || scale.x <= 0 || scale.x != scale.y || scale.x != scale.z)
        throw std::invalid_argument("Earth collider requires a positive uniform scale");
    return collider;
}
