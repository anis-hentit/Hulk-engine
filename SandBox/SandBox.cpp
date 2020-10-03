
//#define SOL_CXX_LUA

#include <DirectXMath.h>

#include "Hulk.h"
#include <iostream>
#include <WindowsX.h>
#include <wrl/client.h>


#include "dxgidebug.h"



using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Hulk;


 

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

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




class SandBox : public Hulk::D3DApp
{
public:
	int Run();
	SandBox(HINSTANCE hInstance);
	~SandBox();

	virtual bool Initialize()override;

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
	void BuildPSO();
	void BuildFrameResources();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void CalculateNormals();

	void OnKeyboardInput(const Hulk::GameTimer& gt);
	void UpdateCamera(const Hulk::GameTimer& gt);
	void UpdateMainPassCB(const Hulk::GameTimer& gt);
	void UpdateObjectCBs(Hulk::GameTimer const & gt);
	void UpdateMaterialCB(const GameTimer& gt);


private:
	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	ComPtr<ID3D12DescriptorHeap> mSamplerHeap;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;
	std::vector<std::unique_ptr<Hulk::FrameResource>> mFrameResources;

	ComPtr<ID3D12RootSignature> mRootSignature;


	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	

	std::unordered_map<std::string, std::unique_ptr<Hulk::MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Hulk::Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Hulk::Texture>> mTextures;
	

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

	PassConstants mMainPassCB;

	UINT mPassCbvOffset = 0;
	UINT mMaterialCbvOffset = 0;//not used anymore changed it

	
	bool mIsWireframe = false;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
	XMMATRIX lightRotMat = XMMatrixIdentity();;

	float mTheta = 1.5f*XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;
	UINT NumberOfObjectsToDraw=2;
	
	int mCurrentFrameResourceIndex = 0;
	FrameResource* mCurrentFrameResource = nullptr;

	POINT mLastMousePos;

	
	float skull_albedo[4] = { 0.941176534f, 0.9725481f, 1.000000000f, 1.000000000f };
	
};






int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
#endif
	
	
	
	try
	{	
		SandBox theApp(hInstance);
		
		if (!theApp.Initialize())
		{
			
			return 0;
		}
		 
		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		
		return 0;
	}
}

void Hulk::CreateApplication()
{
	WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}



int SandBox::Run()
{
	MSG msg = {0};
 
	mTimer.Reset();
	
	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		
		else
        {
			
			mTimer.Tick();
			
			if( !mAppPaused )
			{
				//CalculateFrameStats();
				
				Update(mTimer);
				ImGuiUpdate();
                Draw(mTimer);
				
			}
			else
			{
				Sleep(100);
			}
        }
    }
	
	return (int)msg.wParam;
}


SandBox::SandBox(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
	
}

SandBox::~SandBox()
{
	Shutdown();
	
}

bool SandBox::Initialize()
{
	
	
	if (!D3DApp::Initialize())
		return false;
	 
	//Reset the command list to prep for initialization commands
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	
	BuildTextures();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildMaterials();
	BuildShapeGeometry();
	BuildSkullGeo();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();//at this moment used for srvs for textures and one last srv for imgui
	BuildShaderResourcesViews();
	//BuildSamplerViews(); using static samplers
	BuildPSO();
	//Execute the init commands
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//Wait until init is complete
	FlushCommandQueue();
	
	ImGuiInitialize(mhMainWnd,md3dDevice.Get(), gNumFrameResources,
		mBackBufferFormat);
	
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

	SetCapture(mhMainWnd);
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
		float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 40.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;

}



void SandBox::BuildMaterials()
{
	auto bricks0 = std::make_unique<Material>();
	bricks0->Name = "bricks";
	bricks0->MatCBIndex = 0;
	bricks0->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(bricks0->Name)));// there is no function to get index from unordered map so i use this trick to get the index to use to get the srv in the heap.
	bricks0->DiffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
	bricks0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks0->Roughness = 0.1f;
	
	auto tile0 = std::make_unique<Material>();
	tile0->Name = "Tile";
	tile0->MatCBIndex = 1;
	tile0->DiffuseSrvHeapIndex = static_cast<int>( distance(mTextures.begin(), mTextures.find(tile0->Name)));
	tile0->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
	tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.2f;
	//XMStoreFloat4x4(&tile0->MatTransform, XMMatrixScaling(2.0f, 2.0f, 1.0f)); can transform text with matTransform matrix too
	
	 auto stoneMat = std::make_unique<Material>();
	 stoneMat ->Name = "stone";
	 stoneMat ->MatCBIndex = 2;
	 stoneMat->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(stoneMat->Name)));
	 stoneMat ->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	 stoneMat ->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	 stoneMat ->Roughness = 0.3f;

	 auto skullMat = std::make_unique<Material>();
	 skullMat ->Name = "bricks";
	 skullMat ->MatCBIndex = 3;
	 skullMat ->DiffuseSrvHeapIndex = static_cast<int>(distance(mTextures.begin(), mTextures.find(skullMat->Name)));
	 skullMat ->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	 skullMat ->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	 skullMat ->Roughness = 0.3f;
	
	mMaterials["bricks"] = std::move(bricks0);
	mMaterials["stone"] = std::move(stoneMat);
	mMaterials["tile"] = std::move(tile0);
	mMaterials["skull"] = std::move(skullMat);
	

}

void SandBox::BuildTextures()
{
	auto floorTileText = std::make_unique<Texture>();
	floorTileText->Name = "Tile";
	floorTileText->Filename = L"E:\\Desktop\\Hulk engine\\Textures\\tile.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(),
		floorTileText->Filename.c_str(), floorTileText->Resource, floorTileText->UploadHeap
	));
	mTextures[floorTileText->Name] = std::move(floorTileText);

	auto bricksTex = std::make_unique<Texture>();
	bricksTex->Name = "bricks";
	bricksTex->Filename = L"E:\\Desktop\\Hulk engine\\Textures\\bricks.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), bricksTex->Filename.c_str(),
		bricksTex->Resource, bricksTex->UploadHeap));
	
	mTextures[bricksTex->Name] = std::move(bricksTex);

	auto stoneTex = std::make_unique<Texture>();
	stoneTex->Name = "stone";
	stoneTex->Filename = L"E:\\Desktop\\Hulk engine\\Textures\\stone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), stoneTex->Filename.c_str(),
		stoneTex->Resource, stoneTex->UploadHeap));

	mTextures[stoneTex->Name] = std::move(stoneTex);

	
	
	
	if (mTextures.size() > 0)//check if there is textures so i can offset imgui srv in the heap 
		imguiDescriptorOffset = mTextures.size();
}



void SandBox::BuildDescriptorHeaps()
{
	
	UINT numDescriptors = 0;

	if (mTextures.size() > 0)
		numDescriptors = mTextures.size();
	


	D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc{};
	SrvHeapDesc.NumDescriptors = numDescriptors + 1;// +1 for the imgui view
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
	
	ComPtr<ID3DBlob> serializedRootSig = nullptr;//this will reference temperarly memory for the actual root signature
	ComPtr<ID3DBlob> errorBlob = nullptr;
	//the call to D3D12SerializeRootSignature reserves memory for the root signature and return information 
	    //about that memory into the output parameter seralizedRootSig(it calculates memory needed by checking
	      //into the rootSigDesc parameter which is a description of our root sig it will go check from there
	      //the size of slotRootParameter in this exemple.
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
	mvsByteCode = d3dUtil::CompileShader(L"E:\\Desktop\\Hulk engine\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"E:\\Desktop\\Hulk engine\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{ 
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"TEXTCOR",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	 



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

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

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

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

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

void SandBox::BuildFrameResources() 

{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}


}
void SandBox::BuildRenderItems()
{
	
	
	auto skullRitem = std::make_unique<RenderItem>();
	skullRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 2.0f, 0.0f));
	skullRitem->ObjCBIndex = 0;
	skullRitem->Geo = mGeometries["skullGeo"].get();
	skullRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
	skullRitem->mMaterial = mMaterials["skull"].get();
	//skullRitem->mMaterial->DiffuseSrvHeapIndex = 0;
	mAllRitems.push_back(std::move(skullRitem));

	//Box render item
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f) * XMMatrixRotationY(0.45f));
	boxRitem->ObjCBIndex = 1;
	boxRitem->Geo = mGeometries["shapeGeo"].get();
	boxRitem->mMaterial = mMaterials["bricks"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	//boxRitem->mMaterial->DiffuseSrvHeapIndex = 1;
	mAllRitems.push_back(std::move(boxRitem));
	
	//Grid Render Item
	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridRitem->ObjCBIndex = 2;
	gridRitem->Geo = mGeometries["shapeGeo"].get();
	gridRitem->mMaterial = mMaterials["tile"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	//gridRitem->mMaterial->DiffuseSrvHeapIndex = 0;
	mAllRitems.push_back(std::move(gridRitem));

	
	

	// left and right cones and spheres (5 each) so we need 4*5 views in the heap 
	     //    + 2 for the grid and box so a total of 22 views in the heap
	UINT objCBIndex = 3;
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
		//leftCylRitem->mMaterial->DiffuseSrvHeapIndex = 1;

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Geo = mGeometries["shapeGeo"].get();
		rightCylRitem->mMaterial = mMaterials["bricks"].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
		//rightCylRitem->mMaterial->DiffuseSrvHeapIndex = 1;
		
		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
		leftSphereRitem->mMaterial = mMaterials["stone"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		//leftSphereRitem->mMaterial->DiffuseSrvHeapIndex = 2;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
		rightSphereRitem->mMaterial = mMaterials["stone"].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		//rightSphereRitem->mMaterial->DiffuseSrvHeapIndex = 2;

		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}
	
	// All the render items in this demo are opaque.
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());

	
}
void SandBox::DrawRenderItems(ID3D12GraphicsCommandList * cmdList, const std::vector<RenderItem*>& ritems)
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
		Tex.Offset(ri->mMaterial->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize); 
		cmdList->SetGraphicsRootDescriptorTable(3, Tex);
		
		D3D12_GPU_VIRTUAL_ADDRESS CbHandle = CB->GetGPUVirtualAddress() + ri->ObjCBIndex*matCBBytesize;


		cmdList->SetGraphicsRootConstantBufferView(0, CbHandle);

		//not using desc table for the material but using directly a rootconstant
		D3D12_GPU_VIRTUAL_ADDRESS matCbHandle = MatCB->GetGPUVirtualAddress() + ri->mMaterial->MatCBIndex*matCBBytesize;
	
		cmdList->SetGraphicsRootConstantBufferView(1, matCbHandle);
		

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void SandBox::CalculateNormals()
{

}

void SandBox::OnKeyboardInput(const GameTimer& gt)
{
	float dt = gt.DeltaTime();

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		lightRotMat = XMMatrixRotationRollPitchYaw(0.0f ,gt.TotalTime(),0.0f);

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		lightRotMat = XMMatrixRotationRollPitchYaw(0.0f, -gt.TotalTime(), 0.0f);


	if (GetAsyncKeyState(VK_UP) & 0x8000)
		lightRotMat = XMMatrixRotationRollPitchYaw(sin(gt.TotalTime()),0.0f, 0.0f);

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		lightRotMat = XMMatrixRotationRollPitchYaw(sin(-gt.TotalTime()), 0.0f, 0.0f);

	if (GetAsyncKeyState('1') & 0x8000)
		mIsWireframe = true;
	else
		mIsWireframe = false;
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
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
	
	
	auto currPassCB = mCurrentFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
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


void SandBox::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize() };
	opaquePsoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize() };
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	
	
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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframePsqoDesc;

	// PSO FOR WIREFRAME

	ZeroMemory(&wireframePsqoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	wireframePsqoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	wireframePsqoDesc.pRootSignature = mRootSignature.Get();
	wireframePsqoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize() };
	wireframePsqoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize() };
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

	UpdateObjectCBs(gt);
	UpdateMaterialCB(gt);
	UpdateMainPassCB(gt);
	

	

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

	// Clear the back buffer and depth buffer.
	
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), clear_color, 0, nullptr);//  
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	
	mCommandList->SetGraphicsRootConstantBufferView(2, mCurrentFrameResource->PassCB->Resource()->GetGPUVirtualAddress());
	
	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);
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
