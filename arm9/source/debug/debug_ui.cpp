#include "debug/debug_ui.h"

#include <cstdio>
#include <functional>

#include <nds.h>

#include "ai/captain.h"
#include "debug/flags.h"
#include "debug/messages.h"
#include "debug/profiler.h"
#include "debug/utilities.h"
#include "file_utils.h"
#include "numeric_types.h"
#include "pikmin_game.h"

using std::string;
using std::function;

using numeric_types::literals::operator"" _brad;

namespace debug_ui {

// Console Utilities
void ClearConsole() {
  printf("\x1b[2J");
}

void ResetColors() {
  printf("\x1b[39m");
}

void PrintTitle(const char* title) {
  int console_width = 64;
  int leading_space = (console_width - strlen(title)) / 2 - 1;
  int following_space = leading_space + (strlen(title) % 2);

  printf("%s %s %s", std::string(leading_space, '-').c_str(), title,
    std::string(following_space, '-').c_str());
}

void InitAlways(DebugUiState& debug_ui) {
  debug_ui.current_spawner = PikminGame::SpawnNames().first;
  debug_ui.level_names = FilesInDirectory("/levels");
}

void UpdateDebugMessages(DebugUiState& debug_ui) {
  ClearConsole();
  ResetColors();
  PrintTitle("LOG");
  auto messages = debug::Messages();
  for (auto message : messages) {
    printf((message + "\n").c_str());
  }
}

void UpdateDebugValues(DebugUiState& debug_ui) {
  ClearConsole();
  ResetColors();
  PrintTitle("VALUES");

  int display_position = 2;
  auto display_values = debug_ui.game->DebugDictionary().DisplayValues();
  for (auto kv : display_values) {
    if (display_position < 22) {
      printf("\x1b[39m%s: \x1b[36;1m%s\n", kv.first.c_str(), kv.second.c_str());
      display_position++;
    }
  }

  ResetColors();
}

void UpdateDebugTimings(DebugUiState& debug_ui) {
  ClearConsole();
  ResetColors();
  PrintTitle("TIMING");

  // For every topic, output the timing on its own line
  for (auto topic : debug::Profiler::Topics()) {
    if (topic.timing.delta() > 0) {
      printf("%-32s %31lu", topic.name.c_str(), topic.timing.delta());
    } else {
      printf("\n");
    }
  }

  ResetColors();
}

void UpdateDebugToggles(DebugUiState& debug_ui) {
  ClearConsole();
  ResetColors();
  PrintTitle("Debug Toggles");

  int touch_offset = 16;
  auto &debug_flags = debug::FlagList();
  for (auto pair : debug_flags) {
    std::string toggleName = pair.first;
    bool toggleActive = pair.second;
    if (toggleActive) {
      printf("\x1b[39m");
    } else {
      printf("\x1b[30;1m");
    }
    printf("+------------------------------+\n");
    printf("| (%s) %*s |\n", (toggleActive ? "*" : " "), 24, toggleName.c_str());
    printf("+------------------------------+\n");

    // figure out if we need to toggle this frame
    if (keysDown() & KEY_TOUCH) {
      touchPosition touch;
      touchRead(&touch);

      if (touch.py > touch_offset and touch.py < touch_offset + 24) {
        //*toggleActive = !(*toggleActive);
        debug_flags[toggleName] = !toggleActive;
      }
    }
    touch_offset += 24;
  }

  ResetColors();
}

void UpdateDebugSpawners(DebugUiState& debug_ui) {
  ClearConsole();
  ResetColors();
  PrintTitle("Spawn Objects");

  printf("+------+ +-%*s-+ +------+", 42, std::string(42, '-').c_str());
  printf("|      | | %*s | |      |", 42, " ");
  printf("|   <  | | %*s | |  >   |", 42, debug_ui.current_spawner->first.c_str());
  printf("|      | | %*s | |      |", 42, " ");
  printf("+------+ +-%*s-+ +------+", 42, std::string(42, '-').c_str());

  if (keysDown() & KEY_TOUCH) {
    touchPosition touch;
    touchRead(&touch);

    if (touch.px > 192) {
      debug_ui.current_spawner++;
      if (debug_ui.current_spawner == PikminGame::SpawnNames().second) {
        debug_ui.current_spawner = PikminGame::SpawnNames().first;
      }
    } else if (touch.px < 64) {
      if (debug_ui.current_spawner == PikminGame::SpawnNames().first) {
        debug_ui.current_spawner = PikminGame::SpawnNames().second;
      }
      debug_ui.current_spawner--;
    } else {
      //Spawn a thingy!!
      auto active_captain = debug_ui.game->RetrieveCaptain(debug_ui.game->ActiveCaptain());
      Vec3 new_position = active_captain->cursor->position();
      Rotation new_rotation = active_captain->cursor->rotation();
      new_rotation.y += 180_brad;
      debug_ui.game->Spawn(debug_ui.current_spawner->first, new_position, new_rotation);
    }
  }

  // Reset the colors when we're done
  ResetColors();
}

void UpdateDebugAi(DebugUiState& debug_ui) {
  ClearConsole();
  ResetColors();
  PrintTitle("AI Profiler");
  PrintTitle("Pikmin");

  auto& active_profiler = debug_ui.game->DebugAiProfilers()["Pikmin"];

  printf("%-25s %12s %12s %12s", "State Name", "Runs", "Average", "Total");
  for (auto state_timing : active_profiler.StateTimings()) {
    std::string name = state_timing.first;
    u32 run_count = state_timing.second.run_count;
    u32 average_time = state_timing.second.total / state_timing.second.run_count;
    u32 total_time = state_timing.second.total;

    printf("%-25s %12lu %12lu %12lu", name.c_str(), run_count, average_time, total_time);
  }

  ResetColors();
}

void UpdateLevelSelect(DebugUiState& debug_ui) {
  ClearConsole();
  ResetColors();
  PrintTitle("Level Select");
  
  for (u8 i = 0; i < debug_ui.level_names.size(); i++) {
    if (debug_ui.current_level == i) {
      printf(" --> ");
    } else {
      printf("     ");
    }
    printf((debug_ui.level_names[i] + "\n").c_str());
  }

  if (keysDown() & KEY_DOWN && debug_ui.current_level < debug_ui.level_names.size() - 1) {
    debug_ui.current_level++;
  }
  if (keysDown() & KEY_UP && debug_ui.current_level > 0) {
    debug_ui.current_level--;
  }
  if (keysDown() & KEY_A) {
    debug_ui.game->LoadLevel("/levels/" + debug_ui.level_names[debug_ui.current_level]);
  }
}

bool DebugSwitcherPressed(const DebugUiState&  debug_ui) {
  return keysDown() & KEY_START;
}

namespace DebugUiNode {
enum DebugUiNode {
  kInit = 0,
  kDebugMessages,
  kDebugLevelSelect,
  kDebugTimings,
  kDebugAi,
  kDebugValues,
  kDebugToggles,
  kDebugSpawners,
};
}

Edge<DebugUiState> init[] = {
  Edge<DebugUiState>{Trigger::kAlways, nullptr, InitAlways, DebugUiNode::kDebugMessages},
  END_OF_EDGES(DebugUiState)
};

Edge<DebugUiState> debug_messages[] = {
  Edge<DebugUiState>{Trigger::kAlways, DebugSwitcherPressed, nullptr, DebugUiNode::kDebugLevelSelect},
  Edge<DebugUiState>{Trigger::kAlways, nullptr, UpdateDebugMessages, DebugUiNode::kDebugMessages}, //Loopback
  END_OF_EDGES(DebugUiState)
};

Edge<DebugUiState> debug_level_select[] = {
  Edge<DebugUiState>{Trigger::kAlways, DebugSwitcherPressed, nullptr, DebugUiNode::kDebugTimings},
  Edge<DebugUiState>{Trigger::kAlways, nullptr, UpdateLevelSelect, DebugUiNode::kDebugLevelSelect}, //Loopback
  END_OF_EDGES(DebugUiState)
};

Edge<DebugUiState> debug_timings[] = {
  Edge<DebugUiState>{Trigger::kAlways, DebugSwitcherPressed, nullptr, DebugUiNode::kDebugAi},
  Edge<DebugUiState>{Trigger::kAlways, nullptr, UpdateDebugTimings, DebugUiNode::kDebugTimings}, //Loopback
  END_OF_EDGES(DebugUiState)
};

Edge<DebugUiState> debug_ai[] = {
  Edge<DebugUiState>{Trigger::kAlways, DebugSwitcherPressed, nullptr, DebugUiNode::kDebugValues},
  Edge<DebugUiState>{Trigger::kAlways, nullptr, UpdateDebugAi, DebugUiNode::kDebugAi}, //Loopback
  END_OF_EDGES(DebugUiState)
};

Edge<DebugUiState> debug_values[] = {
  Edge<DebugUiState>{Trigger::kAlways, DebugSwitcherPressed, nullptr, DebugUiNode::kDebugToggles},
  Edge<DebugUiState>{Trigger::kAlways, nullptr, UpdateDebugValues, DebugUiNode::kDebugValues}, //Loopback
  END_OF_EDGES(DebugUiState)
};

Edge<DebugUiState> debug_toggles[] = {
  Edge<DebugUiState>{Trigger::kAlways, DebugSwitcherPressed, nullptr, DebugUiNode::kDebugSpawners},
  Edge<DebugUiState>{Trigger::kAlways, nullptr, UpdateDebugToggles, DebugUiNode::kDebugToggles}, //Loopback
  END_OF_EDGES(DebugUiState)
};

Edge<DebugUiState> debug_spawners[] = {
  Edge<DebugUiState>{Trigger::kAlways, DebugSwitcherPressed, nullptr, DebugUiNode::kDebugMessages},
  Edge<DebugUiState>{Trigger::kAlways, nullptr, UpdateDebugSpawners, DebugUiNode::kDebugSpawners}, //Loopback
  END_OF_EDGES(DebugUiState)
};

Node<DebugUiState> node_list[] {
  {"Init", true, init},
  {"Messages", true, debug_messages},
  {"LevelSelect", true, debug_level_select},
  {"Timing", true, debug_timings},
  {"Ai", true, debug_ai},
  {"Values", true, debug_values},
  {"Toggles", true, debug_toggles},
  {"Spawners", true, debug_spawners},
};

StateMachine<DebugUiState> machine(node_list);

}  // namespace debug_ui
