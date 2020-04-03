#include "state.h"

// currentIns holds the current instruction being processed.
// We do not allow instruction switching in the middle for a processing chain.
int currentIns;

// instructionState holds the current APDU instruction context
// and state.
// We use joined structure since only one instruction can run
// at any time.
instruction_state_t instructionState;