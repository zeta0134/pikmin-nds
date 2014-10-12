#include "debug.h"
#include <cstdio>
#include <nds.h>


bool debug::g_timing_colors{false};
bool debug::g_render_first_pass_only{false};
bool debug::g_skip_vblank{false};

void debug::DrawCrosshair(Vec3 p, rgb color) {
  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
  glBegin(GL_TRIANGLE);
  glColor(color);
  glPushMatrix();
  glTranslatef32(p.x.data_, p.y.data_, p.z.data_);
  glVertex3v16(0, -1 << 10, 0);
  glVertex3v16(0, 1 << 10, 0);
  glVertex3v16(0, 1 << 10, 0);

  glVertex3v16(1 << 10, 0, 0);
  glVertex3v16(-1 << 10, 0, 0);
  glVertex3v16(-1 << 10, 0, 0);

  glPopMatrix(1);
  glEnd();
}

void debug::DrawGroundPlane(int width, int segments, rgb color) {
  // Derive a dark color by dividing each channel by 2. This is accomplished
  // using a bitmask: 0 rrrr0 gggg0 bbbb0, which removes the bottom bit in each
  // color channel. Shifting the result of this mask to the right results in
  // 0 0rrrr 0gggg 0bbbb, which is the desired result.
  rgb dark_color = (color & 0x7BDE) >> 1;
  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | (1 << 12));
  glBegin(GL_TRIANGLE);
  glPushMatrix();
  glScalef(width / 2, 0, width / 2);
  for (int z = 0; z < segments; z++) {
    for (int x = 0; x < segments; x++) {
      glColor((z + x) % 2 == 0 ? color : dark_color);
      glVertex3f(-1.0f + (2.0f / segments) *  x     , 0,  -1.0f + (2.0f / segments) *  z);
      glVertex3f(-1.0f + (2.0f / segments) * (x + 1), 0,  -1.0f + (2.0f / segments) *  z);
      glVertex3f(-1.0f + (2.0f / segments) * (x + 1), 0,  -1.0f + (2.0f / segments) * (z + 1));

      glVertex3f(-1.0f + (2.0f / segments) * (x + 1), 0,  -1.0f + (2.0f / segments) * (z + 1));
      glVertex3f(-1.0f + (2.0f / segments) *  x     , 0,  -1.0f + (2.0f / segments) *  z);
      glVertex3f(-1.0f + (2.0f / segments) *  x     , 0,  -1.0f + (2.0f / segments) * (z + 1));
    }
  }

  glPopMatrix(1);
  glEnd();
}

void debug::TimingColor(rgb color) {
  if (g_timing_colors) {
    BG_PALETTE_SUB[0] = color;
  }
}

namespace {
void Status(char const* status) {
  printf("\x1b[23;0H[D] %28.28s", status);
}
}  // namespace

void debug::UpdateFlags() {
  // Hold debug modifier [SELECT], then press:
  // A = Render only First Pass
  // B = Skip VBlank, useful for:
  // X = Draw Debug Timings to Bottom Screen (with flashing colors!)

  // Check for debug-related input and update the flags accordingly.
  // Todo(Nick) Make this touchscreen based instead of key combo based.
  if ((keysHeld() & KEY_SELECT) and (keysDown() & KEY_A)) {
    debug::g_render_first_pass_only = not debug::g_render_first_pass_only;
    if (debug::g_render_first_pass_only) {
      Status("Rendering only first pass.");
    } else {
      Status("Rendering every pass.");
    }
  }

  if ((keysHeld() & KEY_SELECT) and (keysDown() & KEY_B)) {
    debug::g_skip_vblank = not debug::g_skip_vblank;
    if (debug::g_skip_vblank) {
      Status("Skipping vBlank");
    } else {
      Status("Not skipping vBlank");
    }
  }

  if ((keysHeld() & KEY_SELECT) and (keysDown() & KEY_X)) {
    debug::g_timing_colors = not debug::g_timing_colors;
    if (debug::g_timing_colors) {
      Status("Rendering Colors");
    } else {
      Status("No more flashing!");
    }
  }
}