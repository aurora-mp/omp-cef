#pragma once

#include <malloc.h>
#include <vector>

#include <shared/packet.hpp>

#include <pawn-natives/NativeFunc.hpp>
#include <pawn/source/amx/amx.h>

struct DynamicArguments
{
    std::vector<Argument> args;
};

struct EventSignature
{
    std::vector<ArgumentType> types;
};

enum AudioMode
{
    AUDIO_MODE_WORLD = 0,
    AUDIO_MODE_UI = 1
};

inline std::string GetStringFromPawn(const cell* pawn_string_data)
{
    if (pawn_string_data == nullptr)
    {
        return "";
    }

    int len = 0;
    amx_StrLen(pawn_string_data, &len);
    if (len == 0)
    {
        return "";
    }

    std::string result(len, '\0');
    amx_GetString(&result[0], pawn_string_data, 0, static_cast<size_t>(len) + 1);
    return result;
}

namespace pawn_natives
{
    template <>
    class ParamCast<DynamicArguments>
    {
    public:
        ParamCast(AMX* amx, cell* params, int idx)
        {
            const int total = params[0] / sizeof(cell);

            for (int i = idx; i + 1 <= total; i += 2)
            {
                cell* type_ptr = nullptr;
                cell* value_ptr = nullptr;

                if (amx_GetAddr(amx, params[i], &type_ptr) != AMX_ERR_NONE ||
                    amx_GetAddr(amx, params[i + 1], &value_ptr) != AMX_ERR_NONE)
                {
                    continue;
                }

                const auto type = static_cast<ArgumentType>(*type_ptr);
                const cell value = *value_ptr;

                switch (type)
                {
                    case ArgumentType::String:
                    {
                        args_.args.emplace_back(GetStringFromPawn(value_ptr));
                        break;
                    }
                    case ArgumentType::Integer:
                    {
                        args_.args.emplace_back(static_cast<int>(value));
                        break;
                    }
                    case ArgumentType::Float:
                    {
                        float f = amx_ctof(value);
                        args_.args.emplace_back(f);
                        break;
                    }
                    case ArgumentType::Bool:
                    {
                        args_.args.emplace_back(value != 0);
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        bool Error() const
        {
            return false;
        }

        operator DynamicArguments() const
        {
            return args_;
        }

        static constexpr int Size = -1;

    private:
        DynamicArguments args_;
    };

    template <>
    class ParamCast<EventSignature>
    {
    public:
        ParamCast(AMX* amx, cell* params, int idx)
        {
            const int total_args = params[0] / sizeof(cell);

            for (int i = idx; i <= total_args; ++i)
            {
                cell* addr_type = nullptr;
                if (amx_GetAddr(amx, params[i], &addr_type) == AMX_ERR_NONE)
                {
                    args_.types.push_back(static_cast<ArgumentType>(*addr_type));
                }
            }
        }

        bool Error() const
        {
            return false;
        }

        operator EventSignature() const
        {
            return args_;
        }

        static constexpr int Size = -1;

    private:
        EventSignature args_;
    };
}  // namespace pawn_natives