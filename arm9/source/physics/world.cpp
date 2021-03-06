#include "world.h"

#include "debug/messages.h"
#include "debug/profiler.h"
#include "debug/utilities.h"
#include "body.h"
#include "numeric_types.h"
#include "vector.h"

#include "stdlib.h"

using physics::World;
using physics::Body;
using physics::BodyHandle;
using numeric_types::fixed;
using numeric_types::literals::operator"" _f;

World::World() {
  tMoveBodies =    debug::Profiler::RegisterTopic("Physics: Move Bodies");
  tCollideBodies = debug::Profiler::RegisterTopic("Physics: Collide Bodies");
  tCollideWorld =  debug::Profiler::RegisterTopic("Physics: Collide World");

  tAA = debug::Profiler::RegisterTopic("Physics: Bodies: A vs A");
  tAP = debug::Profiler::RegisterTopic("Physics: Bodies: A vs P");
  tPP = debug::Profiler::RegisterTopic("Physics: Bodies: P vs P");
}

World::~World() {
}

Body* World::AllocateBody(Handle owner) {
  // This is a fairly naive implementation.

  // Note: A return value of 0 (Null) indicates failure.
  for (int i = 0; i < MAX_PHYSICS_BODIES; i++) {
    if (bodies_[i].active == 0) {
      Body default_zeroed = {};
      bodies_[i] = default_zeroed;

      bodies_[i].touching_ground = 0;
      bodies_[i].is_sensor = 0;
      bodies_[i].collides_with_bodies = 1;
      bodies_[i].collides_with_level = 1;
      bodies_[i].ignores_walls = 0;
      bodies_[i].is_movable = 0;
      bodies_[i].is_pikmin = 0;
      bodies_[i].affected_by_gravity = 1;
      bodies_[i].active = 1;
      bodies_[i].is_very_important = 0;

      bodies_[i].owner = owner;
      bodies_[i].generation = current_generation_;
      rebuild_index_ = true;

      // Old-style handle, kept here for compatability reasons.
      // TODO: Remove this when reliance on this handle is refactored out.
      BodyHandle handle;
      handle.body = &bodies_[i];
      handle.generation = current_generation_;

      // New-style handle, used for safer references to Physics bodies by
      // non-owners.
      bodies_[i].handle.id = i;
      bodies_[i].handle.generation = current_generation_;
      bodies_[i].handle.type = World::kBody;

      // Clear out the neighbor list for newly created bodies, to avoid weirdness
      for (int n = 0; n < MAX_PHYSICS_NEIGHBORS; n++) {
        bodies_[i].neighbors[n].handle.generation = -1;
      }

      return &bodies_[i];
    }
  }
  return nullptr;
}

void World::FreeBody(Body* body) {
  if (body) {
    body->owner = Handle{};
    body->active = 0;
    body->handle.type = World::kNone;
    rebuild_index_ = true;
    current_generation_++;
  }
}

Body* World::RetrieveBody(Handle handle) {
  if (handle.id < MAX_PHYSICS_BODIES) {
    Body* body = &bodies_[handle.id];
    if (body->active and body->handle.Matches(handle)) {
      return body;
    }
  }
  return nullptr;
}

void World::ResetWorld() {
  for (int i = 0; i < MAX_PHYSICS_BODIES; i++) {
    FreeBody(&bodies_[i]);
  }
  RebuildIndex();
}

void World::Wake(Body* body) {
  body->active = 1;
  rebuild_index_ = true;
}

void World::Sleep(Body* body) {
  body->active = 0;
  rebuild_index_ = true;
}

void World::RebuildIndex() {
  active_bodies_ = 0;
  active_pikmin_ = 0;
  important_bodies_ = 0;
  for (int i = 0; i < MAX_PHYSICS_BODIES; i++) {
    if (bodies_[i].active) {
      if (bodies_[i].is_very_important) {
        important_[important_bodies_++] = i;
      } else {
        if (bodies_[i].is_pikmin) {
          pikmin_[active_pikmin_++] = i;
        } else {
          active_[active_bodies_++] = i;
        }
      }
    }
  }
  rebuild_index_ = false;
}

bool World::BodiesOverlap(Body& a, Body& b) {
  if (&a == &b) {
    return false; // Don't collide with yourself.
  }
  bodies_overlapping_++;
  //Check to see if the circles overlap on the XZ plane
  Vec2 axz = Vec2{a.position.x, a.position.z};
  Vec2 bxz = Vec2{b.position.x, b.position.z};
  auto distance2 = (axz - bxz).Length2();
  auto sum = a.radius + b.radius;
  auto radius2 = sum * sum;
  if (distance2 < radius2) {
    //Check to see if their Y values are overlapping also
    if (a.position.y + a.height >= b.position.y) {
      if (b.position.y + b.height >= a.position.y) {
        total_collisions_++;
        return true;
      }
    }
  }
  return false;
}

void World::ResolveCollision(Body& a, Body& b) {
  // If either body is a sensor, then no collision response is
  // performed (objects pass right through) so we bail early
  if (a.is_sensor or b.is_sensor) {
    return;
  }
  // One of the bodies must be able to respond to collisions
  if (a.is_movable || b.is_movable) {
    auto a_direction = (a.position - b.position);
    // only resolve on the XZ plane
    a_direction.y = 0_f;
    auto distance = a_direction.Length();

    if (distance == 0_f) {
      //can't correctly resolve this collision, so pick a direction at random
      //to get these objects apart
      //return;
      distance = 1.0_f;
      a_direction.x = 1.0_f;
      a_direction.z = 1.0_f;
    }
    if (a.is_movable and (!(b.is_pikmin) or a.is_pikmin)) {

      // multiply, so that we move exactly the distance required to undo the
      // overlap between these objects
      // a_direction = a_direction.Normalize();
      auto offset = (a.radius + b.radius) - distance;
      if (offset > (b.radius / 8_f)) {
        offset = b.radius / 8_f;
      }

      a_direction *= offset;
      a_direction *= 1_f / distance;

      a.position = a.position + a_direction;
    }
    if (b.is_movable and (!(a.is_pikmin) or b.is_pikmin)) {
      auto b_direction = (b.position - a.position);

      // perform this resolution only on the XZ plane
      b_direction.y = 0_f;

      // multiply, so that we move exactly the distance required to undo the
      // overlap between these objects
      //b_direction = b_direction.Normalize();
      auto offset = (a.radius + b.radius) - distance;
      if (offset > (a.radius / 8_f)) {
        offset = a.radius / 8_f;
      }

      b_direction *= offset;
      b_direction *= 1_f / distance;

      b.position = b.position + b_direction;
    }
  }
}

void World::PrepareBody(Body& body) {
  //set the old position (used later for comparison)
  body.old_position = body.position;
  body.old_radius = body.radius;

  //clear sensor results for this run
  body.result_groups = 0;
  body.num_results = 0;
}

void World::MoveBody(Body& body) {
  body.position += body.velocity;
  body.velocity += body.acceleration;

  // Gravity!
  if (body.affected_by_gravity) {
    body.velocity.y -= GRAVITY_CONSTANT;
  }
}

void World::MoveBodies() {
  for (int i = 0; i < active_bodies_; i++) {
    // First, make sure this is an active body
    PrepareBody(bodies_[active_[i]]);
    MoveBody(bodies_[active_[i]]);
  }
  for (int i = 0; i < active_pikmin_; i++) {
    // First, make sure this is an active body
    PrepareBody(bodies_[pikmin_[i]]);
    MoveBody(bodies_[pikmin_[i]]);
  }
}

void World::CollideObjectWithObject(Body& A, Body& B) {
  const bool a_senses_b = B.is_sensor and
      (B.collision_group & A.sensor_groups);
  const bool b_senses_a = A.is_sensor and
      (A.collision_group & B.sensor_groups);

  const bool a_pushes_b = A.collides_with_bodies and not B.is_sensor;
  const bool b_pushes_a = B.collides_with_bodies and not A.is_sensor;

  if (a_senses_b or b_senses_a or a_pushes_b or b_pushes_a) {
    if (BodiesOverlap(A, B)) {
      ResolveCollision(A, B);

      //if A is a sensor that B cares about
      if (A.collision_group & B.sensor_groups) {
        B.result_groups = B.result_groups | A.collision_group;
        if (B.num_results < 8) {
          B.collision_results[B.num_results++] = {&A, A.collision_group};
        }
      }
      //if B is a sensor that A cares about
      if (B.collision_group & A.sensor_groups) {
        A.result_groups = A.result_groups | B.collision_group;
        if (A.num_results < 8) {
          A.collision_results[A.num_results++] = {&B, B.collision_group};
        }
      }
    }
  }
}

void World::CollidePikminWithObject(Body& P, Body& A) {
  if ((A.is_sensor and (A.collision_group & P.sensor_groups)) or
      (not A.is_sensor)) {
    if (BodiesOverlap(A, P)) {
      ResolveCollision(A, P);
      if (A.collision_group & P.sensor_groups) {
        P.result_groups = P.result_groups | A.collision_group;
        if (P.num_results < 8) {
          P.collision_results[P.num_results++] = {&A, A.collision_group};
        }
      }
    }
  }
}

void World::CollidePikminWithPikmin(Body& pikmin1, Body& pikmin2) {
  if ((int)pikmin1.position.x == (int)pikmin2.position.x and
      (int)pikmin1.position.z == (int)pikmin2.position.z) {
    if (BodiesOverlap(pikmin1, pikmin2)) {
      ResolveCollision(pikmin1, pikmin2);
    }
  }
}

void World::AddNeighborToObject(Body& object, Body& new_neighbor) {
  // calculate the distance *squared* between this object and the current
  // candidate for neighbor status
  fixed distance = (object.position - new_neighbor.position).Length2();
  // Adjust for the radius on both sides (we want the distance between the
  // nearest potential edge collision)
  //distance -= (object.radius + new_neighbor.radius);
  // Loop through the list of neighbors and find the farthest away object
  // that this object is closer than (if any). If we find an inactive slot,
  // we bail early; it wins.
  fixed biggest_valid_distance = -1000_f;
  int farthest_index = -1;
  for (int i = 0; i < MAX_PHYSICS_NEIGHBORS; i++) {
    if (object.neighbors[i].handle.IsValid()) {
      auto current_body = object.neighbors[i].handle.body;
      // If this object already exists in the list
      if (&new_neighbor == current_body) {
        // Update this distance and STOP.
        object.neighbors[i].distance = distance;
        return;
      }
      // Are we closer than this object, and is this object the farthest
      // object we could replace?
      if (object.neighbors[i].distance > distance and
          object.neighbors[i].distance > biggest_valid_distance) {
          farthest_index = i;
          biggest_valid_distance = distance;
      }
    } else {
      // Note: we don't short-circuit here because if we did, we might miss
      // a chance to find ourselves in the list. We need to look at every
      // neighbor to avoid that scenario.
      farthest_index = i;
      biggest_valid_distance = 1000_f;
    }
  }
  if (farthest_index > -1) {
    object.neighbors[farthest_index].handle = new_neighbor.GetHandle();
    object.neighbors[farthest_index].distance = distance;
  }
}

void World::UpdateNeighbors() {
  // It's a bit weird that we do this check first, non? But it handles the
  // case where bodies were deleted, moving the pointer off the end of the list.
  if (current_neighbor_ >= active_bodies_) {
    current_neighbor_ = 0;
  }

  Body& new_neighbor = bodies_[active_[current_neighbor_]];
  for (int a = 0; a < active_bodies_; a++) {
    Body& current_object = bodies_[active_[a]];
    if (&new_neighbor != &current_object) {
      AddNeighborToObject(current_object, new_neighbor);
    }
  }

  for (int p = 0; p < active_pikmin_; p++) {
    Body& current_object = bodies_[pikmin_[p]];
    AddNeighborToObject(current_object, new_neighbor);
  }
  current_neighbor_++;
}

void World::ProcessCollision() {
  UpdateNeighbors();

  debug::Profiler::StartTopic(tAA);
  for (int a = 0; a < active_bodies_; a++) {
    Body& A = bodies_[active_[a]];
    for (int b = 0; b < MAX_PHYSICS_NEIGHBORS; b++) {
      BodyHandle B = A.neighbors[b].handle;
      if (B.IsValid()) {
        CollideObjectWithObject(A, *(B.body));
      }
    }
    for (int i = 0; i < important_bodies_; i++) {
      CollideObjectWithObject(A, bodies_[important_[i]]);
    }
  }
  debug::Profiler::EndTopic(tAA);

  // Repeat this with pikmin, our special case heros
  // Pikmin need to collide against all active bodies, but not with each other*
  //   *except sometimes
  debug::Profiler::StartTopic(tAP);
  for (int p = 0; p < active_pikmin_; p++) {
    Body& P = bodies_[pikmin_[p]];
    for (int a = 0; a < MAX_PHYSICS_NEIGHBORS; a++) {
      BodyHandle A = P.neighbors[a].handle;;
      if (A.IsValid()) {
        CollidePikminWithObject(P, *(A.body));
      }
    }
    for (int i = 0; i < important_bodies_; i++) {
      CollidePikminWithObject(P, bodies_[important_[i]]);
    }
  }
  debug::Profiler::EndTopic(tAP);

  // Finally, collide 1/8 of the pikmin against the rest of the group.
  // (This really doesn't need to be terribly accurate.)
  debug::Profiler::StartTopic(tPP);
  for (int p1 = iteration %  8; p1 < active_pikmin_; p1 += 8) {
    Body& P1 = bodies_[pikmin_[p1]];
    for (int p2 = 0; p2 < active_pikmin_; p2++) {
      Body& P2 = bodies_[pikmin_[p2]];
      CollidePikminWithPikmin(P1, P2);
    }
  }
  debug::Profiler::EndTopic(tPP);
}

void World::Update() {
  bodies_overlapping_ = 0;
  total_collisions_ = 0;
  if (rebuild_index_) {
    RebuildIndex();
  }
  
  debug::Profiler::StartTopic(tMoveBodies);
  MoveBodies();
  debug::Profiler::EndTopic(tMoveBodies);

  debug::Profiler::StartTopic(tCollideBodies);
  ProcessCollision();
  debug::Profiler::EndTopic(tCollideBodies);

  debug::Profiler::StartTopic(tCollideWorld);
  CollideBodiesWithLevel();
  debug::Profiler::EndTopic(tCollideWorld);

  iteration++;
}

int World::BodiesOverlapping() {
  return bodies_overlapping_;
}

int World::TotalCollisions() {
  return total_collisions_;
}

void World::DebugCircles() {
  for (int i = 0; i < active_bodies_; i++) {
    Body& body = bodies_[active_[i]];

    //pick a color based on the state of this body
    rgb color = RGB5(31,31,31);
    int segments = 12;
    if (body.is_sensor) {
      color = RGB5(31,31,0); //yellow for sensors
    }
    debug::DrawCircle(body.position, body.radius, color, segments);
  }
  for (int i = 0; i < active_pikmin_; i++) {
    Body& body = bodies_[pikmin_[i]];
    //pick a color based on the state of this body
    rgb color = RGB5(31,15,15);
    int segments = 6;
    debug::DrawCircle(body.position, body.radius, color, segments);
  }

  for (int i = 0; i < important_bodies_; i++) {
    Body& body = bodies_[important_[i]];
    //pick a color based on the state of this body
    rgb color = RGB5(15,31,31);
    int segments = 12;
    debug::DrawCircle(body.position, body.radius, color, segments);
  }

  // Draw relationships from the current neighbor
  Body& A = bodies_[active_[current_neighbor_]];
  for (int i = 0; i < MAX_PHYSICS_NEIGHBORS; i++) {
    BodyHandle B = A.neighbors[i].handle;
    if (B.IsValid()) {
        debug::DrawLine(A.position, B.body->position, RGB5(0, 31, 0));
    }
  }
}

void World::CollideBodiesWithLevel() {
  for (int i = 0; i < active_bodies_; i++) {
    // First, make sure this is an active body
    CollideBodyWithLevel(bodies_[active_[i]]);
  }
  for (int i = 0; i < active_pikmin_; i++) {
    // First, make sure this is an active body
    CollideBodyWithLevel(bodies_[pikmin_[i]]);
  }
}

void World::GenerateHeightTable() {
  fixed height_step = 32_f / 128_f;
  for (int i = 0; i < 128; i++) {
    height_table_[i] = height_step * fixed::FromInt(i);
  }
}

void World::SetHeightmap(const u8* raw_heightmap_data) {
  heightmap_data = (u8*)(raw_heightmap_data + 8); // skip over width/height
  int* heightmap_coords = (int*)raw_heightmap_data;
  heightmap_width = heightmap_coords[0];
  heightmap_height = heightmap_coords[1];
  GenerateHeightTable();
}

// Given a world position, figured out the level's height within the loaded
// height map
fixed World::HeightFromMap(int hx, int hz) {
  if (hx < 0) {hx = 0;}
  if (hz < 0) {hz = 0;}
  if (hx >= heightmap_width) {hx = heightmap_width - 1;}
  if (hz >= heightmap_height) {hz = heightmap_height - 1;}

  u8 height_index = heightmap_data[hz * heightmap_width + hx] & 0x7F;
  return height_table_[height_index];
}

fixed World::HeightFromMap(const Vec3& position) {
  // Figure out the body's "pixel" within the heightmap; we simply clamp to
  // integers to do this since one pixel is equivalent to one unit in the world
  int hx = (int)position.x;
  int hz = (int)position.z;

  // Clamp the positions to the map edges, so we don't get weirdness
  return HeightFromMap(hx, hz);
}

const fixed kWallThreshold = 2_f; //this seems reasonable

void World::CollideBodyWithLevel(Body& body) {
  if (!body.collides_with_level) {
    return;
  }

  if (!(body.ignores_walls)) {
    CollideBodyWithTilemap(body, 4);
  }

  fixed current_level_height = HeightFromMap(body.position);
  if (body.position.y < current_level_height) {
    body.position.y = current_level_height;
    body.velocity.y = 0_f;
    body.touching_ground = 1;
  } else {
    body.touching_ground = 0;
  }
}

void World::CollideBodyWithTilemap(Body& body, int max_depth) {
  // Note: tiles are 1 "unit" wide for collision purposes. This simplifies life.
  int tile_diff_x = ((int)body.position.x - (int)body.old_position.x);
  int tile_diff_z = ((int)body.position.z - (int)body.old_position.z);
  int tiles_traversed = abs(tile_diff_x) + abs(tile_diff_z) + 1;

  if (tiles_traversed <= 1) {
    // Early out: we didn't leave our starting tile this frame
    return;
  }

  // Figure out our first intersection and step size for traversing the grid
  Vec3 diff = body.old_position - body.position;

  fixed intersect_x_step, intersect_z_step, next_intersect_x, next_intersect_z;
  if (diff.x == 0_f) {
    intersect_x_step = 0_f;
    next_intersect_x = fixed::FromRaw(0x0FFFFFFF); // Effectively positive infinity
  } else {
    fixed x_fraction = body.position.x - fixed::FromInt((int)body.position.x);
    if (diff.x > 0_f) {
      intersect_x_step = 1_f / diff.x;
      next_intersect_x = (1_f - x_fraction) * intersect_x_step;
    } else {
      intersect_x_step = -1_f / diff.x;
      next_intersect_x = x_fraction * intersect_x_step;
    }
  }

  if (diff.z == 0_f) {
    intersect_z_step = 0_f;
    next_intersect_z = fixed::FromRaw(0x0FFFFFFF); // Effectively positive infinity
  } else {
    fixed z_fraction = body.position.z - fixed::FromInt((int)body.position.z);
    if (diff.z > 0_f) {
      intersect_z_step = 1_f / diff.z;
      next_intersect_z = (1_f - z_fraction) * intersect_z_step;
    } else {
      intersect_z_step = -1_f / diff.z;
      next_intersect_z = z_fraction * intersect_z_step;
    }
  }

  // Now, iterate over all the tiles this object would intersect, and perform height checks as we go
  int tile_x = (int)body.old_position.x;
  int tile_z = (int)body.old_position.z;

  int step_x = (tile_diff_x > 0 ? 1 : -1);
  int step_z = (tile_diff_z > 0 ? 1 : -1);
  bool moved_x;

  fixed current_level_height = HeightFromMap(body.old_position);

  for (int i = 1; i < tiles_traversed; i++) {
    if (next_intersect_x < next_intersect_z) {
      tile_x += step_x;
      next_intersect_x += intersect_x_step;
      moved_x = true;
    } else {
      tile_z += step_z;
      next_intersect_z += intersect_z_step;
      moved_x = false;
    }

    // Check to see if this is a valid move, and otherwise handle the response
    fixed new_level_height = HeightFromMap(tile_x, tile_z);
    if (body.position.y < new_level_height) {
      if (new_level_height - current_level_height > kWallThreshold) {
        // Wall collision here!
        Vec3 intersection;
        Vec3 new_position;
        if (moved_x) {
          fixed x_moved = next_intersect_x - intersect_x_step;          
          intersection.x = body.old_position.x + (diff.x * x_moved);
          intersection.y = body.old_position.y + (diff.y * x_moved);
          intersection.z = body.old_position.z + (diff.z * x_moved);

          new_position.x = intersection.x;
          new_position.y = body.position.y;
          new_position.z = body.position.z;
        } else {          
          fixed z_moved = next_intersect_z - intersect_z_step;
          intersection.x = body.old_position.x + (diff.x * z_moved);
          intersection.y = body.old_position.y + (diff.y * z_moved);
          intersection.z = body.old_position.z + (diff.z * z_moved);

          new_position.x = body.position.x;
          new_position.y = body.position.y;
          new_position.z = intersection.z;
        }

        body.old_position = intersection;
        body.position = new_position;

        if (max_depth > 1) {
          CollideBodyWithTilemap(body, max_depth - 1);
        }
        return;
      }
    }
    current_level_height = new_level_height;
  }

}







