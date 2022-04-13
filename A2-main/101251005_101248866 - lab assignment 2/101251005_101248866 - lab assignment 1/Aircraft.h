#pragma once
#include "Entity.h"


class Aircraft : public Entity
{
public:
	
	/// Enum Type
	/// 
	/// The different types of aircraft are assigned their own Type
	enum Type
	{
		Eagle,
		Raptor,
	};

public:
	/// Constructor
	/// 
	/// @param (type) Type enum for aircraft Type.
	/// @param (game) instance reference of game class
	Aircraft(Type type, Game* game);

	/// Gets the Type from the category namespace in SceneNode.
	/// 
	/// @param no parameters
	/// @returns an enum type from the category namespace.
	virtual unsigned int getCategory() const;
	
	

private:
	virtual void		drawCurrent();
	virtual void		buildCurrent();
	virtual void		updateCurrent();





private:
	Type				mType;
	std::string			materialName;
};

