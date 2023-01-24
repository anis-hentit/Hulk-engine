#pragma once
#include "Hulk/Core/d3dUtil.h"


class HULK_API GpuWave
{
public :

	GpuWave(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, int m, int n, float dx, float dt, float speed, float damping);
	GpuWave(const GpuWave& w) = delete;
	GpuWave& operator=(const GpuWave& rhs) = delete;
	~GpuWave() = default;
};

