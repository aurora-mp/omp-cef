#pragma once

#include <stdexcept>
#include <list>

#include "NativeImport.hpp"
#include "Internal/NativeCast.hpp"

#define PAWN_NATIVES_HAS_FUNC

namespace pawn_natives
{
	int AmxLoad(AMX * amx);

	class NativeFuncBase
	{
	public:
		void Init()
		{
			// Install this as a native.
			//Install(NULL);
		}

	protected:
		NativeFuncBase(unsigned int count, char const * const name, AMX_NATIVE native)
		:
			count_(count * sizeof (cell)),
			name_(name),
			native_(native),
			amx_(0),
			params_(0)
		{
			if (!all_)
				all_ = new std::list<NativeFuncBase *>();
			if (all_)
				all_->push_back(this);
		}
		
		~NativeFuncBase() = default;

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
				try
				{
					if (count_ > (unsigned int)params[0])
						throw std::invalid_argument("Insufficient arguments.");
					ret = this->CallDoInner(amx, params);
				}
				catch (ParamCastFailure const &)
				{
					// Acceptable failure (lookup failed etc.)
				}
				catch (std::exception const & e)
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
					params_ = 0;
					amx_ = 0;
					throw;
				}
				params_ = 0;
				amx_ = 0;
			}
			return (cell)ret;
		}

	private:
		virtual cell CallDoInner(AMX *, cell *) = 0;

		friend int AmxLoad(AMX * amx);

		NativeFuncBase() = delete;
		NativeFuncBase(NativeFuncBase const &) = delete;
		NativeFuncBase(NativeFuncBase const &&) = delete;
		NativeFuncBase const & operator=(NativeFuncBase const &) const = delete;
		NativeFuncBase const & operator=(NativeFuncBase const &&) const = delete;

		unsigned int
			count_;

		char const * const
			name_;

		AMX_NATIVE const
			native_;

		AMX *
			amx_;

		cell *
			params_;

		static std::list<NativeFuncBase *> *
			all_;
	};

	template <typename RET, typename ... TS>
	class NativeFunc : protected NativeFuncBase
	{
	public:
		virtual RET operator()(TS ...) const = 0;

	protected:
		NativeFunc(char const * const name, AMX_NATIVE native) : NativeFuncBase(ParamData<TS ...>::Sum(), name, native) {}
		~NativeFunc() = default;

	private:

		cell CallDoInner(AMX * amx, cell * params)
		{
			RET
				ret = ParamData<TS ...>::Call(this, amx, params);
			return *(cell *)&ret;
		}
	};

	template <typename ... TS>
	class NativeFunc<void, TS ...> : protected NativeFuncBase
	{
	public:
		virtual void operator()(TS ...) const = 0;

	protected:
		NativeFunc(char const * const name, AMX_NATIVE native) : NativeFuncBase(ParamData<TS ...>::Sum(), name, native) {}
		~NativeFunc() = default;

	private:
		cell CallDoInner(AMX * amx, cell * params)
		{
			ParamData<TS ...>::Call(this, amx, params);
			return 0;
		}
	};

	template <typename RET>
	class NativeFunc<RET> : protected NativeFuncBase
	{
	public:
		virtual RET operator()() const = 0;

	protected:
		NativeFunc(char const * const name, AMX_NATIVE native) : NativeFuncBase(0, name, native) {}
		~NativeFunc() = default;

	private:
		cell CallDoInner(AMX * amx, cell * params)
		{
			RET
				ret = ParamData<>::Call(this, amx, params);
			return *(cell *)&ret;
		}
	};

	template <>
	class NativeFunc<void> : protected NativeFuncBase
	{
	public:
		virtual void operator()() const = 0;

	protected:
		NativeFunc(char const * const name, AMX_NATIVE native) : NativeFuncBase(0, name, native) {}
		~NativeFunc() = default;

	private:
		cell CallDoInner(AMX * amx, cell * params)
		{
			ParamData<>::Call(this, amx, params);
			return 0;
		}
	};
}

// The hooks and calls for each class are always static, because otherwise it
// would make installing hooks MUCH harder - we would need stubs that could
// handle class pointers.  Doing that would negate needing a different class for
// every hook type, even when the parameters are the same, but this way is
// probably not much more generated code, and vastly simpler.
// 
// The inheritance from `NativeFuncBase` is protected, because we don't want
// normal users getting in to that data.  However, we do want them to be able to
// use the common `IsEnabled` method, so re-export it.
#define PAWN_NATIVE_DECL(object, func, type) PAWN_NATIVE_DECL_(object, func, type)

#define PAWN_NATIVE_DECL_(object, func, params) \
	template <typename F>                                                       \
	class Native_##func##_ {};                                                  \
	                                                                            \
	template <typename RET, typename ... TS>                                    \
	class Native_##func##_<RET(TS ...)> :                                       \
	    public pawn_natives::NativeFunc<RET, TS ...>                            \
	{                                                                           \
	public:                                                                     \
	    Native_##func##_()                                                      \
	    :                                                                       \
	        pawn_natives::NativeFunc<RET, TS ...>(#func, (AMX_NATIVE)&Call)     \
	    {                                                                       \
	    }                                                                       \
	                                                                            \
	    RET operator()(TS ...) const override;                                  \
	                                                                            \
	private:                                                                    \
	    static cell AMX_NATIVE_CALL Call(AMX * amx, cell * args);               \
	};                                                                          \
	                                                                            \
	template class Native_##func##_<params>;                                    \
	using Native_##func = Native_##func##_<params>;                             \
	                                                                            \
	extern Native_##func func

// We can't pass exceptions to another module easily, so just don't...

#define PAWN_NATIVE_DEFN(object, func, params) PAWN_NATIVE_DEFN_(object, func, params)

#define PAWN_NATIVE_DEFN_(object, func, params) \
	Native_##func func;                                                         \
	                                                                            \
	template <>                                                                 \
	cell AMX_NATIVE_CALL Native_##func::Call(AMX * amx, cell * args)            \
	{                                                                           \
	    return func.CallDoOuter(amx, args);                                     \
	}                                                                           \
	                                                                            \
	template <>                                                                 \
	PAWN_NATIVE__RETURN(params)                                                 \
	    Native_##func::                                                         \
	    operator()(PAWN_NATIVE__PARAMETERS(params)) const;                      \
	                                                                            \
	template <typename RET, typename ... TS>                                    \
	typename pawn_natives::ReturnResolver<RET>::type NATIVE_##func(TS ... args) \
	{                                                                           \
	    try                                                                     \
	    {                                                                       \
	        PAWN_NATIVE__GET_RETURN(params)(func(args ...));                    \
	    }                                                                       \
	    catch (std::exception & e)                                              \
	    {                                                                       \
	        char msg[1024];                                                     \
	        sprintf(msg, "Exception in _" #func ": \"%s\"", e.what());          \
	        LOG_NATIVE_ERROR(msg);                                              \
	    }                                                                       \
	    catch (...)                                                             \
	    {                                                                       \
	        LOG_NATIVE_ERROR("Unknown exception in _" #func);                   \
	    }                                                                       \
	    PAWN_NATIVE__DEFAULT_RETURN(params);                                    \
	}                                                                           \
	                                                                            \
	PAWN_NATIVE_EXTERN template PAWN_NATIVE_DLLEXPORT                           \
	typename pawn_natives::ReturnResolver<PAWN_NATIVE__RETURN(params)>::type    \
	PAWN_NATIVE_API                                                             \
	    NATIVE_##func<PAWN_NATIVE__RETURN(params)>(PAWN_NATIVE__PARAMETERS(params)); \
	                                                                            \
	template <>                                                                 \
	PAWN_NATIVE__RETURN(params)                                                 \
	    Native_##func::                                                         \
	    operator()(PAWN_NATIVE__PARAMETERS(params)) const

#define PAWN_NATIVE_DECLARE PAWN_NATIVE_DECL
#define PAWN_NATIVE_DEFINE  PAWN_NATIVE_DEFN

#define PAWN_NATIVE(object, func, params) PAWN_NATIVE_DECL_(object, func, params); PAWN_NATIVE_DEFN_(object, func, params)


#define PAWN_METHOD_DECL(object, func, type) PAWN_METHOD_DECL_(object, func, type)

#define PAWN_METHOD_DECL_(object, func, params) \
	template <typename F>                                                       \
	class Native_##object##_##func##_ {};                                       \
	                                                                            \
	template <typename RET, class C, typename ... TS>                           \
	class Native_##object##_##func##_<RET(C::*)(TS ...)> :                      \
	    public pawn_natives::NativeFunc<RET, std::shared_ptr<C>, TS ...>        \
	{                                                                           \
	public:                                                                     \
	    Native_##object##_##func##_()                                           \
	    :                                                                       \
	        pawn_natives::NativeFunc<RET, std::shared_ptr<C>, TS ...>(#object "_" #func, (AMX_NATIVE)&Call) \
	    {                                                                       \
	    }                                                                       \
	                                                                            \
	    RET operator()(std::shared_ptr<C> c, TS... args) const override         \
        {																		\
			PAWN_NATIVE__GET_RETURN(params)((*c.*P)(std::forward<TS>(args)...));\
        }																		\
	                                                                            \
	private:                                                                    \
	    static cell AMX_NATIVE_CALL Call(AMX * amx, cell * args);               \
	    using method_type = RET(C::*)(TS ...);                                  \
	    static constexpr method_type P = (method_type)(&object::func);          \
	};                                                                          \
	                                                                            \
	template class Native_##object##_##func##_<PAWN_NATIVE__RETURN(params)(object::*)(PAWN_NATIVE__PARAMETERS(params))>; \
	using Native_##object##_##func = Native_##object##_##func##_<PAWN_NATIVE__RETURN(params)(object::*)(PAWN_NATIVE__PARAMETERS(params))>; \
	                                                                            \
	extern Native_##object##_##func func

#define PAWN_NATIVE_ADD_POINTER(object, ...) std::shared_ptr<object> c, ##__VA_ARGS__

// We can't pass exceptions to another module easily, so just don't...

#define PAWN_METHOD_DEFN(object, func, params) PAWN_METHOD_DEFN_(object, func, params)

#define PAWN_METHOD_DEFN_(object, func, params) \
	Native_##object##_##func func;                                              \
	                                                                            \
	template <>                                                                 \
	cell AMX_NATIVE_CALL Native_##object##_##func::Call(AMX * amx, cell * args) \
	{                                                                           \
	    return func.CallDoOuter(amx, args);                                     \
	}                                                                           \
	                                                                            \
	template <typename RET, class C, typename ... TS>                           \
	typename pawn_natives::ReturnResolver<RET>::type NATIVE_##object##_##func(std::shared_ptr<C> c, TS ... args) \
	{                                                                           \
	    try                                                                     \
	    {                                                                       \
	        PAWN_NATIVE__GET_RETURN(params)(func(c, std::forward<TS>(args)...));\
	    }                                                                       \
	    catch (std::exception & e)                                              \
	    {                                                                       \
	        char msg[1024];                                                     \
	        sprintf(msg, "Exception in _" #object "_" #func ": \"%s\"", e.what()); \
	        LOG_NATIVE_ERROR(msg);                                              \
	    }                                                                       \
	    catch (...)                                                             \
	    {                                                                       \
	        LOG_NATIVE_ERROR("Unknown exception in _" #object "_" #func);       \
	    }                                                                       \
	    PAWN_NATIVE__DEFAULT_RETURN(params);                                    \
	}                                                                           \
	                                                                            \
	PAWN_NATIVE_EXTERN template PAWN_NATIVE_DLLEXPORT                           \
	typename pawn_natives::ReturnResolver<PAWN_NATIVE__RETURN(params)>::type    \
	PAWN_NATIVE_API                                                             \
	    NATIVE_##object##_##func<PAWN_NATIVE__RETURN(params)>(DEFER(PAWN_NATIVE_ADD_POINTER)(object, PAWN_NATIVE__PARAMETERS(params))); \
	                                                                            \
	PAWN_NATIVE__RETURN(params)                                                 \
	    object::func(PAWN_NATIVE__PARAMETERS(params))

#define PAWN_METHOD_DECLARE PAWN_METHOD_DECL
#define PAWN_METHOD_DEFINE  PAWN_METHOD_DEFN

#define PAWN_METHOD(object, func, params) PAWN_METHOD_DECL_(object, func, params); PAWN_METHOD_DEFN_(object, func, params)

#if 0

// Example:

// In your header:
PAWN_NATIVE_DECL(SetPlayerPosAndAngle, bool(int playerid, float x, float y, float z, float a));

// In your code:
PAWN_NATIVE_DEFN(SetPlayerPosAndAngle, bool(int playerid, float x, float y, float z, float a))
{
	// Implementation here...
	SetPlayerPos(playerid, x, y, z);
	return SetPlayerFacingAngle(playerid, a);
}

#endif


