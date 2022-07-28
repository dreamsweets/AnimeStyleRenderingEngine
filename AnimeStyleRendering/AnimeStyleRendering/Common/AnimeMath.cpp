#include "AnimeMath.h"
namespace glm {
vec3 glm::GetPostion(mat4 transformMatrix)
{
    return vec3(transformMatrix[3].x, transformMatrix[3].y, transformMatrix[3].z);
}
vec3 GetScale(mat4 transformMatrix)
{
    return vec3(transformMatrix[0].x, transformMatrix[1].y, transformMatrix[2].z);
}
vec3 GetRotationEuler(mat4 transformMatrix)
{
    return GetEulerRot(GetRotationQuat(transformMatrix));
}
vec3 GetEulerRot(quat quaternion)
{
    vec3 euler = math::eulerAngles(quaternion);
    vec3 degree;
    
    degree.x = degrees(euler.x);
    degree.y = degrees(euler.y);
    degree.z = degrees(euler.z);

    return degree;
}
quat GetRotationQuat(mat4 transformMatrix)
{
    return math::toQuat(transformMatrix);
}
}