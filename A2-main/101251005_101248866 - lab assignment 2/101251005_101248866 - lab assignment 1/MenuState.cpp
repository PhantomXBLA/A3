#include "MenuState.h"
#include "Game.h"
#include "State.h"

MenuState::MenuState(StateStack* stack, Context* context)
	: State(stack, context)
{
	mAllRitems.clear();
	//mContext->game->ResetFrameResources();
	//mContext->game->BuildMaterials();

	//std::unique_ptr<Entity> background = std::make_unique<Entity>(this);
	//background->BuildRenderItems();
	//background->setScale(11.5, 1.0, 10.0);
	//background->setPosition(0, 0, 0);


	UINT objCBIndex = 0;
	auto menuRitem = std::make_unique<RenderItem>();
	menuRitem->World = MathHelper::Identity4x4();
	//XMStoreFloat4x4(&titleRitem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0) * XMMatrixTranslation(500.0f, 100.0f, 800.0f));
	XMStoreFloat4x4(&menuRitem->World, XMMatrixScaling(97.0f, 80.0f, 80.0f) * XMMatrixRotationX(XM_PI * -0.466f) * XMMatrixTranslation(0.0f, -11.0f, 100.0f));
	menuRitem->ObjCBIndex = objCBIndex++;
	menuRitem->Mat = mContext->game->mMaterials["menu"].get();
	menuRitem->Geo = mContext->game->mGeometries["boxGeo"].get();
	menuRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	menuRitem->IndexCount = menuRitem->Geo->DrawArgs["box"].IndexCount;
	menuRitem->StartIndexLocation = menuRitem->Geo->DrawArgs["box"].StartIndexLocation;
	menuRitem->BaseVertexLocation = menuRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mMenuRitem = menuRitem.get();
	mContext->game->mRitemLayer[(int)RenderLayer::Opaque].push_back(menuRitem.get());
	mContext->game->mAllRitems.push_back(std::move(menuRitem));

	//mSceneGraph->attachChild(std::move(background));
	OutputDebugString(L"Menu");
	mContext->mSceneGraph->build();
	//mSceneGraph->build();
	//mContext->game->BuildFrameResources(mAllRitems.size());
	mContext->game->BuildFrameResources();
}

MenuState::~MenuState()
{
}

void MenuState::draw()
{
	mContext->mSceneGraph->draw();
}

bool MenuState::update(const GameTimer& gt)
{
	mContext->mSceneGraph->draw();

	return true;
}

//bool MenuState::handleEvent(WPARAM btnState)
//{
//    // Any Key
//    //
//    requestStackPop();
//    requestStackPush(States::MENU);
//    return true;
//}
//
//bool MenuState::handleRealtimeInput()
//{
//    return true;
//}