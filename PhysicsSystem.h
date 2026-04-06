#pragma once
#include "ECS.h"

class PhysicsSystem {
public:
    static void Update(float deltaTime, Registry& registry);
};