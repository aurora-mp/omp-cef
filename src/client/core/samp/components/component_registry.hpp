#pragma once

#include "component.hpp"
#include <functional>
#include <type_traits>

inline std::vector<ISampComponent*>& GetActiveComponents() 
{
    static std::vector<ISampComponent*> components;
    return components;
}

inline std::vector<std::function<void()>>& GetComponentInitializers() 
{
    static std::vector<std::function<void()>> initializers;
    return initializers;
}

inline void InitializeComponents() 
{
    for (const auto& init : GetComponentInitializers()) {
        init();
    }
}

template<typename T>
T* GetComponent() 
{
    static_assert(std::is_base_of<ISampComponent, T>::value, "T must derive from ISampComponent");

    for (auto* component : GetActiveComponents()) {
        if (auto* casted = dynamic_cast<T*>(component)) {
            return casted;
        }
    }

    return nullptr;
}

#define REGISTER_SAMP_COMPONENT(CLASS_NAME) \
    namespace { \
        struct CLASS_NAME##AutoRegister { \
            CLASS_NAME##AutoRegister() { \
                GetComponentInitializers().emplace_back([] { \
                    static CLASS_NAME instance; \
                    instance.Initialize(); \
                    GetActiveComponents().push_back(&instance); \
                }); \
            } \
        }; \
        static CLASS_NAME##AutoRegister global_##CLASS_NAME##AutoRegister; \
    }