#pragma once

#include "SceneObject.h"
#include "BoundingBox.h"
#include <vector>
#include <memory>

class Scene {
public:
    Scene() = default;

    SceneObject* addObject(const std::string& name = "Object");
    SceneObject* addObject(std::unique_ptr<SceneObject> object);
    void removeObject(SceneObject* object);
    void clear();

    SceneObject* getObject(size_t index) const;
    SceneObject* findObject(const std::string& name) const;
    size_t getObjectCount() const { return m_objects.size(); }

    const std::vector<std::unique_ptr<SceneObject>>& getObjects() const { return m_objects; }

    BoundingBox getSceneBounds() const;
    glm::vec3 getSceneCenter() const;
    float getSceneRadius() const;

private:
    std::vector<std::unique_ptr<SceneObject>> m_objects;
};
