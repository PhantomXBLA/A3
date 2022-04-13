#pragma region step 2
#include "Aircraft.h"
#include "Player.h"
#include "CommandQueue.h"
#include <map>
#include <string>
#include <algorithm>
#include <iostream>

#include "d3dApp.h"


struct AircraftMover
{
	AircraftMover(float vx, float vy)
		: velocity(vx, vy)
	{
	}

	void operator() (Aircraft& aircraft, const GameTimer& gt) const
	{
		//aircraft.accelerate(velocity);
		aircraft.setVelocity(velocity);

		
	}

	XMFLOAT2 velocity;
};

Player::Player()
{
	// Set initial key bindings
	mKeyBinding["W"] = MoveUp;
	mKeyBinding["A"] = MoveLeft;
	mKeyBinding["S"] = MoveDown;
	mKeyBinding["D"] = MoveRight;
	mKeyBinding["P"] = GetPosition;

	// Set initial action bindings
	initializeActions();

	// Assign all categories to player's aircraft
	for (auto& pair : mActionBinding)
		pair.second.category = Category::PlayerAircraft;
}

void Player::handleEvent(CommandQueue& commands, const GameTimer& gt)
{

}



void Player::handleRealtimeInput(CommandQueue& commands, const GameTimer& gt)
{

	if (GetAsyncKeyState('W'))
	{

		auto found = mKeyBinding.find("W");
		if (found != mKeyBinding.end())
			commands.push(mActionBinding[found->second]);

	}
	
	else if (GetAsyncKeyState('A'))
	{
		auto found = mKeyBinding.find("A");
		if (found != mKeyBinding.end())
			commands.push(mActionBinding[found->second]);
	}


	else if (GetAsyncKeyState('S'))
	{

		auto found = mKeyBinding.find("S");
		if (found != mKeyBinding.end())
			commands.push(mActionBinding[found->second]);

	}

	else if (GetAsyncKeyState('D'))
	{

		auto found = mKeyBinding.find("D");
		if (found != mKeyBinding.end())
			commands.push(mActionBinding[found->second]);

	}else
	{
		commands.push(mActionBinding[Stop]);
	}

	
}

void Player::assignKey(Action action, std::string key)
{
	// Remove all keys that already map to action
	for (auto itr = mKeyBinding.begin(); itr != mKeyBinding.end(); )
	{
		if (itr->second == action)
			mKeyBinding.erase(itr++);
		else
			++itr;
	}

	// Insert new binding
	mKeyBinding[key] = action;
}

std::string Player::getAssignedKey(Action action) const
{
	for (auto pair : mKeyBinding)
	{
		if (pair.second == action)
			return pair.first;
	}

	return nullptr;
}

void Player::initializeActions()
{
	const float playerSpeed = 20.f;

	mActionBinding[MoveLeft].action = derivedAction<Aircraft>(AircraftMover(-playerSpeed, 0.f));
	mActionBinding[MoveRight].action = derivedAction<Aircraft>(AircraftMover(playerSpeed, 0.f));
	mActionBinding[MoveUp].action = derivedAction<Aircraft>(AircraftMover(0.f, playerSpeed));
	mActionBinding[MoveDown].action = derivedAction<Aircraft>(AircraftMover(0.f, -playerSpeed));
	mActionBinding[Stop].action = derivedAction<Aircraft>(AircraftMover(0.f, 0.0f));

}

bool Player::isRealtimeAction(Action action)
{
	switch (action)
	{
	case MoveLeft:
	case MoveRight:
	case MoveDown:
	case MoveUp:
		return true;

	default:
		return false;
	}
}

#pragma endregion
