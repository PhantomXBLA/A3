#pragma once
#include "Player.h"
#include "State.h"



class GameState : public State
{
public:
	GameState(StateStack* stack, Context* context);

	virtual void		draw();
	virtual bool		update(const GameTimer& gt);
	//virtual bool		handleEvent(const sf::Event& event);


private:
	World				mWorld;
	Player				mPlayer;
	Game*				game;
};

