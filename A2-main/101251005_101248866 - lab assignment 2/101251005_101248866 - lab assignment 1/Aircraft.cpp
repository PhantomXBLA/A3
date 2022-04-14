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



void Aircraft::drawCurrent() const
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = game->mCurrFrameResource->ObjectCB->Resource();
	auto matCB = game->mCurrFrameResource->MaterialCB->Resource();


	if(mRenderItem)
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

	if (mDesertRitem)
	{

		game->mCommandList.Get()->IASetVertexBuffers(0, 1, &mDesertRitem->Geo->VertexBufferView());
		game->mCommandList.Get()->IASetIndexBuffer(&mDesertRitem->Geo->IndexBufferView());
		game->mCommandList.Get()->IASetPrimitiveTopology(mDesertRitem->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(game->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(mDesertRitem->Mat->DiffuseSrvHeapIndex, game->mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (UINT64)mDesertRitem->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + (UINT64)mDesertRitem->Mat->MatCBIndex * matCBByteSize;

		game->mCommandList.Get()->SetGraphicsRootDescriptorTable(0, tex);
		game->mCommandList.Get()->SetGraphicsRootConstantBufferView(1, objCBAddress);
		game->mCommandList.Get()->SetGraphicsRootConstantBufferView(3, matCBAddress);
		game->mCommandList.Get()->DrawIndexedInstanced(mDesertRitem->IndexCount, 1, mDesertRitem->StartIndexLocation, mDesertRitem->BaseVertexLocation, 0);
	}

	

	
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

	UINT objCBIndex = 0;
	auto desertRitem = std::make_unique<RenderItem>();
	desertRitem->World = MathHelper::Identity4x4();
	//XMStoreFloat4x4(&titleRitem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0) * XMMatrixTranslation(500.0f, 100.0f, 800.0f));
	XMStoreFloat4x4(&desertRitem->World, XMMatrixScaling(97.0f, 80.0f, 80.0f) * XMMatrixRotationX(XM_PI * -0.466f) * XMMatrixTranslation(0.0f, -11.0f, 100.0f));
	desertRitem->ObjCBIndex = objCBIndex++;
	desertRitem->Mat = game->mMaterials["desert"].get();
	desertRitem->Geo = game->mGeometries["boxGeo"].get();
	desertRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	desertRitem->IndexCount = desertRitem->Geo->DrawArgs["box"].IndexCount;
	desertRitem->StartIndexLocation = desertRitem->Geo->DrawArgs["box"].StartIndexLocation;
	desertRitem->BaseVertexLocation = desertRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mDesertRitem = desertRitem.get();
	game->mRitemLayer[(int)RenderLayer::Opaque].push_back(desertRitem.get());
	game->mAllRitems.push_back(std::move(desertRitem));
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



