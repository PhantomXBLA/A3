#pragma once

#include "Category.h"
#include "d3dApp.h"
#include <functional>
#include <cassert>
#include "GameTimer.h"

class SceneNode;


///Struct used for commands
struct Command
{
	Command();

	std::function<void(SceneNode&, const GameTimer& gt)>	action;
	unsigned int											category;
};

template <typename GameObject, typename Function>
std::function<void(SceneNode&, const GameTimer& gt)> derivedAction(Function fn)
{
	return [=](SceneNode& node, const GameTimer gt)
	{
		// Check if cast is safe
		assert(dynamic_cast<GameObject*>(&node) != nullptr);

		// Downcast node and invoke function on it
		fn(static_cast<GameObject&>(node), gt);
	};
}

