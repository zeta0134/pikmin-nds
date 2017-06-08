#ifndef MULTIPASS_RENDERER_H
#define MULTIPASS_RENDERER_H

#include <list>
#include <map>
#include <queue>
#include <string>

#include "debug/profiler.h"
#include "drawable.h"

struct EntityContainer {
  template <typename FixedT, int FixedF>
  using Fixed = numeric_types::Fixed<FixedT, FixedF>;

  Drawable* entity;
  Fixed<s32, 12> near_z;
  Fixed<s32, 12> far_z;
  bool operator<(const EntityContainer& other) const {
    return far_z < other.far_z;
  }
};

class MultipassRenderer {
 public:
  MultipassRenderer();
  void DrawEntity(Drawable entity);

  void Update();
  void Draw();

  void AddEntity(Drawable* entity);
  void RemoveEntity(Drawable* entity);

  void PauseEngine();
  void UnpauseEngine();
  bool IsPaused();

  void SetCamera(Vec3 position, Vec3 subject, numeric_types::Brads fov);

  debug::Profiler& DebugProfiler();

  void EnableEffectsLayer(bool enabled);
  void DebugCircles();

 private:
  bool paused_ = false;
  template <typename FixedT, int FixedF>
  using Fixed = numeric_types::Fixed<FixedT, FixedF>;

  void GatherDrawList();
  void ClearDrawList();
  void SetVRAMforPass(int pass);
  void DrawClearPlane();
  void BailAndResetFrame();

  void CacheCamera();
  void ApplyCameraTransform();

  void InitFrame();
  void GatherPassList();
  bool ProgressMadeThisPass(unsigned int initial_length);
  void SetupDividingPlane();
  bool ValidateDividingPlane();
  void DrawPassList();
  bool LastPass();
  void DrawEffects();

  void WaitForVBlank();

  std::priority_queue<EntityContainer> draw_list_;

  std::list<Drawable*> entities_;
  std::vector<EntityContainer> overlap_list_;
  std::vector<EntityContainer> pass_list_;

  int current_pass_{0};

  int old_keys_;
  int keys_;

  Fixed<s32, 12> near_plane_;
  Fixed<s32, 12> far_plane_;

  Vec3 current_camera_position_;
  Vec3 current_camera_subject_;
  numeric_types::Brads current_camera_fov_;

  Vec3 cached_camera_position_;
  Vec3 cached_camera_subject_;
  numeric_types::Brads cached_camera_fov_;

  unsigned int frame_counter_{0};

  bool effects_enabled{false};
  bool effects_drawn{false};

  debug::Profiler debug_profiler_;

  // Debug Topics
  int tEntityUpdate;
  int tParticleUpdate;
  int tParticleDraw;
  int tFrameInit;
  int tPassInit;
  int tIdle;
  std::vector<int> tPassUpdate;
};

#endif  // MULTIPASS_ENGINE_H
