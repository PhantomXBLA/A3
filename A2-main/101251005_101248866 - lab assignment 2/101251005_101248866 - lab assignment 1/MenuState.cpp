#include "MenuState.h"
#include "Game.h"
#include "State.h"
#include "Background.h"

MenuState::MenuState(StateStack* stack, Context* context)
	: State(stack, context)
{
	
	
	mAllRitems.clear();
	mContext->game->ResetFrameResources();
	mContext->game->BuildMaterials();


	std::unique_ptr<Background> background2 = std::make_unique<Background>(mContext->game);
	background2->materialName = "menu";
	background2->spawnX = 0;
	background2->spawnY = -9.9f;
	mContext->mSceneGraph->attachChild(std::move(background2));


	mContext->mSceneGraph->build();
	mContext->game->BuildFrameResources();

}

MenuState::~MenuState()
{
}

void MenuState::draw()
{
	mContext->mSceneGraph->draw();
}

bool MenuState::update(const GameTimer& gt)
{
	mContext->world->update(gt);


	return true;
}

bool MenuState::handleEvent(WPARAM btnState)
{

	return true;
}

bool MenuState::handleRealtimeInput()
{

	if (GetAsyncKeyState('E'))
	{
		requestStackPop();
		mContext->game->FlushCommandQueue();
		requestStackPush(States::GAME);

	}

	if (GetAsyncKeyState('Q'))
	{
		PostQuitMessage(0);
	}
	return true;
}