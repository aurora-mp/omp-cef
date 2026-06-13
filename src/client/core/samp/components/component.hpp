#pragma once

class ISampComponent {
public:
    virtual ~ISampComponent() = default;
    virtual void Initialize() = 0;
};
