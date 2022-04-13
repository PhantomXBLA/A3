#pragma once
#include <memory>
#include "States.h"
#include "d3dApp.h"
#include "World.h"


class StateStack;

class Player;
class Game;
class World;

class State
{
public:
	typedef std::unique_ptr<State> StatePtr;

	struct Context
	{
		Context(Game* game, Player* player);
		Player* player;
		Game* game;
		World* world;
		
	};


public:
	State(StateStack* stack, Context* context);
	virtual				~State();

	virtual void		draw() = 0;
	virtual bool		update(const GameTimer& gt) = 0;
	//virtual bool		handleEvent(const sf::Event& event) = 0;


protected:
	void				requestStackPush(States::ID stateID);
	void				requestStackPop();
	void				requestStateClear();

	Context				getContext() const;
	StateStack* mStack;


private:

	Context*				mContext;
};
