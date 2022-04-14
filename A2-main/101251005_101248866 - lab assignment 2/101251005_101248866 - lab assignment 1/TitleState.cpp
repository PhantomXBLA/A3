#include "TitleState.h"
#include "Game.h"
#include "State.h"

TitleState::TitleState(StateStack* stack, Context* context)
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
	auto titleRitem = std::make_unique<RenderItem>();
	titleRitem->World= MathHelper::Identity4x4();
	//XMStoreFloat4x4(&titleRitem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0) * XMMatrixTranslation(500.0f, 100.0f, 800.0f));
	XMStoreFloat4x4(&titleRitem->World, XMMatrixScaling(97.0f, 80.0f, 80.0f) * XMMatrixRotationX(XM_PI * -0.466f) * XMMatrixTranslation(0.0f, -11.0f, 100.0f));
	titleRitem->ObjCBIndex = objCBIndex++;
	titleRitem->Mat = mContext->game->mMaterials["title"].get();
	titleRitem->Geo = mContext->game->mGeometries["boxGeo"].get();
	titleRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	titleRitem->IndexCount = titleRitem->Geo->DrawArgs["box"].IndexCount;
	titleRitem->StartIndexLocation = titleRitem->Geo->DrawArgs["box"].StartIndexLocation;
	titleRitem->BaseVertexLocation = titleRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mTitleRitem = titleRitem.get();
	mContext->game->mRitemLayer[(int)RenderLayer::Opaque].push_back(titleRitem.get());
	mContext->game->mAllRitems.push_back(std::move(titleRitem));

	//mSceneGraph->attachChild(std::move(background));
	OutputDebugString(L"Hello");
	mContext->mSceneGraph->build();
	//mSceneGraph->build();
    //mContext->game->BuildFrameResources(mAllRitems.size());
	mContext->game->BuildFrameResources();
}

TitleState::~TitleState()
{
}

void TitleState::draw()
{
	mContext->mSceneGraph->draw();
}

bool TitleState::update(const GameTimer& gt)
{
	mContext->mSceneGraph->draw();

    return true;
}

//bool TitleState::handleEvent(WPARAM btnState)
//{
//    // Any Key
//    //
//    requestStackPop();
//    requestStackPush(States::TITLE);
//    return true;
//}
//
//bool TitleState::handleRealtimeInput()
//{
//    return true;
//}