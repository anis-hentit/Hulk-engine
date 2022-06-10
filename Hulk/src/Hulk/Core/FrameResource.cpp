// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "hkpch.h"

#include "FrameResource.h"

namespace Hulk{
FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT MaterialCount, UINT waveVertCount)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, MaterialCount, true);
    WavesVB = std::make_unique<UploadBuffer<Vertex>>(device, waveVertCount, false);// here object count relates to the number of vertices of the wave
}

FrameResource::~FrameResource()
{

}

}