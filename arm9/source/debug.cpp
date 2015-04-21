#include "debug.h"
#include <cstdio>
#include <nds.h>
#include <map>

using numeric_types::fixed;
using numeric_types::literals::operator"" _f;

bool debug::g_timing_colors{false};
bool debug::g_render_first_pass_only{false};
bool debug::g_skip_vblank{false};
bool debug::g_physics_circles{false};

void debug::nocashNumber(int num) {
  char buffer[20];
  sprintf(buffer, "%i", num);
  nocashMessage(buffer);
}

void debug::DrawCrosshair(Vec3 p, rgb color) {
  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
  glTexParameter(0, 0); //disable textures
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

void debug::DrawCircle(Vec3 p, fixed radius, rgb color, u32 segments) {
  float const radiansPerArc = 360.0 / segments;

  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
  glTexParameter(0, 0); //disable textures
  glBegin(GL_TRIANGLE);
  glColor(color);
  glPushMatrix();
  // We add 0.5 here to avoid a collision with the ground plane.
  glTranslatef32(p.x.data_, p.y.data_ + (1 << 11), p.z.data_);
  glScalef32(radius.data_,radius.data_,radius.data_);
  //spin right round
  for (u32 i = 0; i < segments; ++i) {
    glPushMatrix();
    glRotateY(i * radiansPerArc);
    glVertex3v16(1 << 12, 0, 0);
    glRotateY(radiansPerArc);
    glVertex3v16(1 << 12, 0, 0);
    glVertex3v16(1 << 12, 0, 0);
    glPopMatrix(1);
  }

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
  glTexParameter(0, 0); //disable textures
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

void debug::_TimingColor(rgb color) {
  if (g_timing_colors) {
    BG_PALETTE_SUB[0] = color;
  }
}

namespace {
void Status(char const* status) {
  printf("\x1b[23;0H[D] %28.28s", status);
}
}  // namespace

struct TimingResult {
  u32 start = 0;
  u32 end = 0;
  u32 delta() {
    return end - start;
  }
};

struct TopicInfo {
  const char* name;
  rgb color;
};

TimingResult g_timing_results[static_cast<int>(debug::Topic::kNumTopics)];

std::map<debug::Topic, TopicInfo> g_topic_info{
  {debug::Topic::kUpdate, {
    "Update", 
    RGB8(255, 128, 0)}},
  {debug::Topic::kPhysics, {
    "Physics",
    RGB8(255, 64, 255)}},
  {debug::Topic::kFrameInit, {
    "FrameInit",
    RGB8(0, 128, 0)}},
  {debug::Topic::kPassInit, {
    "PassInit",
    RGB8(255, 255, 0)}},
  {debug::Topic::kPass1, {
    "Pass1",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kPass2, {
    "Pass2",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kPass3, {
    "Pass3",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kPass4, {
    "Pass4",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kPass5, {
    "Pass5",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kPass6, {
    "Pass6",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kPass7, {
    "Pass7",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kPass8, {
    "Pass8",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kPass9, {
    "Pass9",   
    RGB8(0, 0, 255)}},
  {debug::Topic::kIdle, {
    "Idle",   
    RGB8(48, 48, 48)}},
};

void debug::UpdateTimingMode() {
  // Clear the screen
  printf("\x1b[2J");
  printf("-------------TIMING-------------");

  // For every topic, output the timing on its own line
  for (int i = 0; i < static_cast<int>(debug::Topic::kNumTopics); i++) {  
    if (g_timing_results[i].delta() > 0) {
      if (i & 0x1) {
        printf("\x1b[39m");
      } else {
        printf("\x1b[37m");
      }
      int displayLine = i + 2;
      // Print out the topic name if we have it, otherwise print out something
      // generic
      if (g_topic_info.count((debug::Topic)i)) {
        printf("\x1b[%d;0H%s", displayLine, g_topic_info[(debug::Topic)i].name);
      } else {
        printf("\x1b[%d;0HTopic:%d", displayLine, i);
      }
      printf("\x1b[%d;21H%10lu", displayLine, g_timing_results[i].delta());
    }
  }

  // Reset the colors when we're done
  printf("\x1b[39m");
}

std::map<std::string, int> g_debug_ints;
std::map<std::string, fixed> g_debug_fixeds;
std::map<std::string, Vec3> g_debug_vectors;

void debug::DisplayValue(const std::string &name, int value) {
  g_debug_ints[name] = value;
}

void debug::DisplayValue(const std::string &name, fixed value) {
  g_debug_fixeds[name] = value;
}

void debug::DisplayValue(const std::string &name, Vec3 value) {
  g_debug_vectors[name] = value;
}

void debug::UpdateValuesMode() {
  // Clear the screen
  printf("\x1b[2J");
  printf("-------------VALUES-------------");

  int display_position = 2;
  for (auto kv : g_debug_ints) {
    if (display_position < 22) {
      printf("\x1b[%d;0H\x1b[39m%s: \x1b[36;1m%d", display_position, kv.first.c_str(), kv.second);
        display_position++;
    }
  }

  for (auto kv : g_debug_fixeds) {
    if (display_position < 22) {
      printf("\x1b[%d;0H\x1b[39m%s: \x1b[32;1m%.3f", display_position, kv.first.c_str(), (float)kv.second);
        display_position++;
    }
  }

  for (auto kv : g_debug_vectors) {
    if (display_position < 22) {
      Vec3 vector = kv.second;
      printf("\x1b[%d;0H\x1b[39m%s: \x1b[30;1m(\x1b[33;1m%.1f\x1b[30;1m, \x1b[33;1m%.1f\x1b[30;1m, \x1b[33;1m%.1f\x1b[30;1m)", display_position, kv.first.c_str(), (float)vector.x, (float)vector.y, (float)vector.z);
        display_position++;
    }
  }

  // Reset the colors when we're done
  printf("\x1b[39m");
}

enum DebugMode {
  kOff = 0,
  kTiming,
  kValues,
  kNumDebugModes
};

int g_debug_mode = DebugMode::kOff;

void debug::Update() {
  if (keysDown() & KEY_SELECT) {
    g_debug_mode++;
    if (g_debug_mode >= DebugMode::kNumDebugModes) {
      g_debug_mode = DebugMode::kOff;
    }
    // Clear the screen for any switch
    printf("\x1b[2J");
  }

  switch (g_debug_mode) {
    case DebugMode::kOff:
      return;
    case DebugMode::kTiming:
      UpdateTimingMode();
      break;
    case DebugMode::kValues:
      UpdateValuesMode();
      break;
    default:
      printf("\x1b[2Jm");
      printf("Undefined debug mode!");
  }

  return;


  // Hold debug modifier [SELECT], then press:
  // A = Render only First Pass
  // B = Skip VBlank, useful for:
  // X = Draw Debug Timings to Bottom Screen (with flashing colors!)

  // Check for debug-related input and update the flags accordingly.
  // Todo(Nick) Make this touchscreen based instead of key combo based.
  if (keysHeld() & KEY_SELECT) {
    if (keysDown() & KEY_A) {
      debug::g_render_first_pass_only = not debug::g_render_first_pass_only;
      if (debug::g_render_first_pass_only) {
        Status("Rendering only first pass.");
      } else {
        Status("Rendering every pass.");
      }
    }

    if (keysDown() & KEY_B) {
      debug::g_skip_vblank = not debug::g_skip_vblank;
      if (debug::g_skip_vblank) {
        Status("Skipping vBlank");
      } else {
        Status("Not skipping vBlank");
      }
    }

    if (keysDown() & KEY_X) {
      debug::g_timing_colors = not debug::g_timing_colors;
      if (debug::g_timing_colors) {
        Status("Rendering Colors");
      } else {
        Status("No more flashing!");
      }
    }

    if (keysDown() & KEY_Y) {
      debug::g_physics_circles = not debug::g_physics_circles;
      if (debug::g_physics_circles) {
        Status("Physics Circles!");
      } else {
        Status("No more circles.");
      }
    }

    //switch timing topics
    if (keysDown() & KEY_LEFT) {
      debug::PreviousTopic();
    }

    if (keysDown() & KEY_RIGHT) {
      debug::NextTopic();
    }
  }
}

void debug::StartTopic(debug::Topic topic) {
  int index = static_cast<int>(topic);
  g_timing_results[index].start = cpuGetTiming();
  debug::_TimingColor(g_topic_info[topic].color);
}

void debug::EndTopic(debug::Topic topic) {
  int index = static_cast<int>(topic);
  g_timing_results[index].end = cpuGetTiming();
  debug::_TimingColor(RGB8(0,0,0));
}

void debug::StartCpuTimer() {
  // This uses two timers for 32bit precision, so this call will consume
  // timers 0 and 1.
  cpuStartTiming(0); 
}

int g_debug_current_topic = 0;

void debug::NextTopic() {
  g_debug_current_topic++;
  if (g_debug_current_topic >= (int)debug::Topic::kNumTopics) {
    g_debug_current_topic = 0;
  }
}

void debug::PreviousTopic() {
  g_debug_current_topic--;
  if (g_debug_current_topic < 0) {
    g_debug_current_topic = (int)debug::Topic::kNumTopics;
  }
}

void debug::UpdateTopic() {
  //clear the line
  printf("\x1b[22;0H                                ");
  if (g_topic_info.count((Topic)g_debug_current_topic)) {
    printf("\x1b[22;0H%s", g_topic_info[(Topic)g_debug_current_topic].name);
  } else {
    printf("\x1b[22;0HTopic:%d", g_debug_current_topic);
  }
  printf("\x1b[22;21H%10lu", g_timing_results[g_debug_current_topic].delta());
}
