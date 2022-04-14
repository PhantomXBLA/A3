#pragma once
//#include "Game.h"
#include "Aircraft.h"
#include "FrameResource.h"
#include "CommandQueue.h"
#include "Command.h"
//#include "Waves.h"

class World
{
public:
	World(Game* game, State* state);
	void								update(const GameTimer& gt);
	void								drawOcean(Game* gameRef);
	void								buildScene();
	void								draw();
	CommandQueue&						getCommandQueue();
	CommandQueue						mCommandQueue;

	//RenderItem* mWavesRitem = nullptr;
	//std::unique_ptr<Waves> mWaves;


private:
	void								loadTextures();

	void								UpdateWaves(const GameTimer& gt);

#pragma region step 3
	void								adaptPlayerPosition();
	void								adaptPlayerVelocity();
#pragma endregion


private:
	enum Layer
	{
		Background,
		Air,
		LayerCount
	};


private:

	SceneNode*							mSceneGraph;
	std::array<SceneNode*, LayerCount>	mSceneLayers;

	//sf::FloatRect						mWorldBounds;
	Aircraft* mPlayerAircraft;
	Game* mGame;
	State* mState;
	//State* state;
};
