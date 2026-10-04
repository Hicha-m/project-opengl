#include <cassert>
#include <cmath>
#include "cinematic/MainSequence.h"

int main()
{
    Scene scene;
    Timeline timeline;
    CinematicCamera camera;
    assert(!MainSequence::build(timeline, camera, scene));
    SceneObject earth;
    earth.name = "Earth";
    earth.transform.position = glm::vec3(30, 50, 0);
    earth.transform.scale = glm::vec3(10);
    scene.addObject(earth);
    SceneObject clouds;
    clouds.name = "EarthClouds";
    scene.addObject(clouds);
    assert(MainSequence::build(timeline, camera, scene));
    assert(timeline.isPlaying() && timeline.getDuration() == 30);
    assert(std::abs(glm::distance(camera.getPosition(), earth.transform.position) - 30) < 0.001f);
    timeline.update(10);
    assert(std::abs(glm::distance(camera.getPosition(), earth.transform.position) - 40) < 0.001f);
    // Grow the scene after binding tracks: vector reallocations must not break them.
    for (int i = 0; i < 100; ++i) scene.addObject(SceneObject{});
    timeline.update(5);
    assert(std::abs(camera.getFOV() - 55) < 0.001f);
    assert(std::abs(scene.findObject("Earth")->transform.rotation.y - glm::radians(1500.0f)) < 0.001f);
    assert(scene.findObject("Earth")->transform.scale.x == 10);
    timeline.update(30);
    assert(!timeline.isPlaying() && timeline.getTime() == 30);
    assert(std::abs(glm::distance(camera.getPosition(), earth.transform.position) - 300) < 0.001f);
    assert(camera.getFOV() == 65);
    assert(std::abs(scene.findObject("EarthClouds")->transform.rotation.y - glm::radians(150.0f)) < 0.001f);
    timeline.stop();
    assert(scene.findObject("Earth")->transform.rotation.y == 0);
    timeline.play();
    timeline.update(30);
    assert(timeline.getTime() == 30);
}
