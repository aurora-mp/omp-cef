#include <sdk.hpp>

#include "component.hpp"

COMPONENT_ENTRY_POINT()
{
    return new CefOmpComponent();
}