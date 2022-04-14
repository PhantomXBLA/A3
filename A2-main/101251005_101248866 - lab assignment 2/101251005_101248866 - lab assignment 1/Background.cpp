#include "Background.h"
#include "Game.h"





Background::Background(Game* game) : Entity(game)
{
	
}



void Background::drawCurrent() const
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = game->mCurrFrameResource->ObjectCB->Resource();
	auto matCB = game->mCurrFrameResource->MaterialCB->Resource();


	if (mRenderItem)
	{

		game->mCommandList.Get()->IASetVertexBuffers(0, 1, &mRenderItem->Geo->VertexBufferView());
		game->mCommandList.Get()->IASetIndexBuffer(&mRenderItem->Geo->IndexBufferView());
		game->mCommandList.Get()->IASetPrimitiveTopology(mRenderItem->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(game->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(mRenderItem->Mat->DiffuseSrvHeapIndex, game->mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (UINT64)mRenderItem->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + (UINT64)mRenderItem->Mat->MatCBIndex * matCBByteSize;

		game->mCommandList.Get()->SetGraphicsRootDescriptorTable(0, tex);
		game->mCommandList.Get()->SetGraphicsRootConstantBufferView(1, objCBAddress);
		game->mCommandList.Get()->SetGraphicsRootConstantBufferView(3, matCBAddress);
		game->mCommandList.Get()->DrawIndexedInstanced(mRenderItem->IndexCount, 1, mRenderItem->StartIndexLocation, mRenderItem->BaseVertexLocation, 0);
	}
}

void Background::updateCurrent()
{

}

void Background::buildCurrent()
{
	auto backgroundRenderItem = std::make_unique<RenderItem>();

	backgroundRenderItem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&backgroundRenderItem->World, XMMatrixScaling(97.0f, 80.0f, 80.0f) * XMMatrixRotationX(XM_PI * -0.466f) * XMMatrixTranslation(spawnX, spawnY, 100.0f));
	backgroundRenderItem->ObjCBIndex = objCBIndex++;


	//Materials go here
	mRenderItem = backgroundRenderItem.get();
	mRenderItem->Mat = game->mMaterials[materialName].get();
	mRenderItem->Geo = game->mGeometries["boxGeo"].get();
	mRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mRenderItem->IndexCount = mRenderItem->Geo->DrawArgs["box"].IndexCount;
	mRenderItem->StartIndexLocation = mRenderItem->Geo->DrawArgs["box"].StartIndexLocation;
	mRenderItem->BaseVertexLocation = mRenderItem->Geo->DrawArgs["box"].BaseVertexLocation;

	game->mRitemLayer[(int)RenderLayer::Transparent].push_back(backgroundRenderItem.get());

	game->mAllRitems.push_back(std::move(backgroundRenderItem));
}



