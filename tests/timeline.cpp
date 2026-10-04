#include <cassert>
#include <cmath>
#include <limits>
#include "src/animation/Timeline.h"
#include "src/animation/TransformTrack.h"
#include "src/animation/EventTrack.h"
#include "src/animation/CameraTrack.h"

int main()
{
    Timeline timeline;
    Transform transform;
    transform.scale = glm::vec3(3);
    auto movement = std::make_unique<TransformTrack>(transform);
    movement->positionTrack().addKeyframe(0, glm::vec3(0));
    movement->positionTrack().addKeyframe(10, glm::vec3(10));
    timeline.addTrack(std::move(movement));
    CinematicCamera camera;
    camera.positionTrack().addKeyframe(0, glm::vec3(0, 0, 10));
    camera.positionTrack().addKeyframe(10, glm::vec3(10, 0, 10));
    timeline.addTrack(std::make_unique<CameraTrack>(camera));
    std::vector<int> fired;
    auto events = std::make_unique<EventTrack>();
    events->addEvent(10, [&] { fired.push_back(10); });
    events->addEvent(0, [&] { fired.push_back(0); });
    events->addEvent(2, [&] { fired.push_back(2); });
    events->addEvent(2, [&] { fired.push_back(3); });
    timeline.addTrack(std::move(events));
    timeline.setDuration(10);
    timeline.update(5);
    assert(timeline.getTime() == 0 && fired.empty());
    timeline.play();
    timeline.update(5);
    assert((fired == std::vector<int>{0, 2, 3}));
    assert(transform.position.x == 5 && transform.scale.x == 3);
    assert(camera.getPosition().x == 5);
    timeline.pause();
    timeline.update(1);
    assert(timeline.getTime() == 5);
    timeline.play();
    timeline.update(-1);
    timeline.update(std::numeric_limits<float>::quiet_NaN());
    assert(timeline.getTime() == 5);
    timeline.update(50);
    assert(timeline.getTime() == 10 && !timeline.isPlaying());
    assert(fired.back() == 10 && fired.size() == 4);
    assert(transform.position.x == 10);
    timeline.update(1);
    assert(fired.size() == 4);
    timeline.stop();
    assert(transform.position.x == 0 && camera.getPosition().x == 0);
    assert(fired.size() == 4);
    timeline.play();
    timeline.update(10);
    assert(fired.size() == 8);
    timeline.reset();
    assert(timeline.getTime() == 0);
    // A missing dynamic scene target is harmless.
    TransformTrack missing([]() -> Transform* { return nullptr; });
    missing.reset(0);
    missing.update(0, 1);
}
