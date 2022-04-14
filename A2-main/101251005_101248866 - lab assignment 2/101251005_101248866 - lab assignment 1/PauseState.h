#pragma once
#pragma once
#include "State.h"
#include "Game.h"
class PauseState : public State
{
public:
    PauseState(StateStack* stack, Context* context);
    virtual ~PauseState();
    virtual void draw()override;
    virtual bool update(const GameTimer& gt)override;

    RenderItem* mMenuRitem = nullptr;
    virtual bool handleEvent(WPARAM btnState)override;
    virtual bool handleRealtimeInput()override;

};

