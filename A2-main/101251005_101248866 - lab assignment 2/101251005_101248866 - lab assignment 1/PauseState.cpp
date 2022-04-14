#include "PauseState.h"
#include "Game.h"
#include "State.h"
#include "Background.h"

PauseState::PauseState(StateStack* stack, Context* context)
	: State(stack, context)
{


	mAllRitems.clear();
	mContext->game->ResetFrameResources();
	

	mContext->game->BuildMaterials();


	std::unique_ptr<Background> background3 = std::make_unique<Background>(mContext->game);
	background3->materialName = "pause";
	background3->spawnX = 0;
	background3->spawnY = -9.8f;
	mContext->mSceneGraph->attachChild(std::move(background3));


	mContext->mSceneGraph->build();
	
	mContext->game->BuildFrameResources();

}

PauseState::~PauseState()
{
}

void PauseState::draw()
{
	mContext->mSceneGraph->draw();
}

bool PauseState::update(const GameTimer& gt)
{
	mContext->world->update(gt);
	//mContext->mSceneGraph->draw();
	//CommandQueue& commands = mContext->world->getCommandQueue();





	return true;
}

bool PauseState::handleEvent(WPARAM btnState)
{

	return true;
}

bool PauseState::handleRealtimeInput()
{
	if (GetAsyncKeyState('E'))
	{
		mContext->game->FlushCommandQueue();
		requestStackPop();
		
		requestStackPush(States::GAME);


	}
	return true;
}