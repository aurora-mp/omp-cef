#include "include/cef_app.h"
#include "include/cef_render_process_handler.h"
#include "include/wrapper/cef_helpers.h"

#include <algorithm>
#include <map>
#include <string>
#include <variant>
#include <vector>

// Global map to store event names and their callback functions
static std::map<std::string, std::vector<CefRefPtr<CefV8Value>>> events_;

// Queue events that arrive before JS registers a handler (fixes race between server emit and cef.on)
using PendingArg  = std::variant<std::monostate, bool, int, double, CefString>;
using PendingArgs = std::vector<PendingArg>;
static std::map<std::string, std::vector<PendingArgs>> pending_events_;

static bool g_chat_input_open = false;

static CefV8ValueList BuildJsArgs(const PendingArgs& pending)
{
    CefV8ValueList jsArgs;
    jsArgs.reserve(pending.size());

    for (const auto& arg : pending)
    {
        if (std::holds_alternative<bool>(arg))
            jsArgs.push_back(CefV8Value::CreateBool(std::get<bool>(arg)));
        else if (std::holds_alternative<int>(arg))
            jsArgs.push_back(CefV8Value::CreateInt(std::get<int>(arg)));
        else if (std::holds_alternative<double>(arg))
            jsArgs.push_back(CefV8Value::CreateDouble(std::get<double>(arg)));
        else if (std::holds_alternative<CefString>(arg))
            jsArgs.push_back(CefV8Value::CreateString(std::get<CefString>(arg)));
        else
            jsArgs.push_back(CefV8Value::CreateNull());
    }

    return jsArgs;
}

static PendingArgs CapturePendingArgs(CefRefPtr<CefListValue> args)
{
    PendingArgs out;
    if (!args) 
        return out;

    const auto size = args->GetSize();
    if (size <= 1) 
        return out;

    out.reserve(size - 1);

    for (size_t i = 1; i < size; ++i)
    {
        switch (args->GetType(i))
        {
            case VTYPE_BOOL:
                out.emplace_back(args->GetBool(i));
                break;
            case VTYPE_INT:
                out.emplace_back(args->GetInt(i));
                break;
            case VTYPE_DOUBLE:
                out.emplace_back(args->GetDouble(i));
                break;
            case VTYPE_STRING:
                out.emplace_back(args->GetString(i));
                break;
            default:
                out.emplace_back(std::monostate{});
                break;
        }
    }

    return out;
}

static void FlushPendingEvents(const std::string& eventName)
{
    auto pit = pending_events_.find(eventName);
    if (pit == pending_events_.end()) 
        return;

    auto it = events_.find(eventName);
    if (it == events_.end() || it->second.empty()) 
        return;

    for (const auto& pending : pit->second)
    {
        CefV8ValueList jsArgs = BuildJsArgs(pending);
        for (const auto& cb : it->second)
        {
            if (cb) cb->ExecuteFunction(nullptr, jsArgs);
        }
    }

    pending_events_.erase(pit);
}

class V8HandlerImpl : public CefV8Handler
{
public:
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override
    {
        if (name == "emit")
        {
            if (arguments.size() < 1 || !arguments[0]->IsString())
            {
                exception = "Invalid arguments to cef.emit(eventName, ...args)";
                return true;
            }

            CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("emit_event");
            CefRefPtr<CefListValue> list = msg->GetArgumentList();
            list->SetString(0, arguments[0]->GetStringValue());

            for (size_t i = 1; i < arguments.size(); ++i)
            {
                auto arg = arguments[i];

                if (arg->IsBool())
                    list->SetBool(i, arg->GetBoolValue());
                else if (arg->IsInt())
                    list->SetInt(i, arg->GetIntValue());
                else if (arg->IsDouble())
                    list->SetDouble(i, arg->GetDoubleValue());
                else if (arg->IsString())
                    list->SetString(i, arg->GetStringValue());
                else
                    list->SetNull(i);
            }

            CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
            return true;
        }
        else if (name == "on")
        {
            if (arguments.size() != 2 || !arguments[0]->IsString() || !arguments[1]->IsFunction())
            {
                exception = "Invalid arguments to cef.on(eventName, callback)";
                return true;
            }

            const std::string eventName = arguments[0]->GetStringValue();
            CefRefPtr<CefV8Value> callback = arguments[1];

            // Store the callback function to be called later
            events_[eventName].push_back(callback);

            // Flush queued events for this eventName (if any)
            FlushPendingEvents(eventName);
            return true;
        }
        else if (name == "off")
        {
            // cef.off() - clear everything
            if (arguments.size() == 0)
            {
                events_.clear();
                pending_events_.clear();
                return true;
            }

            // cef.off(eventName)
            if (arguments.size() == 1)
            {
                if (!arguments[0]->IsString())
                {
                    exception = "Invalid arguments to cef.off(eventName[, callback])";
                    return true;
                }

                const std::string eventName = arguments[0]->GetStringValue();
                events_.erase(eventName);
                pending_events_.erase(eventName);
                return true;
            }

            // cef.off(eventName, callback)
            if (arguments.size() == 2)
            {
                if (!arguments[0]->IsString() || !arguments[1]->IsFunction())
                {
                    exception = "Invalid arguments to cef.off(eventName, callback)";
                    return true;
                }

                const std::string eventName = arguments[0]->GetStringValue();
                CefRefPtr<CefV8Value> callback = arguments[1];

                auto it = events_.find(eventName);
                if (it == events_.end())
                    return true;

                auto& vec = it->second;
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(),
                        [&](const CefRefPtr<CefV8Value>& cb) {
                            return cb && cb->IsSame(callback);
                        }),
                    vec.end()
                );

                if (vec.empty())
                    events_.erase(it);

                return true;
            }

            exception = "Invalid arguments to cef.off(eventName[, callback])";
            return true;
        }
        else if (name == "set_focus")
        {
            if (arguments.size() < 1 || !arguments[0]->IsBool())
            {
                exception = "Invalid arguments to cef.set_focus(hasFocus: boolean)";
                return true;
            }

            const bool hasFocus = arguments[0]->GetBoolValue();

            CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("cef_set_focus");
            CefRefPtr<CefListValue> list = msg->GetArgumentList();
            list->SetBool(0, hasFocus);

            CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
            return true;
        }
        else if (name == "isChatInputOpen")
        {
            retval = CefV8Value::CreateBool(g_chat_input_open);
            return true;
        }
        else if (name == "exitGame")
        {
            CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("cef_exit_game");

            CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
            return true;
        }

        return false;
    }

    IMPLEMENT_REFCOUNTING(V8HandlerImpl);
};

class RenderProcessHandler : public CefRenderProcessHandler
{
public:
    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override
    {
        CEF_REQUIRE_RENDERER_THREAD();

        CefRefPtr<CefV8Value> global = context->GetGlobal();
        CefRefPtr<CefV8Value> cefObj = CefV8Value::CreateObject(nullptr, nullptr);
        CefRefPtr<CefV8Handler> handler = new V8HandlerImpl();

        cefObj->SetValue("emit", CefV8Value::CreateFunction("emit", handler), V8_PROPERTY_ATTRIBUTE_NONE);
        cefObj->SetValue("on", CefV8Value::CreateFunction("on", handler), V8_PROPERTY_ATTRIBUTE_NONE);
        cefObj->SetValue("off", CefV8Value::CreateFunction("off", handler), V8_PROPERTY_ATTRIBUTE_NONE);
        cefObj->SetValue("set_focus", CefV8Value::CreateFunction("set_focus", handler), V8_PROPERTY_ATTRIBUTE_NONE);
        cefObj->SetValue("isChatInputOpen", CefV8Value::CreateFunction("isChatInputOpen", handler), V8_PROPERTY_ATTRIBUTE_NONE);
        cefObj->SetValue("exitGame", CefV8Value::CreateFunction("exitGame", handler), V8_PROPERTY_ATTRIBUTE_NONE);

        global->SetValue("cef", cefObj, V8_PROPERTY_ATTRIBUTE_NONE);
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) override
    {
        CEF_REQUIRE_RENDERER_THREAD();

        if (message->GetName() == "emit_event")
        {
            CefRefPtr<CefListValue> args = message->GetArgumentList();
            if (args->GetSize() < 1)
                return true; // Not enough arguments

            // The event name is always at index 0
            std::string eventName = args->GetString(0);

            if (eventName == "omp:cef:internal:chatInputState" && args->GetSize() >= 2 && args->GetType(1) == VTYPE_BOOL)
            {
                g_chat_input_open = args->GetBool(1);
            }

            CefV8ValueList jsArgs;
            for (size_t i = 1; i < args->GetSize(); ++i)
            {
                switch (args->GetType(i))
                {
                    case VTYPE_INT:    
                        jsArgs.push_back(CefV8Value::CreateInt(args->GetInt(i))); 
                        break;
                    case VTYPE_DOUBLE: 
                        jsArgs.push_back(CefV8Value::CreateDouble(args->GetDouble(i))); 
                        break;
                    case VTYPE_BOOL:   
                        jsArgs.push_back(CefV8Value::CreateBool(args->GetBool(i))); 
                        break;
                    case VTYPE_STRING: 
                        jsArgs.push_back(CefV8Value::CreateString(args->GetString(i))); 
                        break;
                    default:           
                        jsArgs.push_back(CefV8Value::CreateNull()); 
                        break;
                }
            }

            auto it = events_.find(eventName);
            const bool hasHandler = (it != events_.end() && !it->second.empty());

            CefRefPtr<CefV8Context> context = frame->GetV8Context();
            if (!hasHandler || !context || !context->Enter())
            {
                pending_events_[eventName].push_back(CapturePendingArgs(args));
                return true;
            }

            for (const auto& cb : it->second)
            {
                if (cb) cb->ExecuteFunction(nullptr, jsArgs);
            }

            context->Exit();
            return true;
        }

        return false;
    }

    IMPLEMENT_REFCOUNTING(RenderProcessHandler);
};

class RendererApp : public CefApp, public CefRenderProcessHandler
{
public:
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
    {
        return new RenderProcessHandler();
    }

    IMPLEMENT_REFCOUNTING(RendererApp);
};

// Entry point for the renderer process
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    CefMainArgs args(hInstance);
    CefRefPtr<RendererApp> app = new RendererApp();
    return CefExecuteProcess(args, app, nullptr);
}