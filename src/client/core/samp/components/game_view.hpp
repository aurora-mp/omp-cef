#pragma once

class IGameView 
{
public:
    virtual ~IGameView() = default;

    virtual void SetCursorMode(int mode, bool hideCursorImmediately) = 0;
    virtual void ProcessInputEnabling() = 0;
};
