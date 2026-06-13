#pragma once

#include "samp_version_manager.hpp"

class SampAddresses {
public:
    static SampAddresses& Instance() {
        static SampAddresses inst;
        return inst;
    }

    SampAddresses(const SampAddresses&) = delete;
    SampAddresses& operator=(const SampAddresses&) = delete;

    void Initialize(const SampVersionManager& svm);

    void* SampInitialization() const noexcept;
    void* SampDeinitialization() const noexcept;
    uint8_t* Base() const noexcept { return base_; }
    SampVersion Version() const noexcept { return version_; }

private:
    SampAddresses() = default;

    uint8_t* base_ = nullptr;
    SampVersion version_ = SampVersion::Unknown;
};