#pragma once

static const char *g_humanBoneNames[] = {
    "Hips",                    
    "LeftUpperLeg",            
    "RightUpperLeg",           
    "LeftLowerLeg",            
    "RightLowerLeg",           
    "LeftFoot",                
    "RightFoot",               
    "Spine",                   
    "Chest",                   
    "Neck",                    
    "Head",                    
    "LeftShoulder",            
    "RightShoulder",           
    "LeftUpperArm",            
    "RightUpperArm",           
    "LeftLowerArm",            
    "RightLowerArm",           
    "LeftHand",                
    "RightHand",               
    "LeftToes",                
    "RightToes",               
    "LeftEye",                 
    "RightEye",                
    "Jaw",                     
    "LeftThumbProximal",       
    "LeftThumbIntermediate",   
    "LeftThumbDistal",         
    "LeftIndexProximal",       
    "LeftIndexIntermediate",   
    "LeftIndexDistal",         
    "LeftMiddleProximal",      
    "LeftMiddleIntermediate",  
    "LeftMiddleDistal",        
    "LeftRingProximal",        
    "LeftRingIntermediate",    
    "LeftRingDistal",          
    "LeftLittleProximal",      
    "LeftLittleIntermediate",  
    "LeftLittleDistal",        
    "RightThumbProximal",      
    "RightThumbIntermediate",  
    "RightThumbDistal",        
    "RightIndexProximal",      
    "RightIndexIntermediate",  
    "RightIndexDistal",        
    "RightMiddleProximal",     
    "RightMiddleIntermediate", 
    "RightMiddleDistal",       
    "RightRingProximal",       
    "RightRingIntermediate",   
    "RightRingDistal",         
    "RightLittleProximal",     
    "RightLittleIntermediate", 
    "RightLittleDistal",       
    "UpperChest",              
};
static const int g_humanBoneCount = 55;

static void DiscoverSkeleton();            
static void *g_playerController = nullptr; 
static void *g_mainCharEntity = nullptr;   
static void *g_cachedAnimator =
    nullptr; 

static MmdPlayer *g_directPlayer = nullptr;

static void SafeRefreshEntity() {
  __try {
    int pcOff = SafeOff(OFF_pcEntity, 0x70, "pcEntity");
    void *entity = *(void **)((char *)g_playerController + pcOff);
    if (entity) {
      g_mainCharEntity = entity;
      int ecOff = SafeOff(OFF_entityComplexAnim, 0x110, "entityComplexAnim");
      void *complexAnimCom = *(void **)((char *)entity + ecOff);
      if (complexAnimCom) {
        if (OFF_complexAnimAnimator < 0) {
          __try {
            void *cacClass = il2cpp_object_get_class(complexAnimCom);
            if (cacClass) {
              const char *animNames[] = {"animator", "m_animator",
                                         "_animator",
                                         "<animator>k__BackingField"};
              const char *matchedName = nullptr;
              OFF_complexAnimAnimator = FindFieldInHierarchy(
                  cacClass, animNames, 4, &matchedName);
              if (OFF_complexAnimAnimator >= 0) {
                Log("[OFFSET] ComplexAnimComp.%s = %d (0x%X) [lazy resolve]",
                    matchedName, OFF_complexAnimAnimator,
                    OFF_complexAnimAnimator);
              } else {
                Log("[WARN] ComplexAnimComp animator not found in hierarchy "
                    "(lazy), fallback 0x148");
                DumpFieldsHierarchy(cacClass);
              }
            }
          } __except (1) {
          }
        }
        int caOff =
            SafeOff(OFF_complexAnimAnimator, 0x148, "complexAnimAnimator");
        void *animator = *(void **)((char *)complexAnimCom + caOff);
        if (animator)
          g_cachedAnimator = animator;
      }
    }
  } __except (1) {
  }
}

static void *SafeGetBoneTransform(int humanBone) {
  if (!g_animator_GetBoneTransform || !g_cachedAnimator)
    return nullptr;
  __try {
    void *params[] = {&humanBone};
    return Invoke(g_animator_GetBoneTransform, g_cachedAnimator, params);
  } __except (1) {
    return nullptr;
  }
}

static void SafeGetBoneName(void *transform, char *buf, int sz) {
  buf[0] = 0;
  __try {
    void *nameStr = Invoke(g_object_get_name, transform);
    ReadStrUtf8(nameStr, buf, sz);
  } __except (1) {
  }
}

static void *SafeFindChildRecursive(void *transform, const char *targetName,
                                    int maxDepth) {
  if (!transform || maxDepth <= 0)
    return nullptr;
  __try {
    char name[256] = "";
    if (g_object_get_name) {
      void *nameStr = Invoke(g_object_get_name, transform);
      if (nameStr)
        ReadStrUtf8(nameStr, name, sizeof(name));
    }
    if (strcmp(name, targetName) == 0)
      return transform;

    void *countBoxed = Invoke(g_transform_get_childCount, transform);
    int count = countBoxed ? *(int *)((char *)countBoxed + 16) : 0;
    for (int i = 0; i < count; i++) {
      void *params[] = {&i};
      void *child = Invoke(g_transform_GetChild, transform, params);
      if (child) {
        void *found = SafeFindChildRecursive(child, targetName, maxDepth - 1);
        if (found)
          return found;
      }
    }
  } __except (1) {
  }
  return nullptr;
}

static void *SafeGetComponentTransform(void *component) {
  if (!component || !g_component_get_transform)
    return nullptr;
  __try {
    return Invoke(g_component_get_transform, component);
  } __except (1) {
    return nullptr;
  }
}

static void RefreshEntityAnimator() {
  if (!g_playerController)
    return;
  SafeRefreshEntity();
}

static void RestoreDisabledComponents() {
  if (!g_animator_set_enabled)
    return;

  bool trueValue = true;
  void *params[] = {&trueValue};

  auto EnableComp = [&](void *comp, const char *name) {
    if (!comp)
      return;
    __try {
      Invoke(g_animator_set_enabled, comp, params);
      Log("[IK-RESTORE] %s RE-ENABLED", name);
    } __except (1) {
      Log("[IK-RESTORE] %s re-enable failed (exception)", name);
    }
  };

  for (int bi = 0; bi < s_bipedIKCount; bi++) {
    char name[64];
    sprintf(name, "BipedIK #%d", bi + 1);
    EnableComp(s_bipedIK[bi], name);
  }
  for (int gi = 0; gi < s_grounderIKCount; gi++) {
    char name[64];
    sprintf(name, "GrounderBipedIK #%d", gi + 1);
    EnableComp(s_grounderIK[gi], name);
  }
  for (int i = 0; i < s_followDamperCount; i++) {
    char name[64];
    sprintf(name, "TransformFollowDamper #%d", i + 1);
    EnableComp(s_followDamper[i], name);
  }
  for (int i = 0; i < s_lookAtCount; i++) {
    char name[64];
    sprintf(name, "LookAtComponent #%d", i + 1);
    EnableComp(s_lookAt[i], name);
  }
  EnableComp(s_animatorMono, "AnimatorMono");

  if (g_confirmedSMC && s_eyeIKDisabled) {
    __try {
      *(bool *)((char *)g_confirmedSMC + 0x1dd) = true;
    } __except (1) {}
  }

  s_ikDisabled = false;
  memset(s_bipedIK, 0, sizeof(s_bipedIK));
  s_bipedIKCount = 0;
  memset(s_grounderIK, 0, sizeof(s_grounderIK));
  s_grounderIKCount = 0;
  memset(s_followDamper, 0, sizeof(s_followDamper));
  s_followDamperCount = 0;
  s_animatorMono = nullptr;
  memset(s_lookAt, 0, sizeof(s_lookAt));
  s_lookAtCount = 0;
  s_eyeIKDisabled = false;

  if (s_bbcCount > 0) {
    if (s_bbc_SetAnimPoseRatio) {
      float zero = 0.0f;
      for (int i = 0; i < s_bbcCount; i++) {
        if (!s_bbcInstances[i]) continue;
        __try {
          void *args[] = {&zero};
          void *exc = nullptr;
          il2cpp_runtime_invoke(s_bbc_SetAnimPoseRatio, s_bbcInstances[i],
                                args, &exc);
        } __except (1) {}
      }
    }
    if (s_bbc_ResetCloth) {
      float resetTime = 0.0f;
      for (int i = 0; i < s_bbcCount; i++) {
        if (!s_bbcInstances[i]) continue;
        __try {
          void *args[] = {&resetTime};
          void *exc = nullptr;
          il2cpp_runtime_invoke(s_bbc_ResetCloth, s_bbcInstances[i],
                                args, &exc);
        } __except (1) {}
      }
    }
    Log("[BBC] Restored %d cloth instances (ratio=0, reset)", s_bbcCount);
  }
  s_bbcCount = 0;
  s_skirtBBCIndex = -1;  
  ResetSkirtState();
  memset(s_bbcInstances, 0, sizeof(s_bbcInstances));

  Log("[IK-RESTORE] All components restored");
}

static void SafeSetLocalRotation(void *transform, Quat q) {
  if (!transform || !g_transform_set_localRotation)
    return;
  __try {
    void *params[] = {&q};
    Invoke(g_transform_set_localRotation, transform, params);
  } __except (1) {
  }
}

static void SafeSetLocalPosition(void *transform, Vec3 p) {
  if (!transform || !g_transform_set_localPosition)
    return;
  __try {
    void *params[] = {&p};
    Invoke(g_transform_set_localPosition, transform, params);
  } __except (1) {
  }
}

#include "cloth.h"



static void SafeSetAnimatorEnabled(bool enabled) {
  if (!g_cachedAnimator || !g_animator_set_enabled)
    return;
  __try {
    void *params[] = {&enabled};
    Invoke(g_animator_set_enabled, g_cachedAnimator, params);
  } __except (1) {
  }
}

static bool ReadWorldPosition(void *transform, Vec3 &out) {
  if (!g_transform_get_position || !transform)
    return false;
  __try {
    void *boxed = Invoke(g_transform_get_position, transform);
    if (boxed) {
      float *data = (float *)((char *)boxed + 16);
      out = {data[0], data[1], data[2]};
      return true;
    }
  } __except (1) {
  }
  return false;
}


static MuscleAnim *g_muscleAnim = nullptr;
static MmdPlayer *g_musclePlayer = nullptr;
static uint32_t g_poseHandleGC = 0;
static void *g_cachedMPtr = nullptr;
static void *g_musclesArray = nullptr;
static uint32_t g_musclesArrayGC = 0;

static float g_restBodyPos[3] = {0};
static float g_restBodyRot[4] = {0, 0, 0, 1};

static BoneAnim *g_boneAnim = nullptr;
static MmdPlayer *g_bonePlayer = nullptr;

struct Il2CppHumanPose {
  float bodyPosX, bodyPosY, bodyPosZ;           
  float bodyRotX, bodyRotY, bodyRotZ, bodyRotW; 
  void *muscles; 
};

static void *CreateFloatArray(int count) {
  if (!il2cpp_array_new_specific)
    return nullptr;
  void *domain = il2cpp_domain_get();
  size_t ac;
  void **asms = il2cpp_domain_get_assemblies(domain, &ac);
  void *floatClass = FindClass("System", "Single", asms, ac);
  if (!floatClass) {
    Log("[MUSCLE] System.Single class not found!");
    return nullptr;
  }
  void *arr = il2cpp_array_new_specific(floatClass, count);
  Log("[MUSCLE] Created float[%d] array: %p", count, arr);
  return arr;
}

static float *GetArrayData(void *arr) {
  if (!arr)
    return nullptr;
  return (float *)((char *)arr + 32);
}

static bool InitMusclePoseHandler() {
  if (g_poseHandleGC != 0)
    return true; 
  if (!g_cachedAnimator || !g_humanPoseHandlerClass ||
      !g_humanPoseHandler_ctor) {
    Log("[MUSCLE] Missing: Animator=%p HPHClass=%p ctor=%p", g_cachedAnimator,
        g_humanPoseHandlerClass, g_humanPoseHandler_ctor);
    return false;
  }

  void *avatar = Invoke(g_animator_get_avatar, g_cachedAnimator);
  void *rootTransform = SafeGetComponentTransform(g_cachedAnimator);
  if (!avatar || !rootTransform) {
    Log("[MUSCLE] No avatar=%p or rootTransform=%p", avatar, rootTransform);
    return false;
  }

  void *poseHandler = nullptr;

  __try {
    poseHandler = il2cpp_object_new(g_humanPoseHandlerClass);
    if (!poseHandler) {
      Log("[MUSCLE] Failed to allocate HumanPoseHandler");
      return false;
    }
    void *ctorParams[] = {avatar, rootTransform};
    void *exc = nullptr;
    il2cpp_runtime_invoke(g_humanPoseHandler_ctor, poseHandler, ctorParams,
                          &exc);
    if (exc) {
      Log("[MUSCLE] HumanPoseHandler constructor exception");
      return false;
    }
  } __except (1) {
    Log("[MUSCLE] HumanPoseHandler creation crashed");
    return false;
  }

  g_poseHandleGC = il2cpp_gchandle_new(poseHandler, true);
  Log("[MUSCLE] GC pinned handler: handle=%u obj=%p", g_poseHandleGC,
      poseHandler);

  g_cachedMPtr = *(void **)((char *)poseHandler + 16);
  Log("[MUSCLE] m_Ptr = %p", g_cachedMPtr);
  if (!g_cachedMPtr) {
    Log("[MUSCLE] ERROR: m_Ptr is NULL after construction!");
    return false;
  }

  g_musclesArray = CreateFloatArray(95);
  if (!g_musclesArray) {
    Log("[MUSCLE] Failed to create float[95] array");
    return false;
  }
  g_musclesArrayGC = il2cpp_gchandle_new(g_musclesArray, true);
  Log("[MUSCLE] GC pinned muscles array: handle=%u arr=%p", g_musclesArrayGC,
      g_musclesArray);

  void *mPtrCheck = *(void **)((char *)poseHandler + 16);
  Log("[MUSCLE] m_Ptr after array alloc = %p (was %p)", mPtrCheck,
      g_cachedMPtr);

  if (g_humanPoseHandler_GetHumanPose) {
    Il2CppHumanPose testPose = {};
    __try {
      void *params[] = {&testPose};
      void *exc = nullptr;
      il2cpp_runtime_invoke(g_humanPoseHandler_GetHumanPose, poseHandler,
                            params, &exc);
      if (exc) {
        Log("[MUSCLE] GetHumanPose exception");
      } else {
        g_restBodyPos[0] = testPose.bodyPosX;
        g_restBodyPos[1] = testPose.bodyPosY;
        g_restBodyPos[2] = testPose.bodyPosZ;
        g_restBodyRot[0] = testPose.bodyRotX;
        g_restBodyRot[1] = testPose.bodyRotY;
        g_restBodyRot[2] = testPose.bodyRotZ;
        g_restBodyRot[3] = testPose.bodyRotW;
        Log("[MUSCLE] GetHumanPose OK: pos(%.3f,%.3f,%.3f) "
            "rot(%.3f,%.3f,%.3f,%.3f)",
            testPose.bodyPosX, testPose.bodyPosY, testPose.bodyPosZ,
            testPose.bodyRotX, testPose.bodyRotY, testPose.bodyRotZ,
            testPose.bodyRotW);

        if (g_animator_GetBoneTransform && g_cachedAnimator) {
          void *rootT = SafeGetComponentTransform(g_cachedAnimator);
          void *headT = SafeGetBoneTransform(10); 
          Vec3 rootPos, headPos;
          if (rootT && headT && ReadWorldPosition(rootT, rootPos) &&
              ReadWorldPosition(headT, headPos)) {
            float h = headPos.y - rootPos.y;
            if (h > 0.1f && h < 5.0f) {
              g_charHeight = h;
              Log("[HEIGHT] Character height measured: %.3f m", g_charHeight);
            }
          }
        }
      }
    } __except (1) {
      Log("[MUSCLE] GetHumanPose crashed");
    }
  }

  if (g_humanPoseHandler_GetHumanPose) {
    void **slots = (void **)g_humanPoseHandler_GetHumanPose;
    Log("[MI-DUMP] GetHumanPose MethodInfo at %p:",
        g_humanPoseHandler_GetHumanPose);
    for (int i = 0; i < 10; i++) {
      Log("[MI-DUMP]   [%d] = %p", i, slots[i]);
    }
    void *mp = ((MInfo *)g_humanPoseHandler_GetHumanPose)->mp;
    Log("[MI-DUMP]   MInfo::mp = %p (this is what Hook() uses)", mp);
  }

  Log("[MUSCLE] HumanPoseHandler init complete, g_cachedAnimator=%p",
      g_cachedAnimator);
  return true;
}

static int s_muscleLogCounter = 0;

static Quat QMul(Quat a, Quat b) {
  Quat r;
  r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
  r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
  r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
  r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
  return r;
}

static Quat SafeGetLocalRotation(void *transform) {
  Quat q = {0, 0, 0, 1};
  if (!transform || !g_transform_get_localRotation)
    return q;
  __try {
    void *boxed = Invoke(g_transform_get_localRotation, transform);
    if (boxed)
      q = *(Quat *)((char *)boxed + 16); 
  } __except (1) {
  }
  return q;
}

static Vec3 SafeGetLocalPosition(void *transform) {
  Vec3 p = {0, 0, 0};
  if (!transform || !g_transform_get_localPosition)
    return p;
  __try {
    void *boxed = Invoke(g_transform_get_localPosition, transform);
    if (boxed)
      p = *(Vec3 *)((char *)boxed + 16);
  } __except (1) {
  }
  return p;
}

static bool g_boneRestCaptured = false;
static Quat g_boneRestRot[55];
static Vec3 g_boneRestHipsPos;

static void BoneAnimationTick() {
  if (!g_bonePlayer || !g_bonePlayer->playing)
    return;
  if (!g_boneAnim || !g_boneAnim->loaded)
    return;
  if (!g_cachedAnimator || !g_animator_GetBoneTransform)
    return;

  if (!g_boneRestCaptured) {
    Log("[BONE] Forcing T-pose and capturing game rest pose...");

    SafeSetAnimatorEnabled(false);
    if (g_animator_Rebind) {
      __try {
        Invoke(g_animator_Rebind, g_cachedAnimator);
        Log("[BONE] Animator.Rebind() called");
      } __except (1) {
        Log("[BONE] Rebind() crashed");
      }
    }
    Sleep(50);

    static const char *boneNames[] = {"Hips",
                                      "LeftUpperLeg",
                                      "RightUpperLeg",
                                      "LeftLowerLeg",
                                      "RightLowerLeg",
                                      "LeftFoot",
                                      "RightFoot",
                                      "Spine",
                                      "Chest",
                                      "UpperChest",
                                      "Neck",
                                      "Head",
                                      "LeftShoulder",
                                      "RightShoulder",
                                      "LeftUpperArm",
                                      "RightUpperArm",
                                      "LeftLowerArm",
                                      "RightLowerArm",
                                      "LeftHand",
                                      "RightHand",
                                      "LeftToes",
                                      "RightToes",
                                      "LeftEye",
                                      "RightEye",
                                      "Jaw",
                                      "LeftThumbProximal",
                                      "LeftThumbIntermediate",
                                      "LeftThumbDistal",
                                      "LeftIndexProximal",
                                      "LeftIndexIntermediate",
                                      "LeftIndexDistal",
                                      "LeftMiddleProximal",
                                      "LeftMiddleIntermediate",
                                      "LeftMiddleDistal",
                                      "LeftRingProximal",
                                      "LeftRingIntermediate",
                                      "LeftRingDistal",
                                      "LeftLittleProximal",
                                      "LeftLittleIntermediate",
                                      "LeftLittleDistal",
                                      "RightThumbProximal",
                                      "RightThumbIntermediate",
                                      "RightThumbDistal",
                                      "RightIndexProximal",
                                      "RightIndexIntermediate",
                                      "RightIndexDistal",
                                      "RightMiddleProximal",
                                      "RightMiddleIntermediate",
                                      "RightMiddleDistal",
                                      "RightRingProximal",
                                      "RightRingIntermediate",
                                      "RightRingDistal",
                                      "RightLittleProximal",
                                      "RightLittleIntermediate",
                                      "RightLittleDistal"};

    for (int b = 0; b < 55; b++) {
      void *bt = SafeGetBoneTransform(b);
      if (bt) {
        g_boneRestRot[b] = SafeGetLocalRotation(bt);
      } else {
        g_boneRestRot[b] = {0, 0, 0, 1};
      }
    }
    void *hips = SafeGetBoneTransform(0);
    if (hips)
      g_boneRestHipsPos = SafeGetLocalPosition(hips);

    g_boneRestCaptured = true;

    FILE *df = fopen("plugin\\bone_rest_compare.txt", "w");
    if (df) {
      fprintf(df, "=== Bone Rest Pose Comparison ===\n\n");
      fprintf(df, "Game Hips localPos: (%.4f, %.4f, %.4f)\n\n",
              g_boneRestHipsPos.x, g_boneRestHipsPos.y, g_boneRestHipsPos.z);

      fprintf(df, "%-4s %-30s | %-45s | %-45s | %-45s\n", "Idx", "Bone",
              "Game Rest (x,y,z,w)", "Unity Rest (x,y,z,w)",
              "Frame0 Delta (x,y,z,w)");
      fprintf(df, "------------------------------------------------------------"
                  "-------------------------------------------------\n");

      BoneFrame f0 = g_boneAnim->GetFrame(0);

      for (int b = 0; b < 55; b++) {
        void *bt = SafeGetBoneTransform(b);
        const char *name = (b < 55) ? boneNames[b] : "?";

        fprintf(df,
                "[%2d] %-30s | (%7.4f,%7.4f,%7.4f,%7.4f) | "
                "(%7.4f,%7.4f,%7.4f,%7.4f) | (%7.4f,%7.4f,%7.4f,%7.4f) %s\n",
                b, name, g_boneRestRot[b].x, g_boneRestRot[b].y,
                g_boneRestRot[b].z, g_boneRestRot[b].w,
                g_boneAnim->restPose[b][0], g_boneAnim->restPose[b][1],
                g_boneAnim->restPose[b][2], g_boneAnim->restPose[b][3],
                f0.bones[b][0], f0.bones[b][1], f0.bones[b][2], f0.bones[b][3],
                bt ? "" : "(MISSING in game)");
      }
      fclose(df);
      Log("[BONE] Diagnostic dump written to plugin\\bone_rest_compare.txt");
    }

    Log("[BONE] Rest pose captured. Hips rot=(%.3f,%.3f,%.3f,%.3f) "
        "pos=(%.3f,%.3f,%.3f)",
        g_boneRestRot[0].x, g_boneRestRot[0].y, g_boneRestRot[0].z,
        g_boneRestRot[0].w, g_boneRestHipsPos.x, g_boneRestHipsPos.y,
        g_boneRestHipsPos.z);
  }

  SafeSetAnimatorEnabled(
      false); 

  float time = g_bonePlayer->Tick();
  BoneFrame bf = g_boneAnim->GetFrame(time);

  auto qMul = [](const float a[4], const float b[4], float out[4]) {
    out[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
    out[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
    out[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
    out[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
  };

  int applied = 0;
  for (int b = 0; b < 55; b++) {
    void *boneT = SafeGetBoneTransform(b);
    if (!boneT)
      continue;

    float *delta = bf.bones[b];

    float gameRest[4] = {g_boneRestRot[b].x, g_boneRestRot[b].y,
                         g_boneRestRot[b].z, g_boneRestRot[b].w};
    float finalRot[4];
    qMul(gameRest, delta, finalRot);

    SafeSetLocalRotation(boneT,
                         {finalRot[0], finalRot[1], finalRot[2], finalRot[3]});
    applied++;
  }

  void *hipsT = SafeGetBoneTransform(0);
  if (hipsT) {
    Vec3 hipsPos;
    hipsPos.x = g_boneRestHipsPos.x + bf.hipsDeltaPos[0];
    hipsPos.y = g_boneRestHipsPos.y + bf.hipsDeltaPos[1];
    hipsPos.z = g_boneRestHipsPos.z + bf.hipsDeltaPos[2];
    SafeSetLocalPosition(hipsT, hipsPos);
  }

  static int s_logCount = 0;
  if (s_logCount < 3) {
    BoneFrame f0 = g_boneAnim->GetFrame(0);
    Log("[BONE] t=%.2f applied=%d | Hips gameR(%.3f,%.3f,%.3f,%.3f) "
        "d(%.3f,%.3f,%.3f,%.3f)",
        time, applied, g_boneRestRot[0].x, g_boneRestRot[0].y,
        g_boneRestRot[0].z, g_boneRestRot[0].w, f0.bones[0][0], f0.bones[0][1],
        f0.bones[0][2], f0.bones[0][3]);
    Log("[BONE] LeftUpperArm[14] gameR(%.3f,%.3f,%.3f,%.3f) "
        "d(%.3f,%.3f,%.3f,%.3f)",
        g_boneRestRot[14].x, g_boneRestRot[14].y, g_boneRestRot[14].z,
        g_boneRestRot[14].w, f0.bones[14][0], f0.bones[14][1], f0.bones[14][2],
        f0.bones[14][3]);
    s_logCount++;
  }
}

