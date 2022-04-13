#include "Aircraft.h"
#include "Game.h"





Aircraft::Aircraft(Type type, Game* game) : Entity(game), mType(type)
{
	switch (type)
	{

	case (Eagle):
		materialName = "eagle";
		break;

	case (Raptor):
		materialName = "raptor";
		break;

	}
}


void Aircraft::drawCurrent()
{
}

void Aircraft::updateCurrent()
{

}

void Aircraft::buildCurrent()
{
	auto planeRenderItem = std::make_unique<RenderItem>();

	planeRenderItem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&planeRenderItem->World, XMMatrixScaling(3.0f, 0.000000001f, 3.0f) * XMMatrixRotationX(XM_PI * -0.5f) * XMMatrixTranslation(mPosition.x, mPosition.y, 0.0f)); //change size of object - change position of object
	planeRenderItem->ObjCBIndex = objCBIndex++;


	//Materials go here
	mRenderItem = planeRenderItem.get();
	mRenderItem->Mat = game->mMaterials[materialName].get();
	mRenderItem->Geo = game->mGeometries["boxGeo"].get();
	mRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mRenderItem->IndexCount = mRenderItem->Geo->DrawArgs["box"].IndexCount;
	mRenderItem->StartIndexLocation = mRenderItem->Geo->DrawArgs["box"].StartIndexLocation;
	mRenderItem->BaseVertexLocation = mRenderItem->Geo->DrawArgs["box"].BaseVertexLocation;


	game->mRitemLayer[(int)RenderLayer::Transparent].push_back(planeRenderItem.get());

	game->mAllRitems.push_back(std::move(planeRenderItem));
}

unsigned int Aircraft::getCategory() const
{
	switch (mType)
	{
	case Eagle:
		return Category::Type::PlayerAircraft;

	default:
		return Category::Type::EnemyAircraft;
	}
}

