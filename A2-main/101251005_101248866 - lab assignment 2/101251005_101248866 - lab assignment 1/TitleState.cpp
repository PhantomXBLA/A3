#include "TitleState.h"
#include "Game.h"
#include "State.h"
#include "Background.h"

TitleState::TitleState(StateStack* stack, Context* context)
    : State(stack, context)
{
	mContext->game->ResetFrameResources();
    mAllRitems.clear();
	
	mContext->game->BuildMaterials();


	std::unique_ptr<Background> background = std::make_unique<Background>(mContext->game);
	background->materialName = "title";
	background->spawnX = 0;
	background->spawnY = -10;
	mContext->mSceneGraph->attachChild(std::move(background));


	OutputDebugString(L"Hello");
	mContext->mSceneGraph->build();
	mContext->game->BuildFrameResources();

}

TitleState::~TitleState()
{
}

void TitleState::draw()
{
	mContext->mSceneGraph->draw();
}

bool TitleState::update(const GameTimer& gt)
{
	mContext->world->update(gt);
	mContext->mSceneGraph->draw();

	if (d3dUtil::IsKeyDown('W'))
	{
		requestStackPop();
		requestStackPush(States::MENU);
	}

    return true;
}

bool TitleState::handleEvent(WPARAM btnState)
{

	return true;
}

bool TitleState::handleRealtimeInput()
{

    return true;
}