#include "Scene.h"
#include <algorithm>

SceneObject* Scene::addObject(const std::string& name) {
    m_objects.push_back(std::make_unique<SceneObject>(name));
    return m_objects.back().get();
}

SceneObject* Scene::addObject(std::unique_ptr<SceneObject> object) {
    m_objects.push_back(std::move(object));
    return m_objects.back().get();
}

void Scene::removeObject(SceneObject* object) {
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [object](const std::unique_ptr<SceneObject>& o) {
            return o.get() == object;
        });

    if (it != m_objects.end()) {
        m_objects.erase(it);
    }
}

void Scene::clear() {
    m_objects.clear();
}

void Scene::update() {
    for (auto& obj : m_objects) {
        obj->update();
    }
}

SceneObject* Scene::getObject(size_t index) const {
    if (index < m_objects.size()) {
        return m_objects[index].get();
    }
    return nullptr;
}

SceneObject* Scene::findObject(const std::string& name) const {
    for (const auto& obj : m_objects) {
        if (obj->getName() == name) {
            return obj.get();
        }
    }
    return nullptr;
}

BoundingBox Scene::getSceneBounds() const {
    BoundingBox bounds;
    for (const auto& obj : m_objects) {
        if (obj->isVisible() && obj->getMesh()) {
            bounds.expand(obj->getWorldBounds());
        }
    }
    return bounds;
}

glm::vec3 Scene::getSceneCenter() const {
    BoundingBox bounds = getSceneBounds();
    return bounds.getCenter();
}

float Scene::getSceneRadius() const {
    BoundingBox bounds = getSceneBounds();
    return bounds.getRadius();
}
