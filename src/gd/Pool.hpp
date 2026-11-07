#pragma once
#include "../core/Plan.hpp"

namespace autodeco {

// Finds decoration objects by texture name and size at runtime. No object IDs are hardcoded;
// every candidate is verified to be a collision-free decoration before it is offered.
std::vector<Candidate> queryDecoPool(DecoRule const& rule);

}  // namespace autodeco
