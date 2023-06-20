#pragma once

#include "sead/prim/seadSafeString.h"

const u8 MAX_HOSTNAME_LENGTH = 50;
typedef sead::FixedSafeString<MAX_HOSTNAME_LENGTH + 1> hostname;