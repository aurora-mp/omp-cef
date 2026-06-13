#pragma once

class IChatView
{
public:
    virtual ~IChatView() = default;

    virtual void Clear() = 0;
};
