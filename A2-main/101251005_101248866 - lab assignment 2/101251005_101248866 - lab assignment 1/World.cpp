#include "World.h"
#include "Aircraft.h"
#include "Game.h"

//loadTextures();

World::World(Game* game) : mSceneGraph(new SceneNode(game)), mGame(game)
{
	
}

CommandQueue& World::getCommandQueue()
{
	return mCommandQueue;
}

void World::update(const GameTimer& gt)
{

	while (!mCommandQueue.isEmpty())
		mSceneGraph->onCommand(mCommandQueue.pop(), gt);
		

	mSceneGraph->update(gt);
	//UpdateWaves(gt);

}

void World::draw()
{
	mSceneGraph->draw();
}


void World::loadTextures()
{

}

void World::buildScene()
{
	

	std::unique_ptr<Aircraft> playerAircraft(new Aircraft(Aircraft::Eagle, mGame));
	mPlayerAircraft = playerAircraft.get();
	mPlayerAircraft->mPosition.x = 0;
	mPlayerAircraft->mPosition.y = -13;
	mPlayerAircraft->objCBIndex = 1;
	mSceneGraph->attachChild(std::move(playerAircraft));

	std::unique_ptr<Aircraft> raptorOne(new Aircraft(Aircraft::Raptor, mGame));
	raptorOne->mPosition.x = 6;
	raptorOne->mPosition.y = -15;
	raptorOne->objCBIndex = 2;
	mPlayerAircraft->attachChild(std::move(raptorOne));

	std::unique_ptr<Aircraft> raptorTwo(new Aircraft(Aircraft::Raptor, mGame));
	raptorTwo->mPosition.x = -6;
	raptorTwo->mPosition.y = -15;
	raptorTwo->objCBIndex = 3;
	mPlayerAircraft->attachChild(std::move(raptorTwo));


	mSceneGraph->build();
}

void World::UpdateWaves(const GameTimer& gt)
{

}

#pragma region step 4

void World::adaptPlayerPosition()
{
	
}

void World::adaptPlayerVelocity()
{

}

#pragma endregion