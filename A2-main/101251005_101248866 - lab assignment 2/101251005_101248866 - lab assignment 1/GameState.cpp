#include "GameState.h"

GameState::GameState(StateStack* stack, Context* context): State(stack, context), mWorld(game, this)
{
	mWorld.buildScene();
	OutputDebugString(L"function Called");
}

void GameState::draw()
{
	mWorld.draw();
}

bool GameState::update(const GameTimer& gt)
{
	mWorld.update(gt);

	CommandQueue& commands = mWorld.getCommandQueue();
	mPlayer.handleRealtimeInput(commands, gt);

	return true;
}

//bool GameState::handleEvent(const sf::Event& event)
//{
//	// Game input handling
//	CommandQueue& commands = mWorld.getCommandQueue();
//	mPlayer.handleEvent(event, commands);
//
//	// Escape pressed, trigger the pause screen
//	//if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
//	//	requestStackPush(States::Pause);
//
//	return true;
//}

#pragma endregion