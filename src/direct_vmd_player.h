#pragma once

#include <algorithm>
#include <atomic>
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
  Vec3 bindPosition = {0, 0, 0};
  bool controlsPosition = false;
  bool isRootChannel = false;
  bool cancelsLowerBody = false;
  std::vector<DirectSourceTrack> tracks;
};

struct DirectPoseBone {
  void *transform = nullptr;
  Quat localRotation = {0, 0, 0, 1};
  Vec3 localPosition = {0, 0, 0};
  bool hasPosition = false;
};

struct DirectPoseFrame {
  uint32_t generation = 0;
  float timeSeconds = 0.0f;
  float frame = 0.0f;
  std::vector<DirectPoseBone> bones;
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
  uint32_t durationFrames = 0;
  int mappedTracks = 0;
  int unmappedTracks = 0;
  int unsupportedTracks = 0;
  float positionScale = 0.08f;
  Quat rootBasis = {0, 0, 0, 1};
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
    for (int i = 0; i < g_fingerMapCount; ++i) {
      const FingerMapEntry &entry = g_fingerMap[i];
      if (!strstr(entry.transformName, "Finger"))
        continue;
      auto timelineIt = vmd->boneTimelines.find(entry.mmdName);
      if (timelineIt == vmd->boneTimelines.end())
        continue;
      seen.push_back(entry.mmdName);
      void *transform = root ? SafeFindChildRecursive(root, entry.transformName, 15)
                             : nullptr;
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

  bool BuildBindings() {
    if (!vmd || !vmd->loaded || !g_cachedAnimator) {
      error = "No loaded VMD or active character Animator";
      return false;
    }
    void *root = SafeGetComponentTransform(g_cachedAnimator);
    if (!root) {
      error = "Cannot resolve character root Transform";
      return false;
    }

    bindings.clear();
    mappedTracks = unmappedTracks = unsupportedTracks = 0;
    std::vector<std::string> seen;
    for (const auto &entry : g_directMap)
      AddTrack(entry, root, seen);
    AddFingerTracks(root, seen);

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

    DirectRebindAnimatorToRest();

    Quat rootWorld = {0, 0, 0, 1};
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
      std::sort(binding.tracks.begin(), binding.tracks.end(),
                [](const DirectSourceTrack &a, const DirectSourceTrack &b) {
                  return a.order < b.order;
                });
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
    SafeSetAnimatorEnabled(false);
    prepared = true;
    return true;
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
      Quat combined = {0, 0, 0, 1};
      Vec3 position = {0, 0, 0};
      Vec3 firstPosition = {0, 0, 0};
      bool hasPosition = false;
      for (const auto &track : binding.tracks) {
        const InterpResult current =
            InterpolateBone(track.timeline->keys, out.frame, track.hasPosition);
        combined = ComposeVmdRotation(combined, current.rotation);
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
        const Quat targetRotation = SourceToRootRotation(combined);
        const Quat partRotation =
            ScaleRotation(targetRotation, PartScale(binding.part));
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
  }

  void Apply(const DirectPoseFrame &pose) const {
    if (!prepared)
      return;
    SafeSetAnimatorEnabled(false);
    for (const auto &bone : pose.bones) {
      if (!bone.transform)
        continue;
      SafeSetLocalRotation(bone.transform, bone.localRotation);
      if (bone.hasPosition)
        SafeSetLocalPosition(bone.transform, bone.localPosition);
    }
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
  DirectVmdMotion *motion = new DirectVmdMotion();
  motion->vmd = LoadVmd(path);
  if (!motion->vmd || !motion->vmd->loaded) {
    motion->error = motion->vmd ? motion->vmd->error : "VMD allocation failed";
    return motion;
  }
  motion->durationFrames =
      (std::max)(motion->vmd->boneFrames, motion->vmd->morphFrames);
  if (!motion->BuildBindings())
    return motion;
  return motion;
}

static void PublishDirectStats(const DirectVmdMotion *motion) {
  if (!motion || !motion->prepared) {
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
