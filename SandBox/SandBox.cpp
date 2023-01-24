
//#define SOL_CXX_LUA



#include <DirectXMath.h>

#include "src/Hulk.h"
#include "dxgidebug.h"
#include <map>



using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Hulk;


 

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	std::string name;
	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = Hulk::MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = Hulk::MathHelper::Identity4x4();
	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Hulk::MeshGeometry* Geo = nullptr;
	Hulk::Material* mMaterial = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	int IndexCount = 0;
	int StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	markStencilMirrors,
	Reflected,
	Shadow,
	TreeSpriteAlphaTested,
	Count
};


class SandBox : public Hulk::D3DApp
{
public:
	SandBox();
	~SandBox();

	virtual bool Initialize(HINSTANCE hInstance) override;

private:
	virtual void OnResize()override;
	virtual void Update(const Hulk::GameTimer& gt)override;
	virtual void Draw(const Hulk::GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	
private:
	
	
	void BuildMaterials();
	void BuildTextures();
	void BuildDescriptorHeaps();
	void BuildConstantBuffersViews();
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
	void BuildSamplerViews();
	void BuildShaderResourcesViews();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildSkullGeo();
	void BuildLandGeo();
	void BuildWaveGeo();
	void BuildBoxGeo();
	void BuildTreeSpriteGeo();
	void BuildPSO();
	void BuildFrameResources();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>&ritems);
	void CalculateNormals();

	float GetHillsHeight(float x, float z) const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;

	void OnKeyboardInput(const Hulk::GameTimer& gt);
	void UpdateCamera(const Hulk::GameTimer& gt);
	void UpdateMainPassCB(const Hulk::GameTimer& gt);
	void UpdateReflectedPassCB(const GameTimer& gt);
	void UpdateObjectCBs(Hulk::GameTimer const & gt);
	void UpdateMaterialCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);

private:
	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	ComPtr<ID3D12DescriptorHeap> mSamplerHeap;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout, mTreeSpriteInputLayout;
	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;
	std::vector<std::unique_ptr<Hulk::FrameResource>> mFrameResources;

	ComPtr<ID3D12RootSignature> mRootSignature;


	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;//Scene Index 0
	std::vector<std::unique_ptr<RenderItem>> mAllRitems1;//Scene Index 0

	std::unordered_map<std::string, std::unique_ptr<Hulk::MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Hulk::Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Hulk::Texture>> mTextures;
	std::vector<std::unique_ptr<Hulk::Texture>> mBoltTextures;
	

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	

	// Render items divided by PSO. these are arrays of std::vectors that contains ritems adresses
	std::vector<RenderItem*> mRitemLayerLand[(int)RenderLayer::Count];// wave and land demo
	std::vector<RenderItem*> mRitemLayerShapes[(int)RenderLayer::Count];// shapes and skull demo

	PassConstants mMainPassCB;
	PassConstants mReflectedPassCB; // for reflecting light for reflected object on mirrors

	UINT mPassCbvOffset = 0;
	UINT mMaterialCbvOffset = 0;//not used anymore changed it

	bool renderingBolt = false;
	UINT mBoltTextureIndex = 8;
	XMFLOAT4X4 mBoltTextureTransform = MathHelper::Identity4x4();

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
	XMMATRIX lightRotMat = XMMatrixIdentity();
	
	float mSunTheta = 1.25f*XM_PI;
	float mSunPhi = XM_PIDIV4;
	
	float mTheta = 1.5f*XM_PI;
	float mPhi = XM_PIDIV4;
	UINT NumberOfObjectsToDraw=2;
	
	int mCurrentFrameResourceIndex = 0;
	FrameResource* mCurrentFrameResource = nullptr;

	// cache render items of interest
	RenderItem* mWavesRitem = nullptr;
	RenderItem* mReflectedSkullRitem = nullptr;
	RenderItem* mSkullRitem = nullptr;
	RenderItem* mShadowSkullRitem = nullptr;

	XMFLOAT3 mSkullTranslation = {0.0f, 1.0f, -6.0f};

	float mRadius = 5.0f;
	


	std::unique_ptr<Waves> mWaves;

	POINT mLastMousePos;

	
	
	
};



D3DApp* Hulk::CreateApplication()
{
	return new SandBox();
	
}

SandBox::SandBox()
	: D3DApp()
{
	
}

SandBox::~SandBox()
{
	Shutdown();
	
}

bool SandBox::Initialize(HINSTANCE hInstance)
{
	HK_PROFILE_FUNCTION();
	
	if (!D3DApp::Initialize(hInstance))
		return false;
	 
	//Reset the command list to prep for initialization commands
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	
	mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);//init the waves

	BuildTextures();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildMaterials();
	BuildLandGeo();
	BuildShapeGeometry();
	BuildWaveGeo();
	BuildBoxGeo();
	BuildSkullGeo();
	BuildTreeSpriteGeo();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();//at this moment used for srvs for textures and one other separate heap for one srv for imgui
	BuildShaderResourcesViews();
	//BuildSamplerViews(); using static samplers
	BuildPSO();
	//Execute the init commands
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//Wait until init is complete
	FlushCommandQueue();
	
	ImGuiInitialize((HWND)mMainWnd->GetNativeWindow(),md3dDevice.Get(), gNumFrameResources,
		mBackBufferFormat);
	
	HK_INFO("Initialized APP!");
	
	return true;
}


void SandBox::OnResize()
{
	
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void SandBox::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture((HWND)mMainWnd->GetNativeWindow());
}

void SandBox::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void SandBox::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0 && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;

}



void SandBox::BuildMaterials()
{

	int MatCBIndex = 0;


	auto bricks0 = std::make_unique<Material>();
	bricks0->Name = "bricks";
	bricks0->MatCBIndex = MatCBIndex;
	bricks0->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(bricks0->Name)));// there is no function to get index from unordered map so i use this trick to get the index to use to get the srv in the heap.
	bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks0->Roughness = 0.1f;
	
	auto tile0 = std::make_unique<Material>();
	tile0->Name = "Tile";
	tile0->MatCBIndex = ++MatCBIndex;
	tile0->DiffuseSrvHeapIndex = static_cast<int>( distance(mTextures.begin(), mTextures.find(tile0->Name)));
	tile0->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
	tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.2f;
	//XMStoreFloat4x4(&tile0->MatTransform, XMMatrixScaling(2.0f, 2.0f, 1.0f)); //can transform text with matTransform matrix too
	
	 auto stoneMat = std::make_unique<Material>();
	 stoneMat ->Name = "stone";
	 stoneMat ->MatCBIndex = ++MatCBIndex;
	 stoneMat->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(stoneMat->Name)));
	 stoneMat ->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	 stoneMat ->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	 stoneMat ->Roughness = 0.4f;	

	 auto GrassMat = std::make_unique<Material>();
	 GrassMat->Name = "grass";
	 GrassMat->MatCBIndex = ++MatCBIndex;
	 GrassMat->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(GrassMat->Name)));
	 GrassMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);;
	 GrassMat->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	 GrassMat->Roughness = 0.0f;

	 // This is not a good water material definition, but we do not have all the rendering
	// tools we need (environment reflection), so we fake it for now.
	 auto waterMat = std::make_unique<Material>();
	 waterMat->Name = "water";
	 waterMat->MatCBIndex = ++MatCBIndex;
	 waterMat->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(waterMat->Name)));
	 waterMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	 waterMat->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	 waterMat->Roughness = 0.0f;

	 auto wireFenceMat = std::make_unique<Material>();
	 wireFenceMat->Name = "Bolt1";
	 wireFenceMat->MatCBIndex = ++MatCBIndex;
	 wireFenceMat->DiffuseSrvHeapIndex = mTextures.size();
	 wireFenceMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	 wireFenceMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	 wireFenceMat->Roughness = 0.0f;


	 auto iceMirrorMat = std::make_unique<Material>();
	 iceMirrorMat->Name = "iceMirror";
	 iceMirrorMat->MatCBIndex = ++MatCBIndex;
	 iceMirrorMat->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(iceMirrorMat->Name)));;
	 iceMirrorMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	 iceMirrorMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	 iceMirrorMat->Roughness = 0.5f;



	 auto iceMirrorMatA = std::make_unique<Material>();
	 iceMirrorMatA->Name = "iceMirror2";
	 iceMirrorMatA->MatCBIndex = ++MatCBIndex;
	 iceMirrorMatA->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(iceMirrorMat->Name)));;
	 iceMirrorMatA->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	 iceMirrorMatA->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	 iceMirrorMatA->Roughness = 0.5f;



	 auto skullMat = std::make_unique<Material>();
	 skullMat->Name = "skullMat";
	 skullMat->MatCBIndex = ++MatCBIndex;
	 skullMat->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find("white")));
	 skullMat->DiffuseAlbedo = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	 skullMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	 skullMat->Roughness = 0.3f;




	 auto shadowMat = std::make_unique<Material>();
	 shadowMat->Name = "shadowMat";
	 shadowMat->MatCBIndex = ++MatCBIndex;
	 shadowMat->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find("white")));
	 shadowMat->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	 shadowMat->FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
	 shadowMat->Roughness = 0.0f;


	 auto treeSpriteMat = std::make_unique<Material>();
	 treeSpriteMat->Name = "TreeSprites";
	 treeSpriteMat->MatCBIndex = ++MatCBIndex;
	 treeSpriteMat->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(treeSpriteMat->Name)));
	 treeSpriteMat->DiffuseAlbedo = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
	 treeSpriteMat->FresnelR0 = XMFLOAT3(0.01f,0.01f,0.01f);
	 treeSpriteMat-> Roughness = 0.125f;

	mMaterials["TreeSprites"] = std::move(treeSpriteMat);
	mMaterials["icemirror2"] = std::move(iceMirrorMatA);
	mMaterials["bricks"] = std::move(bricks0);
	mMaterials["stone"] = std::move(stoneMat);
	mMaterials["tile"] = std::move(tile0);
	mMaterials["grass"] = std::move(GrassMat);
	mMaterials["water"] = std::move(waterMat);
	mMaterials["Bolt1"] = std::move(wireFenceMat);
	mMaterials["icemirror"] = std::move(iceMirrorMat);
	mMaterials["skullMat"] = std::move(skullMat);
	mMaterials["shadowMat"] = std::move(shadowMat);
}

void SandBox::BuildTextures()
{
	auto floorTileText = std::make_unique<Texture>();
	floorTileText->Name = "Tile";
	floorTileText->Filename = L"..\\Textures\\tile.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(),
		floorTileText->Filename.c_str(), floorTileText->Resource, floorTileText->UploadHeap
	));
	mTextures[floorTileText->Name] = std::move(floorTileText);

	auto bricksTex = std::make_unique<Texture>();
	bricksTex->Name = "bricks";
	bricksTex->Filename = L"..\\Textures\\bricks3.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), bricksTex->Filename.c_str(),
		bricksTex->Resource, bricksTex->UploadHeap));
	
	mTextures[bricksTex->Name] = std::move(bricksTex);

	auto stoneTex = std::make_unique<Texture>();
	stoneTex->Name = "stone";
	stoneTex->Filename = L"..\\Textures\\stone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), stoneTex->Filename.c_str(),
		stoneTex->Resource, stoneTex->UploadHeap));

	//HK_INFO(stoneTex->Resource->GetDesc().Format); wanted to check which bc format was the texture compressed to
	mTextures[stoneTex->Name] = std::move(stoneTex);

	auto GrassTex = std::make_unique<Texture>();
	GrassTex->Name = "grass";
	GrassTex->Filename = L"..\\Textures\\grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), GrassTex->Filename.c_str(),
		GrassTex->Resource, GrassTex->UploadHeap));

	mTextures[GrassTex->Name] = std::move(GrassTex);

	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "water";
	waterTex->Filename = L"..\\Textures\\water1.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap));

	mTextures[waterTex->Name] = std::move(waterTex);


	auto fenceTex = std::make_unique<Texture>();
	fenceTex->Name = "wirefence";
	fenceTex->Filename = L"..\\Textures\\WireFence.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), fenceTex->Filename.c_str(),
		fenceTex->Resource, fenceTex->UploadHeap));

	mTextures[fenceTex->Name] = std::move(fenceTex);



	auto iceMirrorTex = std::make_unique<Texture>();
	iceMirrorTex->Name = "iceMirror";
	iceMirrorTex->Filename = L"..\\Textures\\ice.dds";
	
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), iceMirrorTex->Filename.c_str(),
		iceMirrorTex->Resource, iceMirrorTex->UploadHeap));

	mTextures[iceMirrorTex->Name] = std::move(iceMirrorTex);



	auto whiteTex = std::make_unique<Texture>();
	whiteTex->Name = "white";
	whiteTex->Filename = L"..\\Textures\\white1x1.dds";

	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), whiteTex->Filename.c_str(),
		whiteTex->Resource, whiteTex->UploadHeap));

	mTextures[whiteTex->Name] = std::move(whiteTex);


	auto TreeSpriteTex = std::make_unique<Texture>();
	TreeSpriteTex->Name = "TreeSprites";
	TreeSpriteTex->Filename = L"..\\Textures\\treearray2.dds";

   ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
			mCommandList.Get(), TreeSpriteTex->Filename.c_str(),
		TreeSpriteTex->Resource, TreeSpriteTex->UploadHeap));



	mTextures[TreeSpriteTex->Name] = std::move(TreeSpriteTex);





	ResourceUploadBatch upload(md3dDevice.Get());
	upload.Begin();


	for (int i = 0; i < 60; ++i)
	{
		std::wstring filename = L"../Textures/BoltAnim/Bolt";

		if (i + 1 <= 9)
			filename += L"0";

		if (i + 1 <= 99)
			filename += L"0";

		std::wstringstream frameNum;
		frameNum << i + 1;
		filename += frameNum.str();
		filename += L".bmp";
		std::wstring S = frameNum.str();
	
		auto Tex = std::make_unique<Texture>();
		Tex->Name = std::string("Bolt") + ws2s(S.c_str());
		Tex->Filename = filename;

		ThrowIfFailed(DirectX::CreateWICTextureFromFile(md3dDevice.Get(),
			upload, Tex->Filename.c_str(),
			Tex->Resource.GetAddressOf()));

		mBoltTextures.push_back(std::move(Tex));
	}

	auto finish = upload.End(mCommandQueue.Get());

	finish.wait();


	

	HK_INFO("Loaded textures!");
}



void SandBox::BuildDescriptorHeaps()
{
	
	UINT numDescriptors = 0;

	if (mTextures.size() > 0 || mBoltTextures.size() > 0)
		numDescriptors = mTextures.size() + mBoltTextures.size();
	
	D3D12_DESCRIPTOR_HEAP_DESC ImguiSrvHeapDesc{}; //separate srv heap for imgui
	ImguiSrvHeapDesc.NumDescriptors =  1;
	ImguiSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ImguiSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ImguiSrvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&ImguiSrvHeapDesc, IID_PPV_ARGS(&ImguiSrvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc{};
	SrvHeapDesc.NumDescriptors = numDescriptors;
	SrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	SrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	SrvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));

	/*D3D12_DESCRIPTOR_HEAP_DESC SamplerHeapDesc{};
	SamplerHeapDesc.NumDescriptors = 1;
	SamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	SamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&SamplerHeapDesc, IID_PPV_ARGS(&mSamplerHeap)));*/

}

void SandBox::BuildConstantBuffersViews()
{
	
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> SandBox::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,// FILTER
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,// u address mode
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,// v address mode
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);//w address mode (for 3d textures)

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,// FILTER
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// u address mode
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,//v address mode
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);//w address mode (for 3d textures)

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,// FILTER
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,// u address mode
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,//v address mode
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);//w address mode (for 3d textures)

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,// FILTER
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// u address mode
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,//v address mode
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);//w address mode (for 3d textures)

	 const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,// FILTER
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,// u address mode
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,//v address mode
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);//w address mode (for 3d textures)
	
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,// FILTER
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// u address mode
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,//v address mode
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);//w address mode (for 3d textures)


	return { pointWrap,pointClamp,
			linearWrap,linearClamp,
			anisotropicWrap,anisotropicClamp };

}

void SandBox::BuildSamplerViews()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSamplerHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SAMPLER_DESC mSamplerViewDesc;
	mSamplerViewDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	mSamplerViewDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	mSamplerViewDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	mSamplerViewDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	mSamplerViewDesc.MinLOD = 0;
	mSamplerViewDesc.MaxLOD = D3D12_FLOAT32_MAX;
	mSamplerViewDesc.MipLODBias = 0;
	mSamplerViewDesc.MaxAnisotropy = 1;
	mSamplerViewDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	md3dDevice->CreateSampler(&mSamplerViewDesc, hDescriptor);
}
void SandBox::BuildShaderResourcesViews()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvHeap->GetCPUDescriptorHandleForHeapStart());

	for (auto& text : mTextures)
	{
		if (text.second->Resource->GetDesc().DepthOrArraySize <= 1)
		{

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = text.second->Resource->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = text.second->Resource->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			md3dDevice->CreateShaderResourceView(text.second->Resource.Get(), &srvDesc, hDescriptor);

			hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
		}

		else
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Format = text.second->Resource->GetDesc().Format;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = text.second->Resource->GetDesc().MipLevels;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = text.second->Resource->GetDesc().DepthOrArraySize;
			md3dDevice->CreateShaderResourceView(text.second->Resource.Get(), &srvDesc, hDescriptor);

			hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);

		}
	}


	for (auto& text : mBoltTextures)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = text->Resource->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = text->Resource->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		md3dDevice->CreateShaderResourceView(text->Resource.Get(), &srvDesc, hDescriptor);

		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
}

void SandBox::BuildRootSignature()
{
	
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	auto staticSamplers = GetStaticSamplers();

	//create 2 descriptors ranges 
	CD3DX12_DESCRIPTOR_RANGE SrvSamplerRanges[2]{};
	SrvSamplerRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	SrvSamplerRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 6);//using sampler shader register s6 but not set up on hlsl yet (its for a sampler on the heap but im using static samplers so i dont need this one)

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);//init the first root parameter as Cbv for the object Constant buffer
	slotRootParameter[1].InitAsConstantBufferView(1);//init the second root parameter as Cbv for the material Constant buffer 
	slotRootParameter[2].InitAsConstantBufferView(2);//init the third root parameter as Cbv for the per Pass Constant buffer
	slotRootParameter[3].InitAsDescriptorTable(1, &SrvSamplerRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);//srv
	slotRootParameter[4].InitAsDescriptorTable(1, &SrvSamplerRanges[1], D3D12_SHADER_VISIBILITY_PIXEL);//sampler not used :D im using static samplers


	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT); //root signature description
	
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	/*	This root signature description will then be serialized with D3D12SerializeRootSignature function, which will create a corresponding binary blob.
		The blob can then be used as input of ID3D12Device::CreateRootSignature to finally create the ID3D12RootSignature referenced object.
		Note: the binary blob is made to store the root signature in permanent memory, e.g. in shader cache, so once it has been computed,
		it can be loaded on next applications run without the need of recomputing it again! but here im not cashing it so im just serializing everytime i start the app*/

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		                                     serializedRootSig.GetAddressOf(),
		                                     errorBlob.ReleaseAndGetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(md3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer()
		           ,serializedRootSig->GetBufferSize()
		           ,IID_PPV_ARGS(&mRootSignature)));

}

void SandBox::BuildShadersAndInputLayout()
{


	const D3D_SHADER_MACRO defines[] =
	{
		"FOG" , "1"
		,NULL,NULL

	};

	
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};


	mShaders["Opaque_VS"] = d3dUtil::CompileShader(L"..\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["Opaque_PS"] = d3dUtil::CompileShader(L"..\\Shaders\\color.hlsl", defines, "PS", "ps_5_0");
	mShaders["Alpha_PS"] = d3dUtil::CompileShader(L"..\\Shaders\\color.hlsl", alphaTestDefines, "PS", "ps_5_0");

	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"..\\Shaders\\TreeSprite.hlsl",nullptr,"VS", "vs_5_0");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"..\\Shaders\\TreeSprite.hlsl",nullptr,"GS", "gs_5_0");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"..\\Shaders\\TreeSprite.hlsl",alphaTestDefines,"PS", "ps_5_0");

	mInputLayout =
	{ 
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"TEXTCOR",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	 
	mTreeSpriteInputLayout =
	{
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"SIZE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	HK_INFO("Initialized Shaders");

}

void SandBox::BuildShapeGeometry()
{

	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f,60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	// Define the SubmeshGeometry that cover different 
	// regions of the vertex/index buffers.

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;




	std::array<Vertex, 16> wallVertices =
	{
	

		// Wall: we tile texture coordinates, and we
		// leave a gap in the middle for the mirror.
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 0
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4 
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 8
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		// Mirror
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};

	std::array<std::uint16_t, 24> wallIndices =
	{

		// Walls
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		// Mirror
		12, 13, 14,
		12, 14, 15
	};


	


	UINT wallVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
	UINT wallIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
		



	SubmeshGeometry wallSubmesh;
	wallSubmesh.IndexCount = (UINT) 18;
	wallSubmesh.StartIndexLocation = wallIndexOffset;
	wallSubmesh.BaseVertexLocation = wallVertexOffset;


	SubmeshGeometry mirrorSubmesh;
	mirrorSubmesh.IndexCount = 6;
	mirrorSubmesh.StartIndexLocation = wallIndexOffset + wallSubmesh.IndexCount;
	mirrorSubmesh.BaseVertexLocation = wallVertexOffset;


	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size() +
		wallVertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexCord = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexCord = grid.Vertices[i].TexC;
		
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexCord = sphere.Vertices[i].TexC;
		
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexCord = cylinder.Vertices[i].TexC;
		
	}


	for (size_t i = 0; i < wallVertices.size(); ++i, ++k)
	{
		vertices[k].Pos = wallVertices[i].Pos;
		vertices[k].Normal = wallVertices[i].Normal;
		vertices[k].TexCord = wallVertices[i].TexCord;

	}



	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(wallIndices), std::end(wallIndices));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

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

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["wall"] = wallSubmesh;
	geo->DrawArgs["mirror"] = mirrorSubmesh;

	mGeometries[geo->Name] = std::move(geo);




}


void SandBox::BuildSkullGeo()
{
	
	SubmeshGeometry skull;
	
	std::ifstream fin("E:\\Desktop\\d3d cd\\Code.Textures\\Chapter 8 Lighting\\LitColumns\\Models\\skull.txt");

	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	
	skull.IndexCount = (tcount * 3);
	skull.BaseVertexLocation = 0;
	skull.StartIndexLocation = 0;
	
	std::vector<Vertex> skullet(vcount);
	
	for (UINT i = 0; i < vcount; i++)
	{
		
		fin >> skullet[i].Pos.x >> skullet[i].Pos.y >> skullet[i].Pos.z;

		fin >> skullet[i].Normal.x >> skullet[i].Normal.y >> skullet[i].Normal.z; 
																	
		// dont have texture coord for skull :(



	}
	
	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<std::uint32_t> indicesS(3 * tcount);
	for (UINT i = 0; i < tcount; i++)
	{
		fin >> indicesS[i * 3 + 0] >> indicesS[i * 3 + 1] >> indicesS[i * 3 + 2];
	}
	
	fin.close();

	
	const size_t vbByteSize = skullet.size() * sizeof(Vertex);
	const size_t ibByteSize = indicesS.size() * sizeof(std::uint32_t);

	auto skullgeo = std::make_unique<MeshGeometry>();
	skullgeo->Name = "skullGeo";

	skullgeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), skullet.data(), vbByteSize, skullgeo->VertexBufferUploader);

	skullgeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indicesS.data(), ibByteSize, skullgeo->IndexBufferUploader);

	skullgeo->VertexByteStride = sizeof(Vertex);
	skullgeo->VertexBufferByteSize = vbByteSize;
	skullgeo->IndexFormat = DXGI_FORMAT_R32_UINT;
	skullgeo->IndexBufferByteSize = ibByteSize;

	skullgeo->DrawArgs["skull"] = skull;

	mGeometries[skullgeo->Name] = std::move(skullgeo);
	
}

void SandBox::BuildLandGeo()
{
	GeometryGenerator Geogen;
	GeometryGenerator::MeshData Grid = Geogen.CreateGrid(160.0f,160.0f,50.0f,50.0f);


	std::vector<Vertex> vertices(Grid.Vertices.size());

	for (size_t i = 0; i < Grid.Vertices.size(); i++) 
	{
		auto& p = Grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x,p.z);
		vertices[i].TexCord = Grid.Vertices[i].TexC;
		

	}

	const UINT VbBytesize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = Grid.GetIndices16();

	const UINT IndexBufferByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "LandGeo";

	ThrowIfFailed(D3DCreateBlob(VbBytesize, geo->VertexBufferCPU.GetAddressOf()));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), VbBytesize);

	ThrowIfFailed(D3DCreateBlob(IndexBufferByteSize, geo->IndexBufferCPU.GetAddressOf()));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), IndexBufferByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), VbBytesize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), IndexBufferByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = VbBytesize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = IndexBufferByteSize;

	SubmeshGeometry submesh;
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;
	submesh.IndexCount = (UINT)indices.size();

	geo->DrawArgs["Gridg"] = submesh;
	mGeometries["LandGeo"] = std::move(geo);



}

void SandBox::BuildWaveGeo()
{
	std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

	// Iterate over each quad.
	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "WaterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

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

	geo->DrawArgs["GridW"] = submesh;

	mGeometries["WaterGeo"] = std::move(geo);
}

void SandBox::BuildBoxGeo()
{
	GeometryGenerator Geogen;
	GeometryGenerator::MeshData box = Geogen.CreateCylinder(2.0f, 2.0f, 4.0f, 40.0f,40.0f);
	
	std::vector<Vertex> vertices(box.Vertices.size());

	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		auto& p = box.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexCord = box.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = box.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "BoxGeo";

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

	mGeometries["BoxGeo"] = std::move(geo);

}

void SandBox::BuildTreeSpriteGeo()
{
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	std::array<TreeSpriteVertex, 16> vertices;

	// since these are points, im giving them world space coord directly. 
	//(im not multiplying by a world matrix in the shader just the view and proj matrix)
	for (int i = 0; i < 16; ++i)
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);

		// lift the trees a lil bit over the land surface
		y += 8.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);


	}
		
	std::array<std::uint16_t, 16> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	const UINT vbSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto Geo = std::make_unique<MeshGeometry>();

	Geo->Name = "TreeSpritesGeo";


	ThrowIfFailed(D3DCreateBlob(vbSize, Geo->VertexBufferCPU.GetAddressOf()));
	CopyMemory(Geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbSize);

	ThrowIfFailed(D3DCreateBlob(ibSize, Geo->IndexBufferCPU.GetAddressOf()));
	CopyMemory(Geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibSize);


	Geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),mCommandList.Get(),vertices.data(),vbSize,Geo->VertexBufferUploader);
	Geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibSize, Geo->IndexBufferUploader);

	Geo->VertexByteStride = sizeof(TreeSpriteVertex);
	Geo->VertexBufferByteSize = vbSize;
	Geo->IndexBufferByteSize = ibSize;
	Geo->IndexFormat = DXGI_FORMAT_R16_UINT;

	SubmeshGeometry submesh;

	submesh.BaseVertexLocation = 0;
	submesh.IndexCount = (UINT)indices.size();;
	submesh.StartIndexLocation = 0;
	

	Geo->DrawArgs["points"] = submesh;

	mGeometries["TreeSpritesGeo"] = std::move(Geo);


}

void SandBox::BuildFrameResources() 

{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 2, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
	}


}
void SandBox::BuildRenderItems()
{
	int ObCBIndex = 0;

	// Land Grid Render Item
	auto landRitem = std::make_unique<RenderItem>();
	landRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&landRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	landRitem->ObjCBIndex = ObCBIndex;
	landRitem->Geo = mGeometries["LandGeo"].get();
	landRitem->mMaterial = mMaterials["grass"].get();
	landRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	landRitem->IndexCount = landRitem->Geo->DrawArgs["Gridg"].IndexCount;
	landRitem->StartIndexLocation = landRitem->Geo->DrawArgs["Gridg"].StartIndexLocation;
	landRitem->BaseVertexLocation = landRitem->Geo->DrawArgs["Gridg"].BaseVertexLocation;

	mRitemLayerLand[(int)RenderLayer::Opaque].push_back(landRitem.get());
	mAllRitems.push_back(std::move(landRitem));


	auto LboxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&LboxRitem->World, XMMatrixTranslation(1.0f, 3.0f, -6.0f));
	XMStoreFloat4x4(&LboxRitem->TexTransform, XMMatrixScaling(1.0f, 1.5f, 1.0f));
	LboxRitem->ObjCBIndex = ++ObCBIndex;
	LboxRitem->mMaterial = mMaterials["Bolt1"].get();
	LboxRitem->Geo = mGeometries["BoxGeo"].get();
	LboxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	LboxRitem->IndexCount = LboxRitem->Geo->DrawArgs["box"].IndexCount;
	LboxRitem->StartIndexLocation = LboxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	LboxRitem->BaseVertexLocation = LboxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	mRitemLayerLand[(int)RenderLayer::AlphaTested].push_back(LboxRitem.get());

	mAllRitems.push_back(std::move(LboxRitem));


	auto waterRitem = std::make_unique<RenderItem>();
	waterRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&waterRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	waterRitem->ObjCBIndex = ++ObCBIndex;
	waterRitem->Geo = mGeometries["WaterGeo"].get();
	waterRitem->mMaterial = mMaterials["water"].get();
	waterRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	waterRitem->IndexCount = waterRitem->Geo->DrawArgs["GridW"].IndexCount;
	waterRitem->StartIndexLocation = waterRitem->Geo->DrawArgs["GridW"].StartIndexLocation;
	waterRitem->BaseVertexLocation = waterRitem->Geo->DrawArgs["GridW"].BaseVertexLocation;

	mWavesRitem = waterRitem.get();

	mRitemLayerLand[(int)RenderLayer::Transparent].push_back(mWavesRitem);
	mAllRitems.push_back(std::move(waterRitem));

	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	treeSpritesRitem->ObjCBIndex = ++ObCBIndex;
	treeSpritesRitem->mMaterial = mMaterials["TreeSprites"].get(); 
	treeSpritesRitem->Geo = mGeometries["TreeSpritesGeo"].get();
	treeSpritesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayerLand[(int)RenderLayer::TreeSpriteAlphaTested].push_back(treeSpritesRitem.get());
	mAllRitems.push_back(std::move(treeSpritesRitem));


	auto skullRitem = std::make_unique<RenderItem>();
	skullRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRitem->ObjCBIndex = ++ObCBIndex;
	skullRitem->Geo = mGeometries["skullGeo"].get();
	skullRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
	skullRitem->mMaterial = mMaterials["skullMat"].get();

	mSkullRitem = skullRitem.get();

	mRitemLayerShapes[(int)RenderLayer::Opaque].push_back(skullRitem.get());
	mAllRitems.push_back(std::move(skullRitem));

	//Box render item
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f) * XMMatrixRotationY(0.45f));
	boxRitem->ObjCBIndex = ++ObCBIndex;
	boxRitem->Geo = mGeometries["shapeGeo"].get();
	boxRitem->mMaterial = mMaterials["bricks"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	
	mRitemLayerShapes[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	mAllRitems.push_back(std::move(boxRitem));
	
	//Grid Render Item
	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridRitem->ObjCBIndex = ++ObCBIndex;
	gridRitem->Geo = mGeometries["shapeGeo"].get();
	gridRitem->mMaterial = mMaterials["tile"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	
	mRitemLayerShapes[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	mAllRitems.push_back(std::move(gridRitem));


	// for the wall in shape scene

	auto wallRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&wallRitem->World, XMMatrixScaling(1.5f, 1.5f, 1.5f) * XMMatrixTranslation(-1.0f, 0.0f, 3.0f) * XMMatrixRotationY(0.0f));
	XMStoreFloat4x4(&wallRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	wallRitem->ObjCBIndex = ++ObCBIndex;
	wallRitem->name = "wall";
	wallRitem->Geo = mGeometries["shapeGeo"].get();
	wallRitem->mMaterial = mMaterials["bricks"].get();
	wallRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wallRitem->IndexCount = wallRitem->Geo->DrawArgs["wall"].IndexCount;
	wallRitem->StartIndexLocation = wallRitem->Geo->DrawArgs["wall"].StartIndexLocation;
	wallRitem->BaseVertexLocation = wallRitem->Geo->DrawArgs["wall"].BaseVertexLocation;
		
	mRitemLayerShapes[(int)RenderLayer::Opaque].push_back(wallRitem.get());
	mAllRitems.push_back(std::move(wallRitem));

	// for the mirror in shape scene
	auto mirrorRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&mirrorRitem->World, XMMatrixScaling(1.5f, 1.5f, 1.5f)* XMMatrixTranslation(-1.0f, 0.0f, 3.0f)* XMMatrixRotationY(0.0f));
	mirrorRitem->TexTransform = MathHelper::Identity4x4();
	mirrorRitem->ObjCBIndex = ++ObCBIndex;
	mirrorRitem->mMaterial = mMaterials["icemirror"].get();
	mirrorRitem->Geo = mGeometries["shapeGeo"].get();
	mirrorRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorRitem->IndexCount = mirrorRitem->Geo->DrawArgs["mirror"].IndexCount;
	mirrorRitem->StartIndexLocation = mirrorRitem->Geo->DrawArgs["mirror"].StartIndexLocation;
	mirrorRitem->BaseVertexLocation = mirrorRitem->Geo->DrawArgs["mirror"].BaseVertexLocation;
	mRitemLayerShapes[(int)RenderLayer::markStencilMirrors].push_back(mirrorRitem.get());
	mRitemLayerShapes[(int)RenderLayer::Transparent].push_back(mirrorRitem.get());
	mAllRitems.push_back(std::move(mirrorRitem));

	// Reflected skull will have different world matrix, so it needs to be its own render item.

	auto reflectedSkullRitem = std::make_unique<RenderItem>();
	*reflectedSkullRitem = *mSkullRitem;
	reflectedSkullRitem->ObjCBIndex = ++ObCBIndex;

	mReflectedSkullRitem = reflectedSkullRitem.get();
	mRitemLayerShapes[(int)RenderLayer::Reflected].push_back(reflectedSkullRitem.get());
	mAllRitems.push_back(std::move(reflectedSkullRitem));


	auto shadowSkullRitem = std::make_unique<RenderItem>();
	*shadowSkullRitem = *mSkullRitem;
	shadowSkullRitem->ObjCBIndex = ++ObCBIndex;
	shadowSkullRitem->mMaterial = mMaterials["shadowMat"].get();


	mShadowSkullRitem = shadowSkullRitem.get();
	mRitemLayerShapes[(int)RenderLayer::Shadow].push_back(shadowSkullRitem.get());
	mAllRitems.push_back(std::move(shadowSkullRitem));
	


	// left and right cones and spheres (5 each) so we need 4*5 views in the heap 
	     
	UINT objCBIndex = ++ObCBIndex;
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		auto rightCylRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Geo = mGeometries["shapeGeo"].get();
		leftCylRitem->mMaterial = mMaterials["bricks"].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
		

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Geo = mGeometries["shapeGeo"].get();
		rightCylRitem->mMaterial = mMaterials["bricks"].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
		
		
		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
		leftSphereRitem->mMaterial = mMaterials["stone"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
		rightSphereRitem->mMaterial = mMaterials["stone"].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		


		mRitemLayerShapes[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
		mRitemLayerShapes[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
		mRitemLayerShapes[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());
		mRitemLayerShapes[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());

		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}
	
	

	
}
void SandBox::DrawRenderItems(ID3D12GraphicsCommandList * cmdList, const std::vector<RenderItem*>&ritems)
{

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBBytesize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto MatCB = mCurrentFrameResource->MaterialCB->Resource();
	auto CB = mCurrentFrameResource->ObjectCB->Resource();
	

	

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];
	
		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
	
		CD3DX12_GPU_DESCRIPTOR_HANDLE Tex(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	if(renderingBolt == true)
		Tex.Offset(mBoltTextureIndex, mCbvSrvUavDescriptorSize);
	else
		Tex.Offset(ri->mMaterial->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize); 

		cmdList->SetGraphicsRootDescriptorTable(3, Tex);
		
		D3D12_GPU_VIRTUAL_ADDRESS CbHandle = CB->GetGPUVirtualAddress() + ri->ObjCBIndex*matCBBytesize;


		cmdList->SetGraphicsRootConstantBufferView(0, CbHandle);

		//not using desc table for the material but using directly a rootconstant
		D3D12_GPU_VIRTUAL_ADDRESS matCbHandle = MatCB->GetGPUVirtualAddress() + ri->mMaterial->MatCBIndex*matCBBytesize;
	
		cmdList->SetGraphicsRootConstantBufferView(1, matCbHandle);
		

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	renderingBolt = false;
}

void SandBox::CalculateNormals()
{
	// TO DO for object that dont have normale (dont have them yet)
}

float SandBox::GetHillsHeight(float x, float z) const
{

	return 0.3f*(z*sinf(0.1f*x)+ x*cosf(0.1f*z));
}

XMFLOAT3 SandBox::GetHillsNormal(float x, float z) const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}



void SandBox::OnKeyboardInput(const GameTimer& gt)
{
	float dt = gt.DeltaTime();

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		lightRotMat *= XMMatrixRotationRollPitchYaw(0.0f, -gt.DeltaTime(), 0.0f);

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		lightRotMat *= XMMatrixRotationRollPitchYaw(0.0f, gt.DeltaTime(), 0.0f);


	if (GetAsyncKeyState(VK_UP) & 0x8000)
		lightRotMat *= XMMatrixRotationRollPitchYaw(gt.DeltaTime(),0.0f, 0.0f);

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		lightRotMat *= XMMatrixRotationRollPitchYaw(-gt.DeltaTime(), 0.0f, 0.0f);

	if (GetAsyncKeyState('1') & 0x8000)
		mIsWireframe = true;
	

	if (GetAsyncKeyState('Q') & 0x8000)
		mSkullTranslation.x -= 1.0f * dt;

	if (GetAsyncKeyState('D') & 0x8000)
		mSkullTranslation.x += 1.0f * dt;

	if (GetAsyncKeyState('Z') & 0x8000)
		mSkullTranslation.y += 1.0f * dt;

	if (GetAsyncKeyState('S') & 0x8000)
		mSkullTranslation.y -= 1.0f * dt;

	// Don't let user move below ground plane.
	mSkullTranslation.y = MathHelper::Max(mSkullTranslation.y, 0.0f);

	// Update the new world matrix.
	XMMATRIX skullRotate = XMMatrixRotationY(0.5f * MathHelper::Pi);
	XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	XMMATRIX skullOffset = XMMatrixTranslation(mSkullTranslation.x, mSkullTranslation.y, mSkullTranslation.z);
	XMMATRIX skullWorld = skullRotate * skullScale * skullOffset;
	XMStoreFloat4x4(&mSkullRitem->World, skullWorld);

	
	// Update reflection world matrix.
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, -3.0f); // xy plane (actually z-3=0 so z=3)
	XMMATRIX R = XMMatrixReflect(mirrorPlane);
	XMStoreFloat4x4(&mReflectedSkullRitem->World, skullWorld * R);

	
	// Update shadow skull world matrix

	XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	XMVECTOR toMainLight = -XMLoadFloat3(&mMainPassCB.Lights[0].Direction);
	
	XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
	XMMATRIX shadowOffsetY = XMMatrixTranslation(0.0f, 0.001f, 0.0f);

	XMStoreFloat4x4(&mShadowSkullRitem->World, skullWorld * S * shadowOffsetY);


	mSkullRitem->NumFramesDirty = gNumFrameResources;
	mReflectedSkullRitem->NumFramesDirty = gNumFrameResources;
	mShadowSkullRitem->NumFramesDirty = gNumFrameResources;
}

void SandBox::UpdateCamera(const GameTimer & gt)
{

	
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	
	XMStoreFloat4x4(&mView, view);
}

void SandBox::UpdateMainPassCB(const GameTimer & gt)
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

	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mMainWnd->GetWidth(), (float)mMainWnd->GetHeight());
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mMainWnd->GetWidth(), 1.0f / mMainWnd->GetHeight());
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	


	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.8f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	XMVECTOR lightt;

	lightt = XMLoadFloat3(&mMainPassCB.Lights[1].Direction);
	XMStoreFloat3(&mMainPassCB.Lights[1].Direction, XMVector3Transform(lightt, lightRotMat));

	lightt = XMLoadFloat3(&mMainPassCB.Lights[0].Direction);
	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, XMVector3Transform(lightt, lightRotMat));

	lightt = XMLoadFloat3(&mMainPassCB.Lights[2].Direction);
	XMStoreFloat3(&mMainPassCB.Lights[2].Direction, XMVector3Transform(lightt, lightRotMat));

	
	
	
	auto currPassCB = mCurrentFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void SandBox::UpdateReflectedPassCB(const GameTimer& gt)
{
	mReflectedPassCB = mMainPassCB;

	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, -3.0f); // yx plane (z = 0) ( actually its z - 3= 0 so its z = 3 (cuz the mirror is at z = 3)) couldv constructed it using XMPlaneFromPointNormal but since we want xy plane we just set it directly
	XMMATRIX R = XMMatrixReflect(mirrorPlane);

	// Reflect the lighting.
	for (int i = 0; i < 3; ++i) // dont forget to change the upper bound if we add more lights in the light array
	{
		XMVECTOR lightDir = XMLoadFloat3(&mMainPassCB.Lights[i].Direction);
		XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mReflectedPassCB.Lights[i].Direction, reflectedLightDir);
	}

	// Reflected pass stored in index 1
	auto currPassCB = mCurrentFrameResource->PassCB.get();
	currPassCB->CopyData(1, mReflectedPassCB);
}


void SandBox::UpdateObjectCBs(GameTimer const & gt)
{

	auto currObjectCB = mCurrentFrameResource->ObjectCB.get();

	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX TexTrans = XMLoadFloat4x4((&e->TexTransform));
			
			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform,XMMatrixTranspose(TexTrans));
			
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
			e->NumFramesDirty--;
		}
	}


	
}

void SandBox::UpdateMaterialCB(const GameTimer & gt)
{



	auto CurrMaterialCB = mCurrentFrameResource->MaterialCB.get();

	for (auto& e : mMaterials)
	{
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTrabsform = XMLoadFloat4x4(&mat->MatTransform);
			MaterialConstants matConst;
			matConst.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConst.FresnelR0 = mat->FresnelR0;
			matConst.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConst.MatTransform, XMMatrixTranspose(matTrabsform));

			CurrMaterialCB->CopyData(mat->MatCBIndex, matConst);
			mat->NumFramesDirty--;

		}
	}
}

void SandBox::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrentFrameResource->WavesVB.get();
	for (int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);

		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexCord.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexCord.y =  0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();

}

void SandBox::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0); // translation in u direction
	float& tv = waterMat->MatTransform(3, 1); // translation in v direction

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;


	// animate bolt texture
	static float t = 0.0f;
	t += gt.DeltaTime();

	if (t >= 0.033333f)
	{
		mBoltTextureIndex++;
		t = 0.0f;

		if (mBoltTextureIndex == 68) // out of bound check ( the bolt textures are the last textures uploaded on the heap)
			mBoltTextureIndex = mTextures.size();
	}

}


void SandBox::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//

	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["Opaque_VS"]->GetBufferPointer()),mShaders["Opaque_VS"]->GetBufferSize()};
	opaquePsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["Opaque_PS"]->GetBufferPointer()), mShaders["Opaque_PS"]->GetBufferSize() };
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


	//
	// PSO for transparent objects.
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC TransparentPsoDesc = opaquePsoDesc;


	// Blend equation of d3d12 :  final color = source color X source color Blend factor [ some operation (+,-,max,min) ] destination color X destination color Blend factor.
	//                            final Alpha = source Alpha x source Alpha blend factor  [ some operation (+,-,max,min) ]  destination Alpha x destination Alpha blend factor.
	// 
	// source is the fragment currently validated by pixel shader while destination is the one already present in the back buffer.
	// 
	// side note : for D3D12_BLEND_OP_REV_SUBTRACT the formula for final color is : destination color X destination color Blend factor [-] source color X source color Blend factor.
	// same thing for final alpha using  D3D12_BLEND_OP_REV_SUBTRACT : destination Alpha x destination Alpha blend factor [-] source Alpha x source Alpha blend factor.


	D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
	blendDesc.BlendEnable = true;
	blendDesc.LogicOpEnable = false;	// cant use logic operations if traditional blend operations are enabled gotta choose to use one of them.

	blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;//source color blend factor
	blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//dest color blend factor
	blendDesc.BlendOp = D3D12_BLEND_OP_ADD;//blend operation used for color

	blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;		//source Alpha blend factor
	blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;  //dest Alpha blend factor
	blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;  // Alpha blending operation used
	blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	TransparentPsoDesc.BlendState.RenderTarget[0] = blendDesc;
	
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&TransparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));


	// PSO FOR ALPHA TEST

	D3D12_GRAPHICS_PIPELINE_STATE_DESC AlphaTestedPsoDesc = opaquePsoDesc;

	//D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
	blendDesc.BlendEnable = true;
	blendDesc.LogicOpEnable = false;	// cant use logic operations if traditional blend operations are enabled gotta choose to use one of them.

	blendDesc.SrcBlend = D3D12_BLEND_ONE;//source color blend factor
	blendDesc.DestBlend = D3D12_BLEND_ONE;//dest color blend factor
	blendDesc.BlendOp = D3D12_BLEND_OP_ADD;//blend operation used for color

	blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;		//source Alpha blend factor
	blendDesc.DestBlendAlpha = D3D12_BLEND_ONE;  //dest Alpha blend factor
	blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;  // Alpha blending operation used
	blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	AlphaTestedPsoDesc.BlendState.RenderTarget[0] = blendDesc;
	AlphaTestedPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["Alpha_PS"]->GetBufferPointer()), mShaders["Alpha_PS"]->GetBufferSize() };
	AlphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // so the backface of the wirefence box appears from the front.



	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&AlphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));


	// PSO for treesprite alphatested

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
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));






	// PSO FOR marking Stencil Mirrors

	D3D12_GRAPHICS_PIPELINE_STATE_DESC StencilMirrorsMarking = opaquePsoDesc;

	StencilMirrorsMarking.BlendState.RenderTarget[0].RenderTargetWriteMask = 0; // disable writes to the back buffer.

	// 
	StencilMirrorsMarking.DepthStencilState.DepthEnable = true;
	StencilMirrorsMarking.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // disabling writes to the depth buffer
	StencilMirrorsMarking.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // did not have to do it cuz its initialized to this value by default ( created the opaquePso's depthstencil state 
																							// with opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); 

	StencilMirrorsMarking.DepthStencilState.StencilEnable = true;
	StencilMirrorsMarking.DepthStencilState.StencilReadMask = 0xff; // doesnt do anything (1111 1111) 
	StencilMirrorsMarking.DepthStencilState.StencilWriteMask = 0xff; // same

	StencilMirrorsMarking.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS; // stencil test always passes  for stencilMirrors
	StencilMirrorsMarking.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP; // doesnt matter cuz it always passes
	StencilMirrorsMarking.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP; // depth test fails if an object is occluding the mirror in this case do not replace by ref,
																										// just keep the value already present.

	StencilMirrorsMarking.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE; // replace by the ref value if everything passes.


	//  not rendering backfacing polygons, so these settings do not rly matter

	StencilMirrorsMarking.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS; 
	StencilMirrorsMarking.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP; 
	StencilMirrorsMarking.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP; 
	StencilMirrorsMarking.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&StencilMirrorsMarking, IID_PPV_ARGS(&mPSOs["markStencilMirrors"])));



	// PSO FOR Stencil Reflected objects

	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawReflectionsPsoDesc = opaquePsoDesc;

	drawReflectionsPsoDesc.DepthStencilState.DepthEnable = true;
	drawReflectionsPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	drawReflectionsPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	
	drawReflectionsPsoDesc.DepthStencilState.StencilEnable = true;
	drawReflectionsPsoDesc.DepthStencilState.StencilReadMask = 0xff;
	drawReflectionsPsoDesc.DepthStencilState.StencilWriteMask = 0xff;

	drawReflectionsPsoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL; // so the reflected objects only appear on region marked by the stencilMirrors on the stencilbuffer
	drawReflectionsPsoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

	//  not rendering backfacing polygons, so these settings do not rly matter

	drawReflectionsPsoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL; // so the reflected objects only appear on region marked by the stencilMirrors on the stencilbuffer
	drawReflectionsPsoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;              


	drawReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	drawReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;


	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs["drawStencilReflection"])));










	// PSO FOR SHADOW

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = TransparentPsoDesc;

	shadowPsoDesc.DepthStencilState.DepthEnable = true;
	shadowPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	shadowPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	
	shadowPsoDesc.DepthStencilState.StencilEnable = true;
	shadowPsoDesc.DepthStencilState.StencilReadMask = 0xff;
	shadowPsoDesc.DepthStencilState.StencilWriteMask = 0xff;

	shadowPsoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowPsoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowPsoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowPsoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	shadowPsoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowPsoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowPsoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowPsoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
									

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs["Shadow"])));




	// PSO FOR WIREFRAME

	D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframePsqoDesc;
	
	

	ZeroMemory(&wireframePsqoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	wireframePsqoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	wireframePsqoDesc.pRootSignature = mRootSignature.Get();
	wireframePsqoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["Opaque_VS"]->GetBufferPointer()), mShaders["Opaque_VS"]->GetBufferSize() };
	wireframePsqoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["Opaque_PS"]->GetBufferPointer()), mShaders["Opaque_PS"]->GetBufferSize() };
	wireframePsqoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	wireframePsqoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	wireframePsqoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	wireframePsqoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	wireframePsqoDesc.SampleMask = UINT_MAX;
	wireframePsqoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	wireframePsqoDesc.NumRenderTargets = 1;
	wireframePsqoDesc.RTVFormats[0] = mBackBufferFormat;
	wireframePsqoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	wireframePsqoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	wireframePsqoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&wireframePsqoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));


}

void SandBox::Update(const GameTimer& gt)
{

	

	OnKeyboardInput(gt);
	UpdateCamera(gt);
	

	//Cycle through the circular frame resources array
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % gNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence)
	{
		
		HANDLE evenHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, evenHandle));
		WaitForSingleObject(evenHandle, INFINITE);
		CloseHandle(evenHandle);
		
		
	}

	
	if (SceneIndex == 0)  // only do it when wer rendering wave scene
	{  
		
		AnimateMaterials(gt);
		UpdateWaves(gt);
	}
	
	UpdateObjectCBs(gt);
	UpdateMaterialCB(gt);
	UpdateMainPassCB(gt);
	UpdateReflectedPassCB(gt);
	
}


void SandBox::Draw(const GameTimer& gt)
{
	

	auto cmdListAlloc = mCurrentFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	if (mIsWireframe)
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
	}
	else
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
	}

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));



	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// Clear the back buffer and depth buffer.
	
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), clear_color, 0, nullptr);//  
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	
	
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	mCommandList->SetGraphicsRootConstantBufferView(2, mCurrentFrameResource->PassCB->Resource()->GetGPUVirtualAddress());
	
	if (SceneIndex == 0)// Land and wave scene
	{
		
		DrawRenderItems(mCommandList.Get(), mRitemLayerLand[(int)RenderLayer::Opaque]);



	


		mCommandList->SetPipelineState(mPSOs["transparent"].Get());
		
		if (mIsWireframe)
			mCommandList->SetPipelineState(mPSOs["opaque_wireframe"].Get()); // if we are in wireframe use the opaque wireframe pso (no use in making and using a blending wireframe pso)

		DrawRenderItems(mCommandList.Get(), mRitemLayerLand[(int)RenderLayer::Transparent]);



		mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayerLand[(int)RenderLayer::TreeSpriteAlphaTested]);


		renderingBolt = true;
		mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayerLand[(int)RenderLayer::AlphaTested]);
	

		

	}
		
	
	 if (SceneIndex == 1)// shapes and skull scene
	{
		
		

		DrawRenderItems(mCommandList.Get(), mRitemLayerShapes[(int)RenderLayer::Opaque]);


		mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayerShapes[(int)RenderLayer::AlphaTested]);

		mCommandList->OMSetStencilRef(1); // Mark the visible mirror pixels in the stencil buffer with the value 1
		mCommandList->SetPipelineState(mPSOs["markStencilMirrors"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayerShapes[(int)RenderLayer::markStencilMirrors]);


		UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

		mCommandList->SetGraphicsRootConstantBufferView(2, mCurrentFrameResource->PassCB->Resource()->GetGPUVirtualAddress() + 1*passCBByteSize); // use the other passCB with accurate reflected light
		mCommandList->SetPipelineState(mPSOs["drawStencilReflection"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayerShapes[(int)RenderLayer::Reflected]);


		mCommandList->SetGraphicsRootConstantBufferView(2, mCurrentFrameResource->PassCB->Resource()->GetGPUVirtualAddress());
		mCommandList->OMSetStencilRef(0);

		mCommandList->SetPipelineState(mPSOs["transparent"].Get());

		if (mIsWireframe)
			mCommandList->SetPipelineState(mPSOs["opaque_wireframe"].Get()); // if we are in wireframe use the opaque wireframe pso (no use in making and using a blending wireframe pso)

		DrawRenderItems(mCommandList.Get(), mRitemLayerShapes[(int)RenderLayer::Transparent]);


		mCommandList->OMSetStencilRef(0);
		mCommandList->SetPipelineState(mPSOs["Shadow"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayerShapes[(int)RenderLayer::Shadow]);
	}
		

	ID3D12DescriptorHeap* ImguidescriptorHeaps[] = { ImguiSrvHeap.Get() };// separate srv heap for imgui
	mCommandList->SetDescriptorHeaps(_countof(ImguidescriptorHeaps), ImguidescriptorHeaps);
	RenderOverlay(mCommandList.Get());
	
	

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
	mCurrentFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

	
}
