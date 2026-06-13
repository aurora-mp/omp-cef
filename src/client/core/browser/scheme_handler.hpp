#pragma once

#include "include/cef_resource_handler.h"
#include "include/cef_scheme.h"

#include <cstdint>
#include <string>
#include <vector>

class ResourceManager;

// Handles individual resource requests for the custom "cef://" scheme
// Fetches file data from ResourceManager and serves it to the browser
class LocalResourceHandler : public CefResourceHandler {
public:
    LocalResourceHandler(ResourceManager& resource_manager);

    bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) override;
    void GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t& response_length, CefString& redirectUrl) override;
    bool ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) override;
    void Cancel() override;

private:
    ResourceManager& resource_manager_;

    std::vector<uint8_t> data_;
    std::string mime_type_;
    size_t read_offset_ = 0;

    IMPLEMENT_REFCOUNTING(LocalResourceHandler);
};

// Factory that creates LocalResourceHandler instances for each request
class LocalSchemeHandlerFactory : public CefSchemeHandlerFactory {
public:
    LocalSchemeHandlerFactory(ResourceManager& resource_manager);

    CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
        const CefString& scheme_name, CefRefPtr<CefRequest> request) override;

private:
    ResourceManager& resource_manager_;

    IMPLEMENT_REFCOUNTING(LocalSchemeHandlerFactory);
};