#pragma once
#include "Entity.h"


class Background : public Entity
{
public:


public:
	/// Constructor

	/// @param (game) instance reference of game class
	Background(Game* game);

	/// Gets the Type from the category namespace in SceneNode.
	/// 
	/// @param no parameters
	/// @returns an enum type from the category namespace.

	std::string			materialName;
	float				spawnX;
	float				spawnY;

private:
	virtual void		drawCurrent() const;
	virtual void		buildCurrent();
	virtual void		updateCurrent();





private:
	
	////RenderItem*			mDesertRitem = nullptr;
};

