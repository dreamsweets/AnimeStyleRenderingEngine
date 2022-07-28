#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace glm {
constexpr vec3 front{ 0.f, 0.f, 1.f };
constexpr vec3 up{ 0.f, 1.f, 0.f };
constexpr vec3 right{ 1.f, 0.f, 0.f };

vec3 GetPostion(mat4 transformMatrix);
vec3 GetScale(mat4 transformMatrix);
vec3 GetRotationEuler(mat4 transformMatrix);
vec3 GetEulerRot(quat quaternion);
quat GetRotationQuat(mat4 transformMatrix);


} // namespace glm

namespace math = glm;

