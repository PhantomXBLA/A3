#include "Entity.h"

Entity::Entity(Game* game) : SceneNode(game)
{
	//mPosition.x = 0;
	//mPosition.y = 0;
}

void Entity::setVelocity(DirectX::XMFLOAT2 velocity)
{
	mVelocity = velocity;
	//OutputDebugStringA(std::string("Acceleration Set").c_str());
}

void Entity::setVelocity(float vx, float vy)
{
	mVelocity.x = vx;
	mVelocity.y = vy;
}

DirectX::XMFLOAT2 Entity::getVelocity() const
{
	return mVelocity;
}

void Entity::updateCurrent(const GameTimer& gt)
{

	mPosition.x += mVelocity.x * gt.DeltaTime();
	mPosition.y += mVelocity.y * gt.DeltaTime();

	mRenderItem->World = getWorldTransform();
	mRenderItem->NumFramesDirty++;
}

void Entity::accelerate(DirectX::XMFLOAT2 velocity)
{
	mVelocity.x += velocity.x;
	mVelocity.y += velocity.y;

	
}

void Entity::accelerate(float vx, float vy)
{
	mVelocity.x += vx;
	mVelocity.y += vy;
}
