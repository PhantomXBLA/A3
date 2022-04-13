#pragma once

#include "SceneNode.h"

class Entity: public SceneNode
{
public:
	
	///Constructor
	/// @param (game) is an instance reference of Game class 
	Entity(Game* game);

	
	void setVelocity(DirectX::XMFLOAT2 velocity);
	
	///Sets Velocity for entity
	///
	/// @param (x) setting entities X direction
	/// @param (y) setting entities Y direction
	void setVelocity(float vx, float vy);



	DirectX::XMFLOAT2 getVelocity() const;

	void accelerate(DirectX::XMFLOAT2 velocity);

	
	void accelerate(float vx, float vy);
	


	
	DirectX::XMVECTOR spawnPos = {5,0,0,0};

	float spawnX, spawnY;
	UINT objCBIndex;



private:

	///Recieves enetities updated velocity
	///
	/// @param (gt) passed in to update the entity via time
	virtual void updateCurrent(const GameTimer& gt);
	DirectX::XMFLOAT2 mVelocity;

private:



};

