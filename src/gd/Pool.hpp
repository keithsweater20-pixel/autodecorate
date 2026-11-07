#pragma once
#include "../core/Plan.hpp"

namespace autodeco {

// Counters for the last pool search, shown in the toast when nothing usable is found.
struct PoolStats { int keys = 0, noFrame = 0, noObject = 0, notDeco = 0, tooSmall = 0, verified = 0; };
PoolStats const& poolStats();

// Finds decoration objects by texture name and size at runtime. No object IDs are hardcoded;
// every candidate is verified to be a collision-free decoration before it is offered.
std::vector<Candidate> queryDecoPool(DecoRule const& rule);

}  // namespace autodeco
