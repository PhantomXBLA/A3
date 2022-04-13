#include "World.h"
#include "Aircraft.h"
#include "Game.h"

//loadTextures();

World::World(Game* game, State* state) : mSceneGraph(new SceneNode(game)), mGame(game)
{
	
}

CommandQueue& World::getCommandQueue()
{
	return mCommandQueue;
}

void World::update(const GameTimer& gt)
{

	while (!mCommandQueue.isEmpty())
		mSceneGraph->onCommand(mCommandQueue.pop(), gt);
		

	mSceneGraph->update(gt);
	//UpdateWaves(gt);

}

void World::draw()
{
}


void World::loadTextures()
{

}

void World::buildScene()
{
	//UINT objCBIndex = 0;

	//auto wavesRitem = std::make_unique<RenderItem>();
	//wavesRitem->World = MathHelper::Identity4x4();
	////XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0) * XMMatrixTranslation(500.0f, 100.0f, 800.0f));
	//XMStoreFloat4x4(&wavesRitem->World, XMMatrixScaling(10.0f, 10.0f, 10.0f) * XMMatrixRotationX(XM_PI * -0.5f) * XMMatrixTranslation(0.0f, 0.0f, 800.0f));
	//wavesRitem->ObjCBIndex = objCBIndex++;
	//wavesRitem->Mat = mGame->mMaterials["desert"].get();
	//wavesRitem->Geo = mGame->mGeometries["desertGeo"].get();
	//wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	//wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	//wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	//mWavesRitem = wavesRitem.get(); //The randomness of waves/ this has been set to be almost non existent
	//mGame->mRitemLayer[(int)RenderLayer::Opaque].push_back(wavesRitem.get());

	//mGame->mAllRitems.push_back(std::move(wavesRitem));
	

	std::unique_ptr<Aircraft> playerAircraft(new Aircraft(Aircraft::Eagle, mGame));
	mPlayerAircraft = playerAircraft.get();
	mPlayerAircraft->mPosition.x = 0;
	mPlayerAircraft->mPosition.y = -13;
	mPlayerAircraft->objCBIndex = 1;
	mSceneGraph->attachChild(std::move(playerAircraft));

	

	std::unique_ptr<Aircraft> raptorOne(new Aircraft(Aircraft::Raptor, mGame));
	raptorOne->mPosition.x = 6;
	raptorOne->mPosition.y = -15;
	raptorOne->objCBIndex = 2;
	mPlayerAircraft->attachChild(std::move(raptorOne));


	std::unique_ptr<Aircraft> raptorTwo(new Aircraft(Aircraft::Raptor, mGame));
	raptorTwo->mPosition.x = -6;
	raptorTwo->mPosition.y = -15;
	raptorTwo->objCBIndex = 3;
	mPlayerAircraft->attachChild(std::move(raptorTwo));


	mSceneGraph->build();
}

void World::UpdateWaves(const GameTimer& gt)
{
	//// Every quarter second, generate a random wave.
	//static float t_base = 0.0f;
	////if ((mTimer.TotalTime() - t_base) >= 0.025f)
	////{
	////	t_base += 1.25f;

	////	int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
	////	int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

	////	float r = MathHelper::RandF(0.2f, 0.5f);

	////	mWaves->Disturb(i, j, r);
	////}

	//// Update the wave simulation.
	//mWaves->Update(gt.DeltaTime());

	//// Update the wave vertex buffer with the new solution.
	//auto currWavesVB = mGame->mCurrFrameResource->WavesVB.get();
	//for (int i = 0; i < mWaves->VertexCount(); ++i)
	//{
	//	Vertex v;

	//	v.Pos = mWaves->Position(i);
	//	v.Normal = mWaves->Normal(i);

	//	// Derive tex-coords from position by 
	//	// mapping [-w/2,w/2] --> [0,1]
	//	v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
	//	v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

	//	currWavesVB->CopyData(i, v);
	//}

	//// Set the dynamic VB of the wave renderitem to the current frame VB.
	//mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

#pragma region step 4

void World::adaptPlayerPosition()
{
	
}

void World::adaptPlayerVelocity()
{

}

#pragma endregion