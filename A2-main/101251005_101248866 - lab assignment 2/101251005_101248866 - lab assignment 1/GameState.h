#pragma once
#include "Player.h"
#include "State.h"



class GameState : public State
{
public:
	GameState(StateStack* stack, Context* context);
	virtual ~GameState();
	virtual void		draw() override;
	virtual bool		update(const GameTimer& gt) override;
	
	virtual bool handleEvent(WPARAM btnState)override;
	virtual bool handleRealtimeInput()override;


private:

	//World				mWorld;
	//Player				mPlayer;
	//Game*				game;
};

