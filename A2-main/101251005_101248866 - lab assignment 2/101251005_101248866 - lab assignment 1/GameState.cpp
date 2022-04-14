#include "GameState.h"
#include "Game.h"


GameState::GameState(StateStack* stack, Context* context): State(stack, context)
{
	


	mAllRitems.clear();
	mContext->game->ResetFrameResources();
	mContext->game->BuildMaterials();
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

	return true;
}

bool GameState::handleEvent(WPARAM btnState)
{
	return true;
}

bool GameState::handleRealtimeInput()
{
	if (GetAsyncKeyState('P'))
	{
		
		requestStackPop();
		mContext->game->FlushCommandQueue();
		requestStackPush(States::PAUSE);
	}
	
	return true;
}

#pragma endregion