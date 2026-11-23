// Compare the binding tier and actual desktop D3D12 shader-visible heap limits.
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <cstdio>

using Microsoft::WRL::ComPtr;

namespace {

void TryHeap(ID3D12Device* device, UINT count, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = count;
    desc.Flags = flags;
    desc.NodeMask = 1;
    ComPtr<ID3D12DescriptorHeap> heap;
    const HRESULT result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
    std::printf("  count=%u visible=%s result=0x%08lx heap=%s\n", count,
                flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE ? "yes" : "no",
                static_cast<unsigned long>(result), heap ? "created" : "null");
}

void Probe(IDXGIAdapter1* adapter, const char* kind) {
    DXGI_ADAPTER_DESC1 description{};
    if (FAILED(adapter->GetDesc1(&description))) return;
    std::printf("%s: %ls\n", kind, description.Description);
    LARGE_INTEGER driver{};
    if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driver))) {
        std::printf("  driver_version=%u.%u.%u.%u\n",
                    HIWORD(driver.HighPart), LOWORD(driver.HighPart),
                    HIWORD(driver.LowPart), LOWORD(driver.LowPart));
    }

    ComPtr<ID3D12Device> device;
    const HRESULT created = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    if (FAILED(created)) {
        std::printf("  D3D12CreateDevice=0x%08lx\n", static_cast<unsigned long>(created));
        return;
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)))) {
        std::printf("  binding_tier=%u\n", static_cast<unsigned>(options.ResourceBindingTier));
    }
#if defined(__ID3D12Device12_INTERFACE_DEFINED__)
    D3D12_FEATURE_DATA_D3D12_OPTIONS19 heap_options{};
    const HRESULT queried =
        device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &heap_options, sizeof(heap_options));
    if (SUCCEEDED(queried)) {
        std::printf("  max_visible_view_descriptors=%u\n", heap_options.MaxViewDescriptorHeapSize);
    } else {
        std::printf("  OPTIONS19=0x%08lx\n", static_cast<unsigned long>(queried));
    }
#else
    std::puts("  OPTIONS19 is unavailable in the build SDK");
#endif
    std::printf("  descriptor_increment=%u\n",
                device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    constexpr auto visible = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    TryHeap(device.Get(), 1'000'000, visible);
    TryHeap(device.Get(), 1'000'001, visible);
    TryHeap(device.Get(), 1'250'000, visible);
    TryHeap(device.Get(), 1'250'000, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    std::printf("  device_removed_reason=0x%08lx\n",
                static_cast<unsigned long>(device->GetDeviceRemovedReason()));
}

}  // namespace

int main() {
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return 1;
    for (UINT index = 0;; ++index) {
        ComPtr<IDXGIAdapter1> adapter;
        const HRESULT result = factory->EnumAdapters1(index, &adapter);
        if (result == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(result)) return 1;
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            Probe(adapter.Get(), "Hardware");
        }
    }
    ComPtr<IDXGIAdapter1> warp;
    if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)))) {
        Probe(warp.Get(), "WARP");
    } else {
        std::puts("WARP: unavailable");
    }
    return 0;
}
