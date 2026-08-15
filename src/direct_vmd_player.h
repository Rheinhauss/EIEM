#pragma once

#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

enum DirectMotionPart {
  DIRECT_PART_ROOT,
  DIRECT_PART_SPINE,
  DIRECT_PART_HEAD,
  DIRECT_PART_LEFT_ARM,
  DIRECT_PART_RIGHT_ARM,
  DIRECT_PART_LEGS,
  DIRECT_PART_FINGERS,
};

struct DirectSourceTrack {
  std::string name;
  const VmdBoneTimeline *timeline = nullptr;
  bool hasPosition = false;
  int order = 0;
};

struct DirectBoneBinding {
  void *transform = nullptr;
  int humanBone = HB_None;
  DirectMotionPart part = DIRECT_PART_SPINE;
  Quat bindLocal = {0, 0, 0, 1};
  Quat parentBindFromRoot = {0, 0, 0, 1};
  Quat armTwistRetargetRoot = {0, 0, 0, 1};
  Quat elbowHingeRetargetRoot = {0, 0, 0, 1};
  Vec3 bindPosition = {0, 0, 0};
  bool controlsPosition = false;
  bool isRootChannel = false;
  bool cancelsLowerBody = false;
  bool armTwistRetargetReady = false;
  bool elbowHingeRetargetReady = false;
  std::vector<DirectSourceTrack> tracks;
};

struct DirectPoseBone {
  void *transform = nullptr;
  Quat localRotation = {0, 0, 0, 1};
  Vec3 localPosition = {0, 0, 0};
  bool hasPosition = false;
};

struct DirectIkBinding {
  void *upperLeg = nullptr;
  void *lowerLeg = nullptr;
  void *foot = nullptr;
  void *toes = nullptr;
  const VmdBoneTimeline *footTimeline = nullptr;
  const VmdBoneTimeline *toeTimeline = nullptr;
  std::string footTrackName;
  std::string toeTrackName;
  Vec3 bindFootFromRoot = {0, 0, 0};
  Vec3 bindToeFromFoot = {0, 0, 0};
  Vec3 bindPoleFromUpperParent = {0, 0, 1};
  mutable Vec3 targetFootFromRoot = {0, 0, 0};
  mutable Vec3 targetToeFromFoot = {0, 0, 0};
  mutable bool targetCalibrated = false;
  float upperLength = 0.0f;
  float lowerLength = 0.0f;
  bool legValid = false;
  bool toeValid = false;
};

struct DirectPoseIk {
  bool legEnabled = false;
  bool toeEnabled = false;
  float weight = 1.0f;
  Vec3 allParentOffsetRoot = {0, 0, 0};
  Vec3 footOffsetRoot = {0, 0, 0};
  Vec3 toeOffsetRoot = {0, 0, 0};
  Quat allParentRotationRoot = {0, 0, 0, 1};
  Quat footRotationRoot = {0, 0, 0, 1};
};

struct DirectPoseFrame {
  uint32_t generation = 0;
  float timeSeconds = 0.0f;
  float frame = 0.0f;
  std::vector<DirectPoseBone> bones;
  DirectPoseIk ik[2] = {};
};

struct DirectMapEntry {
  const char *vmdName;
  int humanBone;
  const char *fallbackTransform;
  DirectMotionPart part;
  bool position;
  bool rootChannel;
  int order;
};

static void DirectRebindAnimatorToRest() {
  SafeSetAnimatorEnabled(true);
  if (g_animator_Rebind) {
    __try { Invoke(g_animator_Rebind, g_cachedAnimator); } __except (1) {}
  }
  if (g_animator_Update) {
    __try {
      float zero = 0.0f;
      void *args[] = {&zero};
      Invoke(g_animator_Update, g_cachedAnimator, args);
    } __except (1) {}
  }
}

static void *DirectGetParentTransform(void *transform) {
  if (!transform || !g_transform_get_parent)
    return nullptr;
  __try {
    return Invoke(g_transform_get_parent, transform);
  } __except (1) {
    return nullptr;
  }
}

static int DirectGetChildCount(void *transform) {
  if (!transform || !g_transform_get_childCount)
    return 0;
  __try {
    void *boxed = Invoke(g_transform_get_childCount, transform);
    const int count = boxed ? *(int *)((char *)boxed + 16) : 0;
    return count > 0 && count < 4096 ? count : 0;
  } __except (1) {
    return 0;
  }
}

static void *DirectGetChild(void *transform, int index) {
  if (!transform || index < 0 || !g_transform_GetChild)
    return nullptr;
  __try {
    void *args[] = {&index};
    return Invoke(g_transform_GetChild, transform, args);
  } __except (1) {
    return nullptr;
  }
}

// Multiple source tracks intentionally target the same game bone. BuildBindings
// groups them by Transform and preserves this MMD hierarchy order.
static const DirectMapEntry g_directMap[] = {
    {u8"全ての親", HB_Hips, nullptr, DIRECT_PART_ROOT, true, true, 0},
    {u8"センター", HB_Hips, nullptr, DIRECT_PART_ROOT, true, true, 10},
    {u8"グルーブ", HB_Hips, nullptr, DIRECT_PART_ROOT, true, true, 20},
    {u8"下半身", HB_Hips, nullptr, DIRECT_PART_ROOT, false, true, 30},
    {u8"上半身", HB_Spine, nullptr, DIRECT_PART_SPINE, false, false, 0},
    {u8"上半身2", HB_Chest, nullptr, DIRECT_PART_SPINE, false, false, 10},
    {u8"上半身3", HB_UpperChest, nullptr, DIRECT_PART_SPINE, false, false, 20},
    {u8"首", HB_Neck, nullptr, DIRECT_PART_HEAD, false, false, 0},
    {u8"頭", HB_Head, nullptr, DIRECT_PART_HEAD, false, false, 0},
    {u8"左肩", HB_LeftShoulder, nullptr, DIRECT_PART_LEFT_ARM, false, false, 0},
    {u8"左腕", HB_LeftUpperArm, nullptr, DIRECT_PART_LEFT_ARM, false, false, 0},
    {u8"左腕捩", HB_LeftUpperArm, nullptr, DIRECT_PART_LEFT_ARM, false, false, 10},
    {u8"左ひじ", HB_LeftLowerArm, nullptr, DIRECT_PART_LEFT_ARM, false, false, 0},
    {u8"左手捩", HB_LeftLowerArm, nullptr, DIRECT_PART_LEFT_ARM, false, false, 10},
    {u8"左手首", HB_LeftHand, nullptr, DIRECT_PART_LEFT_ARM, false, false, 0},
    {u8"右肩", HB_RightShoulder, nullptr, DIRECT_PART_RIGHT_ARM, false, false, 0},
    {u8"右腕", HB_RightUpperArm, nullptr, DIRECT_PART_RIGHT_ARM, false, false, 0},
    {u8"右腕捩", HB_RightUpperArm, nullptr, DIRECT_PART_RIGHT_ARM, false, false, 10},
    {u8"右ひじ", HB_RightLowerArm, nullptr, DIRECT_PART_RIGHT_ARM, false, false, 0},
    {u8"右手捩", HB_RightLowerArm, nullptr, DIRECT_PART_RIGHT_ARM, false, false, 10},
    {u8"右手首", HB_RightHand, nullptr, DIRECT_PART_RIGHT_ARM, false, false, 0},
    {u8"左足", HB_LeftUpperLeg, nullptr, DIRECT_PART_LEGS, false, false, 0},
    {u8"左ひざ", HB_LeftLowerLeg, nullptr, DIRECT_PART_LEGS, false, false, 0},
    {u8"左足首", HB_LeftFoot, nullptr, DIRECT_PART_LEGS, false, false, 0},
    {u8"左つま先", HB_LeftToes, nullptr, DIRECT_PART_LEGS, false, false, 0},
    {u8"右足", HB_RightUpperLeg, nullptr, DIRECT_PART_LEGS, false, false, 0},
    {u8"右ひざ", HB_RightLowerLeg, nullptr, DIRECT_PART_LEGS, false, false, 0},
    {u8"右足首", HB_RightFoot, nullptr, DIRECT_PART_LEGS, false, false, 0},
    {u8"右つま先", HB_RightToes, nullptr, DIRECT_PART_LEGS, false, false, 0},
    {u8"両目", 21, "eyeLfJoint", DIRECT_PART_HEAD, false, false, 0},
    {u8"両目", 22, "eyeRtJoint", DIRECT_PART_HEAD, false, false, 0},
    {u8"左目", 21, "eyeLfJoint", DIRECT_PART_HEAD, false, false, 0},
    {u8"右目", 22, "eyeRtJoint", DIRECT_PART_HEAD, false, false, 0},
    {u8"あご", 23, "jawJoint", DIRECT_PART_HEAD, false, false, 0},
};

struct DirectVmdMotion {
  VmdFile *vmd = nullptr;
  std::vector<DirectBoneBinding> bindings;
  void *rootTransform = nullptr;
  DirectIkBinding ikBindings[2] = {};
  uint32_t durationFrames = 0;
  int mappedTracks = 0;
  int unmappedTracks = 0;
  int unsupportedTracks = 0;
  int ikTracks = 0;
  float positionScale = 0.08f;
  Quat rootBasis = {0, 0, 0, 1};
  bool analyzed = false;
  bool prepared = false;
  std::string error;

  ~DirectVmdMotion() { FreeVmd(vmd); }

  float Duration() const {
    return durationFrames > 0 ? durationFrames / 30.0f : 1.0f / 30.0f;
  }

  static bool IsUnsupportedName(const std::string &name) {
    return name.find(u8"ＩＫ") != std::string::npos ||
           name.find("IK") != std::string::npos ||
           name.find("ik") != std::string::npos;
  }

  static float PartScale(DirectMotionPart part) {
    float scale = g_motionScale;
    switch (part) {
    case DIRECT_PART_SPINE: scale *= g_scaleSpine; break;
    case DIRECT_PART_HEAD: scale *= g_scaleHead; break;
    case DIRECT_PART_LEFT_ARM: scale *= g_scaleLArm; break;
    case DIRECT_PART_RIGHT_ARM: scale *= g_scaleRArm; break;
    case DIRECT_PART_LEGS: scale *= g_scaleLegs; break;
    case DIRECT_PART_FINGERS: scale *= g_scaleFingers; break;
    default: break;
    }
    return scale;
  }

  static bool ReadWorldRotation(void *transform, Quat &out) {
    if (!transform || !g_camGetRot)
      return false;
    __try {
      float value[4] = {};
      g_camGetRot(transform, value);
      out = QuatNormalize({value[0], value[1], value[2], value[3]});
      return true;
    } __except (1) {
      return false;
    }
  }

  bool CalculateRootBasis(Quat rootWorld) {
    void *left = SafeGetBoneTransform(HB_LeftShoulder);
    void *right = SafeGetBoneTransform(HB_RightShoulder);
    if (!left || !right) {
      left = SafeGetBoneTransform(HB_LeftUpperArm);
      right = SafeGetBoneTransform(HB_RightUpperArm);
    }
    if (!left || !right) {
      left = SafeGetBoneTransform(HB_LeftUpperLeg);
      right = SafeGetBoneTransform(HB_RightUpperLeg);
    }

    Vec3 leftPosition, rightPosition;
    if (!left || !right || !ReadWorldPosition(left, leftPosition) ||
        !ReadWorldPosition(right, rightPosition)) {
      rootBasis = {0, 0, 0, 1};
      Log("[VMD-DIRECT] Root basis fallback: no bilateral bind landmarks");
      return false;
    }

    Vec3 up = Vec3Normalize(QuatRotate(rootWorld, {0, 1, 0}));
    Vec3 rightAxis = Vec3Sub(rightPosition, leftPosition);
    // Remove shoulder slope so an idle-pose lean does not become a permanent
    // pitch/roll correction for every animation joint.
    const float vertical = Vec3Dot(rightAxis, up);
    rightAxis = Vec3Normalize({rightAxis.x - up.x * vertical,
                               rightAxis.y - up.y * vertical,
                               rightAxis.z - up.z * vertical});
    Vec3 forward = Vec3Normalize(Vec3Cross(rightAxis, up));
    if (Vec3Dot(rightAxis, rightAxis) < 0.5f ||
        Vec3Dot(forward, forward) < 0.5f) {
      rootBasis = {0, 0, 0, 1};
      Log("[VMD-DIRECT] Root basis fallback: degenerate bind landmarks");
      return false;
    }
    up = Vec3Normalize(Vec3Cross(forward, rightAxis));
    const Quat anatomicalWorld = QuatFromBasis(rightAxis, up, forward);
    rootBasis = QuatNormalize(QuatMul(QuatInv(rootWorld), anatomicalWorld));
    Log("[VMD-DIRECT] Root basis q=(%.4f,%.4f,%.4f,%.4f) forward=(%.3f,%.3f,%.3f)",
        rootBasis.x, rootBasis.y, rootBasis.z, rootBasis.w,
        forward.x, forward.y, forward.z);
    return true;
  }

  Quat SourceToRootRotation(Quat source) const {
    const Quat unityCanonical = VmdToUnityRotation(source);
    return QuatNormalize(
        QuatMul(QuatMul(rootBasis, unityCanonical), QuatInv(rootBasis)));
  }

  void *ResolveTransform(const DirectMapEntry &entry, void *root) {
    int bone = entry.humanBone;
    void *transform = bone >= 0 ? SafeGetBoneTransform(bone) : nullptr;
    if (!transform && entry.fallbackTransform && root)
      transform = SafeFindChildRecursive(root, entry.fallbackTransform, 15);
    if (transform)
      return transform;

    // Humanoid avatars may omit optional bones. Collapse that VMD layer into
    // the nearest usable parent instead of silently dropping it.
    switch (bone) {
    case HB_UpperChest:
      transform = SafeGetBoneTransform(HB_Chest);
      if (!transform)
        transform = SafeGetBoneTransform(HB_Spine);
      break;
    case HB_Chest:
      transform = SafeGetBoneTransform(HB_Spine);
      break;
    case HB_Neck:
    case HB_LeftShoulder:
    case HB_RightShoulder:
      transform = SafeGetBoneTransform(HB_UpperChest);
      if (!transform)
        transform = SafeGetBoneTransform(HB_Chest);
      if (!transform)
        transform = SafeGetBoneTransform(HB_Spine);
      break;
    case HB_LeftHand:
      transform = SafeGetBoneTransform(HB_LeftLowerArm);
      break;
    case HB_RightHand:
      transform = SafeGetBoneTransform(HB_RightLowerArm);
      break;
    case HB_LeftFoot:
      transform = SafeGetBoneTransform(HB_LeftLowerLeg);
      break;
    case HB_RightFoot:
      transform = SafeGetBoneTransform(HB_RightLowerLeg);
      break;
    case HB_LeftToes:
      transform = SafeGetBoneTransform(HB_LeftFoot);
      break;
    case HB_RightToes:
      transform = SafeGetBoneTransform(HB_RightFoot);
      break;
    case 21: // LeftEye
    case 22: // RightEye
    case 23: // Jaw
      transform = SafeGetBoneTransform(HB_Head);
      break;
    default:
      break;
    }
    return transform;
  }

  DirectBoneBinding *FindOrAddBinding(void *transform,
                                      const DirectMapEntry &entry) {
    for (auto &binding : bindings) {
      if (binding.transform == transform)
        return &binding;
    }
    DirectBoneBinding binding;
    binding.transform = transform;
    binding.humanBone = entry.humanBone;
    binding.part = entry.part;
    binding.controlsPosition = entry.position;
    binding.isRootChannel = entry.rootChannel;
    bindings.push_back(binding);
    return &bindings.back();
  }

  void AddTrack(const DirectMapEntry &entry, void *root,
                std::vector<std::string> &seen) {
    auto timelineIt = vmd->boneTimelines.find(entry.vmdName);
    if (timelineIt == vmd->boneTimelines.end())
      return;
    seen.push_back(entry.vmdName);
    void *transform = ResolveTransform(entry, root);
    if (!transform) {
      ++unmappedTracks;
      return;
    }
    DirectBoneBinding *binding = FindOrAddBinding(transform, entry);
    binding->controlsPosition = binding->controlsPosition || entry.position;
    binding->isRootChannel = binding->isRootChannel || entry.rootChannel;
    DirectSourceTrack track;
    track.name = entry.vmdName;
    track.timeline = &timelineIt->second;
    track.hasPosition = entry.position;
    track.order = entry.order;
    binding->tracks.push_back(track);
    ++mappedTracks;
  }

  void AddFingerTracks(void *root, std::vector<std::string> &seen) {
    std::vector<const FingerMapEntry *> requested;
    for (int i = 0; i < g_fingerMapCount; ++i) {
      const FingerMapEntry &entry = g_fingerMap[i];
      if (!strstr(entry.transformName, "Finger"))
        continue;
      auto timelineIt = vmd->boneTimelines.find(entry.mmdName);
      if (timelineIt == vmd->boneTimelines.end())
        continue;
      requested.push_back(&entry);
    }

    std::vector<void *> resolved(requested.size(), nullptr);
    if (root && !requested.empty()) {
      struct WalkItem {
        void *transform;
        int depth;
      };
      WalkItem stack[4096];
      size_t top = 0;
      size_t visited = 0;
      size_t resolvedCount = 0;
      stack[top++] = {root, 0};
      while (top > 0 && visited < 4096 && resolvedCount < requested.size()) {
        const WalkItem item = stack[--top];
        ++visited;
        char name[256] = {};
        SafeGetBoneName(item.transform, name, (int)sizeof(name));
        if (name[0]) {
          for (size_t i = 0; i < requested.size(); ++i) {
            if (!resolved[i] && strcmp(name, requested[i]->transformName) == 0) {
              resolved[i] = item.transform;
              ++resolvedCount;
            }
          }
        }
        if (item.depth >= 15)
          continue;
        const int childCount = DirectGetChildCount(item.transform);
        for (int i = childCount - 1; i >= 0 && top < 4096; --i) {
          void *child = DirectGetChild(item.transform, i);
          if (child)
            stack[top++] = {child, item.depth + 1};
        }
      }
      Log("[VMD-DIRECT] Hierarchy scan: visited=%zu resolved=%zu/%zu",
          visited, resolvedCount, requested.size());
    }

    for (size_t i = 0; i < requested.size(); ++i) {
      const FingerMapEntry &entry = *requested[i];
      auto timelineIt = vmd->boneTimelines.find(entry.mmdName);
      seen.push_back(entry.mmdName);
      void *transform = resolved[i];
      if (!transform) {
        ++unmappedTracks;
        continue;
      }
      DirectMapEntry spec{entry.mmdName, HB_None, entry.transformName,
                          DIRECT_PART_FINGERS, false, false, 0};
      DirectBoneBinding *binding = FindOrAddBinding(transform, spec);
      DirectSourceTrack track;
      track.name = entry.mmdName;
      track.timeline = &timelineIt->second;
      binding->tracks.push_back(track);
      ++mappedTracks;
    }
  }

  static bool IsArmTwistTrack(const std::string &name) {
    return name == u8"左腕捩" || name == u8"右腕捩" ||
           name == u8"左手捩" || name == u8"右手捩";
  }

  static bool IsElbowTrack(const std::string &name) {
    return name == u8"左ひじ" || name == u8"右ひじ";
  }

  static bool IsLeftArmTrack(const std::string &name) {
    return name == u8"左腕" || name == u8"左腕捩" ||
           name == u8"左ひじ" || name == u8"左手捩" ||
           name == u8"左手首";
  }

  void CaptureArmAxes(DirectBoneBinding &binding, Quat rootWorld) {
    const bool upper = binding.humanBone == HB_LeftUpperArm ||
                       binding.humanBone == HB_RightUpperArm;
    const bool lower = binding.humanBone == HB_LeftLowerArm ||
                       binding.humanBone == HB_RightLowerArm;
    if (!upper && !lower)
      return;
    // Do not calibrate an optional-bone fallback Transform as an arm segment.
    if (SafeGetBoneTransform(binding.humanBone) != binding.transform)
      return;

    const bool left = binding.humanBone == HB_LeftUpperArm ||
                      binding.humanBone == HB_LeftLowerArm;
    const int childBone = upper
        ? (left ? HB_LeftLowerArm : HB_RightLowerArm)
        : (left ? HB_LeftHand : HB_RightHand);
    void *child = SafeGetBoneTransform(childBone);
    Vec3 startPosition, endPosition;
    if (!child || !ReadWorldPosition(binding.transform, startPosition) ||
        !ReadWorldPosition(child, endPosition))
      return;

    const Quat rootInverse = QuatInv(rootWorld);
    const Vec3 targetAxisRoot = Vec3Normalize(QuatRotate(
        rootInverse, Vec3Sub(endPosition, startPosition)));
    if (Vec3Dot(targetAxisRoot, targetAxisRoot) < 0.5f)
      return;
    const Vec3 sourceAxisRoot = QuatRotate(
        rootBasis, left ? Vec3{-1, 0, 0} : Vec3{1, 0, 0});
    binding.armTwistRetargetRoot =
        QuatFromTo(sourceAxisRoot, targetAxisRoot);
    binding.armTwistRetargetReady = true;

    if (lower) {
      const Vec3 anatomicalForwardRoot =
          QuatRotate(rootBasis, {0, 0, 1});
      Vec3 bendDirection = Vec3Sub(
          anatomicalForwardRoot,
          Vec3Scale(targetAxisRoot,
                    Vec3Dot(anatomicalForwardRoot, targetAxisRoot)));
      bendDirection = Vec3Normalize(bendDirection);
      const Vec3 targetHingeRoot =
          Vec3Normalize(Vec3Cross(targetAxisRoot, bendDirection));
      const Vec3 sourceHingeRoot = QuatRotate(
          rootBasis, left ? Vec3{0, 1, 0} : Vec3{0, -1, 0});
      if (Vec3Dot(targetHingeRoot, targetHingeRoot) > 0.5f) {
        binding.elbowHingeRetargetRoot =
            QuatFromTo(sourceHingeRoot, targetHingeRoot);
        binding.elbowHingeRetargetReady = true;
      }
    }

    Log("[VMD-DIRECT] Arm axes bone=%d twist=%d hinge=%d",
        binding.humanBone, binding.armTwistRetargetReady ? 1 : 0,
        binding.elbowHingeRetargetReady ? 1 : 0);
  }

  static bool HasSeenTrack(const std::vector<std::string> &seen,
                           const std::string &name) {
    return std::find(seen.begin(), seen.end(), name) != seen.end();
  }

  static void MarkSupportedTrack(std::vector<std::string> &seen,
                                 const std::string &name,
                                 int &supportedCount) {
    if (!HasSeenTrack(seen, name)) {
      seen.push_back(name);
      ++supportedCount;
    }
  }

  const VmdBoneTimeline *FindIkTrack(bool right, bool toe,
                                     std::string &matchedName) const {
    static const char *const names[2][2][2] = {
        {{u8"\u5de6\u8db3\uff29\uff2b", u8"\u5de6\u8db3IK"},
         {u8"\u5de6\u3064\u307e\u5148\uff29\uff2b",
          u8"\u5de6\u3064\u307e\u5148IK"}},
        {{u8"\u53f3\u8db3\uff29\uff2b", u8"\u53f3\u8db3IK"},
         {u8"\u53f3\u3064\u307e\u5148\uff29\uff2b",
          u8"\u53f3\u3064\u307e\u5148IK"}},
    };
    for (const char *candidate : names[right ? 1 : 0][toe ? 1 : 0]) {
      const auto it = vmd->boneTimelines.find(candidate);
      if (it != vmd->boneTimelines.end()) {
        matchedName = it->first;
        return &it->second;
      }
    }
    return nullptr;
  }

  void BuildIkBindings(std::vector<std::string> &seen) {
    ikTracks = 0;
    for (int side = 0; side < 2; ++side) {
      const bool right = side != 0;
      DirectIkBinding &chain = ikBindings[side];
      chain = {};
      chain.upperLeg = SafeGetBoneTransform(
          right ? HB_RightUpperLeg : HB_LeftUpperLeg);
      chain.lowerLeg = SafeGetBoneTransform(
          right ? HB_RightLowerLeg : HB_LeftLowerLeg);
      chain.foot = SafeGetBoneTransform(right ? HB_RightFoot : HB_LeftFoot);
      chain.toes = SafeGetBoneTransform(right ? HB_RightToes : HB_LeftToes);
      chain.legValid = chain.upperLeg && chain.lowerLeg && chain.foot;
      chain.toeValid = chain.legValid && chain.toes;

      chain.footTimeline =
          FindIkTrack(right, false, chain.footTrackName);
      if (chain.footTimeline) {
        seen.push_back(chain.footTrackName);
        if (chain.legValid) {
          ++mappedTracks;
          ++ikTracks;
        } else {
          ++unmappedTracks;
        }
      }
      chain.toeTimeline = FindIkTrack(right, true, chain.toeTrackName);
      if (chain.toeTimeline) {
        seen.push_back(chain.toeTrackName);
        if (chain.toeValid) {
          ++mappedTracks;
          ++ikTracks;
        } else {
          ++unmappedTracks;
        }
      }
    }
  }

  bool CaptureIkBinding(DirectIkBinding &chain, Vec3 rootPosition,
                        Quat rootWorld) {
    if (!chain.legValid)
      return false;
    Vec3 upperPosition, kneePosition, footPosition;
    if (!ReadWorldPosition(chain.upperLeg, upperPosition) ||
        !ReadWorldPosition(chain.lowerLeg, kneePosition) ||
        !ReadWorldPosition(chain.foot, footPosition)) {
      chain.legValid = chain.toeValid = false;
      return false;
    }
    const Vec3 upperVector = Vec3Sub(kneePosition, upperPosition);
    const Vec3 lowerVector = Vec3Sub(footPosition, kneePosition);
    chain.upperLength = sqrtf(Vec3Dot(upperVector, upperVector));
    chain.lowerLength = sqrtf(Vec3Dot(lowerVector, lowerVector));
    if (chain.upperLength < 1e-4f || chain.lowerLength < 1e-4f) {
      chain.legValid = chain.toeValid = false;
      return false;
    }

    const Quat rootInverse = QuatInv(rootWorld);
    chain.bindFootFromRoot = QuatRotate(
        rootInverse, Vec3Sub(footPosition, rootPosition));
    chain.targetFootFromRoot = chain.bindFootFromRoot;
    chain.targetCalibrated = false;
    const Vec3 hipToFoot = Vec3Normalize(Vec3Sub(footPosition, upperPosition));
    Vec3 poleWorld = Vec3Sub(
        upperVector, Vec3Scale(hipToFoot, Vec3Dot(upperVector, hipToFoot)));
    if (Vec3Dot(poleWorld, poleWorld) < 1e-8f)
      poleWorld = QuatRotate(rootWorld, {0, 0, 1});
    Quat upperParentWorld = rootWorld;
    void *upperParent = DirectGetParentTransform(chain.upperLeg);
    if (upperParent)
      ReadWorldRotation(upperParent, upperParentWorld);
    chain.bindPoleFromUpperParent = Vec3Normalize(
        QuatRotate(QuatInv(upperParentWorld), poleWorld));

    if (chain.toeValid) {
      Vec3 toePosition;
      if (ReadWorldPosition(chain.toes, toePosition)) {
        chain.bindToeFromFoot = QuatRotate(
            rootInverse, Vec3Sub(toePosition, footPosition));
        chain.targetToeFromFoot = chain.bindToeFromFoot;
      } else {
        chain.toeValid = false;
      }
    }
    return true;
  }

  bool SampleIkEnabled(const std::string &name, float frame) const {
    const auto it = vmd->ikTimelines.find(name);
    return it == vmd->ikTimelines.end() ? true
                                        : it->second.Sample(frame, true);
  }

  void ResetRuntimeCalibration() {
    for (DirectIkBinding &chain : ikBindings)
      chain.targetCalibrated = false;
  }

  bool AnalyzeSourceTracks() {
    analyzed = false;
    prepared = false;
    bindings.clear();
    rootTransform = nullptr;
    ikBindings[0] = {};
    ikBindings[1] = {};
    mappedTracks = unmappedTracks = unsupportedTracks = ikTracks = 0;
    error.clear();
    if (!vmd || !vmd->loaded) {
      error = vmd ? vmd->error : "No VMD data";
      return false;
    }

    std::vector<std::string> seen;
    for (const DirectMapEntry &entry : g_directMap) {
      if (vmd->boneTimelines.find(entry.vmdName) != vmd->boneTimelines.end())
        MarkSupportedTrack(seen, entry.vmdName, mappedTracks);
    }
    for (int i = 0; i < g_fingerMapCount; ++i) {
      const FingerMapEntry &entry = g_fingerMap[i];
      if (strstr(entry.transformName, "Finger") &&
          vmd->boneTimelines.find(entry.mmdName) != vmd->boneTimelines.end())
        MarkSupportedTrack(seen, entry.mmdName, mappedTracks);
    }
    for (int side = 0; side < 2; ++side) {
      for (int toe = 0; toe < 2; ++toe) {
        std::string matchedName;
        if (FindIkTrack(side != 0, toe != 0, matchedName)) {
          MarkSupportedTrack(seen, matchedName, mappedTracks);
          ++ikTracks;
        }
      }
    }
    for (const auto &timeline : vmd->boneTimelines) {
      if (HasSeenTrack(seen, timeline.first))
        continue;
      if (IsUnsupportedName(timeline.first))
        ++unsupportedTracks;
      else
        ++unmappedTracks;
    }
    if (mappedTracks == 0) {
      error = "VMD has no supported standard bone tracks";
      return false;
    }
    analyzed = true;
    return true;
  }

  void ClearTargetBindings() {
    prepared = false;
    bindings.clear();
    rootTransform = nullptr;
    ikBindings[0] = {};
    ikBindings[1] = {};
  }

  bool BuildBindings() {
    ClearTargetBindings();
    error.clear();
    if (!vmd || !vmd->loaded || !g_cachedAnimator) {
      error = "No loaded VMD or active character Animator";
      return false;
    }
    void *root = SafeGetComponentTransform(g_cachedAnimator);
    if (!root) {
      error = "Cannot resolve character root Transform";
      return false;
    }

    rootTransform = root;
    mappedTracks = unmappedTracks = unsupportedTracks = ikTracks = 0;
    std::vector<std::string> seen;
    Log("[VMD-DIRECT] Prepare phase 1/4: map humanoid tracks");
    for (const auto &entry : g_directMap)
      AddTrack(entry, root, seen);
    AddFingerTracks(root, seen);
    BuildIkBindings(seen);

    for (const auto &timeline : vmd->boneTimelines) {
      if (std::find(seen.begin(), seen.end(), timeline.first) != seen.end())
        continue;
      if (IsUnsupportedName(timeline.first))
        ++unsupportedTracks;
      else
        ++unmappedTracks;
    }
    if (mappedTracks == 0 || bindings.empty()) {
      error = "VMD has no bone tracks that map to this character";
      return false;
    }

    Log("[VMD-DIRECT] Prepare phase 2/4: Animator.Rebind/Update(0)");
    DirectRebindAnimatorToRest();
    Log("[VMD-DIRECT] Prepare phase 2/4 complete");

    Quat rootWorld = {0, 0, 0, 1};
    Log("[VMD-DIRECT] Prepare phase 3/4: capture bind rotations");
    const bool hasRootWorld = ReadWorldRotation(root, rootWorld);
    if (hasRootWorld)
      CalculateRootBasis(rootWorld);
    else
      rootBasis = {0, 0, 0, 1};
    Log("[VMD-DIRECT] VMD basis conversion: pos(-X,+Y,-Z) rot(-X,+Y,-Z,+W)");
    for (auto &binding : bindings) {
      binding.bindLocal = SafeGetLocalRotation(binding.transform);
      binding.bindPosition = SafeGetLocalPosition(binding.transform);
      void *parent = DirectGetParentTransform(binding.transform);
      Quat parentWorld = rootWorld;
      if (parent)
        ReadWorldRotation(parent, parentWorld);
      binding.parentBindFromRoot =
          QuatNormalize(QuatMul(QuatInv(rootWorld), parentWorld));
      if (hasRootWorld)
        CaptureArmAxes(binding, rootWorld);
      std::sort(binding.tracks.begin(), binding.tracks.end(),
                [](const DirectSourceTrack &a, const DirectSourceTrack &b) {
                  return a.order < b.order;
                });
    }

    Vec3 rootPosition = {0, 0, 0};
    if (hasRootWorld && ReadWorldPosition(root, rootPosition)) {
      for (DirectIkBinding &chain : ikBindings)
        CaptureIkBinding(chain, rootPosition, rootWorld);
    } else {
      for (DirectIkBinding &chain : ikBindings)
        chain.legValid = chain.toeValid = false;
    }
    Log("[VMD-DIRECT] Standard leg IK tracks: %d mapped, display timelines=%zu",
        ikTracks, vmd->ikTimelines.size());
    int axialTwistTracks = 0;
    for (const DirectBoneBinding &binding : bindings) {
      for (const DirectSourceTrack &track : binding.tracks) {
        if (IsArmTwistTrack(track.name))
          ++axialTwistTracks;
      }
    }
    if (axialTwistTracks > 0) {
      Log("[VMD-DIRECT] Axial-only arm/hand twist fallback: %d tracks",
          axialTwistTracks);
    }

    // MMD lower/upper body are siblings below center/groove, while Humanoid
    // Spine is a child of Hips. Mark the earliest available upper-body layer
    // so Sample() can remove the lower-body rotation inherited through Hips.
    static const char *upperPriority[] = {u8"上半身", u8"上半身2", u8"上半身3"};
    bool markedUpperRoot = false;
    for (const char *name : upperPriority) {
      for (auto &binding : bindings) {
        for (const auto &track : binding.tracks) {
          if (!markedUpperRoot && track.name == name) {
            binding.cancelsLowerBody = true;
            markedUpperRoot = true;
          }
        }
      }
      if (markedUpperRoot)
        break;
    }

    Vec3 rootPos, headPos;
    void *head = SafeGetBoneTransform(HB_Head);
    if (head && ReadWorldPosition(root, rootPos) && ReadWorldPosition(head, headPos)) {
      const float height = headPos.y - rootPos.y;
      if (height > 0.1f && height < 5.0f)
        positionScale = height / 20.0f;
    }
    prepared = true;
    Log("[VMD-DIRECT] Prepare phase 4/4 complete");
    return true;
  }

  void SampleIkPose(float frame, DirectPoseIk outIk[2]) const {
    const auto allIt = vmd->boneTimelines.find(
        u8"\u5168\u3066\u306e\u89aa");
    InterpResult allCurrent = {};
    InterpResult allFirst = {};
    allCurrent.rotation = allFirst.rotation = {0, 0, 0, 1};
    if (allIt != vmd->boneTimelines.end()) {
      allCurrent = InterpolateBone(allIt->second.keys, frame, true);
      allFirst = InterpolateBone(allIt->second.keys, 0.0f, true);
    }
    Quat allRotation = SourceToRootRotation(allCurrent.rotation);
    if (g_yawOffsetDeg != 0.0f) {
      const float halfYaw = g_yawOffsetDeg * 0.00872664626f;
      const Quat yaw = {0, sinf(halfYaw), 0, cosf(halfYaw)};
      allRotation = QuatNormalize(QuatMul(yaw, allRotation));
    }
    allRotation = ScaleRotation(allRotation, PartScale(DIRECT_PART_ROOT));
    const Vec3 allSourceDelta = Vec3Sub(allCurrent.position,
                                        allFirst.position);
    const float translationScale = positionScale * g_motionScale;
    const Vec3 allOffset = Vec3Scale(
        QuatRotate(rootBasis, VmdToUnityPosition(allSourceDelta)),
        translationScale);

    for (int side = 0; side < 2; ++side) {
      const DirectIkBinding &chain = ikBindings[side];
      DirectPoseIk &pose = outIk[side];
      pose = {};
      pose.allParentRotationRoot = allRotation;
      pose.weight = (std::max)(0.0f, (std::min)(1.0f,
          PartScale(DIRECT_PART_LEGS)));
      if (!chain.footTimeline || !chain.legValid || pose.weight <= 0.0f)
        continue;

      const InterpResult footCurrent =
          InterpolateBone(chain.footTimeline->keys, frame, true);
      const InterpResult footFirst =
          InterpolateBone(chain.footTimeline->keys, 0.0f, true);
      const Vec3 footSourceDelta = Vec3Sub(footCurrent.position,
                                           footFirst.position);
      const Vec3 footOffset = Vec3Scale(
          QuatRotate(rootBasis, VmdToUnityPosition(footSourceDelta)),
          translationScale);
      pose.allParentOffsetRoot = Vec3Add(
          allOffset,
          {g_posOffsetX, g_posOffsetY, g_posOffsetZ});
      pose.footOffsetRoot = footOffset;
      pose.legEnabled = SampleIkEnabled(chain.footTrackName, frame);

      const Quat footRelative = QuatNormalize(QuatMul(
          {footCurrent.rotation.x, footCurrent.rotation.y,
           footCurrent.rotation.z, footCurrent.rotation.w},
          QuatInv({footFirst.rotation.x, footFirst.rotation.y,
                   footFirst.rotation.z, footFirst.rotation.w})));
      pose.footRotationRoot = ScaleRotation(
          SourceToRootRotation(footRelative), PartScale(DIRECT_PART_LEGS));

      if (chain.toeTimeline && chain.toeValid) {
        const InterpResult toeCurrent =
            InterpolateBone(chain.toeTimeline->keys, frame, true);
        const InterpResult toeFirst =
            InterpolateBone(chain.toeTimeline->keys, 0.0f, true);
        const Vec3 toeSourceDelta = Vec3Sub(toeCurrent.position,
                                            toeFirst.position);
        pose.toeOffsetRoot = Vec3Scale(
            QuatRotate(rootBasis, VmdToUnityPosition(toeSourceDelta)),
            translationScale);
        pose.toeEnabled = SampleIkEnabled(chain.toeTrackName, frame);
      }
    }
  }

  void Sample(float timeSeconds, uint32_t generation, DirectPoseFrame &out) const {
    out.generation = generation;
    out.timeSeconds = timeSeconds;
    out.frame = timeSeconds * 30.0f;
    out.bones.resize(bindings.size());

    Quat rootAllSource = {0, 0, 0, 1};
    Quat rootCommonSource = {0, 0, 0, 1};
    for (const auto &rootBinding : bindings) {
      if (!rootBinding.isRootChannel)
        continue;
      for (const auto &track : rootBinding.tracks) {
        const InterpResult current =
            InterpolateBone(track.timeline->keys, out.frame, false);
        rootAllSource =
            ComposeVmdRotation(rootAllSource, current.rotation);
        if (track.name != u8"下半身")
          rootCommonSource =
              ComposeVmdRotation(rootCommonSource, current.rotation);
      }
    }

    Quat rootAllTarget = SourceToRootRotation(rootAllSource);
    Quat rootCommonTarget = SourceToRootRotation(rootCommonSource);
    if (g_yawOffsetDeg != 0.0f) {
      const float halfYaw = g_yawOffsetDeg * 0.00872664626f;
      const Quat yaw = {0, sinf(halfYaw), 0, cosf(halfYaw)};
      rootAllTarget = QuatNormalize(QuatMul(yaw, rootAllTarget));
      rootCommonTarget = QuatNormalize(QuatMul(yaw, rootCommonTarget));
    }
    const Quat rootAllApplied =
        ScaleRotation(rootAllTarget, PartScale(DIRECT_PART_ROOT));
    const Quat rootCommonApplied =
        ScaleRotation(rootCommonTarget, PartScale(DIRECT_PART_ROOT));

    for (size_t i = 0; i < bindings.size(); ++i) {
      const DirectBoneBinding &binding = bindings[i];
      Quat combinedRoot = {0, 0, 0, 1};
      Vec3 position = {0, 0, 0};
      Vec3 firstPosition = {0, 0, 0};
      bool hasPosition = false;
      for (const auto &track : binding.tracks) {
        const InterpResult current =
            InterpolateBone(track.timeline->keys, out.frame, track.hasPosition);
        Quat unityRotation = VmdToUnityRotation(current.rotation);
        Quat layerRoot;
        if (IsElbowTrack(track.name)) {
          const Vec3 sourceHinge =
              IsLeftArmTrack(track.name) ? Vec3{0, 1, 0}
                                         : Vec3{0, -1, 0};
          unityRotation = ExtractTwistRotation(unityRotation, sourceHinge);
          layerRoot = QuatNormalize(QuatMul(
              QuatMul(rootBasis, unityRotation), QuatInv(rootBasis)));
          if (binding.elbowHingeRetargetReady) {
            layerRoot = QuatNormalize(QuatMul(
                QuatMul(binding.elbowHingeRetargetRoot, layerRoot),
                QuatInv(binding.elbowHingeRetargetRoot)));
          }
        } else if (IsArmTwistTrack(track.name)) {
          unityRotation =
              ExtractTwistRotation(unityRotation, {1, 0, 0});
          layerRoot = QuatNormalize(QuatMul(
              QuatMul(rootBasis, unityRotation), QuatInv(rootBasis)));
          if (binding.armTwistRetargetReady) {
            layerRoot = QuatNormalize(QuatMul(
                QuatMul(binding.armTwistRetargetRoot, layerRoot),
                QuatInv(binding.armTwistRetargetRoot)));
          }
        } else {
          layerRoot = QuatNormalize(QuatMul(
              QuatMul(rootBasis, unityRotation), QuatInv(rootBasis)));
        }
        combinedRoot = ComposeVmdRotation(combinedRoot, layerRoot);
        if (track.hasPosition) {
          const InterpResult first =
              InterpolateBone(track.timeline->keys, 0.0f, true);
          position.x += current.position.x;
          position.y += current.position.y;
          position.z += current.position.z;
          firstPosition.x += first.position.x;
          firstPosition.y += first.position.y;
          firstPosition.z += first.position.z;
          hasPosition = true;
        }
      }

      Quat deltaRoot;
      if (binding.isRootChannel) {
        deltaRoot = rootAllApplied;
      } else {
        const Quat partRotation =
            ScaleRotation(combinedRoot, PartScale(binding.part));
        if (binding.cancelsLowerBody) {
          deltaRoot = QuatNormalize(QuatMul(
              QuatMul(QuatInv(rootAllApplied), rootCommonApplied),
              partRotation));
        } else {
          deltaRoot = partRotation;
        }
      }

      const Quat parent = binding.parentBindFromRoot;
      const Quat deltaParent = QuatNormalize(
          QuatMul(QuatMul(QuatInv(parent), deltaRoot), parent));

      DirectPoseBone &pose = out.bones[i];
      pose.transform = binding.transform;
      pose.localRotation =
          QuatNormalize(QuatMul(deltaParent, binding.bindLocal));
      pose.hasPosition = hasPosition && binding.controlsPosition;
      pose.localPosition = binding.bindPosition;
      if (pose.hasPosition) {
        const float scale = positionScale * g_motionScale;
        const Vec3 sourceDelta = {position.x - firstPosition.x,
                                  position.y - firstPosition.y,
                                  position.z - firstPosition.z};
        const Vec3 unityDelta = VmdToUnityPosition(sourceDelta);
        const Vec3 targetDelta = QuatRotate(rootBasis, unityDelta);
        pose.localPosition.x += targetDelta.x * scale + g_posOffsetX;
        pose.localPosition.y += targetDelta.y * scale + g_posOffsetY;
        pose.localPosition.z += targetDelta.z * scale + g_posOffsetZ;
      }
    }
    SampleIkPose(out.frame, out.ik);
  }

  bool SetWorldRotationFromIk(void *transform, Quat worldRotation) const {
    if (!transform)
      return false;
    void *parent = DirectGetParentTransform(transform);
    Quat parentWorld = {0, 0, 0, 1};
    if (parent && !ReadWorldRotation(parent, parentWorld))
      return false;
    const Quat local = QuatNormalize(
        QuatMul(QuatInv(parentWorld), QuatNormalize(worldRotation)));
    SafeSetLocalRotation(transform, local);
    return true;
  }

  void ApplyLegIk(const DirectIkBinding &chain,
                  const DirectPoseIk &pose) const {
    if (!rootTransform || !chain.legValid || !pose.legEnabled ||
        pose.weight <= 0.0f)
      return;

    Vec3 rootPosition, upperPosition, kneePosition, footPosition;
    Quat rootWorld, upperWorld, lowerWorld;
    if (!ReadWorldPosition(rootTransform, rootPosition) ||
        !ReadWorldRotation(rootTransform, rootWorld) ||
        !ReadWorldPosition(chain.upperLeg, upperPosition) ||
        !ReadWorldPosition(chain.lowerLeg, kneePosition) ||
        !ReadWorldPosition(chain.foot, footPosition) ||
        !ReadWorldRotation(chain.upperLeg, upperWorld) ||
        !ReadWorldRotation(chain.lowerLeg, lowerWorld))
      return;

    // VMD IK positions are model-controller coordinates. Without the source
    // PMX rest positions, binding them to the target avatar's T-pose makes the
    // first animated FK pose snap back to rest and spreads the thighs. Remove
    // the sampled controller transform from the first actual FK result and use
    // that as this avatar's controller base instead.
    if (!chain.targetCalibrated) {
      const Quat rootInverse = QuatInv(rootWorld);
      const Vec3 currentFootRoot = QuatRotate(
          rootInverse, Vec3Sub(footPosition, rootPosition));
      chain.targetFootFromRoot = Vec3Sub(
          QuatRotate(QuatInv(pose.allParentRotationRoot),
                     Vec3Sub(currentFootRoot, pose.allParentOffsetRoot)),
          pose.footOffsetRoot);
      chain.targetToeFromFoot = chain.bindToeFromFoot;
      if (chain.toeValid) {
        Vec3 toePosition;
        if (ReadWorldPosition(chain.toes, toePosition)) {
          const Vec3 currentToeFromFootRoot = QuatRotate(
              rootInverse, Vec3Sub(toePosition, footPosition));
          const Quat toeParentRotation = QuatNormalize(QuatMul(
              pose.allParentRotationRoot, pose.footRotationRoot));
          chain.targetToeFromFoot = Vec3Sub(
              QuatRotate(QuatInv(toeParentRotation),
                         currentToeFromFootRoot),
              pose.toeOffsetRoot);
        }
      }
      chain.targetCalibrated = true;
      Log("[VMD-DIRECT] Calibrated %s IK from first applied FK pose",
          chain.footTrackName.c_str());
      return;
    }

    const Vec3 footTargetRoot = Vec3Add(
        pose.allParentOffsetRoot,
        QuatRotate(pose.allParentRotationRoot,
                   Vec3Add(chain.targetFootFromRoot,
                           pose.footOffsetRoot)));
    Vec3 footTarget = Vec3Add(
        rootPosition, QuatRotate(rootWorld, footTargetRoot));
    footTarget = Vec3Add(footPosition,
        Vec3Scale(Vec3Sub(footTarget, footPosition), pose.weight));

    Quat upperParentWorld = rootWorld;
    void *upperParent = DirectGetParentTransform(chain.upperLeg);
    if (upperParent)
      ReadWorldRotation(upperParent, upperParentWorld);
    const Vec3 poleWorld = QuatRotate(
        upperParentWorld, chain.bindPoleFromUpperParent);
    Vec3 desiredKnee;
    if (!SolveTwoBoneKneePosition(upperPosition, footTarget,
                                  chain.upperLength, chain.lowerLength,
                                  poleWorld, desiredKnee))
      return;

    const Quat upperDelta = QuatFromTo(
        Vec3Sub(kneePosition, upperPosition),
        Vec3Sub(desiredKnee, upperPosition));
    if (!SetWorldRotationFromIk(
            chain.upperLeg, QuatMul(upperDelta, upperWorld)))
      return;

    if (!ReadWorldPosition(chain.lowerLeg, kneePosition) ||
        !ReadWorldPosition(chain.foot, footPosition) ||
        !ReadWorldRotation(chain.lowerLeg, lowerWorld))
      return;
    const Quat lowerDelta = QuatFromTo(
        Vec3Sub(footPosition, kneePosition),
        Vec3Sub(footTarget, kneePosition));
    SetWorldRotationFromIk(chain.lowerLeg,
                           QuatMul(lowerDelta, lowerWorld));

    if (!pose.toeEnabled || !chain.toeValid)
      return;
    Vec3 toePosition;
    Quat footWorld;
    if (!ReadWorldPosition(chain.foot, footPosition) ||
        !ReadWorldPosition(chain.toes, toePosition) ||
        !ReadWorldRotation(chain.foot, footWorld))
      return;
    const Quat toeParentRotation = QuatNormalize(QuatMul(
        pose.allParentRotationRoot, pose.footRotationRoot));
    const Vec3 toeTargetRoot = Vec3Add(
        footTargetRoot,
        QuatRotate(toeParentRotation,
                   Vec3Add(chain.targetToeFromFoot,
                           pose.toeOffsetRoot)));
    Vec3 toeTarget = Vec3Add(
        rootPosition, QuatRotate(rootWorld, toeTargetRoot));
    toeTarget = Vec3Add(toePosition,
        Vec3Scale(Vec3Sub(toeTarget, toePosition), pose.weight));
    const Quat footDelta = QuatFromTo(
        Vec3Sub(toePosition, footPosition),
        Vec3Sub(toeTarget, footPosition));
    SetWorldRotationFromIk(chain.foot, QuatMul(footDelta, footWorld));
  }

  void Apply(const DirectPoseFrame &pose) const {
    if (!prepared)
      return;
    SafeSetAnimatorEnabled(false);
    for (const DirectPoseBone &bone : pose.bones) {
      if (!bone.transform)
        continue;
      SafeSetLocalRotation(bone.transform, bone.localRotation);
      if (bone.hasPosition)
        SafeSetLocalPosition(bone.transform, bone.localPosition);
    }
    for (int side = 0; side < 2; ++side)
      ApplyLegIk(ikBindings[side], pose.ik[side]);
  }
};

static SRWLOCK g_directPoseLock = SRWLOCK_INIT;
static SRWLOCK g_directClockLock = SRWLOCK_INIT;
static SRWLOCK g_directMotionLock = SRWLOCK_INIT;
static DirectPoseFrame g_directPoseBuffer;
static std::atomic<bool> g_directPosePending{false};
static std::atomic<uint32_t> g_directGeneration{1};
static std::atomic<float> g_directCurrentTime{0.0f};
static std::atomic<bool> g_directPlaying{false};

static bool IsDirectVmdMode() {
  return g_motionSource.load(std::memory_order_acquire) ==
         MOTION_SOURCE_DIRECT_VMD;
}

static MmdPlayer *GetActiveMotionPlayer() {
  return IsDirectVmdMode() ? g_directPlayer : g_musclePlayer;
}

static const VmdFile *GetActiveMotionVmd() {
  return IsDirectVmdMode() && g_directMotion ? g_directMotion->vmd : g_morphVmd;
}

static DirectVmdMotion *CreateDirectVmdMotion(const char *path) {
  std::unique_ptr<DirectVmdMotion> motion(new DirectVmdMotion());
  motion->vmd = LoadVmd(path);
  if (!motion->vmd || !motion->vmd->loaded) {
    motion->error = motion->vmd ? motion->vmd->error : "VMD allocation failed";
    return motion.release();
  }
  motion->durationFrames =
      (std::max)(motion->vmd->boneFrames, motion->vmd->morphFrames);
  if (!motion->AnalyzeSourceTracks())
    return motion.release();
  return motion.release();
}

// Takes ownership of an already parsed VMD. The GUI load path uses this to
// avoid parsing a large action twice on the game window thread.
static DirectVmdMotion *CreateDirectVmdMotion(VmdFile *parsedVmd) {
  std::unique_ptr<VmdFile, void (*)(VmdFile *)> parsed(parsedVmd, FreeVmd);
  std::unique_ptr<DirectVmdMotion> motion(new DirectVmdMotion());
  motion->vmd = parsed.release();
  if (!motion->vmd || !motion->vmd->loaded) {
    motion->error = motion->vmd ? motion->vmd->error : "VMD allocation failed";
    return motion.release();
  }
  motion->durationFrames =
      (std::max)(motion->vmd->boneFrames, motion->vmd->morphFrames);
  if (!motion->AnalyzeSourceTracks())
    return motion.release();
  return motion.release();
}

static void PublishDirectStats(const DirectVmdMotion *motion) {
  if (!motion || !motion->analyzed || !motion->vmd) {
    g_directReady.store(false, std::memory_order_release);
    return;
  }
  g_directMappedBones.store(motion->mappedTracks, std::memory_order_release);
  g_directUnmappedBones.store(motion->unmappedTracks, std::memory_order_release);
  g_directUnsupportedBones.store(motion->unsupportedTracks,
                                 std::memory_order_release);
  g_directTotalFrames.store(motion->durationFrames, std::memory_order_release);
  g_directMorphTracks.store((int)motion->vmd->morphTimelines.size(),
                            std::memory_order_release);
  g_directCameraKeys.store((int)motion->vmd->cameraKeys.size(),
                          std::memory_order_release);
  g_directIkTracks.store(motion->ikTracks, std::memory_order_release);
  g_directReady.store(true, std::memory_order_release);
  g_directLastError[0] = '\0';
}

static void ReplaceDirectMotion(DirectVmdMotion *motion) {
  AcquireSRWLockExclusive(&g_directMotionLock);
  DirectVmdMotion *old = g_directMotion;
  g_directMotion = motion;
  ReleaseSRWLockExclusive(&g_directMotionLock);
  delete old;
}

static void SetExternalComponentEnabled(void *component, bool enabled) {
  if (!component || !g_animator_set_enabled)
    return;
  __try {
    void *args[] = {&enabled};
    Invoke(g_animator_set_enabled, component, args);
  } __except (1) {}
}

static void PrepareDirectExternalComponents() {
  if (s_ikDisabled || !g_cachedAnimator)
    return;
  void *root = SafeGetComponentTransform(g_cachedAnimator);
  if (!root || !g_componentClass)
    return;
  void *componentType = il2cpp_class_get_type(g_componentClass);
  void *typeObject = componentType ? il2cpp_type_get_object(componentType)
                                   : nullptr;
  if (!typeObject)
    return;

  struct WalkItem { void *transform; int depth; };
  WalkItem stack[128];
  int top = 0;
  stack[top++] = {root, 0};
  while (top > 0) {
    const WalkItem item = stack[--top];
    void *gameObject = nullptr;
    __try { gameObject = Invoke(g_component_get_gameObject, item.transform); }
    __except (1) { gameObject = nullptr; }
    if (gameObject) {
      void *getComponents =
          FindMethod(il2cpp_object_get_class(gameObject), "GetComponents", 1);
      if (getComponents) {
        void *args[] = {typeObject};
        void *exception = nullptr;
        void *array = nullptr;
        __try {
          array = il2cpp_runtime_invoke(getComponents, gameObject, args,
                                        &exception);
        } __except (1) { array = nullptr; }
        if (array && !exception) {
          const int count = *(int *)((char *)array + 24);
          void **items = (void **)((char *)array + 32);
          for (int i = 0; i < count; ++i) {
            if (!items[i])
              continue;
            void *klass = il2cpp_object_get_class(items[i]);
            const char *name = klass ? il2cpp_class_get_name(klass) : "";
            if (strcmp(name, "BipedIK") == 0 && s_bipedIKCount < MAX_IK)
              s_bipedIK[s_bipedIKCount++] = items[i];
            else if (strcmp(name, "GrounderBipedIK") == 0 &&
                     s_grounderIKCount < MAX_IK)
              s_grounderIK[s_grounderIKCount++] = items[i];
            else if (strcmp(name, "LookAtComponent") == 0 &&
                     s_lookAtCount < MAX_IK)
              s_lookAt[s_lookAtCount++] = items[i];
            else if (strcmp(name, "AnimatorMono") == 0)
              s_animatorMono = items[i];
          }
        }
      }
    }
    if (item.depth < 3 && g_transform_get_childCount && g_transform_GetChild) {
      int childCount = 0;
      __try {
        void *boxed = Invoke(g_transform_get_childCount, item.transform);
        childCount = boxed ? *(int *)((char *)boxed + 16) : 0;
      } __except (1) { childCount = 0; }
      for (int i = 0; i < childCount && top < 128; ++i) {
        void *args[] = {&i};
        void *child = nullptr;
        __try { child = Invoke(g_transform_GetChild, item.transform, args); }
        __except (1) { child = nullptr; }
        if (child)
          stack[top++] = {child, item.depth + 1};
      }
    }
  }

  for (int i = 0; i < s_bipedIKCount; ++i)
    SetExternalComponentEnabled(s_bipedIK[i], false);
  for (int i = 0; i < s_grounderIKCount; ++i)
    SetExternalComponentEnabled(s_grounderIK[i], false);
  for (int i = 0; i < s_lookAtCount; ++i)
    SetExternalComponentEnabled(s_lookAt[i], false);
  SetExternalComponentEnabled(s_animatorMono, false);
  if (g_confirmedSMC) {
    __try {
      *(bool *)((char *)g_confirmedSMC + 0x1dd) = false;
      s_eyeIKDisabled = true;
    } __except (1) {}
  }
  s_ikDisabled = true;
  Log("[VMD-DIRECT] Disabled external pose writers: biped=%d grounder=%d look=%d animatorMono=%p",
      s_bipedIKCount, s_grounderIKCount, s_lookAtCount, s_animatorMono);
}

static void InvalidateDirectPlayback() {
  g_directGeneration.fetch_add(1, std::memory_order_acq_rel);
  g_directPosePending.store(false, std::memory_order_release);
  g_directPlaying.store(false, std::memory_order_release);
  g_directCurrentTime.store(0.0f, std::memory_order_release);
  AcquireSRWLockExclusive(&g_directClockLock);
  if (g_directPlayer)
    g_directPlayer->Stop();
  ReleaseSRWLockExclusive(&g_directClockLock);
}

static void SampleDirectMorphs(float frame) {
  const VmdFile *source = g_morphVmd;
  if (!source && g_directMotion)
    source = g_directMotion->vmd;
  if (!source || !source->loaded)
    return;

  static const char *mouthNames[5] = {u8"あ", u8"い", u8"う", u8"え", u8"お"};
  for (int i = 0; i < 5; ++i) {
    auto it = source->morphTimelines.find(mouthNames[i]);
    g_mouthWeights[i] = it == source->morphTimelines.end()
                             ? 0.0f
                             : it->second.Sample(frame);
  }
  g_mouthWeightsFromMuscle = true;
  for (int i = 0; i < NUM_EXTRA_MORPHS; ++i) {
    auto it = source->morphTimelines.find(g_extraMorphs[i].vmdNameUtf8);
    const float weight = it == source->morphTimelines.end()
                             ? 0.0f
                             : it->second.Sample(frame);
    g_extraMorphs[i].weight = weight;
    g_extraMorphs[i].prevWeight = weight;
  }
}

static void UpdateDirectCameraTracking() {
  if (!g_cachedAnimator)
    return;
  void *root = SafeGetComponentTransform(g_cachedAnimator);
  void *hips = SafeGetBoneTransform(HB_Hips);
  void *head = SafeGetBoneTransform(HB_Head);
  if (g_cameraNeedsCapture && g_cameraPlayer.HasData()) {
    g_cameraNeedsCapture = false;
    Vec3 rootPos = {0, 0, 0};
    if (root && ReadWorldPosition(root, rootPos))
      g_charWorldPos = rootPos;
    if (hips && ReadWorldPosition(hips, g_camInitHipsWorldPos))
      g_charWorldPos = g_camInitHipsWorldPos;
    Quat rootRot = {0, 0, 0, 1};
    if (DirectVmdMotion::ReadWorldRotation(root, rootRot)) {
      g_charYaw = atan2f(2.0f * (rootRot.w * rootRot.y + rootRot.x * rootRot.z),
                         1.0f - 2.0f * (rootRot.y * rootRot.y +
                                        rootRot.z * rootRot.z));
    }
    Vec3 headPos;
    if (head && ReadWorldPosition(head, headPos)) {
      g_headWorldPos = headPos;
      const float height = headPos.y - rootPos.y;
      if (height > 0.1f && height < 5.0f)
        g_charHeight = height;
    }
    if (g_charHeight > 0.0f) {
      if (g_camRefHeight <= 0.0f)
        g_camRefHeight = CAM_REF_HEIGHT;
      g_camHeightScale = g_charHeight / g_camRefHeight;
    } else {
      g_camHeightScale = 1.0f;
    }
    CaptureAndDisableCinemachine();
    g_cameraActive = true;
    g_camInitInterest = g_cameraPlayer.SampleInterest(0.0f);
    Log("[VMD-DIRECT] Camera captured: pos=(%.3f,%.3f,%.3f) height=%.3f",
        g_charWorldPos.x, g_charWorldPos.y, g_charWorldPos.z, g_charHeight);
  }
  if (!g_cameraActive)
    return;
  Vec3 rootNow;
  if (root && ReadWorldPosition(root, rootNow))
    g_charWorldPos = rootNow;
  Vec3 hipsNow;
  if (hips && ReadWorldPosition(hips, hipsNow)) {
    g_charWorldPos.x += hipsNow.x - g_camInitHipsWorldPos.x;
    g_charWorldPos.z += hipsNow.z - g_camInitHipsWorldPos.z;
  }
  if (head && ReadWorldPosition(head, g_headWorldPos)) {
    Quat headRot;
    if (DirectVmdMotion::ReadWorldRotation(head, headRot)) {
      g_headForward.x = 2.0f * (headRot.w * headRot.y + headRot.x * headRot.z);
      g_headForward.y = 2.0f * (headRot.y * headRot.z - headRot.w * headRot.x);
      g_headForward.z = 1.0f - 2.0f * (headRot.x * headRot.x +
                                      headRot.y * headRot.y);
    }
  }
}

static void DirectAnimationTick() {
  if (!IsDirectVmdMode() || !g_directPlayer)
    return;
  if (g_directPosePending.load(std::memory_order_acquire))
    return;

  AcquireSRWLockExclusive(&g_directClockLock);
  if (!g_directPlayer->playing) {
    ReleaseSRWLockExclusive(&g_directClockLock);
    return;
  }
  const float previousTime = g_directPlayer->currentTime;
  const float frame = g_directPlayer->Tick();
  (void)frame;
  SyncAudioToMotion(g_directPlayer, previousTime);
  const float sampleTime = g_directPlayer->currentTime;
  const bool stillPlaying = g_directPlayer->playing;
  ReleaseSRWLockExclusive(&g_directClockLock);
  DirectPoseFrame next;
  const uint32_t generation = g_directGeneration.load(std::memory_order_acquire);
  AcquireSRWLockShared(&g_directMotionLock);
  if (!g_directMotion || !g_directMotion->prepared) {
    ReleaseSRWLockShared(&g_directMotionLock);
    return;
  }
  g_directMotion->Sample(sampleTime, generation, next);
  ReleaseSRWLockShared(&g_directMotionLock);
  AcquireSRWLockExclusive(&g_directPoseLock);
  g_directPoseBuffer = std::move(next);
  ReleaseSRWLockExclusive(&g_directPoseLock);
  g_directCurrentTime.store(sampleTime, std::memory_order_release);
  g_directPlaying.store(stillPlaying, std::memory_order_release);
  g_directPosePending.store(true, std::memory_order_release);
  if (g_gameHwnd)
    PostMessageW(g_gameHwnd, WM_USER + 2, (WPARAM)generation, 0);
}

static void ApplyPendingDirectPose(uint32_t generation) {
  DirectPoseFrame pose;
  AcquireSRWLockShared(&g_directPoseLock);
  pose = g_directPoseBuffer;
  ReleaseSRWLockShared(&g_directPoseLock);
  g_directPosePending.store(false, std::memory_order_release);
  if (!IsDirectVmdMode() || !g_directMotion ||
      generation != g_directGeneration.load(std::memory_order_acquire) ||
      pose.generation != generation)
    return;
  g_directMotion->Apply(pose);
  SampleDirectMorphs(pose.frame);
  UpdateDirectCameraTracking();
  if (g_cameraActive)
    ApplyCameraFrame(pose.timeSeconds);
}
