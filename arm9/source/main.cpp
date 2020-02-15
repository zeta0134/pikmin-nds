#include <array>
#include <functional>
#include <stdio.h>
#include <set>

#include <filesystem.h>
#include <nds.h>

#include "ai/captain.h"
#include "debug/messages.h"
#include "debug/utilities.h"
#include "render/multipass_renderer.h"
#include "file_utils.h"
#include "level_loader.h"
#include "particle_library.h"
#include "pikmin_game.h"

using captain_ai::CaptainState;

using numeric_types::literals::operator"" _f;
using numeric_types::literals::operator"" _brad;
using numeric_types::fixed;

using debug::Topic;

using namespace std;

// Initialize the console using the full version of the console init function so
// that VRAM bank H can be used instead of the default bank, bank C.
void InitDebugConsole() {
  vramSetBankH(VRAM_H_SUB_BG);
  videoSetModeSub(MODE_0_2D);

  PrintConsole* const kDefaultConsole{nullptr};
  s32 const kConsoleLayer{0};
  s32 const kConsoleMapBase{15};
  s32 const kConsoleTileBase{0};
  bool const kConsoleOnMainDisplay{true};
  bool const kLoadConsoleGraphics{true};
  consoleInit(kDefaultConsole, kConsoleLayer, BgType_Text4bpp, BgSize_T_256x256,
      kConsoleMapBase, kConsoleTileBase, not kConsoleOnMainDisplay,
      kLoadConsoleGraphics);
}

void InitSubScreen() {
  InitDebugConsole();
}

void InitMainScreen() {
  videoSetMode(MODE_0_3D);
  glInit();
  glEnable(GL_TEXTURE_2D | GL_BLEND | GL_OUTLINE/* | GL_FOG*/);

  // Fog!
  glFogColor(4, 4, 4, 31);
  glFogOffset(0);
  glFogShift(5);
  for (u32 i = 0; i < 32; i++) {
    glFogDensity(i, i * 4);
  }

  glClearColor(4, 4, 4, 31);
  glClearDepth(0x7FFF);
  glViewport(0, 0, 255, 191);

  GFX_CLEAR_COLOR = GFX_CLEAR_COLOR | POLY_FOG;

  Vec3 light0_direction = Vec3{1_f, -1_f, -1_f}.Normalize() * 0.99_f;
  Vec3 light1_direction = Vec3{-1_f, -1_f, -1_f}.Normalize() * 0.99_f;

  // Setup default lights; these will be overridden in the main engine.
  //*
  glLight(0, RGB15(31, 31, 31),
    floattov10((float)light0_direction.x),
    floattov10((float)light0_direction.y),
    floattov10((float)light0_direction.z));
  //*/
  //*
  glLight(1, RGB15(31, 31, 31),
    floattov10((float)light1_direction.x),
    floattov10((float)light1_direction.y),
    floattov10((float)light1_direction.z));
  //*/
  /*
  glLight(2, RGB15(0, 0, 0),
    floattov10((float)light1_direction.x),
    floattov10((float)light1_direction.y),
    floattov10((float)light1_direction.z));
  //*/
  /*
  glLight(3, RGB15(0, 0, 0),
      floattov10((float)light1_direction.x),
      floattov10((float)light1_direction.y),
      floattov10((float)light1_direction.z));
  //*/

  //ds uses a table for shinyness..this generates a half-ass one
	glMaterialShinyness();
}

string BaseName(string full_filename) {
  auto index = full_filename.rfind(".");
  if (index != string::npos) {
    return full_filename.substr(0, index);
  } else {
    return "";
  }
}

string FileExtension(string filename) {
  auto index = filename.rfind(".");
  if (index != string::npos) {
    return filename.substr(index + 1);
  } else {
    return "";
  }
}

map<string, u32> texture_extension_formats = {
  {"2bpp", GL_RGB4},
  {"t2bpp", GL_RGB4},
  {"4bpp", GL_RGB16},
  {"t4bpp", GL_RGB16},
  {"a3i5", GL_RGB32_A3},
  {"a5i3", GL_RGB8_A5},
};

set<string> texture_extension_is_transparent {
  "t2bpp",
  "t4bpp",
};

template<typename T>
void LoadFileWithMetadata(T* vram_allocator, typename T::Metadata metadata, string filename, string identifier) {
  vector<char> buffer = LoadEntireFile("/textures/" + filename);
  if (buffer.size() > 0) {
    vram_allocator->Load(
      identifier, (u8*)buffer.data(), buffer.size(), metadata);
  }
}

void LoadDsgxFile(DsgxAllocator* dsgx_allocator, string filename, string identifier) {
  vector<char> buffer = LoadEntireFile("/actors/" + filename);
  if (buffer.size() > 0) {
    dsgx_allocator->Load(
        identifier, (u8*)buffer.data(), buffer.size());
  }
}

void LoadTexturesFromNitroFS(PikminGame& game) {
  auto texture_files = FilesInDirectory("/textures");
  for (string filename : texture_files) {
    // Collect metadata about this file
    auto extension = FileExtension(filename);
    Texture metadata;
    if (texture_extension_formats.count(extension) > 0) {
      metadata.format = texture_extension_formats[extension];
    } else {
      //debug::Log(("Skipping extension: " + extension).c_str());
      continue;
    }
    if (texture_extension_is_transparent.count(extension) > 0) {
      metadata.transparency = Texture::kTransparent;
    } else {
      metadata.transparency = Texture::kDisplayed;
    }

    LoadFileWithMetadata(game.TextureAllocator(), metadata, filename, filename);

    // Attempt to find and load a matching palette file, if one exists
    string const palette_filename = BaseName(filename) + ".pal";
    LoadFileWithMetadata(game.TexturePaletteAllocator(), TexturePalette{}, palette_filename, filename);
  }
}

void LoadActors(PikminGame& game) {
  auto texture_files = FilesInDirectory("/actors");
  for (string filename : texture_files) {
    auto extension = FileExtension(filename);
    if (extension == "dsgx") {
      // load and parse the DSGX data
      LoadDsgxFile(game.ActorAllocator(), filename, BaseName(filename));
      // apply texture offsets from our previously loaded textures and palettes
      Dsgx* actor = game.ActorAllocator()->Retrieve(BaseName(filename));
      actor->ApplyTextures(game.TextureAllocator(), game.TexturePaletteAllocator());
    }
  }
}

void LoadTextures(PikminGame& game) {
  // VRAM is not memory mapped to the CPU when in texture mode, so all
  // modifications to textures must be done by changing the bank to a mode
  // where it is mapped to the CPU, performing the modifications, and
  // switching it back to texture mode.
  vramSetBankC(VRAM_C_LCD);
  vramSetBankG(VRAM_G_LCD);
  LoadTexturesFromNitroFS(game);
  vramSetBankC(VRAM_C_TEXTURE);
  vramSetBankG(VRAM_G_TEX_PALETTE);
}

void Init(PikminGame& game) {
  if (!(nitroFSInit(NULL))) {
    debug::Log("Filesystem FAILURE");
  }

  InitMainScreen();
  InitSubScreen();

  LoadTextures(game);
  LoadActors(game);
  particle_library::Init(game.TextureAllocator(), game.TexturePaletteAllocator());
  game.LoadLevel("/levels/collision_test.level");

  game.InitSound("/soundbank.bin");

  glPushMatrix();
}

void GameLoop(PikminGame& game) {
  for (;;) {
    touchPosition touchXY;
    touchRead(&touchXY);

    //start debug timings for this loop
    debug::Profiler::StartTimer();

    game.Step();
    game.renderer().Draw();

    oamUpdate(&oamSub);
  }
}

#ifdef BUILD_TESTS
#include "tests/testing_framework.h"

int main() {
  debug::Log("Hello tests!");

  init_test_console();
  run_all_tests();

  while(true) {
    // Die forever
    swiWaitForVBlank();
  }
}

#else 

int main() {
  MultipassRenderer* renderer = new MultipassRenderer();
  PikminGame* game = new PikminGame(*renderer);

  debug::Log("Hello World!");
  Init(*game);
  GameLoop(*game);
  return 0;
}

#endif
