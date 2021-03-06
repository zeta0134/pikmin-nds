#ifndef PROJECT_SETTINGS_H_
#define PROJECT_SETTINGS_H_

// Used to increase the overlap between multipass slices.
// Might be useful to reduce visible seams. Higher values make the 3DS
// engine draw more extra polygons at the rear of a pass, and will likely
// cause artifacts with alpha-blended polygons.
#ifndef CLIPPING_FUDGE_FACTOR
#define CLIPPING_FUDGE_FACTOR 0
#endif

// As polygon counts are estimated (can't use the hardware to directly count)
// we need to be able to fudge on the max per pass to achieve a good balance
// between performance (average closer to 2048 polygons every pass) and
// sanity (not accidentally omitting polygons because we guess badly)
#ifndef MAX_POLYGONS_PER_PASS
#define MAX_POLYGONS_PER_PASS 1800
#endif

// This is a cheap CPU limiter; don't ever try to squeeze more than this many
// objects into a single pass. This resolves some issues with many (MANY)
// small objects causing frames to skip because the GPU chokes on that many
// display lists in a row
#ifndef MAX_OBJECTS_PER_PASS
#define MAX_OBJECTS_PER_PASS 35
#endif

// Maximum number of entities, total. Used to initialize various structs
// in the multipass engine, acts as a limiter for both scene objects and
// static bits of a level.
#ifndef MAX_ENTITIES
#define MAX_ENTITIES 256
#endif

// Maxiumum number of particles the engine can handle at once. Any particles
// spawned above this limit silently fail.
#ifndef MAX_PARTICLES
#define MAX_PARTICLES 256
#endif

// Field of view, used by all 3D perspective transformations. (Ignored by ortho
// projections)
#ifndef FIELD_OF_VIEW
#define FIELD_OF_VIEW 45.0_brad
#endif

// Maximum number of Physics Bodies that can be declared.
#ifndef MAX_PHYSICS_BODIES
#define MAX_PHYSICS_BODIES 256
#endif

// Number of physics neighbors to consider. Increasing this gives more accuracy
// when many objects are near each other, but decreases performance heavily.
// Shoot for something low but reasonable here.
#ifndef MAX_PHYSICS_NEIGHBORS
#define MAX_PHYSICS_NEIGHBORS 6
#endif

// How fast objects accelerate towards the ground, per frame
#ifndef GRAVITY_CONSTANT
#define GRAVITY_CONSTANT (4.5_f / 30_f)
#endif

// Collision groups for the physics engine
#define PLAYER_GROUP  (0x1 << 0)
#define PIKMIN_GROUP  (0x1 << 1)
#define WHISTLE_GROUP (0x1 << 2)
#define ATTACK_GROUP (0x1 << 3)
#define DETECT_GROUP (0x1 << 4)
#define ONION_FEET_GROUP (0x1 << 5)
#define ONION_BEAM_GROUP (0x1 << 6)
#define FIRE_HAZARD_GROUP (0x1 << 7)
#define WATER_HAZARD_GROUP (0x1 << 8)
#define ELECTRIC_HAZARD_GROUP (0x1 << 9)
#define TREASURE_GROUP (0x1 << 10)

// Camera distance and height constants (play with these if you adjust the FOV)
#define CAMERA_LOW_HEIGHT 0.5_f
#define CAMERA_HIGH_HEIGHT 2.0_f
#define CAMERA_CLOSEST 17_f
#define CAMERA_STEP 14_f

#endif  // PROJECT_SETTINGS_H_
