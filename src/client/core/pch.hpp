#pragma once

// Standard
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

// Dependencies
#include <nlohmann/json.hpp>

// Shared
#include <shared/packet.hpp>

// Project
#include "samp/components/component_registry.hpp"
#include "system/logger.hpp"
#include "hooks/hook_manager.hpp"