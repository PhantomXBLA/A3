#include "SceneNode.h"
#include "Game.h"
#include "Command.h"
#include <algorithm>
#include <cassert>


SceneNode::SceneNode(Game* game) : mChildren(), mParent(nullptr), game(game)
{

}

void SceneNode::attachChild(Ptr child)
{
	child->mParent = this;
	mChildren.push_back(std::move(child));
}

SceneNode::Ptr SceneNode::detachChild(const SceneNode& node)
{
	auto found = std::find_if(mChildren.begin(), mChildren.end(), [&](Ptr& p) { return p.get() == &node; });
	assert(found != mChildren.end());

	Ptr result = std::move(*found);
	result->mParent = nullptr;
	mChildren.erase(found);
	return result;
}

void SceneNode::update(const GameTimer& gt)
{
	updateCurrent(gt);
	updateChildren(gt);
}

void SceneNode::updateCurrent(const GameTimer& gt)
{

}

void SceneNode::draw() const
{
	// Draw node and children with changed transform
	drawCurrent();
	drawChildren();
}

void SceneNode::drawCurrent() const
{

}

void SceneNode::drawChildren() const
{
	for (const Ptr& child : mChildren)
		child->draw();
}

void SceneNode::build()
{
	buildCurrent();
	buildChildren();
}

void SceneNode::buildCurrent()
{
	//Empty for now
}

void SceneNode::buildChildren()
{
	for (const Ptr& child : mChildren)
	{
		child->build();
	}
}

void SceneNode::updateChildren(const GameTimer& gt)
{
	for (Ptr& child : mChildren) 
	{
		child->update(gt);
	}
		
}


DirectX::XMFLOAT4X4 SceneNode::getWorldTransform() const
{
	XMFLOAT4X4 transform = MathHelper::Identity4x4();
	XMMATRIX worldTransformMatrix; //setting to matrix because you cannot directly multiply two xmfloat4x4
	XMMATRIX getTransformMatrix;
	
	for (const SceneNode* node = this; node != nullptr; node = node->mParent)
	{
		//transform = node->getTransform() * transform; -> this is what we gotta do
		worldTransformMatrix = XMLoadFloat4x4(&transform);
		getTransformMatrix = XMLoadFloat4x4(&getTransform());

		worldTransformMatrix = getTransformMatrix * worldTransformMatrix;
	}

	XMStoreFloat4x4(&transform, worldTransformMatrix);
	
	return transform;
}

DirectX::XMFLOAT4X4 SceneNode::getTransform() const
{
	XMFLOAT4X4 transform;
	XMStoreFloat4x4(&transform, XMMatrixScaling(3.0f, 0.000000001f, 3.0f) * XMMatrixRotationX(XM_PI * -0.5f) * XMMatrixTranslation(mPosition.x, mPosition.y, 0.0f)); //change size of object - change position of object
	return transform;
}


void SceneNode::onCommand(const Command& command, const GameTimer& gt)
{
	// Command current node, if category matches
	if (command.category & getCategory())
		command.action(*this, gt);

	// Command children
	for (Ptr& child : mChildren)
		child->onCommand(command, gt);
}

unsigned int SceneNode::getCategory() const
{
	return Category::Scene;
}



