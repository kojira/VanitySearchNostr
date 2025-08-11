// Optimized Nostr npub pattern matching implementation - simplified for inline functions
#include "NostrOptimized.h"
#include <cstring>

// Only keep helper functions that are not inlined
void NostrOptimized::xCoordToBech32Bits(const Point& p, uint8_t bits[32]) {
    // Extract X coordinate as 32 bytes (cast away const for compatibility)
    const_cast<Int&>(p.x).Get32Bytes(bits);
}

// All other functions are now inline in the header file