#include "pikmin_game.h"

#include <malloc.h>
#include <unistd.h>
#include <maxmod9.h>

#include "debug/draw.h"
#include "debug/flags.h"
#include "debug/profiler.h"
#include "render/multipass_renderer.h"
#include "dsgx.h"
#include "level_loader.h"
#include "file_utils.h"
#include "soundbank.h"

// External libnds memory management variables, for debugging
extern u8 *fake_heap_end;   // current heap start
extern u8 *fake_heap_start;   // current heap end 

using pikmin_ai::PikminState;
using pikmin_ai::PikminType;
using captain_ai::CaptainState;
using onion_ai::OnionState;
using posy_ai::PosyState;
using fire_spout_ai::FireSpoutState;
using health_ai::HealthState;
using static_ai::StaticState;
using treasure_ai::TreasureState;

using numeric_types::literals::operator"" _f;
using numeric_types::literals::operator"" _brad;
using numeric_types::Brads;
using numeric_types::fixed;

int PikminSave::PikminCount(PikminType type) {
  // Note to self: *Probably* shouldn't do it this way
  return ((int*)this)[(int)type - 1];
}

void PikminSave::AddPikmin(PikminType type, int num_pikmin) {
  // Note to self: here too. Seriously. Shame on you.
  ((int*)this)[(int)type - 1] += num_pikmin;
}

int PikminGame::TotalPikmin() {
  int total =
    current_save_data_.PikminCount(PikminType::kRedPikmin) +
    current_save_data_.PikminCount(PikminType::kYellowPikmin) +
    current_save_data_.PikminCount(PikminType::kBluePikmin) +
    PikminInField();
  return total;
}

PikminGame::PikminGame(MultipassRenderer& renderer) : renderer_{renderer} {
  camera_.game = this;
  ui_.game = this;
  ui_.debug_state.game = this;

  // Setup initial debug flags
  debug::RegisterFlag("Draw Effects Layer");
  debug::RegisterFlag("Draw Physics Circles");
  debug::RegisterFlag("Draw Renderer Circles");
  debug::RegisterFlag("Skip VBlank");
  debug::RegisterFlag("Render First Pass Only");

  debug::RegisterWorld(&world_);
  debug::RegisterRenderer(&renderer_);

  tAI = debug::Profiler::RegisterTopic("Game: AI / Logic");
  tPhysicsUpdate = debug::Profiler::RegisterTopic("Game: Physics");

  ai_profilers_.emplace("Pikmin", debug::AiProfiler());
}

PikminGame::~PikminGame() {
}

void PikminGame::InitSound(std::string soundbank_filename) {
  soundbank_ = LoadEntireFile(soundbank_filename);
  mmInitDefaultMem((u8*)soundbank_.data());
  mmLoadEffect(SFX_FOOTSTEP_HARD);
}

MultipassRenderer& PikminGame::renderer() {
  return renderer_;
}

physics::World& PikminGame::world() {
  return world_;
}

camera_ai::CameraState& PikminGame::camera() {
  return camera_;
}

VramAllocator<Texture>* PikminGame::TextureAllocator() {
  return &texture_allocator_;
}

VramAllocator<TexturePalette>* PikminGame::TexturePaletteAllocator() {
  return &texture_palette_allocator_;
}

VramAllocator<Sprite>* PikminGame::SpriteAllocator() {
  return &sprite_allocator_;
}

DsgxAllocator* PikminGame::ActorAllocator() {
  return &dsgx_allocator_;
}

Drawable* PikminGame::allocate_entity() {
  if (entities_.size() >= kMaxEntities) {
    return nullptr;
  }
  entities_.push_back(new Drawable());
  renderer_.AddEntity(entities_.back());
  return entities_.back();
}

unsigned int PikminGame::CurrentFrame() {
  return current_frame_;
}

template <typename StateType, unsigned int size>
Handle PikminGame::SpawnObject(std::array<StateType, size>& object_list, int type) {
  unsigned int slot = 0;
  while (slot < object_list.size() and object_list[slot].active) {
    slot++;
  }
  if (slot >= object_list.size()) {
    debug::Log("Failed to spawn object type " + std::to_string(type) + ", list size of " + std::to_string(object_list.size()) + " is full!");
    return Handle();
  }

  StateType& new_object = object_list[slot];

  // clear the slot to defaults, then set the ID based on the slot chosen
  new_object = StateType();
  new_object.handle.id = slot;
  new_object.handle.generation = current_generation_;
  new_object.handle.type = type;

  new_object.active = true;

  new_object.entity = allocate_entity();
  new_object.body = world_.AllocateBody(new_object.handle);
  new_object.game = this;

  const bool too_many_objects = new_object.entity == nullptr;
  if (too_many_objects) {
    debug::Log("Failed to spawn object type " + std::to_string(type) + ", entity list is full!");
    return Handle();
  }

  debug::Log("Spawned new object of type " + std::to_string(type) + " with ID " + std::to_string(slot) + "");
  return new_object.handle;
}

template <typename StateType, unsigned int size>
void PikminGame::RemoveObject(Handle handle, std::array<StateType, size>& object_list) {
  if (handle.type == 0) {
    //debug::Log("Refusing to remove handle with type 0 (kNone)");
    return;
  }
  if (handle.id < object_list.size()) {
    auto& object_to_delete = object_list[handle.id];
    if (!object_to_delete.active) {
      debug::Log("Refusing to remove inactive object, type: " + std::to_string(handle.type) + ", id: " + std::to_string(handle.id));
      return;
    }
    if (handle.Matches(object_to_delete.handle)) {
      debug::Log("Removed object type " + std::to_string(handle.type) + " with ID " + std::to_string(handle.id));
      // similar to cleanup object, again minus the state allocation
      renderer_.RemoveEntity(object_to_delete.entity);
      entities_.remove(object_to_delete.entity);
      delete object_to_delete.entity;
      if (object_to_delete.body) {
        world_.FreeBody(object_to_delete.body);
      }
      current_generation_++;
      object_to_delete = StateType{};
      object_to_delete.active = false;
    } else {
      debug::Log("Failed to remove object type " + std::to_string(handle.type) + ", handle does not match!");
      // Invalid handle! Stale, possibly?
    }
  } else {
    debug::Log("Failed to remove object type " + std::to_string(handle.type) + ", id " + std::to_string(handle.id) + " is invalid!");
    // Invalid ID!
  }
}


Handle PikminGame::SpawnCaptain() {
  CaptainState* captain = RetrieveCaptain(SpawnObject(captains, PikminGame::kCaptain));
  if (captain) {
    captain->cursor = allocate_entity();
    captain->whistle = allocate_entity();
    captain->squad.captain = captain;
  } else {
    // How did we fail here?
    debug::Log("Failed to spawn captain!");
  }
  return captain->handle;
}

void PikminGame::RemoveCaptain(Handle handle) {
  CaptainState* captain = RetrieveCaptain(handle);
  if (captain) {
    renderer_.RemoveEntity(captain->cursor);
    entities_.remove(captain->cursor);
    delete captain->cursor;

    renderer_.RemoveEntity(captain->whistle);
    entities_.remove(captain->whistle);
    delete captain->whistle;

    RemoveObject(handle, captains);
  }
}

Handle PikminGame::SpawnHealth() {
  for (unsigned int i = 0; i < health.size(); i++) {
    if (!(health[i].active)) {
      health[i] = HealthState();
      health[i].handle.id = i;
      health[i].handle.generation = current_generation_;
      health[i].handle.type = PikminGame::kHealth;
      health[i].active = true;
      return health[i].handle;
    }
  }
  return Handle();
}

HealthState* PikminGame::RetrieveHealth(Handle handle) {
  if (handle.id < health.size()) {
    auto object = &health[handle.id];
    if (object->active and object->handle.Matches(handle)) {
      return object;
    }
  }
  return nullptr;
}

void PikminGame::RemoveHealth(Handle handle) {
  HealthState* object = RetrieveHealth(handle);
  if (object) {
    object->active = false;
    object->handle = Handle();
  }
}

CaptainState* PikminGame::RetrieveCaptain(Handle handle) {
  if (handle.id < captains.size()) {
    auto object = &captains[handle.id];
    if (object->active and object->handle.Matches(handle)) {
      return object;
    }
  }
  return nullptr;
}

FireSpoutState* PikminGame::RetrieveFireSpout(Handle handle) {
  if (handle.id < fire_spouts.size()) {
    auto object = &fire_spouts[handle.id];
    if (object->active and object->handle.Matches(handle)) {
      return object;
    }
  }
  return nullptr;
}

OnionState* PikminGame::RetrieveOnion(Handle handle) {
  if (handle.id < onions.size()) {
    auto object = &onions[handle.id];
    if (object->active and object->handle.Matches(handle)) {
      return object;
    }
  }
  return nullptr;
}

PikminState* PikminGame::RetrievePikmin(Handle handle) {
  if (handle.id < pikmin.size()) {
    auto object = &pikmin[handle.id];
    if (object->active and object->handle.Matches(handle)) {
      return object;
    }
  }
  return nullptr;
}

PosyState* PikminGame::RetrievePelletPosy(Handle handle) {
  if (handle.id < posies.size()) {
    auto object = &posies[handle.id];
    if (object->active and object->handle.Matches(handle)) {
      return object;
    }
  }
  return nullptr;
}

StaticState* PikminGame::RetrieveStatic(Handle handle) {
  if (handle.id < statics.size()) {
    auto object = &statics[handle.id];
    if (object->active and object->handle.Matches(handle)) {
      return object;
    }
  }
  return nullptr;
}

TreasureState* PikminGame::RetrieveTreasure(Handle handle) {
  if (handle.id < treasures.size()) {
    auto object = &treasures[handle.id];
    if (object->active and object->handle.Matches(handle)) {
      return object;
    }
  }
  return nullptr;
}

// Generic object return given a handle, for a few cases where we
// need to access the object in a general way, and don't need any
// of the specialized variables yet
PikminGameState* PikminGame::Retrieve(Handle handle) {
  switch (handle.type) {
    case PikminGame::kCaptain:    return (PikminGameState*)RetrieveCaptain(handle);
    case PikminGame::kHealth:     return (PikminGameState*)RetrieveHealth(handle);
    case PikminGame::kPikmin:     return (PikminGameState*)RetrievePikmin(handle);
    case PikminGame::kPelletPosy: return (PikminGameState*)RetrievePelletPosy(handle);
    case PikminGame::kStatic:     return (PikminGameState*)RetrieveStatic(handle);
    case PikminGame::kTreasure:   return (PikminGameState*)RetrieveTreasure(handle);
    case PikminGame::kFireSpout:  return (PikminGameState*)RetrieveFireSpout(handle);
    case PikminGame::kOnion:      return (PikminGameState*)RetrieveOnion(handle);
    default: return nullptr;
  };
}

void PikminGame::PauseGame() {
  paused_ = true;
  renderer_.PauseEngine();
}

void PikminGame::UnpauseGame() {
  paused_ = false;
  renderer_.UnpauseEngine();
}

bool PikminGame::IsPaused() {
  return paused_;
}

void PikminGame::RemoveEverything() {
  // Run a standard remove, then re-initialize all objects in inactive mode
  for (auto i = captains.begin(); i != captains.end(); i++) {
    RemoveCaptain(i->handle);
  }
  for (unsigned int i = 0; i < pikmin.size(); i++) {
    RemoveObject(pikmin[i].handle, pikmin);
  }

  for (unsigned int i = 0; i < onions.size(); i++) {
    RemoveObject(onions[i].handle, onions); 
  }

  for (unsigned int i = 0; i < posies.size(); i++) {
    RemoveObject(posies[i].handle, posies); 
  }

  for (unsigned int i = 0; i < fire_spouts.size(); i++) {
    RemoveObject(fire_spouts[i].handle, fire_spouts); 
  }

  for (unsigned int i = 0; i < statics.size(); i++) {
    RemoveObject(statics[i].handle, statics); 
  }

  for (unsigned int i = 0; i < treasures.size(); i++) {
    RemoveObject(treasures[i].handle, treasures); 
  }

  world_.ResetWorld();
}

void PikminGame::LoadLevel(std::string filename) {
  // Clean the slate!
  RemoveEverything();
  level_loader::LoadLevel(*this, filename);

  // For now, always spawn a captain!
  if (!captains[0].active) {
    Spawn("Captain", Vec3{0_f,0_f,0_f});
  }

  // Grab our captain (if one exists) and make sure the camera is following him
  CaptainState* captain = RetrieveCaptain(ActiveCaptain());
  if (captain) {
    camera().follow_captain = captain->handle;
  }
}

void PikminGame::RunAi() {
  debug::Profiler::StartTopic(tAI);
  for (auto i = captains.begin(); i != captains.end(); i++) {
    if (i->active) {
      captain_ai::machine.RunLogic(*i);
      squad_ai::machine.RunLogic((*i).squad);
      i->Update();
      i->whistle->set_position(i->whistle_body->position);
      i->cursor->set_position(i->cursor_body->position);
      if (i->dead) {
        RemoveCaptain(i->handle);
      }
    }
  }

  ai_profilers_["Pikmin"].ClearTimingData();
  for (unsigned int i = 0; i < pikmin.size(); i++) {
    if (pikmin[i].active) {
      //pikmin_ai::machine.RunLogic(pikmin[i], &ai_profilers_["Pikmin"]);
      pikmin_ai::machine.RunLogic(pikmin[i]);
      pikmin[i].Update();
      if (pikmin[i].dead) {
        RemoveObject(pikmin[i].handle, pikmin);
      }
    }
  }

  for (unsigned int o = 0; o < onions.size(); o++) {
    onion_ai::machine.RunLogic(onions[o]);
    onions[o].Update();
  }

  for (unsigned int p = 0; p < posies.size(); p++) {
    if (posies[p].active) {
      posy_ai::machine.RunLogic(posies[p]);
      posies[p].Update();
      if (posies[p].dead) {
        RemoveObject(posies[p].handle, posies);
      }
    }
  }

  for (unsigned int f = 0; f < fire_spouts.size(); f++) {
    if (fire_spouts[f].active) {
      fire_spout_ai::machine.RunLogic(fire_spouts[f]);
      fire_spouts[f].Update();
      if (fire_spouts[f].dead) {
        RemoveObject(fire_spouts[f].handle, fire_spouts);
      }
    }
  }

  for (unsigned int t = 0; t < treasures.size(); t++) {
    if (treasures[t].active) {
      treasure_ai::machine.RunLogic(treasures[t]);
      treasures[t].Update();
      if (treasures[t].dead) {
        RemoveObject<TreasureState>(treasures[t].handle, treasures);
      }
    }
  }

  camera_ai::machine.RunLogic(camera_);

  debug::Profiler::EndTopic(tAI);
}

void PikminGame::Step() {
  current_step_++;
  if (current_step_ % 2 == 0) {
    // On even frames, run AI
    scanKeys();
    ui::machine.RunLogic(ui_);

    if (IsPaused()) {
      return;
    }

    RunAi();
    current_frame_++;
  } else {
    // On odd frames, run the World, and update the engine bits
    renderer_.Update();

    if (!IsPaused()) {
      debug::Profiler::StartTopic(tPhysicsUpdate);
      world_.Update();
      debug::Profiler::EndTopic(tPhysicsUpdate);

      // Update some debug details about the world
      DebugDictionary().Set("Physics: Bodies Overlapping: ", world().BodiesOverlapping());
      DebugDictionary().Set("Physics: Total Collisions: ", world().TotalCollisions());
    }
  }

  // Update basic system level debug info:
  struct mallinfo mi = mallinfo();
  DebugDictionary().Set("Program Size: ", ((int)fake_heap_start) - 0x02000000);
  DebugDictionary().Set("Heap Used: ", mi.uordblks);
  DebugDictionary().Set("Heap Free: ", mi.fordblks + (fake_heap_end - (u8*)sbrk(0)));

  DebugDictionary().Set("DSGX Size: ", DsgxAllocator::kPoolSize);
  DebugDictionary().Set("DSGX Used: ", ActorAllocator()->Used());
  DebugDictionary().Set("DSGX Free: ", ActorAllocator()->Free());
}

Handle PikminGame::ActiveCaptain() {
  return captains[0].handle;
}

OnionState* PikminGame::Onion(PikminType type) {
  for (unsigned int i = 0; i < onions.size(); i++) {
    if (onions[i].pikmin_type == type) {
      return &onions[i];
    }
  }
  return nullptr;
}

int PikminGame::PikminInField() {
  int count = 0;
  for (int slot = 0; slot < 100; slot++) {
    if (pikmin[slot].active) {
      count++;
    }
  }
  return count;
}

PikminSave* PikminGame::CurrentSaveData() {
  return &current_save_data_;
}

std::array<PikminState, 100>& PikminGame::PikminList() {
  return pikmin;
}

const std::map<std::string, std::function<PikminGameState*(PikminGame*)>> PikminGame::spawn_ = {
  {"Captain", [](PikminGame* game) -> PikminGameState* {
    return game->RetrieveCaptain(game->SpawnCaptain());
  }},
  {"Enemy:PelletPosy", [](PikminGame* game) -> PikminGameState* {
    return game->RetrievePelletPosy(game->SpawnObject(game->posies, PikminGame::kPelletPosy));
  }},
  {"Pikmin:Red", [](PikminGame* game) -> PikminGameState* {
    auto pikmin = game->RetrievePikmin(game->SpawnObject(game->pikmin, PikminGame::kPikmin));
    if (pikmin) {
      pikmin->type = PikminType::kRedPikmin;
    }
    return pikmin;
  }},
  {"Pikmin:Yellow", [](PikminGame* game) -> PikminGameState* {
    auto pikmin = game->RetrievePikmin(game->SpawnObject(game->pikmin, PikminGame::kPikmin));
    if (pikmin) {
      pikmin->type = PikminType::kYellowPikmin;
    }
    return pikmin;
  }},
  {"Pikmin:Blue", [](PikminGame* game) -> PikminGameState* {
    auto pikmin = game->RetrievePikmin(game->SpawnObject(game->pikmin, PikminGame::kPikmin));
    if (pikmin) {
      pikmin->type = PikminType::kBluePikmin;
    }
    return pikmin;
  }},
  {"Onion:Red", [](PikminGame* game) -> PikminGameState* {
    auto onion = game->RetrieveOnion(game->SpawnObject(game->onions, PikminGame::kOnion));
    if (onion) {
      onion->pikmin_type = PikminType::kRedPikmin;
    }
    return onion;
  }},
  {"Onion:Yellow", [](PikminGame* game) -> PikminGameState* {
    auto onion = game->RetrieveOnion(game->SpawnObject(game->onions, PikminGame::kOnion));
    if (onion) {
      onion->pikmin_type = PikminType::kYellowPikmin;
    }
    return onion;
  }},
  {"Onion:Blue", [](PikminGame* game) -> PikminGameState* {
    auto onion = game->RetrieveOnion(game->SpawnObject(game->onions, PikminGame::kOnion));
    if (onion) {
      onion->pikmin_type = PikminType::kBluePikmin;
    }
    return onion;
  }},
  {"Hazard:FireSpout", [](PikminGame* game) -> PikminGameState* {
    return game->RetrieveFireSpout(game->SpawnObject(game->fire_spouts, PikminGame::kFireSpout));
  }},
  {"Static", [](PikminGame* game) -> PikminGameState* {
    auto static_object = game->RetrieveStatic(game->SpawnObject(game->statics, PikminGame::kStatic));
    if (static_object) {
      // Statics don't actually need a physics body, so get rid of that here
      game->world_.FreeBody(static_object->body);
      static_object->body = nullptr;
    }
    return static_object;

  }},
  {"Corpse:Pellet:Red", [](PikminGame* game) -> PikminGameState* {
    auto treasure = game->RetrieveTreasure(game->SpawnObject(game->treasures, PikminGame::kTreasure));
    if (treasure) {
      treasure->pikmin_affinity = PikminType::kRedPikmin;
      treasure->entity->set_actor(treasure->game->ActorAllocator()->Retrieve("pellet"));
      treasure->body->radius = 2_f;

      treasure->weight = 1;
      treasure->carry_slots = 2;
      treasure->pikmin_seeds = 2;
    }
    return treasure;
  }},
};

std::pair<PikminGame::SpawnMap::const_iterator, PikminGame::SpawnMap::const_iterator> PikminGame::SpawnNames() {
  return std::make_pair(spawn_.begin(), spawn_.end());
}

Handle PikminGame::Spawn(const std::string& name, Vec3 position, Rotation rotation) {
  PikminGameState* object = spawn_.at(name)(this);
  object->set_position(position);
  object->entity->set_rotation(rotation);
  return object->handle;
}

debug::Dictionary& PikminGame::DebugDictionary() {
  return debug_dictionary_;
}

std::map<std::string, debug::AiProfiler>& PikminGame::DebugAiProfilers() {
  return ai_profilers_;
}
