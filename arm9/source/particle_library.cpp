#include "particle.h"
#include "particle_library.h"
#include "project_settings.h"

using numeric_types::literals::operator"" _f;
using numeric_types::literals::operator"" _brad;
using numeric_types::Brads;
using numeric_types::fixed;

namespace particle_library {

Particle dirt_cloud;
Particle fire;
Particle smoke;
Particle piki_star;
Particle rock;

void Init(VramAllocator<Texture>* texture_allocator, VramAllocator<TexturePalette>* palette_allocator) {
  dirt_cloud.texture = texture_allocator->Retrieve("smoke1.a5i3");
  dirt_cloud.palette = palette_allocator->Retrieve("smoke1.a5i3");
  // Perhaps we should be reading in the width/height from the image on disk?
  dirt_cloud.texture.format_width = TEXTURE_SIZE_32;
  dirt_cloud.texture.format_height = TEXTURE_SIZE_32;
  dirt_cloud.lifespan = 12;
  dirt_cloud.alpha = 0.75_f;
  dirt_cloud.fade_rate = dirt_cloud.alpha / fixed::FromInt(dirt_cloud.lifespan);
  dirt_cloud.scale = 1.0_f;
  dirt_cloud.scale_rate = 0.02_f;
  dirt_cloud.color = RGB15(13,8,6);

  fire.texture = texture_allocator->Retrieve("fire.a3i5");
  fire.palette = palette_allocator->Retrieve("fire.a3i5");
  // Perhaps we should be reading in the width/height from the image on disk?
  fire.texture.format_width = TEXTURE_SIZE_32;
  fire.texture.format_height = TEXTURE_SIZE_32;
  fire.lifespan = 16;
  fire.fade_rate = 1_f / 32_f;
  fire.scale = 2.0_f;
  fire.scale_rate = 0.08_f;

  smoke.texture = texture_allocator->Retrieve("smoke1.a5i3");
  smoke.palette = palette_allocator->Retrieve("smoke1.a5i3");
  smoke.texture.format_width = TEXTURE_SIZE_32;
  smoke.texture.format_height = TEXTURE_SIZE_32;
  smoke.lifespan = 16;
  smoke.alpha = 0.5_f;
  smoke.fade_rate = 0.5_f / 16_f;
  smoke.scale = 2.0_f;
  smoke.scale_rate = 0.1_f;

  piki_star.texture = texture_allocator->Retrieve("star.a5i3");
  piki_star.palette = palette_allocator->Retrieve("star.a5i3");
  piki_star.texture.format_width = TEXTURE_SIZE_16;
  piki_star.texture.format_height = TEXTURE_SIZE_16;
  piki_star.lifespan = 32;
  piki_star.scale = 0.6_f;
  piki_star.alpha = 0.75_f;
  piki_star.scale_rate = piki_star.scale / -32_f;
  piki_star.color_a = RGB15(31,31,8);
  piki_star.color_b = RGB15(31,8,8);
  piki_star.color_change_rate = 6;
  piki_star.rotation = 45_brad;
  piki_star.rotation_rate = 5_brad;

  rock.texture = texture_allocator->Retrieve("rock.t2bpp");
  rock.palette = palette_allocator->Retrieve("rock.t2bpp");
  // Perhaps we should be reading in the width/height from the image on disk?
  rock.texture.format_width = TEXTURE_SIZE_16;
  rock.texture.format_height = TEXTURE_SIZE_16;
  rock.lifespan = 16;
  rock.fade_rate = 1_f / 32_f;
  rock.scale = 0.4_f;
  rock.velocity = Vec3{0_f,1_f,0_f};
  rock.acceleration = Vec3{0_f,-GRAVITY_CONSTANT,0_f};
}

// Utility functions for setting particle properties and variance

//returns a random vector from -1 to 1 in all directions
Vec3 RandomSpread() {
  return Vec3{
    fixed::FromRaw((rand() & ((1 << 13) - 1)) - (1 << 12)),
    fixed::FromRaw((rand() & ((1 << 13) - 1)) - (1 << 12)),
    fixed::FromRaw((rand() & ((1 << 13) - 1)) - (1 << 12))
  };
}

Vec3 FireSpread() {
  return RandomSpread() * 0.06_f;
}

Vec3 RockSpread() {
  auto vel = RandomSpread();
  vel.x *= 0.4_f;
  vel.y *= 0.06_f;
  vel.z *= 0.4_f;
  return vel;
}

Vec3 DirtCloudSpread() {
  auto vel = RandomSpread();
  vel.x *= 0.3_f;
  vel.y = 0_f;
  vel.z *= 0.3_f;
  return vel;
}

void SpreadPikiStar(Particle* particle) {
  particle->position += RandomSpread() * 0.6_f;
  particle->velocity += RandomSpread() * 0.06_f;
  particle->acceleration = particle->velocity * (-1_f / 32_f);
  particle->color_weight = rand() & 32;
  particle->rotation = numeric_types::Brads::Raw(degreesToAngle(rand()));
  particle->rotation_rate = numeric_types::Brads::Raw(degreesToAngle(rand() % 8 - 4));
}

}
