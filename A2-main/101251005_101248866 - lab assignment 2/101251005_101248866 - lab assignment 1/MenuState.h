#pragma once
#include "State.h"
#include "Game.h"
class MenuState : public State
{
public:
    MenuState(StateStack* stack, Context* context);
    virtual ~MenuState();
    virtual void draw()override;
    virtual bool update(const GameTimer& gt)override;

    RenderItem* mMenuRitem = nullptr;
	virtual bool handleEvent(WPARAM btnState)override;
	virtual bool handleRealtimeInput()override;

};