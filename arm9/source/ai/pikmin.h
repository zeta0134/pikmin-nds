#ifndef AI_PIKMIN_H
#define AI_PIKMIN_H

#include "ai/pikmin_game_state.h"

namespace squad_ai {
struct SquadState;
}

namespace pikmin_ai {

enum class PikminType {
  kNone,
  kRedPikmin,
  kYellowPikmin,
  kBluePikmin,
};

namespace PikminNode {
enum PikminNode {
  kInit = 0,
  kIdle,
  kGrabbed,
  kThrown,
  kTargeting,
  kChasing,
  kStandingAttack,
  kJump,
  kClimbIntoOnion,
  kSlideDownFromOnion,
  kLiftTreasure,
  kCarryTreasure,
  kSeed,
  kGrowing,
  kSprout,
  kPlucked,
};
}

struct PikminState : PikminGameState {
  PikminType type = PikminType::kRedPikmin;
  squad_ai::SquadState* current_squad{nullptr};

  //parent: used for being thrown (and later chewed?)
  Drawable* parent{nullptr};
  Vec3 parent_initial_location;
  Vec3 child_offset;

  bool has_target{false};
  Vec2 target;

  Handle active_treasure;

  Handle chase_target_body;

  Handle attack_target_body;

  //cache values for not updating so often
  numeric_types::Brads target_facing_angle;

  int starting_state{PikminNode::kIdle};
};

extern StateMachine<PikminState> machine;

}  // namespace pikmin_ai

#endif
