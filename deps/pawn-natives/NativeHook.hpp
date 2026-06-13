#pragma once

#include <stdexcept>
#include <list>

#include <subhook/subhook.h>

#include "NativeImport.hpp"
#include "Internal/NativeCast.hpp"

#define PAWN_NATIVES_HAS_HOOK

namespace pawn_natives
{
	void Load(void **ppData);

	class NativeHookBase
	{
	public:
		void Init()
		{
			// If a known native matches the stored name, install the hook.
			//Install(NULL);
		}

		void Enable()
		{
			hook_.Install();
		}

		void Disable()
		{
			hook_.Remove();
		}

		bool IsEnabled() const
		{
			return hook_.IsInstalled();
		}

	protected:
		bool Recursing()
		{
			// Get if we are already in the native, and then flip it.
			bool
				ret = recursing_;
			recursing_ = !ret;
			return ret;
		}

		NativeHookBase(unsigned int count, char const * const name, AMX_NATIVE replacement)
		:
			count_(count * sizeof (cell)),
			name_(name),
			replacement_(replacement),
			hook_(),
			amx_(0),
			params_(0),
			recursing_(false)
		{
			if (!all_)
				all_ = new std::list<NativeHookBase *>();
			if (all_)
				all_->push_back(this);
		}

		~NativeHookBase() = default;

		subhook::Hook const & GetHook() const { return hook_; }
		subhook::Hook & GetHook() { return hook_; }

		AMX * GetAMX() const { return amx_; }
		cell * GetParams() const { return params_; }
		
		cell CallDoOuter(AMX * amx, cell * params)
		{
			cell
				ret = 0;
			if (amx && params)
			{
				// Check that there are enough parameters.
				amx_ = amx;
				params_ = params;
				recursing_ = true;
				try
				{
					if (count_ > (unsigned int)params[0])
						throw std::invalid_argument("Insufficient arguments.");
					subhook::ScopedHookRemove
						undo(&hook_);
					ret = this->CallDoInner(amx, params);
				}
				catch (std::exception & e)
				{
					char
						msg[1024];
					sprintf(msg, "Exception in %s: \"%s\"", name_, e.what());
					LOG_NATIVE_ERROR(msg);
				}
				catch (...)
				{
					char
						msg[1024];
					sprintf(msg, "Unknown exception in in %s", name_);
					LOG_NATIVE_ERROR(msg);
					recursing_ = false;
					params_ = 0;
					amx_ = 0;
					throw;
				}
				recursing_ = false;
				params_ = 0;
				amx_ = 0;
			}
			return (cell)ret;
		}

	private:
		virtual cell CallDoInner(AMX *, cell *) = 0;

		friend int AmxLoad(AMX * amx);

		NativeHookBase() = delete;
		NativeHookBase(NativeHookBase const &) = delete;
		NativeHookBase(NativeHookBase const &&) = delete;
		NativeHookBase const & operator=(NativeHookBase const &) const = delete;
		NativeHookBase const & operator=(NativeHookBase const &&) const = delete;

		unsigned int
			count_;

		char const * const
			name_;

		AMX_NATIVE const
			replacement_;

		subhook::Hook
			hook_;

		AMX *
			amx_;

		cell *
			params_;

		bool
			recursing_;

		static std::list<NativeHookBase *> *
			all_;
	};

	template <typename FUNC_TYPE>
	class NativeHook {};

	// A pretty horrible combination of templates that make hooks quite seamless.
	template <typename RET>
	class NativeHook0 : protected NativeHookBase
	{
	public:
		typedef RET (*implementation_t)();

		class ScopedCall
		{
		public:
			inline RET operator()()
			{
				return original_();
			}

			~ScopedCall()
			{
				if (removed_)
					hook_.Install();
			}

		private:
			ScopedCall(subhook::Hook & hook, implementation_t original)
				:
				hook_(hook),
				original_(original),
				removed_(hook.Remove())
			{
			}

			ScopedCall(ScopedCall const &) = default;
			ScopedCall & operator=(ScopedCall const &) = default;

			ScopedCall(ScopedCall && that)
				:
				hook_(that.hook_),
				original_(that.original_),
				removed_(that.removed_)
			{
				that.removed_ = false;
			}

			ScopedCall & operator=(ScopedCall const &&) = delete;

			ScopedCall() = delete;

			friend class NativeHook0<RET>;

			subhook::Hook &
				hook_;

			implementation_t const
				original_;

			bool
				removed_;
		};

		inline RET operator()()
		{
			RET
				ret;
			if (Recursing())
			{
				subhook::ScopedHookRemove
					undo(&GetHook());
				ret = original_();
			}
			else
			{
				ret = Do();
			}
			Recursing();
			return ret;
		}

		ScopedCall operator*()
		{
			ScopedCall
				ret(GetHook(), original_);
			return ret;
		}

	protected:
		NativeHook0(char const * const name, implementation_t original, AMX_NATIVE replacement) : NativeHookBase(0, name, replacement), original_(original) {}
		~NativeHook0() = default;

	private:
		cell CallDoInner(AMX *, cell *)
		{
			RET
				ret = this->Do();
			return *(cell *)&ret;
		}

		virtual RET Do() const = 0;

		implementation_t const
			original_;
	};

	// Template specialisation for void returns, since they can't use "return X()".
	template <>
	class NativeHook0<void> : protected NativeHookBase
	{
	public:
		typedef void (*implementation_t)();

		class ScopedCall
		{
		public:
			inline void operator()()
			{
				original_();
			}

			~ScopedCall()
			{
				if (removed_)
					hook_.Install();
			}

		private:
			ScopedCall(subhook::Hook & hook, implementation_t original)
				:
				hook_(hook),
				original_(original),
				removed_(hook.Remove())
			{
			}

			ScopedCall(ScopedCall const &) = default;
			ScopedCall & operator=(ScopedCall const &) = default;

			ScopedCall(ScopedCall && that)
				:
				hook_(that.hook_),
				original_(that.original_),
				removed_(that.removed_)
			{
				that.removed_ = false;
			}

			ScopedCall & operator=(ScopedCall const &&) = delete;

			ScopedCall() = delete;

			friend class NativeHook0<void>;

			subhook::Hook &
				hook_;

			implementation_t const
				original_;

			bool
				removed_;
		};

		inline void operator()()
		{
			if (Recursing())
			{
				subhook::ScopedHookRemove
					undo(&GetHook());
				original_();
			}
			else
			{
				Do();
			}
			Recursing();
		}

		ScopedCall operator*()
		{
			ScopedCall
				ret(GetHook(), original_);
			return ret;
		}

	protected:
		NativeHook0(char const * const name, implementation_t original, AMX_NATIVE replacement) : NativeHookBase(0, name, replacement), original_(original) {}
		~NativeHook0() = default;

	private:
		cell CallDoInner(AMX *, cell *)
		{
			this->Do();
			return 1;
		}

		virtual void Do() const = 0;

		implementation_t const
			original_;
	};

	template <typename RET>
	class NativeHook<RET()> : public NativeHook0<RET> { protected: NativeHook(char const * const name, typename NativeHook0<RET>::implementation_t original, AMX_NATIVE replacement) : NativeHook0<RET>(name, original, replacement) {} };
};

#if defined PAWN_NATIVES_STORAGE
namespace pawn_natives
{
	std::list<NativeHookBase *> *
		NativeHookBase::all_;

	void Load(void **ppData)
	{
	}
};
#endif

// Defer declaring the other classes to a super macro file.
#define PAWN_HOOK_TEMPLATE   typename A
#define PAWN_HOOK_NAME       NativeHook1
#define PAWN_HOOK_TYPES      A
#define PAWN_HOOK_PARAMETERS A a
#define PAWN_HOOK_CALLING    a
#define PAWN_HOOK_NUMBER     1
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B
#define PAWN_HOOK_NAME       NativeHook2
#define PAWN_HOOK_TYPES      A, B
#define PAWN_HOOK_PARAMETERS A a, B b
#define PAWN_HOOK_CALLING    a, b
#define PAWN_HOOK_NUMBER     2
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C
#define PAWN_HOOK_NAME       NativeHook3
#define PAWN_HOOK_TYPES      A, B, C
#define PAWN_HOOK_PARAMETERS A a, B b, C c
#define PAWN_HOOK_CALLING    a, b, c
#define PAWN_HOOK_NUMBER     3
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D
#define PAWN_HOOK_NAME       NativeHook4
#define PAWN_HOOK_TYPES      A, B, C, D
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d
#define PAWN_HOOK_CALLING    a, b, c, d
#define PAWN_HOOK_NUMBER     4
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E
#define PAWN_HOOK_NAME       NativeHook5
#define PAWN_HOOK_TYPES      A, B, C, D, E
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e
#define PAWN_HOOK_CALLING    a, b, c, d, e
#define PAWN_HOOK_NUMBER     5
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F
#define PAWN_HOOK_NAME       NativeHook6
#define PAWN_HOOK_TYPES      A, B, C, D, E, F
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f
#define PAWN_HOOK_CALLING    a, b, c, d, e, f
#define PAWN_HOOK_NUMBER     6
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G
#define PAWN_HOOK_NAME       NativeHook7
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g
#define PAWN_HOOK_NUMBER     7
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H
#define PAWN_HOOK_NAME       NativeHook8
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h
#define PAWN_HOOK_NUMBER     8
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I
#define PAWN_HOOK_NAME       NativeHook9
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H, I
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h, I i
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h, i
#define PAWN_HOOK_NUMBER     9
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J
#define PAWN_HOOK_NAME       NativeHook10
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H, I, J
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h, I i, J j
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h, i, j
#define PAWN_HOOK_NUMBER     10
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K
#define PAWN_HOOK_NAME       NativeHook11
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H, I, J, K
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h, i, j, k
#define PAWN_HOOK_NUMBER     11
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K, typename L
#define PAWN_HOOK_NAME       NativeHook12
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H, I, J, K, L
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h, i, j, k, l
#define PAWN_HOOK_NUMBER     12
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K, typename L, typename M
#define PAWN_HOOK_NAME       NativeHook13
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H, I, J, K, L, M
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h, i, j, k, l, m
#define PAWN_HOOK_NUMBER     13
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K, typename L, typename M, typename N
#define PAWN_HOOK_NAME       NativeHook14
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H, I, J, K, L, M, N
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h, i, j, k, l, m, n
#define PAWN_HOOK_NUMBER     14
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K, typename L, typename M, typename N, typename O
#define PAWN_HOOK_NAME       NativeHook15
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H, I, J, K, L, M, N, O
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o
#define PAWN_HOOK_NUMBER     15
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

#define PAWN_HOOK_TEMPLATE   typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K, typename L, typename M, typename N, typename O, typename P
#define PAWN_HOOK_NAME       NativeHook16
#define PAWN_HOOK_TYPES      A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P
#define PAWN_HOOK_PARAMETERS A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p
#define PAWN_HOOK_CALLING    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p
#define PAWN_HOOK_NUMBER     16
#include "Internal/NativeHookImpl.hpp"
#undef PAWN_HOOK_NUMBER
#undef PAWN_HOOK_CALLING
#undef PAWN_HOOK_PARAMETERS
#undef PAWN_HOOK_TYPES
#undef PAWN_HOOK_NAME
#undef PAWN_HOOK_TEMPLATE

// The hooks and calls for each class are always static, because otherwise it
// would make installing hooks MUCH harder - we would need stubs that could
// handle class pointers.  Doing that would negate needing a different class for
// every hook type, even when the parameters are the same, but this way is
// probably not much more generated code, and vastly simpler.
// 
// The inheritance from `NativeHookBase` is protected, because we don't want
// normal users getting in to that data.  However, we do want them to be able to
// use the common `IsEnabled` method, so re-export it.
#define PAWN_HOOK_DECL(nspace, func, type) PAWN_HOOK_DECL_(nspace, func, type)

#define PAWN_HOOK_DECL_(nspace, func, type) \
	PAWN_NATIVE_EXPORT PAWN_NATIVE__RETURN(type) PAWN_NATIVE_API                \
	    PAWN_NATIVE_##nspace##_##func(PAWN_NATIVE__NAMED(type));                \
	                                                                            \
	namespace nspace                                                            \
	{                                                                           \
	    class Native_##nspace##_##func :                                        \
	        public pawn_natives::NativeHook<type>                               \
	    {                                                                       \
	    public:                                                                 \
	        Native_##nspace##_##func() :                                        \
	            NativeHook<type>(#func, &sampgdk_##func, (AMX_NATIVE)&Call) {}  \
	                                                                            \
	        using NativeHookBase::IsEnabled;                                    \
	                                                                            \
	    private:                                                                \
	        friend PAWN_NATIVE_DLLEXPORT PAWN_NATIVE__RETURN(type) PAWN_NATIVE_API\
	            ::PAWN_NATIVE_##nspace##_##func(PAWN_NATIVE__NAMED(type));      \
	                                                                            \
	        static cell AMX_NATIVE_CALL                                         \
	            Call(AMX * amx, cell * params);                                 \
	                                                                            \
	        PAWN_NATIVE__RETURN(type)                                           \
	            Do(CAT(PAWN_NATIVE__WITHOUT_RETURN_, type)) const;              \
	    };                                                                      \
	                                                                            \
	    extern Native_##nspace##_##func func;                                   \
	}

// We can't pass exceptions to another module easily, so just don't...

#define PAWN_HOOK_DEFN(nspace, func, type) PAWN_HOOK_DEFN_(nspace, func, type)

#define PAWN_HOOK_DEFN_(nspace, func, type) \
	nspace::Native_##nspace##_##func nspace::func;                              \
	                                                                            \
	cell AMX_NATIVE_CALL                                                        \
	    nspace::Native_##nspace##_##func::Call(AMX * amx, cell * params)        \
	{                                                                           \
	    return ::nspace::func.CallDoOuter(amx, params);                         \
	}                                                                           \
	                                                                            \
	PAWN_NATIVE_EXPORT PAWN_NATIVE__RETURN(type) PAWN_NATIVE_API                \
	    PAWN_NATIVE_##nspace##_##func(PAWN_NATIVE__NAMED(type))                 \
	{                                                                           \
	    try                                                                     \
	    {                                                                       \
	        PAWN_NATIVE__MAYBE_RETURN(type)(::nspace::func(PAWN_NATIVE__CALLING(type))); \
	    }                                                                       \
	    catch (std::exception & e)                                              \
	    {                                                                       \
	        char                                                                \
	            msg[1024];                                                      \
	        sprintf(msg, "Exception in _" #func ": \"%s\"", e.what());          \
	        LOG_NATIVE_ERROR(msg);                                              \
	    }                                                                       \
	    catch (...)                                                             \
	    {                                                                       \
	        LOG_NATIVE_ERROR("Unknown exception in _" #func);                   \
	    }                                                                       \
	    if (!nspace::func.Recursing())                                          \
	        nspace::func.Recursing();                                           \
	    PAWN_NATIVE__DEFAULT_RETURN(type);                                      \
	}                                                                           \
	                                                                            \
	PAWN_NATIVE__RETURN(type)                                                   \
	    nspace::Native_##nspace##_##func::                                      \
	    Do(CAT(PAWN_NATIVE__WITHOUT_RETURN_, type)) const

#define PAWN_HOOK_DECLARE PAWN_HOOK_DECL
#define PAWN_HOOK_DEFINE  PAWN_HOOK_DEFN

#define PAWN_HOOK(nspace, func, type) PAWN_HOOK_DECL_(nspace, func, type); PAWN_HOOK_DEFN_(nspace, func, type)

#if 0

// Example:

// In you header:
#undef SetPlayerPos
PAWN_HOOK_DECL(fixes, SetPlayerPos, bool(int playerid, float x, float y, float z));

// In your code:
PAWN_HOOK_DEFN(fixes, SetPlayerPos, bool(int playerid, float x, float y, float z))
{
	// Implementation here...
	gLastX[playerid] = x;
	gLastY[playerid] = y;
	// No need to worry about hooks for this function - they are removed while
	// the hook is running.
	return SetPlayerPos(playerid, x, y, z);
}

#endif


