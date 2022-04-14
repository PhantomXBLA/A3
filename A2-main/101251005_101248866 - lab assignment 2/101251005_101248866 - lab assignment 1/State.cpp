#include "State.h"
#include "StateStack.h"
#include "Game.h"


State::Context::Context(Game* game, Player* player) : game(game), player(player), mSceneGraph(new SceneNode(game)), world(new World(game))
{
}

State::State(StateStack* stack, Context* context)
	: mStack(stack)
	, mContext(context)
{
}

State::~State()
{
}

void State::requestStackPush(States::ID stateID)
{
	mStack->pushState(stateID);
}

void State::requestStackPop()
{
	mStack->popState();
	OutputDebugString(L"State Popped");
}

void State::requestStateClear()
{
	mStack->clearStates();
}



