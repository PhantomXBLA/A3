#pragma once


//#include "SceneNode.h"
//#include "Aircraft.h"



#include "Waves.h"
#include "World.h"
#include "Player.h"
#include "StateStack.h"


enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class Game : public D3DApp
{
public:

	///Constructor
	///
	/// @param (hInstance) is a class instance
	Game(HINSTANCE hInstance);

	///
	Game(const Game& rhs) = delete;

	Game& operator=(const Game& rhs) = delete;
	
	///Destructor
	/// @params No parameters
	~Game();

	/// This Initialize function
	///
	/// @param has no parameters
	/// @returns returns a bool value
	virtual bool Initialize()override;

private:
	//enum Layer
	//{
	//	Background,
	//	Air,
	//	LayerCount
	//};

private:
	virtual void OnResize()override;

	///Update Function
	///
	/// @param (gt) used to pass game timer to the update function
	/// @returns no return type
	virtual void Update(const GameTimer& gt)override;

	///Draw Function
	///
	/// @param (gt) passing in the game timer
	/// @returns no return type
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	//Aircraft* mPlayerAircraft;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);
	void					registerStates();

	///Load Textures function
	///
	/// @param no parameters
	/// @returns no return type
	void LoadTextures();

	void BuildRootSignature();
	
	///Build Descriptor Heaps function
	///
	/// @param no parameters
	/// @returns no return type
	void BuildDescriptorHeaps();

	///Build Shaders and Input Layout function
	///
	/// @param no parameters
	/// @returns no return type
	void BuildShadersAndInputLayout();

	void BuildLandGeometry();

	void BuildBoxGeometry();
	void BuildSphereGeometry();
	void BuildCylinderGeometry();
	void BuildTorusGeometry();
	void BuildWedgeGeometry();

	void BuildPyramidGeometry();
	void BuildConeGeometry();
	void BuildDiamondGeometry();
	void BuildTriPrismGeometry();
	void BuildTreeSpritesGeometry();

	void BuildShapeGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	
	
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	Player					mPlayer;
	StateStack				mStateStack;

	void					processEvents(const GameTimer& gt);

	//SceneNode sceneNode;
	//std::array<SceneNode*, LayerCount>	mSceneLayers;
	//float GetHillsHeight(float x, float z)const;
	//XMFLOAT3 GetHillsNormal(float x, float z)const;

public:
	void BuildWavesGeometry();
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

	RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

	//World					world;
	PassConstants mMainPassCB;
	PassConstants mSecondaryPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 50.0f;

	POINT mLastMousePos;

};

