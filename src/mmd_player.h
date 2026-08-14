#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

struct Vec3 { float x, y, z; };
struct Quat { float x, y, z, w; };

static Vec3 Vec3Add(Vec3 a, Vec3 b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

static Vec3 Vec3Sub(Vec3 a, Vec3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static Vec3 Vec3Scale(Vec3 value, float scale) {
  return {value.x * scale, value.y * scale, value.z * scale};
}

static float Vec3Dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 Vec3Cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y,
          a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

static Vec3 Vec3Normalize(Vec3 value) {
  const float length = sqrtf(Vec3Dot(value, value));
  if (length < 1e-6f)
    return {0, 0, 0};
  return {value.x / length, value.y / length, value.z / length};
}

static Quat QuatMul(Quat a, Quat b) {
  return {
    a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
    a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
    a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
    a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
  };
}

static Quat QuatInv(Quat q) {
  return { -q.x, -q.y, -q.z, q.w };
}

static Quat QuatNormalize(Quat q) {
  const float length = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  if (length < 1e-6f)
    return {0, 0, 0, 1};
  return {q.x / length, q.y / length, q.z / length, q.w / length};
}

static Vec3 QuatRotate(Quat rotation, Vec3 value) {
  rotation = QuatNormalize(rotation);
  const Quat vector = {value.x, value.y, value.z, 0.0f};
  const Quat result = QuatMul(QuatMul(rotation, vector), QuatInv(rotation));
  return {result.x, result.y, result.z};
}

// Build a quaternion whose local X/Y/Z axes point along right/up/forward.
// The vectors are expected to be an orthonormal right-handed basis.
static Quat QuatFromBasis(Vec3 right, Vec3 up, Vec3 forward) {
  const float m00 = right.x,   m01 = up.x,   m02 = forward.x;
  const float m10 = right.y,   m11 = up.y,   m12 = forward.y;
  const float m20 = right.z,   m21 = up.z,   m22 = forward.z;
  Quat result = {0, 0, 0, 1};
  const float trace = m00 + m11 + m22;
  if (trace > 0.0f) {
    const float s = sqrtf(trace + 1.0f) * 2.0f;
    result.w = 0.25f * s;
    result.x = (m21 - m12) / s;
    result.y = (m02 - m20) / s;
    result.z = (m10 - m01) / s;
  } else if (m00 > m11 && m00 > m22) {
    const float s = sqrtf(1.0f + m00 - m11 - m22) * 2.0f;
    result.w = (m21 - m12) / s;
    result.x = 0.25f * s;
    result.y = (m01 + m10) / s;
    result.z = (m02 + m20) / s;
  } else if (m11 > m22) {
    const float s = sqrtf(1.0f + m11 - m00 - m22) * 2.0f;
    result.w = (m02 - m20) / s;
    result.x = (m01 + m10) / s;
    result.y = 0.25f * s;
    result.z = (m12 + m21) / s;
  } else {
    const float s = sqrtf(1.0f + m22 - m00 - m11) * 2.0f;
    result.w = (m10 - m01) / s;
    result.x = (m02 + m20) / s;
    result.y = (m12 + m21) / s;
    result.z = 0.25f * s;
  }
  return QuatNormalize(result);
}

static float QuatDot(Quat a, Quat b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static Quat QuatSlerp(Quat a, Quat b, float t) {
  a = QuatNormalize(a);
  b = QuatNormalize(b);
  float dot = QuatDot(a, b);
  if (dot < 0.0f) {
    b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w;
    dot = -dot;
  }

  if (dot > 0.9995f) {
    Quat r = {
      a.x + (b.x - a.x) * t,
      a.y + (b.y - a.y) * t,
      a.z + (b.z - a.z) * t,
      a.w + (b.w - a.w) * t
    };
    float len = sqrtf(r.x*r.x + r.y*r.y + r.z*r.z + r.w*r.w);
    if (len > 0.0001f) { r.x /= len; r.y /= len; r.z /= len; r.w /= len; }
    return r;
  }

  float theta = acosf(dot);
  float sinTheta = sinf(theta);
  float wa = sinf((1.0f - t) * theta) / sinTheta;
  float wb = sinf(t * theta) / sinTheta;

  return {
    wa * a.x + wb * b.x,
    wa * a.y + wb * b.y,
    wa * a.z + wb * b.z,
    wa * a.w + wb * b.w
  };
}

// Smallest world-space rotation that points one direction at another. This
// remains stable at 180 degrees, which is important for straight IK limbs.
static Quat QuatFromTo(Vec3 from, Vec3 to) {
  from = Vec3Normalize(from);
  to = Vec3Normalize(to);
  const float dot = (std::max)(-1.0f, (std::min)(1.0f, Vec3Dot(from, to)));
  if (dot > 0.999999f)
    return {0, 0, 0, 1};
  if (dot < -0.999999f) {
    Vec3 axis = fabsf(from.x) < 0.8f ? Vec3Cross(from, {1, 0, 0})
                                    : Vec3Cross(from, {0, 1, 0});
    axis = Vec3Normalize(axis);
    return {axis.x, axis.y, axis.z, 0.0f};
  }
  const Vec3 axis = Vec3Cross(from, to);
  return QuatNormalize({axis.x, axis.y, axis.z, 1.0f + dot});
}

// Analytic two-bone triangle. poleHint is a direction from the hip toward the
// preferred side of the knee plane. The target may be unreachable; in that
// case the triangle is clamped while retaining the requested direction.
static bool SolveTwoBoneKneePosition(Vec3 hip, Vec3 target,
                                     float upperLength, float lowerLength,
                                     Vec3 poleHint, Vec3 &kneeOut) {
  if (upperLength < 1e-5f || lowerLength < 1e-5f)
    return false;
  Vec3 towardTarget = Vec3Sub(target, hip);
  const float rawDistance = sqrtf(Vec3Dot(towardTarget, towardTarget));
  if (rawDistance < 1e-5f)
    return false;
  const Vec3 direction = Vec3Scale(towardTarget, 1.0f / rawDistance);
  const float minDistance = fabsf(upperLength - lowerLength) + 1e-5f;
  const float maxDistance = upperLength + lowerLength - 1e-5f;
  const float distance = (std::max)(minDistance,
      (std::min)(maxDistance, rawDistance));

  Vec3 pole = Vec3Sub(poleHint,
      Vec3Scale(direction, Vec3Dot(poleHint, direction)));
  if (Vec3Dot(pole, pole) < 1e-8f) {
    const Vec3 axis = fabsf(direction.y) < 0.8f ? Vec3{0, 1, 0}
                                               : Vec3{0, 0, 1};
    pole = Vec3Cross(direction, axis);
  }
  pole = Vec3Normalize(pole);

  const float along = (upperLength * upperLength -
                       lowerLength * lowerLength + distance * distance) /
                      (2.0f * distance);
  const float heightSquared =
      (std::max)(0.0f, upperLength * upperLength - along * along);
  const float height = sqrtf(heightSquared);
  kneeOut = Vec3Add(hip, Vec3Add(Vec3Scale(direction, along),
                                 Vec3Scale(pole, height)));
  return true;
}

// Slerp without clamping t so the existing 0..2 motion-scale controls keep
// their extrapolation behavior in direct-VMD mode.
static Quat ScaleRotation(Quat delta, float scale) {
  return QuatNormalize(QuatSlerp({0, 0, 0, 1}, delta, scale));
}

// Compose collapsed MMD hierarchy layers from parent to child. Keeping this
// operation explicit prevents container iteration order from changing poses.
static Quat ComposeVmdRotation(Quat accumulated, Quat nextLayer) {
  return QuatNormalize(QuatMul(accumulated, nextLayer));
}

// VMD/MMD body coordinates face the opposite X/Z directions from Unity's
// canonical humanoid coordinates. This is a 180-degree basis rotation around
// Y: positions (-x,y,-z), rotations (-x,y,-z,w).
static Vec3 VmdToUnityPosition(Vec3 value) {
  return {-value.x, value.y, -value.z};
}

static Quat VmdToUnityRotation(Quat value) {
  return QuatNormalize({-value.x, value.y, -value.z, value.w});
}

static float VmdBezierAxis(float p1, float p2, float s) {
  const float u = 1.0f - s;
  return 3.0f * u * u * s * p1 + 3.0f * u * s * s * p2 + s * s * s;
}

static float VmdBezierEval(uint8_t x1Byte, uint8_t y1Byte,
                           uint8_t x2Byte, uint8_t y2Byte, float t) {
  if (t <= 0.0f)
    return 0.0f;
  if (t >= 1.0f)
    return 1.0f;
  const float x1 = (std::min)(x1Byte, (uint8_t)127) / 127.0f;
  const float y1 = (std::min)(y1Byte, (uint8_t)127) / 127.0f;
  const float x2 = (std::min)(x2Byte, (uint8_t)127) / 127.0f;
  const float y2 = (std::min)(y2Byte, (uint8_t)127) / 127.0f;
  if (fabsf(x1 - y1) < 1e-5f && fabsf(x2 - y2) < 1e-5f)
    return t;

  float lo = 0.0f, hi = 1.0f, s = t;
  for (int i = 0; i < 16; ++i) {
    const float x = VmdBezierAxis(x1, x2, s);
    if (fabsf(x - t) < 1e-5f)
      break;
    if (x < t)
      lo = s;
    else
      hi = s;
    s = (lo + hi) * 0.5f;
  }
  return VmdBezierAxis(y1, y2, s);
}


struct InterpResult {
  Vec3 position;
  Quat rotation;
  bool hasPosition;
};

static InterpResult InterpolateBone(
    const std::vector<VmdBoneKeyframe> &keys,
    float frame,
    bool isPositionBone)
{
  InterpResult result;
  result.hasPosition = isPositionBone;
  result.rotation = { 0, 0, 0, 1 }; 
  result.position = { 0, 0, 0 };

  if (keys.empty()) return result;

  if (frame <= keys.front().frame) {
    const auto &k = keys.front();
    result.rotation = QuatNormalize({ k.rot[0], k.rot[1], k.rot[2], k.rot[3] });
    if (isPositionBone) result.position = { k.pos[0], k.pos[1], k.pos[2] };
    return result;
  }

  if (frame >= keys.back().frame) {
    const auto &k = keys.back();
    result.rotation = QuatNormalize({ k.rot[0], k.rot[1], k.rot[2], k.rot[3] });
    if (isPositionBone) result.position = { k.pos[0], k.pos[1], k.pos[2] };
    return result;
  }

  int lo = 0, hi = (int)keys.size() - 1;
  while (lo < hi - 1) {
    int mid = (lo + hi) / 2;
    if (keys[mid].frame <= frame)
      lo = mid;
    else
      hi = mid;
  }

  const auto &prev = keys[lo];
  const auto &next = keys[hi];

  float range = (float)(next.frame - prev.frame);
  float t = (range > 0.0f) ? (frame - prev.frame) / range : 0.0f;
  t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;

  Quat qa = { prev.rot[0], prev.rot[1], prev.rot[2], prev.rot[3] };
  Quat qb = { next.rot[0], next.rot[1], next.rot[2], next.rot[3] };
  // Bone curves are stored in the destination key. The first 16 bytes are a
  // 4x4 matrix: X1 rows 0..3, Y1 rows 4..7, X2 rows 8..11, Y2 rows 12..15.
  const uint8_t *ip = next.interp;
  const float tx = VmdBezierEval(ip[0], ip[4], ip[8], ip[12], t);
  const float ty = VmdBezierEval(ip[1], ip[5], ip[9], ip[13], t);
  const float tz = VmdBezierEval(ip[2], ip[6], ip[10], ip[14], t);
  const float tr = VmdBezierEval(ip[3], ip[7], ip[11], ip[15], t);
  result.rotation = QuatNormalize(QuatSlerp(qa, qb, tr));

  if (isPositionBone) {
    Vec3 pa = { prev.pos[0], prev.pos[1], prev.pos[2] };
    Vec3 pb = { next.pos[0], next.pos[1], next.pos[2] };
    result.position = {pa.x + (pb.x - pa.x) * tx,
                       pa.y + (pb.y - pa.y) * ty,
                       pa.z + (pb.z - pa.z) * tz};
  }

  return result;
}

struct MmdPlayer {
  bool playing;
  bool loop;
  float currentTime;    
  float speed;          
  float totalDuration;  

  LARGE_INTEGER lastTick;
  LARGE_INTEGER freq;

  MmdPlayer() : playing(false), loop(false), currentTime(0),
                speed(1.0f), totalDuration(0) {
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTick);
  }

  void Start(float duration) {
    totalDuration = duration;
    currentTime = 0;
    playing = true;
    QueryPerformanceCounter(&lastTick);
  }

  void Stop() {
    playing = false;
    currentTime = 0;
  }

  void TogglePause() {
    if (playing) {
      playing = false;
    } else {
      playing = true;
      QueryPerformanceCounter(&lastTick); 
    }
  }

  float Tick() {
    if (!playing) return currentTime * 30.0f;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsed = (double)(now.QuadPart - lastTick.QuadPart) / (double)freq.QuadPart;
    lastTick = now;

    currentTime += (float)(elapsed * speed);

    if (currentTime >= totalDuration) {
      if (loop) {
        currentTime = fmodf(currentTime, totalDuration);
      } else {
        currentTime = totalDuration;
        playing = false;
      }
    }

    return currentTime * 30.0f; 
  }
};
