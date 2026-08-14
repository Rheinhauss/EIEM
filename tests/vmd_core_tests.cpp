#define _CRT_SECURE_NO_WARNINGS

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/vmd_parser.h"
#include "../src/mmd_player.h"

static bool Near(float a, float b, float epsilon = 1e-4f) {
  return fabsf(a - b) <= epsilon;
}

static void WriteFixed(FILE *file, const char *text, size_t width) {
  char buffer[64] = {};
  assert(width <= sizeof(buffer));
  memcpy(buffer, text, (std::min)(strlen(text), width));
  fwrite(buffer, 1, width, file);
}

static void FillLinearBoneCurve(uint8_t curve[64]) {
  memset(curve, 0, 64);
  for (int channel = 0; channel < 4; ++channel) {
    curve[channel] = 20;
    curve[4 + channel] = 20;
    curve[8 + channel] = 107;
    curve[12 + channel] = 107;
  }
}

static void WriteBone(FILE *file, const char *sjisName, uint32_t frame,
                      float px, float py, float pz, Quat rotation,
                      const uint8_t curve[64]) {
  WriteFixed(file, sjisName, 15);
  fwrite(&frame, 4, 1, file);
  const float position[3] = {px, py, pz};
  fwrite(position, 4, 3, file);
  const float quaternion[4] = {rotation.x, rotation.y, rotation.z, rotation.w};
  fwrite(quaternion, 4, 4, file);
  fwrite(curve, 1, 64, file);
}

static void WriteMorph(FILE *file, const char *sjisName, uint32_t frame,
                       float weight) {
  WriteFixed(file, sjisName, 15);
  fwrite(&frame, 4, 1, file);
  fwrite(&weight, 4, 1, file);
}

static void WriteCamera(FILE *file, uint32_t frame) {
  const float distance = -30.0f;
  const float position[3] = {1.0f, 2.0f, 3.0f};
  const float rotation[3] = {0.1f, 0.2f, 0.3f};
  uint8_t interpolation[24] = {};
  const uint32_t fov = 45;
  const uint8_t perspective = 0;
  fwrite(&frame, 4, 1, file);
  fwrite(&distance, 4, 1, file);
  fwrite(position, 4, 3, file);
  fwrite(rotation, 4, 3, file);
  fwrite(interpolation, 1, sizeof(interpolation), file);
  fwrite(&fov, 4, 1, file);
  fwrite(&perspective, 1, 1, file);
}

static std::string MakeFixture() {
  const char *path = "vmd_core_fixture.tmp";
  FILE *file = fopen(path, "wb");
  assert(file);
  WriteFixed(file, "Vocaloid Motion Data 0002", 30);
  WriteFixed(file, "test", 20);

  uint8_t linear[64];
  FillLinearBoneCurve(linear);
  const uint32_t boneCount = 4;
  fwrite(&boneCount, 4, 1, file);
  // Shift-JIS: center and upper body.
  WriteBone(file, "\x83\x5A\x83\x93\x83\x5E\x81\x5B", 0, 0, 0, 0,
            {0, 0, 0, 1}, linear);
  WriteBone(file, "\x83\x5A\x83\x93\x83\x5E\x81\x5B", 10, 10, 20, 30,
            {0, 0.70710678f, 0, 0.70710678f}, linear);
  // Duplicate frame: the parser must keep this last record.
  WriteBone(file, "\x83\x5A\x83\x93\x83\x5E\x81\x5B", 10, 12, 24, 36,
            {0, 0.70710678f, 0, 0.70710678f}, linear);
  WriteBone(file, "\x8F\xE3\x94\xBC\x90\x67", 20, 0, 0, 0,
            {0, 0, 0, 2}, linear); // Parser normalizes this quaternion.

  const uint32_t morphCount = 2;
  fwrite(&morphCount, 4, 1, file);
  WriteMorph(file, "\x82\xA0", 0, 0.0f);
  WriteMorph(file, "\x82\xA0", 20, 1.0f);
  const uint32_t cameraCount = 1;
  fwrite(&cameraCount, 4, 1, file);
  WriteCamera(file, 15);
  fclose(file);
  return path;
}

static std::string MakeZeroQuaternionFixture() {
  const char *path = "vmd_invalid_quaternion.tmp";
  FILE *file = fopen(path, "wb");
  assert(file);
  WriteFixed(file, "Vocaloid Motion Data 0002", 30);
  WriteFixed(file, "test", 20);
  uint8_t linear[64];
  FillLinearBoneCurve(linear);
  const uint32_t boneCount = 1;
  fwrite(&boneCount, 4, 1, file);
  WriteBone(file, "\x83\x5A\x83\x93\x83\x5E\x81\x5B", 0, 0, 0, 0,
            {0, 0, 0, 0}, linear);
  const uint32_t morphCount = 0;
  fwrite(&morphCount, 4, 1, file);
  fclose(file);
  return path;
}

static void TestParserAndSampling() {
  const std::string path = MakeFixture();
  VmdFile *vmd = LoadVmd(path.c_str());
  assert(vmd && vmd->loaded);
  assert(vmd->boneFrames == 20);
  assert(vmd->morphFrames == 20);
  assert(vmd->cameraFrames == 15);
  assert(vmd->cameraKeys.size() == 1);
  assert(vmd->cameraKeys[0].frame == 15);
  assert(Near(vmd->cameraKeys[0].position[2], 3.0f));
  assert(vmd->boneTimelines.size() == 2);

  auto center = vmd->boneTimelines.find(u8"センター");
  assert(center != vmd->boneTimelines.end());
  assert(center->second.keys.size() == 2);
  assert(Near(center->second.keys.back().pos[0], 12.0f));
  const InterpResult middle = InterpolateBone(center->second.keys, 5.0f, true);
  assert(Near(middle.position.x, 6.0f, 0.02f));
  assert(Near(QuatDot(middle.rotation, middle.rotation), 1.0f, 1e-3f));

  auto upper = vmd->boneTimelines.find(u8"上半身");
  assert(upper != vmd->boneTimelines.end());
  const Quat normalized = {upper->second.keys[0].rot[0],
                           upper->second.keys[0].rot[1],
                           upper->second.keys[0].rot[2],
                           upper->second.keys[0].rot[3]};
  assert(Near(QuatDot(normalized, normalized), 1.0f));

  auto mouth = vmd->morphTimelines.find(u8"あ");
  assert(mouth != vmd->morphTimelines.end());
  assert(Near(mouth->second.Sample(10.0f), 0.5f));
  FreeVmd(vmd);

  FILE *truncated = fopen(path.c_str(), "wb");
  assert(truncated);
  fwrite("Vocaloid Motion Data 0002", 1, 25, truncated);
  fclose(truncated);
  vmd = LoadVmd(path.c_str());
  assert(vmd && !vmd->loaded && !vmd->error.empty());
  FreeVmd(vmd);
  remove(path.c_str());

  const std::string invalidQuaternion = MakeZeroQuaternionFixture();
  vmd = LoadVmd(invalidQuaternion.c_str());
  assert(vmd && !vmd->loaded);
  assert(vmd->error.find("quaternion") != std::string::npos);
  FreeVmd(vmd);
  remove(invalidQuaternion.c_str());
}

static void TestBezierAndRetargetMath() {
  VmdBoneKeyframe first, second;
  first.frame = 0;
  second.frame = 10;
  first.rot[3] = second.rot[3] = 1.0f;
  second.pos[0] = second.pos[1] = second.pos[2] = 10.0f;
  FillLinearBoneCurve(second.interp);
  // Ease X heavily while Y/Z remain linear.
  second.interp[0] = 0;
  second.interp[4] = 0;
  second.interp[8] = 127;
  second.interp[12] = 0;
  std::vector<VmdBoneKeyframe> keys = {first, second};
  const InterpResult sample = InterpolateBone(keys, 5.0f, true);
  assert(sample.position.x < 3.0f);
  assert(Near(sample.position.y, 5.0f, 0.02f));

  const Quat bind = QuatNormalize({0.2f, -0.1f, 0.3f, 0.9f});
  const Quat parent = QuatNormalize({0.0f, 0.3826834f, 0.0f, 0.9238795f});
  const Quat identity = {0, 0, 0, 1};
  const Quat identityInParent = QuatNormalize(
      QuatMul(QuatMul(QuatInv(parent), identity), parent));
  const Quat neutralFinal =
      QuatNormalize(QuatMul(identityInParent, bind));
  assert(fabsf(QuatDot(neutralFinal, bind)) > 0.99999f);

  const Quat delta = QuatNormalize({0.258819f, 0, 0, 0.9659258f});
  const Quat converted = QuatNormalize(
      QuatMul(QuatMul(QuatInv(parent), delta), parent));
  const Quat finalRotation = QuatNormalize(QuatMul(converted, bind));
  assert(Near(QuatDot(finalRotation, finalRotation), 1.0f, 1e-4f));

  const Quat x90 = QuatNormalize({0.70710678f, 0, 0, 0.70710678f});
  const Quat y90 = QuatNormalize({0, 0.70710678f, 0, 0.70710678f});
  const Quat parentThenChild = ComposeVmdRotation(x90, y90);
  const Quat childThenParent = ComposeVmdRotation(y90, x90);
  assert(fabsf(QuatDot(parentThenChild, childThenParent)) < 0.999f);
  assert(Near(QuatDot(parentThenChild, parentThenChild), 1.0f));

  const Quat backwardBasis = QuatFromBasis(
      {-1, 0, 0}, {0, 1, 0}, {0, 0, -1});
  const Vec3 convertedRight = QuatRotate(backwardBasis, {1, 0, 0});
  const Vec3 convertedForward = QuatRotate(backwardBasis, {0, 0, 1});
  assert(Near(convertedRight.x, -1.0f));
  assert(Near(convertedForward.z, -1.0f));
  const Quat sourceX30 = QuatNormalize({0.258819f, 0, 0, 0.9659258f});
  const Quat targetX30 = QuatNormalize(QuatMul(
      QuatMul(backwardBasis, sourceX30), QuatInv(backwardBasis)));
  const Quat expectedNegativeX30 =
      QuatNormalize({-0.258819f, 0, 0, 0.9659258f});
  assert(fabsf(QuatDot(targetX30, expectedNegativeX30)) > 0.99999f);

  const Vec3 convertedVmdPosition = VmdToUnityPosition({1, 2, 3});
  assert(Near(convertedVmdPosition.x, -1.0f));
  assert(Near(convertedVmdPosition.y, 2.0f));
  assert(Near(convertedVmdPosition.z, -3.0f));
  const Quat convertedVmdX = VmdToUnityRotation(sourceX30);
  assert(fabsf(QuatDot(convertedVmdX, expectedNegativeX30)) > 0.99999f);
  const Quat sourceY30 = QuatNormalize({0, 0.258819f, 0, 0.9659258f});
  const Quat convertedVmdY = VmdToUnityRotation(sourceY30);
  assert(fabsf(QuatDot(convertedVmdY, sourceY30)) > 0.99999f);

  // Lower and upper body are siblings in MMD. If Humanoid Hips already
  // applied common*lower, this local compensation makes the upper-body world
  // delta common*upper instead of common*lower*upper.
  const Quat common = QuatNormalize({0, 0.130526f, 0, 0.991445f});
  const Quat lower = QuatNormalize({0.173648f, 0, 0, 0.984808f});
  const Quat upper = QuatNormalize({0, 0, 0.087156f, 0.996195f});
  const Quat rootAll = ComposeVmdRotation(common, lower);
  const Quat compensation = QuatNormalize(
      QuatMul(QuatMul(QuatInv(rootAll), common), upper));
  const Quat actualUpperWorld = QuatNormalize(QuatMul(rootAll, compensation));
  const Quat expectedUpperWorld = QuatNormalize(QuatMul(common, upper));
  assert(fabsf(QuatDot(actualUpperWorld, expectedUpperWorld)) > 0.99999f);
}

int main() {
  TestParserAndSampling();
  TestBezierAndRetargetMath();
  puts("vmd_core_tests: PASS");
  return 0;
}
