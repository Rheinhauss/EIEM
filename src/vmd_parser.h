#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <windows.h>

struct VmdBoneKeyframe {
  std::string boneName;
  uint32_t frame = 0;
  float pos[3] = {};
  float rot[4] = {0, 0, 0, 1};
  uint8_t interp[64] = {};
};

struct VmdBoneTimeline {
  std::string boneName;
  std::vector<VmdBoneKeyframe> keys;
};

struct VmdMorphKeyframe {
  std::string morphName;
  uint32_t frame = 0;
  float weight = 0.0f;
};

struct VmdMorphTimeline {
  std::string morphName;
  std::vector<VmdMorphKeyframe> keys;

  float Sample(float frameF) const {
    if (keys.empty())
      return 0.0f;
    if (frameF <= (float)keys.front().frame)
      return keys.front().weight;
    if (frameF >= (float)keys.back().frame)
      return keys.back().weight;
    size_t lo = 0, hi = keys.size() - 1;
    while (lo + 1 < hi) {
      size_t mid = (lo + hi) / 2;
      if ((float)keys[mid].frame <= frameF)
        lo = mid;
      else
        hi = mid;
    }
    const float span = (float)(keys[hi].frame - keys[lo].frame);
    const float t = span > 0.0f ? (frameF - keys[lo].frame) / span : 0.0f;
    return keys[lo].weight + (keys[hi].weight - keys[lo].weight) * t;
  }
};

struct VmdCameraKeyframe {
  uint32_t frame = 0;
  float distance = 0.0f;
  float position[3] = {};
  float rotation[3] = {};
  uint8_t interp[24] = {};
  uint32_t fov = 45;
  uint8_t perspective = 0;
};

struct VmdFile {
  char signature[31] = {};
  char modelName[256] = {};
  uint32_t totalFrames = 0;
  uint32_t boneFrames = 0;
  uint32_t morphFrames = 0;
  uint32_t cameraFrames = 0;

  std::map<std::string, VmdBoneTimeline> boneTimelines;
  std::map<std::string, VmdMorphTimeline> morphTimelines;
  std::vector<VmdCameraKeyframe> cameraKeys;

  bool loaded = false;
  std::string error;
};

static std::string SjisToUtf8(const char *sjis, int maxLen) {
  int sjisLen = 0;
  while (sjisLen < maxLen && sjis[sjisLen] != '\0')
    ++sjisLen;
  if (sjisLen == 0)
    return "";

  const int wLen = MultiByteToWideChar(932, 0, sjis, sjisLen, nullptr, 0);
  if (wLen <= 0)
    return "";
  std::vector<wchar_t> wide((size_t)wLen);
  if (!MultiByteToWideChar(932, 0, sjis, sjisLen, wide.data(), wLen))
    return "";

  const int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wLen,
                                           nullptr, 0, nullptr, nullptr);
  if (utf8Len <= 0)
    return "";
  std::string result((size_t)utf8Len, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), wLen, &result[0], utf8Len,
                      nullptr, nullptr);
  return result;
}

struct VmdReader {
  FILE *file = nullptr;
  uint64_t size = 0;
  uint64_t offset = 0;

  bool Read(void *dst, size_t bytes) {
    if (!file || bytes > size - offset)
      return false;
    if (bytes && fread(dst, 1, bytes, file) != bytes)
      return false;
    offset += bytes;
    return true;
  }

  template <typename T> bool ReadOne(T &value) {
    return Read(&value, sizeof(T));
  }

  bool CanReadRecords(uint32_t count, uint64_t recordSize) const {
    return recordSize > 0 && (uint64_t)count <= (size - offset) / recordSize;
  }
};

template <typename T>
static void CompactVmdKeys(std::vector<T> &keys) {
  std::stable_sort(keys.begin(), keys.end(),
                   [](const T &a, const T &b) { return a.frame < b.frame; });
  size_t out = 0;
  for (size_t i = 0; i < keys.size(); ++i) {
    if (out && keys[out - 1].frame == keys[i].frame)
      keys[out - 1] = keys[i]; // VMD's last duplicate key wins.
    else
      keys[out++] = keys[i];
  }
  keys.resize(out);
}

static bool IsFiniteVmdFloat(float value) {
  return std::isfinite(value) != 0;
}

static VmdFile *LoadVmd(const char *path) {
  VmdFile *vmd = new VmdFile();
  FILE *file = fopen(path, "rb");
  if (!file) {
    vmd->error = "Cannot open file";
    return vmd;
  }

  _fseeki64(file, 0, SEEK_END);
  const __int64 fileSize = _ftelli64(file);
  _fseeki64(file, 0, SEEK_SET);
  if (fileSize < 54) {
    vmd->error = "File too small for VMD header";
    fclose(file);
    return vmd;
  }

  VmdReader reader{file, (uint64_t)fileSize, 0};
  char signature[30] = {};
  char modelName[20] = {};
  if (!reader.Read(signature, sizeof(signature)) ||
      !reader.Read(modelName, sizeof(modelName))) {
    vmd->error = "Truncated VMD header";
    fclose(file);
    return vmd;
  }
  memcpy(vmd->signature, signature, sizeof(signature));
  if (strncmp(signature, "Vocaloid Motion Data 0002", 25) != 0) {
    vmd->error = "Invalid VMD 0002 signature";
    fclose(file);
    return vmd;
  }
  const std::string modelUtf8 = SjisToUtf8(modelName, (int)sizeof(modelName));
  strncpy(vmd->modelName, modelUtf8.c_str(), sizeof(vmd->modelName) - 1);

  constexpr uint32_t kMaxKeys = 5000000;
  constexpr uint64_t kBoneRecordSize = 15 + 4 + 12 + 16 + 64;
  constexpr uint64_t kMorphRecordSize = 15 + 4 + 4;
  constexpr uint64_t kCameraRecordSize = 4 + 4 + 12 + 12 + 24 + 4 + 1;

  uint32_t boneKeyCount = 0;
  if (!reader.ReadOne(boneKeyCount) || boneKeyCount > kMaxKeys ||
      !reader.CanReadRecords(boneKeyCount, kBoneRecordSize)) {
    vmd->error = "Invalid or truncated bone keyframe section";
    fclose(file);
    return vmd;
  }
  for (uint32_t i = 0; i < boneKeyCount; ++i) {
    char name[15] = {};
    VmdBoneKeyframe key;
    if (!reader.Read(name, sizeof(name)) || !reader.ReadOne(key.frame) ||
        !reader.Read(key.pos, sizeof(key.pos)) ||
        !reader.Read(key.rot, sizeof(key.rot)) ||
        !reader.Read(key.interp, sizeof(key.interp))) {
      vmd->error = "Truncated bone keyframe at index " + std::to_string(i);
      fclose(file);
      return vmd;
    }
    bool finite = true;
    for (float value : key.pos)
      finite = finite && IsFiniteVmdFloat(value);
    for (float value : key.rot)
      finite = finite && IsFiniteVmdFloat(value);
    if (!finite) {
      vmd->error = "Non-finite bone keyframe at index " + std::to_string(i);
      fclose(file);
      return vmd;
    }
    const float qlen = sqrtf(key.rot[0] * key.rot[0] +
                             key.rot[1] * key.rot[1] +
                             key.rot[2] * key.rot[2] +
                             key.rot[3] * key.rot[3]);
    if (qlen < 1e-6f) {
      vmd->error = "Invalid zero-length bone quaternion at index " +
                   std::to_string(i);
      fclose(file);
      return vmd;
    }
    for (float &component : key.rot)
      component /= qlen;
    key.boneName = SjisToUtf8(name, (int)sizeof(name));
    auto &timeline = vmd->boneTimelines[key.boneName];
    timeline.boneName = key.boneName;
    timeline.keys.push_back(key);
    vmd->boneFrames = (std::max)(vmd->boneFrames, key.frame);
  }
  for (auto &entry : vmd->boneTimelines)
    CompactVmdKeys(entry.second.keys);

  uint32_t morphKeyCount = 0;
  if (!reader.ReadOne(morphKeyCount) || morphKeyCount > kMaxKeys ||
      !reader.CanReadRecords(morphKeyCount, kMorphRecordSize)) {
    vmd->error = "Invalid or truncated morph keyframe section";
    fclose(file);
    return vmd;
  }
  for (uint32_t i = 0; i < morphKeyCount; ++i) {
    char name[15] = {};
    VmdMorphKeyframe key;
    if (!reader.Read(name, sizeof(name)) || !reader.ReadOne(key.frame) ||
        !reader.ReadOne(key.weight)) {
      vmd->error = "Truncated morph keyframe at index " + std::to_string(i);
      fclose(file);
      return vmd;
    }
    if (!IsFiniteVmdFloat(key.weight)) {
      vmd->error = "Non-finite morph keyframe at index " + std::to_string(i);
      fclose(file);
      return vmd;
    }
    key.morphName = SjisToUtf8(name, (int)sizeof(name));
    auto &timeline = vmd->morphTimelines[key.morphName];
    timeline.morphName = key.morphName;
    timeline.keys.push_back(key);
    vmd->morphFrames = (std::max)(vmd->morphFrames, key.frame);
  }
  for (auto &entry : vmd->morphTimelines)
    CompactVmdKeys(entry.second.keys);

  // Older files may end after morphs. They are still valid motion VMDs.
  if (reader.offset < reader.size) {
    uint32_t cameraKeyCount = 0;
    if (!reader.ReadOne(cameraKeyCount) || cameraKeyCount > kMaxKeys ||
        !reader.CanReadRecords(cameraKeyCount, kCameraRecordSize)) {
      vmd->error = "Invalid or truncated camera keyframe section";
      fclose(file);
      return vmd;
    }
    vmd->cameraKeys.reserve(cameraKeyCount);
    for (uint32_t i = 0; i < cameraKeyCount; ++i) {
      VmdCameraKeyframe key;
      if (!reader.ReadOne(key.frame) || !reader.ReadOne(key.distance) ||
          !reader.Read(key.position, sizeof(key.position)) ||
          !reader.Read(key.rotation, sizeof(key.rotation)) ||
          !reader.Read(key.interp, sizeof(key.interp)) ||
          !reader.ReadOne(key.fov) || !reader.ReadOne(key.perspective)) {
        vmd->error = "Truncated camera keyframe at index " + std::to_string(i);
        fclose(file);
        return vmd;
      }
      bool finite = IsFiniteVmdFloat(key.distance);
      for (float value : key.position)
        finite = finite && IsFiniteVmdFloat(value);
      for (float value : key.rotation)
        finite = finite && IsFiniteVmdFloat(value);
      if (!finite) {
        vmd->error = "Non-finite camera keyframe at index " + std::to_string(i);
        fclose(file);
        return vmd;
      }
      vmd->cameraKeys.push_back(key);
      vmd->cameraFrames = (std::max)(vmd->cameraFrames, key.frame);
    }
    CompactVmdKeys(vmd->cameraKeys);
  }

  fclose(file);
  vmd->totalFrames = (std::max)(vmd->boneFrames,
      (std::max)(vmd->morphFrames, vmd->cameraFrames));
  vmd->loaded = true;
  return vmd;
}

static void FreeVmd(VmdFile *vmd) {
  delete vmd;
}

static void DumpVmd(const VmdFile *vmd, FILE *out) {
  if (!vmd || !out)
    return;
  fprintf(out, "=== VMD File Info ===\n");
  fprintf(out, "Signature: %.30s\n", vmd->signature);
  fprintf(out, "Model: %s\n", vmd->modelName);
  fprintf(out, "Loaded: %s\n", vmd->loaded ? "YES" : "NO");
  if (!vmd->loaded) {
    fprintf(out, "Error: %s\n", vmd->error.c_str());
    return;
  }
  fprintf(out, "Frames: total=%u bone=%u morph=%u camera=%u\n",
          vmd->totalFrames, vmd->boneFrames, vmd->morphFrames,
          vmd->cameraFrames);
  fprintf(out, "Bone timelines: %zu\n", vmd->boneTimelines.size());
  fprintf(out, "Morph timelines: %zu\n", vmd->morphTimelines.size());
  fprintf(out, "Camera keys: %zu\n\n", vmd->cameraKeys.size());
  for (const auto &entry : vmd->boneTimelines)
    fprintf(out, "  [BONE] %s: %zu keys\n", entry.first.c_str(),
            entry.second.keys.size());
  for (const auto &entry : vmd->morphTimelines)
    fprintf(out, "  [MORPH] %s: %zu keys\n", entry.first.c_str(),
            entry.second.keys.size());
}
