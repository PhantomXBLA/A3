#include "Game.h"
#include "Aircraft.h"
#include "States.h"
#include "GameState.h"
#include "TitleState.h"
#include "MenuState.h"
//#include "MenuState.h"
//#include "OptionState.h"
//#include "PauseState.h"



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Game theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

const int gNumFrameResources = 3;

//SceneNode* scn = new SceneNode(this);
//World* world = new World();


Game::Game(HINSTANCE hInstance)
	: D3DApp(hInstance), mStateStack(State::Context(this, &mPlayer))
{
}

Game::~Game()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool Game::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//mWaves = std::make_unique<Waves>(128, 128, 2.0f, 0.03f, 2.0f, 0.2f); //Adjust size of water

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildLandGeometry();
	BuildWavesGeometry();
	BuildBoxGeometry();
	//BuildSphereGeometry();
	//BuildCylinderGeometry();
	//BuildTorusGeometry();
	//BuildWedgeGeometry();
	//BuildTriPrismGeometry();
	//BuildPyramidGeometry();
	//BuildConeGeometry();
	//BuildDiamondGeometry();

	BuildTreeSpritesGeometry();
	RegisterStates();
	
	

	BuildMaterials();
	//mStateStack.pushState(States::MENU);
	mStateStack.pushState(States::TITLE);
	//mStateStack.pushState(States::GAME);
	BuildRenderItems();
	BuildFrameResources();
	
	BuildPSOs();

	

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void Game::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void Game::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);
	mStateStack.update(gt);
	//world.update(gt);
	processEvents(gt);


	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	//UpdateWaves(gt);

}

void Game::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Navy, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	mStateStack.draw();
	
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
	//sceneNode.draw(this);
	
	//world.draw();
}

void Game::processEvents(const GameTimer& gt)
{
	//CommandQueue& commands = world.getCommandQueue();

	//mPlayer.handleEvent(commands, gt);
	//mPlayer.handleRealtimeInput(commands, gt);
}

void Game::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Game::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Game::OnMouseMove(WPARAM btnState, int x, int y)
{
	//if ((btnState & MK_LBUTTON) != 0)
	//{
	//	// Make each pixel correspond to a quarter of a degree.
	//	float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
	//	float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

	//	// Update angles based on input to orbit camera around box.
	//	mTheta += dx;
	//	mPhi += dy;

	//	// Restrict the angle mPhi.
	//	mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	//}
	//else if ((btnState & MK_RBUTTON) != 0)
	//{
	//	// Make each pixel correspond to 0.2 unit in the scene.
	//	float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
	//	float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

	//	// Update the camera radius based on input.
	//	mRadius += dx - dy;

	//	// Restrict the radius.
	//	mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	//}

	//mLastMousePos.x = x;
	//mLastMousePos.y = y;
}

void Game::OnKeyboardInput(const GameTimer& gt)
{
}

void Game::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	//XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);;
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR down = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void Game::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto desertMat = mMaterials["desert"].get();

	float& tu = desertMat->MatTransform(3, 0);
	float& tv = desertMat->MatTransform(3, 1);


	tu += 0.1f * gt.DeltaTime(); //change water flow direction
	tv += -1 * gt.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	desertMat->MatTransform(3, 0) = tu;
	desertMat->MatTransform(3, 1) = tv;



	// Material has changed, so need to update cbuffer.
	desertMat->NumFramesDirty = gNumFrameResources;

}

void Game::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void Game::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void Game::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	//========================================================================== LIGHTS! =====================================================

	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	mMainPassCB.AmbientLight = { 0.0f, 0.25f, 0.25f, 1.0f };

	//Directional lights
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.5f, 0.5f, 0.5f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	//Point lights 
	mMainPassCB.Lights[3].Position = { -18.5f, 5.0f, -18.5f };
	mMainPassCB.Lights[3].Strength = { 80.0f, 0.0f, 0.0f };
	mMainPassCB.Lights[3].FalloffStart = 5.0f;
	mMainPassCB.Lights[3].FalloffEnd = 15.0f;

	mMainPassCB.Lights[4].Position = { 18.5f, 5.0f, -18.5f };
	mMainPassCB.Lights[4].Strength = { 80.0f, 0.0f, 0.0f };
	mMainPassCB.Lights[4].FalloffStart = 5.0f;
	mMainPassCB.Lights[4].FalloffEnd = 15.0f;

	mMainPassCB.Lights[5].Position = { -18.5f, 5.0f, 18.5f };
	mMainPassCB.Lights[5].Strength = { 80.0f, 0.0f, 0.0f };
	mMainPassCB.Lights[5].FalloffStart = 5.0f;
	mMainPassCB.Lights[5].FalloffEnd = 15.0f;

	mMainPassCB.Lights[6].Position = { 18.5f, 5.0f, 18.5f };
	mMainPassCB.Lights[6].Strength = { 80.0f, 0.0f, 0.0f };
	mMainPassCB.Lights[6].FalloffStart = 5.0f;
	mMainPassCB.Lights[6].FalloffEnd = 15.0f;

	//Glow
	mMainPassCB.Lights[7].Position = { 0.0f, 0.2f, 0.0f };
	mMainPassCB.Lights[7].Strength = { 10.0f, 0.0f, 0.0f };
	mMainPassCB.Lights[7].FalloffStart = 30.0f;
	mMainPassCB.Lights[7].FalloffEnd = 50.0f;


	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void Game::UpdateWaves(const GameTimer& gt)
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
	//auto currWavesVB = mCurrFrameResource->WavesVB.get();
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

void Game::LoadTextures()
{
	auto titleTex = std::make_unique<Texture>();
	titleTex->Name = "titleTex";
	titleTex->Filename = L"../../Textures/TitleScreen.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), titleTex->Filename.c_str(),
		titleTex->Resource, titleTex->UploadHeap));

	auto menuTex = std::make_unique<Texture>();
	menuTex->Name = "menuTex";
	menuTex->Filename = L"../../Textures/MenuScreen.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), menuTex->Filename.c_str(),
		menuTex->Resource, menuTex->UploadHeap));

	auto desertTex = std::make_unique<Texture>();
	desertTex->Name = "desertTex";
	desertTex->Filename = L"../../Textures/desert.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), desertTex->Filename.c_str(),
		desertTex->Resource, desertTex->UploadHeap));

	auto eagleTex = std::make_unique<Texture>();
	eagleTex->Name = "eagleTex";
	eagleTex->Filename = L"../../Textures/EagleDDS.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), eagleTex->Filename.c_str(),
		eagleTex->Resource, eagleTex->UploadHeap));

	auto raptorTex = std::make_unique<Texture>();
	raptorTex->Name = "raptorTex";
	raptorTex->Filename = L"../../Textures/RaptorDDS.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), raptorTex->Filename.c_str(),
		raptorTex->Resource, raptorTex->UploadHeap));

	mTextures[titleTex->Name] = std::move(titleTex);
	mTextures[menuTex->Name] = std::move(menuTex);
	mTextures[desertTex->Name] = std::move(desertTex);
	mTextures[eagleTex->Name] = std::move(eagleTex);
	mTextures[raptorTex->Name] = std::move(raptorTex);

}

void Game::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Performance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void Game::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 10;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto titleTex = mTextures["titleTex"]->Resource;
	auto menuTex = mTextures["menuTex"]->Resource;
	auto desertTex = mTextures["desertTex"]->Resource;
	auto eagleTex = mTextures["eagleTex"]->Resource;
	auto raptorTex = mTextures["raptorTex"]->Resource;


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = desertTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(desertTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = eagleTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(eagleTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = raptorTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(raptorTex.Get(), &srvDesc, hDescriptor);

	//Title Descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = titleTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(titleTex.Get(), &srvDesc, hDescriptor);

	//Title Descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = menuTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(menuTex.Get(), &srvDesc, hDescriptor);

}

void Game::BuildShadersAndInputLayout()
{

	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};


	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_0");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void Game::BuildTreeSpritesGeometry()
{
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 40;
	std::array<TreeSpriteVertex, 40> vertices;
	for (UINT i = 0; i < treeCount; ++i)
	{
		float x = MathHelper::RandF(-100.0f, 100.0f);
		float z = MathHelper::RandF(-100.0f, 100.0f);
		float y = 8;

		// Move tree slightly above land height.
		//y += 8.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 15.0f);
	}

	std::array<std::uint16_t, 40> indices =
	{
		0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15, 16,
		17, 18, 19, 20, 21, 22, 23, 25, 26, 27, 28, 29, 30,
		31, 32, 33, 34, 35, 36, 37, 38, 39
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpritesGeo"] = std::move(geo);
}

void Game::BuildBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);

	std::vector<Vertex> vertices(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		auto& p = box.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = box.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["box"] = submesh;

	mGeometries["boxGeo"] = std::move(geo);
}

void Game::BuildSphereGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);

	std::vector<Vertex> vertices(sphere.Vertices.size());
	for (size_t i = 0; i < sphere.Vertices.size(); ++i)
	{
		auto& p = sphere.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = sphere.Vertices[i].Normal;
		vertices[i].TexC = sphere.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = sphere.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "sphereGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = 0;
	sphereSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["sphere"] = sphereSubmesh;

	mGeometries["sphereGeo"] = std::move(geo);
}

void Game::BuildCylinderGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	std::vector<Vertex> vertices(cylinder.Vertices.size());
	for (size_t i = 0; i < cylinder.Vertices.size(); ++i)
	{
		auto& p = cylinder.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = cylinder.Vertices[i].Normal;
		vertices[i].TexC = cylinder.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = cylinder.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "cylinderGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = 0;
	cylinderSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	mGeometries["cylinderGeo"] = std::move(geo);
}

void Game::BuildTorusGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData torus = geoGen.CreateTorus(1.0f, 1.7f, 12, 20, XM_PI);

	std::vector<Vertex> vertices(torus.Vertices.size());
	for (size_t i = 0; i < torus.Vertices.size(); ++i)
	{
		auto& p = torus.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = torus.Vertices[i].Normal;
		vertices[i].TexC = torus.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = torus.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "torusGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry torusSubmesh;
	torusSubmesh.IndexCount = (UINT)torus.Indices32.size();
	torusSubmesh.StartIndexLocation = 0;
	torusSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["torus"] = torusSubmesh;

	mGeometries["torusGeo"] = std::move(geo);
}

void Game::BuildWedgeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData wedge = geoGen.CreateWedge(2.0f, 1.0f, 1.0f);;

	std::vector<Vertex> vertices(wedge.Vertices.size());
	for (size_t i = 0; i < wedge.Vertices.size(); ++i)
	{
		auto& p = wedge.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = wedge.Vertices[i].Normal;
		vertices[i].TexC = wedge.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = wedge.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "wedgeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry wedgeSubmesh;
	wedgeSubmesh.IndexCount = (UINT)wedge.Indices32.size();
	wedgeSubmesh.StartIndexLocation = 0;
	wedgeSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["wedge"] = wedgeSubmesh;

	mGeometries["wedgeGeo"] = std::move(geo);
}

void Game::BuildTriPrismGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData TriPrism = geoGen.CreateTriPrism(2.0f, 4.0f, 3, 6);

	std::vector<Vertex> vertices(TriPrism.Vertices.size());
	for (size_t i = 0; i < TriPrism.Vertices.size(); ++i)
	{
		auto& p = TriPrism.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = TriPrism.Vertices[i].Normal;
		vertices[i].TexC = TriPrism.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = TriPrism.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "TriPrismGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry TriPrismSubmesh;
	TriPrismSubmesh.IndexCount = (UINT)TriPrism.Indices32.size();
	TriPrismSubmesh.StartIndexLocation = 0;
	TriPrismSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["TriPrism"] = TriPrismSubmesh;

	mGeometries["TriPrismGeo"] = std::move(geo);

}

void Game::BuildPyramidGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(2.0f, 4.0f, 4, 6);

	std::vector<Vertex> vertices(pyramid.Vertices.size());
	for (size_t i = 0; i < pyramid.Vertices.size(); ++i)
	{
		auto& p = pyramid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = pyramid.Vertices[i].Normal;
		vertices[i].TexC = pyramid.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = pyramid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "pyramidGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry pyramidSubmesh;
	pyramidSubmesh.IndexCount = (UINT)pyramid.Indices32.size();
	pyramidSubmesh.StartIndexLocation = 0;
	pyramidSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["pyramid"] = pyramidSubmesh;

	mGeometries["pyramidGeo"] = std::move(geo);
}

void Game::BuildConeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData cone = geoGen.CreateCone(1.0f, 1.0f, 20);

	std::vector<Vertex> vertices(cone.Vertices.size());
	for (size_t i = 0; i < cone.Vertices.size(); ++i)
	{
		auto& p = cone.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = cone.Vertices[i].Normal;
		vertices[i].TexC = cone.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = cone.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "coneGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry coneSubmesh;
	coneSubmesh.IndexCount = (UINT)cone.Indices32.size();
	coneSubmesh.StartIndexLocation = 0;
	coneSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["cone"] = coneSubmesh;

	mGeometries["coneGeo"] = std::move(geo);
}

void Game::BuildDiamondGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData diamond = geoGen.CreateDiamond(2.0f, 4.0f, 4, 3);

	std::vector<Vertex> vertices(diamond.Vertices.size());
	for (size_t i = 0; i < diamond.Vertices.size(); ++i)
	{
		auto& p = diamond.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = diamond.Vertices[i].Normal;
		vertices[i].TexC = diamond.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = diamond.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "diamondGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry diamondSubmesh;
	diamondSubmesh.IndexCount = (UINT)diamond.Indices32.size();
	diamondSubmesh.StartIndexLocation = 0;
	diamondSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["diamond"] = diamondSubmesh;

	mGeometries["diamondGeo"] = std::move(geo);
}

void Game::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(250.0f, 250.0f, 50, 50);

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = 0.1; // Land_Height
		vertices[i].Normal = grid.Vertices[i].Normal;
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["landGeo"] = std::move(geo);
}

void Game::BuildWavesGeometry()
{
	////std::vector<std::uint16_t> indices(3 * world.mWaves->TriangleCount()); // 3 indices per face
	////assert(world.mWaves->VertexCount() < 0x0000ffff);

	//// Iterate over each quad.
	//int m = world.mWaves->RowCount();
	//int n = world.mWaves->ColumnCount();
	//int k = 0;
	//for (int i = 0; i < m - 1; ++i)
	//{
	//	for (int j = 0; j < n - 1; ++j)
	//	{
	//		indices[k] = i * n + j;
	//		indices[k + 1] = i * n + j + 1;
	//		indices[k + 2] = (i + 1) * n + j;

	//		indices[k + 3] = (i + 1) * n + j;
	//		indices[k + 4] = i * n + j + 1;
	//		indices[k + 5] = (i + 1) * n + j + 1;

	//		k += 6; // next quad
	//	}
	//}
	//
	//
	
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(250.0f, 250.0f, 50, 50);
	
	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = 0.1; // Land_Height
		vertices[i].Normal = grid.Vertices[i].Normal;
		vertices[i].TexC = grid.Vertices[i].TexC;
	}
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	std::vector<std::uint16_t> indices = grid.GetIndices16();

	/*UINT vbByteSize = world.mWaves->VertexCount() * sizeof(Vertex);*/
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "desertGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["desertGeo"] = std::move(geo);
}



void Game::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;

	//there is abug with F2 key that is supposed to turn on the multisampling!
//Set4xMsaaState(true);
	//m4xMsaaState = true;

	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//transparentPsoDesc.BlendState.AlphaToCoverageEnable = true;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// PSO for alpha tested objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	//
	// PSO for tree sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};
	//step1
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));
}

void Game::BuildFrameResources() //change 5 to mwaves->vertex count
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), 10));
	}
}

void Game::ResetFrameResources()
{
	mFrameResources.clear();
}

void Game::BuildMaterials()
{
	UINT MatCBIndex = 0;
	UINT DiffuseSrvHeapIndex = 0;

	auto desert = std::make_unique<Material>();
	desert->Name = "desert";
	desert->MatCBIndex = MatCBIndex++;
	desert->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	desert->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	desert->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	desert->Roughness = 0.0f;
	mMaterials["desert"] = std::move(desert);

	auto eagle = std::make_unique<Material>();
	eagle->Name = "eagle";
	eagle->MatCBIndex = MatCBIndex++;
	eagle->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	eagle->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	eagle->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	eagle->Roughness = 0.0f;
	mMaterials["eagle"] = std::move(eagle);

	auto raptor = std::make_unique<Material>();
	raptor->Name = "raptor";
	raptor->MatCBIndex = MatCBIndex++;
	raptor->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	raptor->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	raptor->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	raptor->Roughness = 0.0f;
	mMaterials["raptor"] = std::move(raptor);

	auto title = std::make_unique<Material>();
	title->Name = "title";
	title->MatCBIndex = MatCBIndex++;
	title->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	title->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	title->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	title->Roughness = 0.0f;
	mMaterials["title"] = std::move(title);

	auto menu = std::make_unique<Material>();
	menu->Name = "title";
	menu->MatCBIndex = MatCBIndex++;
	menu->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	menu->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	menu->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	menu->Roughness = 0.0f;
	mMaterials["menu"] = std::move(menu);

}

void Game::RegisterStates()
{
	mStateStack.registerState<TitleState>(States::TITLE);
	mStateStack.registerState<GameState>(States::GAME);
	mStateStack.registerState<MenuState>(States::MENU);
	//mStateStack.registerState<PauseState>(States::Pause);
	//mStateStack.registerState<OptionState>(States::Options);
}

void Game::BuildRenderItems()
{
	//for (std::size_t i = 0; i < LayerCount; ++i)
	//{
	//	SceneNode::Ptr layer(new SceneNode());
	//	mSceneLayers[i] = layer.get();

	//	sceneNode.attachChild(std::move(layer));
	//}

	UINT objCBIndex = 0;

	auto desertRitem = std::make_unique<RenderItem>();
	desertRitem->World = MathHelper::Identity4x4();
	//XMStoreFloat4x4(&desertRitem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0) * XMMatrixTranslation(500.0f, 100.0f, 10.0f));
	XMStoreFloat4x4(&desertRitem->World, XMMatrixScaling(10.0f, 10.0f, 10.0f) * XMMatrixRotationX(XM_PI * -0.5f) * XMMatrixTranslation(0.0f, 0.0f, 800.0f));
	desertRitem->ObjCBIndex = objCBIndex++;
	desertRitem->Mat = mMaterials["desert"].get();
	desertRitem->Geo = mGeometries["desertGeo"].get();
	desertRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	desertRitem->IndexCount = desertRitem->Geo->DrawArgs["grid"].IndexCount;
	desertRitem->StartIndexLocation = desertRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	desertRitem->BaseVertexLocation = desertRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	mDesertRitem = desertRitem.get();
	mRitemLayer[(int)RenderLayer::Opaque].push_back(desertRitem.get());
	mAllRitems.push_back(std::move(desertRitem));
	
	//world.buildScene();
	
}

void Game::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{

	//UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	//UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	//auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	//auto matCB = mCurrFrameResource->MaterialCB->Resource();

	//// For each render item...
	//for (size_t i = 0; i < ritems.size(); ++i)
	//{
	//	auto ri = ritems[i];

	//	cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
	//	cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
	//	cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

	//	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	//	tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

	//	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
	//	D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

	//	cmdList->SetGraphicsRootDescriptorTable(0, tex);
	//	cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
	//	cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

	//	cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	//}


}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Game::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}