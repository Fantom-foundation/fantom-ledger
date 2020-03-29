#include "state.h"

// currentIns holds the current instruction being processed.
// We do not allow instruction switching in the middle for a processing chain.
int currentIns;
