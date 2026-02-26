#include "raylib.h"
#include "dn_cycle.h"
#include "rover.h"

#include <cmath>

Vector3 center{ .0f, .0f, .0f };

Vector3 vecAdd(Vector3 a, Vector3 b)
{
    Vector3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

Vector3 vecSub(Vector3 a, Vector3 b)
{
    Vector3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

Vector3 vecNormalize(Vector3 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    if (len == 0)
        return { 0,0,0 };

    Vector3 r;

    r.x = v.x / len;
    r.y = v.y / len;
    r.z = v.z / len;

    return r;
}