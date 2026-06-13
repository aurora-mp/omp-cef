/*
 *  This Source Code Form is subject to the terms of the Mozilla Public License,
 *  v. 2.0. If a copy of the MPL was not distributed with this file, You can
 *  obtain one at http://mozilla.org/MPL/2.0/.
 *
 *  The original code is copyright (c) 2025, open.mp team and contributors.
 */

#pragma once
#include <sdk.hpp>

inline int getConfigOptionAsInt(ICore* core, const Impl::String& cvar)
{
	if (!core)
	{
		return 0;
	}

	IConfig* config = &core->getConfig();
	auto res = config->getNameFromAlias(cvar);
	bool* booleanVar = nullptr;
	int* integerVar = nullptr;
	if (!res.second.empty())
	{
		if (res.first)
		{
			core->logLn(LogLevel::Warning, "Deprecated config/console variable \"%s\", use \"%.*s\" instead.", cvar.c_str(), PRINT_VIEW(res.second));
		}
		if (!(integerVar = config->getInt(res.second)))
		{
			booleanVar = config->getBool(res.second);
		}
	}
	else
	{
		if (!(integerVar = config->getInt(cvar)))
		{
			booleanVar = config->getBool(cvar);
		}
	}
	if (integerVar)
	{
		return *integerVar;
	}
	else if (booleanVar)
	{
		core->logLn(LogLevel::Warning, "Boolean config/console variable \"%s\" retreived as integer.", cvar.c_str());
		return *booleanVar;
	}
	else
	{
		return 0;
	}
}

inline bool getConfigOptionAsBool(ICore* core, const Impl::String& cvar)
{
	if (!core)
	{
		return false;
	}

	IConfig* config = &core->getConfig();
	auto res = config->getNameFromAlias(cvar);
	bool* booleanVar = nullptr;
	int* integerVar = nullptr;
	if (!res.second.empty())
	{
		if (res.first)
		{
			core->logLn(LogLevel::Warning, "Deprecated config/console variable \"%s\", use \"%.*s\" instead.", cvar.c_str(), PRINT_VIEW(res.second));
		}
		if (!(booleanVar = config->getBool(res.second)))
		{
			integerVar = config->getInt(res.second);
		}
	}
	else
	{
		if (!(booleanVar = config->getBool(cvar)))
		{
			integerVar = config->getInt(cvar);
		}
	}
	if (booleanVar)
	{
		return *booleanVar;
	}
	else if (integerVar)
	{
		core->logLn(LogLevel::Warning, "Integer config/console variable \"%s\" retreived as boolean.", cvar.c_str());
		return *integerVar != 0;
	}
	else
	{
		return false;
	}
}

inline float getConfigOptionAsFloat(ICore* core, const Impl::String& cvar)
{
	if (!core)
	{
		return 0.0f;
	}

	IConfig* config = &core->getConfig();
	auto res = config->getNameFromAlias(cvar);
	float* var = nullptr;
	if (!res.second.empty())
	{
		if (res.first)
		{
			core->logLn(LogLevel::Warning, "Deprecated config/console variable \"%s\", use \"%.*s\" instead.", cvar.c_str(), PRINT_VIEW(res.second));
		}
		var = config->getFloat(res.second);
	}
	else
	{
		var = config->getFloat(cvar);
	}
	if (var)
	{
		return *var;
	}
	else
	{
		return 0.0f;
	}
}

inline int getConfigOptionAsString(ICore* core, const Impl::String& cvar, Impl::String& buffer)
{
	if (!core)
	{
		return 0;
	}

	// Special case, converting `gamemode0` to `pawn.main_scripts[0]`.  It is the only string to
	// array change.
	IConfig* config = &core->getConfig();
	bool gm = cvar.substr(0, 8) == "gamemode";
	auto res = config->getNameFromAlias(gm ? "gamemode" : cvar);
	if (!res.second.empty())
	{
		if (res.first)
		{
			core->logLn(LogLevel::Warning, "Deprecated config/console variable \"%s\", use \"%.*s\" instead.", cvar.c_str(), PRINT_VIEW(res.second));
		}
		if (gm)
		{
			size_t i = std::stoi("0" + cvar.substr(8));
			Impl::DynamicArray<StringView> mainScripts(i + 1);
			size_t n = config->getStrings(res.second, Span<StringView>(mainScripts.data(), mainScripts.size()));
			if (i < n)
			{
				buffer = mainScripts[i].data();
			}
		}
		else
		{
			buffer = config->getString(res.second).data();
		}
	}
	else
	{
		const auto& data = config->getString(cvar).data();

		// If key is invalid (no such variable exists), getString returns an uninitialized StringView.
		// So we need to check validity before copying data.

		if (data != nullptr)
		{
			buffer = data;
		}
	}
	return buffer.length();
}
