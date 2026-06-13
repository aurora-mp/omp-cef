#pragma once

#include <core.hpp>
#include <amx/amx.h>
#include <array>
#include <string>
#include <vector>

#include <Server/Components/Actors/actors.hpp>
#include <Server/Components/Checkpoints/checkpoints.hpp>
#include <Server/Components/Classes/classes.hpp>
#include <Server/Components/Console/console.hpp>
#include <Server/Components/Databases/databases.hpp>
#include <Server/Components/Dialogs/dialogs.hpp>
#include <Server/Components/Fixes/fixes.hpp>
#include <Server/Components/GangZones/gangzones.hpp>
#include <Server/Components/Menus/menus.hpp>
#include <Server/Components/Objects/objects.hpp>
#include <Server/Components/Pickups/pickups.hpp>
#include <Server/Components/Recordings/recordings.hpp>
#include <Server/Components/TextDraws/textdraws.hpp>
#include <Server/Components/TextLabels/textlabels.hpp>
#include <Server/Components/Timers/timers.hpp>
#include <Server/Components/Variables/variables.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>
#include <Server/Components/CustomModels/custommodels.hpp>
#include <Server/Components/NPCs/npcs.hpp>

#define SCRIPT_API(name, prototype) PAWN_NATIVE(openmp_scripting, name, prototype)
#define SCRIPT_API_FAILRET(name, failret, prototype) PAWN_NATIVE_FAILRET(openmp_scripting, failret, name, prototype)
#define EXTERN_API(name, prototype) PAWN_NATIVE_DECL(openmp_scripting, name, prototype)

constexpr int NUM_AMX_FUNCS = 52;

int AMXAPI amx_GetNativeByIndex(AMX const* amx, int index, AMX_NATIVE_INFO* ret);
int AMXAPI amx_MakeAddr(AMX* amx, cell* phys_addr, cell* amx_addr);
int AMXAPI amx_StrSize(const cell* cstr, int* length);

enum DefaultReturnValue
{
	DefaultReturnValue_False,
	DefaultReturnValue_True
};

struct PawnLookup
{
	ICore* core = nullptr;
	IConfig* config = nullptr;
	IPlayerPool* players = nullptr;
	IActorsComponent* actors = nullptr;
	ICheckpointsComponent* checkpoints = nullptr;
	IClassesComponent* classes = nullptr;
	IConsoleComponent* console = nullptr;
	IDatabasesComponent* databases = nullptr;
	IDialogsComponent* dialogs = nullptr;
	IGangZonesComponent* gangzones = nullptr;
	IFixesComponent* fixes = nullptr;
	IMenusComponent* menus = nullptr;
	IObjectsComponent* objects = nullptr;
	IPickupsComponent* pickups = nullptr;
	IRecordingsComponent* recordings = nullptr;
	ITextDrawsComponent* textdraws = nullptr;
	ITextLabelsComponent* textlabels = nullptr;
	ITimersComponent* timers = nullptr;
	IVariablesComponent* vars = nullptr;
	IVehiclesComponent* vehicles = nullptr;
	ICustomModelsComponent* models = nullptr;
	INPCComponent* npcs = nullptr;
};

PawnLookup* getAmxLookups();

namespace Impl
{
template <typename T>
class PawnRef
{
public:
	using value_type = T;

	PawnRef()
		: value_ {}
	{
	}
	PawnRef(T val)
		: value_(val)
	{
	}

	T get() const { return value_; }
	T& ref() { return value_; }
	const T& ref() const { return value_; }

	operator T() const { return value_; }

	PawnRef& operator=(T val)
	{
		value_ = val;
		return *this;
	}

private:
	T value_;
};

template <>
class PawnRef<String>
{
public:
	using value_type = String;

	PawnRef(size_t maxLen = 256)
		: value_ {}
		, maxLen_(maxLen)
	{
	}
	PawnRef(String val, size_t maxLen = 256)
		: value_(val)
		, maxLen_(maxLen)
	{
	}

	String get() const { return value_; }
	String& ref() { return value_; }
	const String& ref() const { return value_; }

	operator String() const { return value_; }

	PawnRef& operator=(String val)
	{
		value_ = val;
		return *this;
	}

	size_t getMaxLen() const { return maxLen_; }
	const char* c_str() const { return value_.c_str(); }

private:
	String value_;
	size_t maxLen_;
};

template <typename T>
class PawnRef<DynamicArray<T>>
{
public:
	using value_type = DynamicArray<T>;

	PawnRef(size_t maxLen = 256)
		: value_ {}
		, maxLen_(maxLen)
	{
	}
	PawnRef(const DynamicArray<T>& val, size_t maxLen = 256)
		: value_(val)
		, maxLen_(maxLen)
	{
	}
	PawnRef(DynamicArray<T>&& val, size_t maxLen = 256)
		: value_(std::move(val))
		, maxLen_(maxLen)
	{
	}

	DynamicArray<T> get() const { return value_; }
	DynamicArray<T>& ref() { return value_; }
	const DynamicArray<T>& ref() const { return value_; }

	operator DynamicArray<T>() const { return value_; }

	PawnRef& operator=(const DynamicArray<T>& val)
	{
		value_ = val;
		return *this;
	}

	PawnRef& operator=(DynamicArray<T>&& val)
	{
		value_ = std::move(val);
		return *this;
	}

	T& operator[](size_t index) { return value_[index]; }
	const T& operator[](size_t index) const { return value_[index]; }

	size_t getMaxLen() const { return maxLen_; }

	size_t size() const { return value_.size(); }
	void resize(size_t newSize) { value_.resize(newSize); }
	void clear() { value_.clear(); }
	bool empty() const { return value_.empty(); }

	typename DynamicArray<T>::iterator begin() { return value_.begin(); }
	typename DynamicArray<T>::iterator end() { return value_.end(); }
	typename DynamicArray<T>::const_iterator begin() const { return value_.begin(); }
	typename DynamicArray<T>::const_iterator end() const { return value_.end(); }

private:
	DynamicArray<T> value_;
	size_t maxLen_;
};

// Native parameter for runtime native calls
struct NativeParam
{
	enum class Type
	{
		Int,
		Float,
		Bool,
		String,
		ArrayInt,
		ArrayFloat,
		RefInt,
		RefFloat,
		RefBool,
		RefString,
		RefArrayInt,
		RefArrayFloat
	};

	Type type;
	union
	{
		int intValue;
		float floatValue;
		bool boolValue;
		const char* stringValue;
		void* refPtr;
		const void* arrayPtr;
	};
	size_t arraySize; // For string/array refs and array values

	// Value constructors
	NativeParam(int v)
		: type(Type::Int)
		, intValue(v)
		, arraySize(0)
	{
	}
	NativeParam(unsigned int v)
		: type(Type::Int)
		, intValue(int(v))
		, arraySize(0)
	{
	}
	NativeParam(float v)
		: type(Type::Float)
		, floatValue(v)
		, arraySize(0)
	{
	}
	NativeParam(bool v)
		: type(Type::Bool)
		, boolValue(v)
		, arraySize(0)
	{
	}
	NativeParam(const char* v)
		: type(Type::String)
		, stringValue(v)
		, arraySize(0)
	{
	}
	NativeParam(const String& v)
		: type(Type::String)
		, stringValue(v.c_str())
		, arraySize(0)
	{
	}
	NativeParam(const DynamicArray<int>& arr)
		: type(Type::ArrayInt)
		, arrayPtr(arr.data())
		, arraySize(arr.size())
	{
	}
	NativeParam(const DynamicArray<float>& arr)
		: type(Type::ArrayFloat)
		, arrayPtr(arr.data())
		, arraySize(arr.size())
	{
	}

	// Reference parameter constructors
	NativeParam(PawnRef<int>& ref)
		: type(Type::RefInt)
		, refPtr(&ref)
		, arraySize(0)
	{
	}
	NativeParam(PawnRef<float>& ref)
		: type(Type::RefFloat)
		, refPtr(&ref)
		, arraySize(0)
	{
	}
	NativeParam(PawnRef<bool>& ref)
		: type(Type::RefBool)
		, refPtr(&ref)
		, arraySize(0)
	{
	}
	NativeParam(PawnRef<String>& ref)
		: type(Type::RefString)
		, refPtr(&ref)
		, arraySize(ref.getMaxLen())
	{
	}
	NativeParam(PawnRef<DynamicArray<int>>& ref)
		: type(Type::RefArrayInt)
		, refPtr(&ref)
		, arraySize(ref.getMaxLen())
	{
	}
	NativeParam(PawnRef<DynamicArray<float>>& ref)
		: type(Type::RefArrayFloat)
		, refPtr(&ref)
		, arraySize(ref.getMaxLen())
	{
	}
};

} // namespace Impl

struct IPawnScript
{
	// Wrap the AMX API.
	virtual int Allot(int cells, cell* amx_addr, cell** phys_addr) = 0;
	virtual int Callback(cell index, cell* result, const cell* params) = 0;
	virtual int Cleanup() = 0;
	virtual int Clone(AMX* amxClone, void* data) const = 0;
	virtual int Exec(cell* retval, int index) = 0;
	virtual int FindNative(char const* name, int* index) const = 0;
	virtual int FindPublic(char const* funcname, int* index) const = 0;
	virtual int FindPubVar(char const* varname, cell* amx_addr) const = 0;
	virtual int FindTagId(cell tag_id, char* tagname) const = 0;
	virtual int Flags(uint16_t* flags) const = 0;
	virtual int GetAddr(cell amx_addr, cell** phys_addr) const = 0;
	virtual int GetNative(int index, char* funcname) const = 0;
	virtual int GetNativeByIndex(int index, AMX_NATIVE_INFO* ret) const = 0;
	virtual int GetPublic(int index, char* funcname) const = 0;
	virtual int GetPubVar(int index, char* varname, cell* amx_addr) const = 0;
	virtual int GetString(char const* dest, const cell* source, bool use_wchar, size_t size) const = 0;
	virtual int GetString(char* dest, const cell* source, bool use_wchar, size_t size) = 0;
	virtual int GetTag(int index, char* tagname, cell* tag_id) const = 0;
	virtual int GetUserData(long tag, void** ptr) const = 0;
	virtual int Init(void* program) = 0;
	virtual int InitJIT(void* reloc_table, void* native_code) = 0;
	virtual int MakeAddr(cell* phys_addr, cell* amx_addr) const = 0;
	virtual int MemInfo(long* codesize, long* datasize, long* stackheap) const = 0;
	virtual int NameLength(int* length) const = 0;
	virtual AMX_NATIVE_INFO* NativeInfo(char const* name, AMX_NATIVE func) const = 0;
	virtual int NumNatives(int* number) const = 0;
	virtual int NumPublics(int* number) const = 0;
	virtual int NumPubVars(int* number) const = 0;
	virtual int NumTags(int* number) const = 0;
	virtual int Push(cell value) = 0;
	virtual int PushArray(cell* amx_addr, cell** phys_addr, const cell array[], int numcells) = 0;
	virtual int PushString(cell* amx_addr, cell** phys_addr, StringView string, bool pack, bool use_wchar) = 0;
	virtual int RaiseError(int error) = 0;
	virtual int Register(const AMX_NATIVE_INFO* nativelist, int number) = 0;

	// Don't forget:
	//
	//   using IPawnScript::Register;
	//
	// In inheriting classes.
	inline int Register(char const* name, AMX_NATIVE func)
	{
		AMX_NATIVE_INFO nativelist = { name, func };
		return Register(&nativelist, 1);
	}

	virtual int Release(cell amx_addr) = 0;
	virtual int SetCallback(AMX_CALLBACK callback) = 0;
	virtual int SetDebugHook(AMX_DEBUG debug) = 0;
	virtual int SetString(cell* dest, StringView source, bool pack, bool use_wchar, size_t size) const = 0;
	virtual int SetUserData(long tag, void* ptr) = 0;
	virtual int StrLen(const cell* cstring, int* length) const = 0;
	virtual int StrSize(const cell* cstr, int* length) const = 0;
	virtual int UTF8Check(char const* string, int* length) const = 0;
	virtual int UTF8Get(char const* string, char const** endptr, cell* value) const = 0;
	virtual int UTF8Len(const cell* cstr, int* length) const = 0;
	virtual int UTF8Put(char* string, char** endptr, int maxchars, cell value) const = 0;

	virtual cell GetCIP() const = 0;
	virtual cell GetHEA() const = 0;
	virtual cell GetSTP() const = 0;
	virtual cell GetSTK() const = 0;
	virtual cell GetHLW() const = 0;
	virtual cell GetFRM() const = 0;

	virtual void SetCIP(cell v) = 0;
	virtual void SetHEA(cell v) = 0;
	virtual void SetSTP(cell v) = 0;
	virtual void SetSTK(cell v) = 0;
	virtual void SetHLW(cell v) = 0;
	virtual void SetFRM(cell v) = 0;

	virtual AMX* GetAMX() = 0;

	virtual void PrintError(int err) = 0;

	virtual int GetID() const = 0;
	virtual bool IsLoaded() const = 0;

	/// Call a native with an array of type-erased parameters
	/// @param name The name of the native function to call
	/// @param params Span of NativeParam objects containing the parameters
	/// @return The cell value returned by the native (use amx_ctof for float returns)
	virtual cell CallNativeArray(const char* name, Span<Impl::NativeParam> params) = 0;

	/// Templated wrapper around CallNativeArray for convenient SDK usage, with return type.
	/// Automatically converts variadic arguments to NativeParam array.
	/// @param name The name of the native function to call
	/// @param args Variadic arguments of supported types
	/// @return The return value converted to type RET
	template <typename RET = cell, typename... Args>
	RET CallNative(const char* name, Args&&... args)
	{
		cell result = 0;
		if constexpr (sizeof...(args) == 0)
		{
			result = CallNativeArray(name, Span<Impl::NativeParam>());
		}
		else
		{
			Impl::DynamicArray<Impl::NativeParam> params;
			params.reserve(sizeof...(args));
			(params.push_back(Impl::NativeParam(std::forward<Args>(args))), ...);
			result = CallNativeArray(name, Span<Impl::NativeParam>(params.data(), params.size()));
		}

		if constexpr (std::is_same_v<RET, float>)
		{
			return amx_ctof(result);
		}
		else if constexpr (std::is_same_v<RET, bool>)
		{
			return result != 0;
		}
		else
		{
			return static_cast<RET>(result);
		}
	}

	template <typename... T>
	void Call(cell& ret, int idx, T... args)
	{
		// Check if the public exists.
		if (idx == INT_MAX)
		{
			return;
		}
		int err = CallChecked(idx, ret, args...);
		// Step 1: Try call a crashdetect-like callback, but don't get caught in a loop.

		// Step 2: Print it.
		if (err != AMX_ERR_NONE)
		{
			PrintError(err);
		}
	}

	template <typename... T>
	cell Call(char const* name, DefaultReturnValue defaultRetValue, T... args)
	{
		int idx;
		cell ret = defaultRetValue;
		if (!FindPublic(name, &idx))
		{
			Call(ret, idx, args...);
		}
		return ret;
	}

	template <typename... T>
	inline cell Call(std::string const& name, DefaultReturnValue defaultRetValue, T... args)
	{
		return Call(name.c_str(), defaultRetValue, args...);
	}

	// Call a function using an idx we know is correct.
	template <typename... T>
	int CallChecked(int idx, cell& ret, T... args)
	{
		cell amx_addr = GetHEA();
		// Push all the arguments, using templates to resolve the correct function to use.
		int err = PushOne(args...);
		if (err == AMX_ERR_NONE)
			err = Exec(&ret, idx);
		// Release everything at once.  Technically all that `Release` does is reset the heap back
		// to where it was before the call to any memory-allocating functions.  You could do that
		// for every allocating parameter in reverse order, or just do it once for all of them together.
		Release(amx_addr);
		return err;
	}

	inline int PushOne()
	{
		return AMX_ERR_NONE;
	}

	template <typename O, typename... T>
	inline int PushOne(O arg, T... args)
	{
		int ret = PushOne(args...);
		if (ret == AMX_ERR_NONE)
			return Push((cell)arg);
		return ret;
	}

	template <typename... T>
	inline int PushOne(float arg, T... args)
	{
		int ret = PushOne(args...);
		if (ret == AMX_ERR_NONE)
			return Push(amx_ftoc(arg));
		return ret;
	}

	template <typename... T>
	inline int PushOne(double arg, T... args)
	{
		int ret = PushOne(args...);
		if (ret == AMX_ERR_NONE)
		{
			float a = (float)arg;
			return Push(amx_ftoc(a));
		}
		return ret;
	}

	// TL;DR: BAD
	template <typename... T>
	inline int PushOne(char* arg, T... args) = delete;

	template <typename... T>
	inline int PushOne(StringView arg, T... args)
	{
		int ret = PushOne(args...);
		if (ret == AMX_ERR_NONE)
			return PushString(nullptr, nullptr, arg, false, false);
		return ret;
	}

	template <typename U, size_t N, typename... T>
	inline int PushOne(StaticArray<U, N> const& arg, T... args)
	{
		int ret = PushOne(args...);
		if (ret == AMX_ERR_NONE)
			return PushArray(nullptr, nullptr, arg.data(), arg.size());
		return ret;
	}

	template <typename U, typename... T>
	inline int PushOne(std::vector<U> const& arg, T... args)
	{
		int ret = PushOne(args...);
		if (ret == AMX_ERR_NONE)
			return PushArray(nullptr, nullptr, arg.data(), arg.size());
		return ret;
	}
};

struct PawnEventHandler
{
	virtual void onAmxLoad(IPawnScript& script) = 0;
	virtual void onAmxUnload(IPawnScript& script) = 0;
};

static const UID PawnComponent_UID = UID(0x78906cd9f19c36a6);
struct IPawnComponent : public IComponent
{
	PROVIDE_UID(PawnComponent_UID);

	/// Get the PawnEventHandler event dispatcher
	virtual IEventDispatcher<PawnEventHandler>& getEventDispatcher() = 0;

	virtual const StaticArray<void*, NUM_AMX_FUNCS>& getAmxFunctions() const = 0;
	virtual IPawnScript const* getScript(AMX* amx) const = 0;
	virtual IPawnScript* getScript(AMX* amx) = 0;

	/// Get a set of all the available scripts.
	virtual IPawnScript* mainScript() = 0;
	virtual const Span<IPawnScript*> sideScripts() = 0;
};
