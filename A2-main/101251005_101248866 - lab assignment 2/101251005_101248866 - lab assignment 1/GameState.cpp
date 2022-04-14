#include "GameState.h"
#include "Game.h"


GameState::GameState(StateStack* stack, Context* context): State(stack, context)
{
	
	mContext->game->ResetFrameResources();

	mAllRitems.clear();

	mContext->game->BuildMaterials();
	
	mContext->mSceneGraph->build();
	mContext->world->buildScene();
	
	mContext->game->BuildFrameResources();
	OutputDebugString(L"function Called");
}

GameState::~GameState()
{
}

void GameState::draw()
{
	mContext->world->draw();
}

bool GameState::update(const GameTimer& gt)
{
	mContext->world->update(gt);

	CommandQueue& commands = mContext->world->getCommandQueue();
	mContext->player->handleRealtimeInput(commands, gt);

	if (GetAsyncKeyState('P'))
	{
		requestStackPop();
		requestStackPush(States::PAUSE);
	}

	return true;
}

bool GameState::handleEvent(WPARAM btnState)
{
	requestStackPop();
	requestStackPush(States::GAME);
	return true;
}

bool GameState::handleRealtimeInput()
{
	return true;
}

#pragma endregion