#include "scene/SceneSetup.h"
#include <stdexcept>

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
            && r.starTexture.loadTexture("textures/space/2k_stars.jpg", true);
    }
}

bool SceneSetup::build(Scene& scene, LightManager& lightManager, SceneResources& resources)
{
    if (!loadResources(resources)) return false;
    SceneObject earthObject("Earth",&resources.earthSphere.getMesh(),&resources.earthShader);
    earthObject.transform.position = glm::vec3(30.0f, 50.0f, 0.0f);
    earthObject.transform.scale = glm::vec3(10.0f);
    earthObject.material.addTexture("dayMap",&resources.earthDayTexture,0);
    earthObject.material.addTexture("nightMap",&resources.earthNightTexture,1);
    earthObject.material.addTexture("specularMap",&resources.earthSpecularTexture,2);
    earthObject.material.addTexture("normalMap",&resources.earthNormalTexture,3);
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

    sunObject.transform.position = glm::vec3(100.0f, 200.0f, 0.0f);
    sunObject.transform.scale = glm::vec3(50.0f);
    sunObject.material.addTexture("sunMap",&resources.sunTexture,0);
    sunObject.material.receivesLighting = false;
    scene.addObject(sunObject);

    SceneObject starObject("Stars",&resources.starSphere.getMesh(),&resources.starShader);

    starObject.transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    starObject.transform.scale = glm::vec3(500.0f);
    starObject.material.addTexture("starMap",&resources.starTexture,0);
    starObject.material.depthLEqual = true;
    starObject.material.depthWrite = false;
    scene.addObject(starObject);

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
    if (auto* stars = scene.findObject("Stars"))
        stars->transform.position = cameraPosition;
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
