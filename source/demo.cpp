
//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <sdkddkver.h>
#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <dxgi.h>
#include <d3d11.h>
#include "../thirdparty/DXUT/Optional/SDKmisc.h"
#include "../thirdparty/ConvertUTF/include/ConvertUTF.h"
#include "demo.h"

struct scene_mesh_vertex_varying_binding
{
    DirectX::XMFLOAT2 m_texcoord_0;
    DirectX::XMFLOAT2 m_texcoord_1;
};

struct scene_mesh_subset_data
{
    std::vector<scene_mesh_vertex_varying_binding> m_vertex_varying_binding;

    std::vector<uint32_t> m_indices;

    std::string m_normal_texture_image_uri;

    DirectX::XMFLOAT4 m_base_color_factor;

    std::string m_base_color_texture_image_uri;

    float m_metallic_factor;

    float m_roughness_factor;

    std::string m_metallic_roughness_texture_image_uri;
};

static void import_scene(std::vector<scene_mesh_subset_data> &out_mesh_subset_data, char const *path);

static constexpr uint32_t const baked_texture_width = 2048U;
static constexpr uint32_t const baked_texture_height = 2048U;

void Demo::Init(ID3D11Device3 *device, ID3D11DeviceContext *device_context)
{
    this->m_sampler = NULL;
    {
        D3D11_SAMPLER_DESC sampler_desc;
        sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.MipLODBias = 0.0;
        sampler_desc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        sampler_desc.BorderColor[0] = 0.0F;
        sampler_desc.BorderColor[1] = 0.0F;
        sampler_desc.BorderColor[2] = 0.0F;
        sampler_desc.BorderColor[3] = 0.0F;
        sampler_desc.MinLOD = 0.0F;
        sampler_desc.MaxLOD = 4096.0F;

        HRESULT res_device_create_sampler = device->CreateSamplerState(&sampler_desc, &this->m_sampler);
        assert(SUCCEEDED(res_device_create_sampler));
    }

    std::cout << "NOTE: The hard-coding file name value \"keqing-lolita.gltf\" should be changed!" << std::endl;

    std::string const file_name = "keqing-lolita.gltf";

    std::vector<scene_mesh_subset_data> mesh_subset_data;
    import_scene(mesh_subset_data, file_name.c_str());

    this->m_scene_mesh_subsets.resize(mesh_subset_data.size());

    std::cout << "TODO: At this moment, only normal texture, base color texture and metallic roughness texture are considered!" << std::endl;
    std::cout << "TODO: At this moment, the normal scale is ignored!" << std::endl;

    std::unordered_map<std::string, ID3D11ShaderResourceView *> mapped_textures;

    for (size_t mesh_subset_index = 0U; mesh_subset_index < mesh_subset_data.size(); ++mesh_subset_index)
    {
        scene_mesh_subset_data const &in_mesh_subset_data = mesh_subset_data[mesh_subset_index];

        Demo_Mesh_Subset &out_mesh_subset = this->m_scene_mesh_subsets[mesh_subset_index];

        out_mesh_subset.m_vertex_count = static_cast<uint32_t>(in_mesh_subset_data.m_vertex_varying_binding.size());

        out_mesh_subset.m_index_count = static_cast<uint32_t>(in_mesh_subset_data.m_indices.size());

        {
            uint32_t const buffer_size = sizeof(scene_mesh_vertex_varying_binding) * out_mesh_subset.m_vertex_count;

            D3D11_BUFFER_DESC const buffer_desc = {
                buffer_size,
                D3D11_USAGE_IMMUTABLE,
                D3D11_BIND_SHADER_RESOURCE,
                0U,
                D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS,
                sizeof(uint32_t)};

            D3D11_SUBRESOURCE_DATA const buffer_initial_data = {
                in_mesh_subset_data.m_vertex_varying_binding.data(),
                buffer_size,
                buffer_size};

            ID3D11Buffer *vertex_varying_buffer = NULL;
            HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, &buffer_initial_data, &vertex_varying_buffer);
            assert(SUCCEEDED(res_device_create_buffer));

            WCHAR const debug_object_name[] = {L"Vertex Varying Buffer"};
            HRESULT res_device_child_set_debug_object_name = vertex_varying_buffer->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(debug_object_name), debug_object_name);
            assert(SUCCEEDED(res_device_child_set_debug_object_name));

            uint32_t const num_elements = ((buffer_size - 1U) / sizeof(uint32_t)) + 1U;
            uint32_t const size_elements = sizeof(uint32_t) * num_elements;
            D3D11_SHADER_RESOURCE_VIEW_DESC const shader_resource_view_desc = {
                .Format = DXGI_FORMAT_R32_TYPELESS,
                .ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
                .BufferEx = {
                    0U,
                    static_cast<UINT>(num_elements),
                    D3D11_BUFFEREX_SRV_FLAG_RAW}};
            HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(vertex_varying_buffer, &shader_resource_view_desc, &out_mesh_subset.m_vertex_varying_buffer_view);
            assert(SUCCEEDED(res_device_create_shader_resource_view));

            vertex_varying_buffer->Release();
        }

        {
            uint32_t const buffer_size = sizeof(uint32_t) * out_mesh_subset.m_index_count;

            D3D11_BUFFER_DESC const buffer_desc = {
                buffer_size,
                D3D11_USAGE_IMMUTABLE,
                D3D11_BIND_SHADER_RESOURCE,
                0U,
                D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS,
                sizeof(uint32_t)};

            D3D11_SUBRESOURCE_DATA const buffer_initial_data = {
                in_mesh_subset_data.m_indices.data(),
                buffer_size,
                buffer_size};

            ID3D11Buffer *index_buffer = NULL;
            HRESULT res_device_create_buffer = device->CreateBuffer(&buffer_desc, &buffer_initial_data, &index_buffer);
            assert(SUCCEEDED(res_device_create_buffer));

            WCHAR const debug_object_name[] = {L"Index Buffer"};
            HRESULT res_device_child_set_debug_object_name = index_buffer->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(debug_object_name), debug_object_name);
            assert(SUCCEEDED(res_device_child_set_debug_object_name));

            uint32_t const num_elements = ((buffer_size - 1U) / sizeof(uint32_t)) + 1U;
            uint32_t const size_elements = sizeof(uint32_t) * num_elements;
            D3D11_SHADER_RESOURCE_VIEW_DESC const shader_resource_view_desc = {
                .Format = DXGI_FORMAT_R32_TYPELESS,
                .ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
                .BufferEx = {
                    0U,
                    static_cast<UINT>(num_elements),
                    D3D11_BUFFEREX_SRV_FLAG_RAW}};
            HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(index_buffer, &shader_resource_view_desc, &out_mesh_subset.m_index_buffer_view);
            assert(SUCCEEDED(res_device_create_shader_resource_view));

            index_buffer->Release();
        }
        if (!in_mesh_subset_data.m_normal_texture_image_uri.empty())
        {
            std::string normal_image_asset_file_name_dds;
            {
                std::string normal_image_asset_file_name;

                size_t dir_name_pos = file_name.find_last_of("/\\");
                if (std::string::npos != dir_name_pos)
                {
                    normal_image_asset_file_name = file_name.substr(0U, dir_name_pos + 1U);
                }
                else
                {
                    normal_image_asset_file_name += "./";
                }

                size_t ext_name_pos = in_mesh_subset_data.m_normal_texture_image_uri.find_last_of(".");
                if (std::string::npos != ext_name_pos)
                {
                    normal_image_asset_file_name += in_mesh_subset_data.m_normal_texture_image_uri.substr(0U, ext_name_pos + 1U);
                }
                else
                {
                    normal_image_asset_file_name += in_mesh_subset_data.m_normal_texture_image_uri;
                }

                normal_image_asset_file_name_dds = (normal_image_asset_file_name + "dds");
            }

            std::unordered_map<std::string, ID3D11ShaderResourceView *>::const_iterator found;
            if (mapped_textures.end() != (found = mapped_textures.find(normal_image_asset_file_name_dds)))
            {
                assert(NULL != found->second);
                out_mesh_subset.m_normal_texture_view = found->second;
            }
            else
            {
                std::wstring normal_image_asset_file_name_dds_utf16;
                {
                    // llvm::convertUTF8ToUTF16String
                    assert(normal_image_asset_file_name_dds_utf16.empty());

                    std::string src_utf8 = normal_image_asset_file_name_dds;
                    std::wstring &dst_utf16 = normal_image_asset_file_name_dds_utf16;

                    assert(dst_utf16.empty());

                    if (!src_utf8.empty())
                    {
                        llvm::UTF8 const *src = reinterpret_cast<const llvm::UTF8 *>(src_utf8.data());
                        llvm::UTF8 const *src_end = src + src_utf8.size();

                        // Allocate the same number of UTF-16 code units as UTF-8 code units. Encoding
                        // as UTF-16 should always require the same amount or less code units than the
                        // UTF-8 encoding.  Allocate one extra byte for the null terminator though,
                        // so that someone calling DstUTF16.data() gets a null terminated string.
                        // We resize down later so we don't have to worry that this over allocates.
                        dst_utf16.resize(src_utf8.size() + 1U);
                        llvm::UTF16 *dst = reinterpret_cast<llvm::UTF16 *>(dst_utf16.data());
                        llvm::UTF16 *dst_end = dst + dst_utf16.size();

                        llvm::ConversionResult c_r = llvm::ConvertUTF8toUTF16(&src, src_end, &dst, dst_end, llvm::strictConversion);
                        assert(llvm::conversionOK == c_r);

                        dst_utf16.resize(dst - reinterpret_cast<llvm::UTF16 *>(dst_utf16.data()));
                    }
                }

                out_mesh_subset.m_normal_texture_view = NULL;
                HRESULT res_create_shader_resource_view_from_file = DXUTCreateShaderResourceViewFromFile(device, normal_image_asset_file_name_dds_utf16.c_str(), &out_mesh_subset.m_normal_texture_view);
                assert(SUCCEEDED(res_create_shader_resource_view_from_file));

                mapped_textures.emplace_hint(found, normal_image_asset_file_name_dds, out_mesh_subset.m_normal_texture_view);
            }
        }
        else
        {
            DXGI_FORMAT const fake_normal_texture_format = DXGI_FORMAT_R8G8B8A8_UNORM;

            DirectX::XMFLOAT4 normal_data(0.5F, 0.5F, 1.0F, 1.0F);

            DirectX::PackedVector::XMUBYTEN4 packed_normal_data;
            DirectX::PackedVector::XMStoreUByteN4(&packed_normal_data, DirectX::XMLoadFloat4(&normal_data));

            std::string normal_image_asset_file_name_fake;
            {
                {
                    std::string normal_image_asset_file_name;

                    size_t dir_name_pos = file_name.find_last_of("/\\");
                    if (std::string::npos != dir_name_pos)
                    {
                        normal_image_asset_file_name = file_name.substr(0U, dir_name_pos + 1U);
                    }
                    else
                    {
                        normal_image_asset_file_name += "./";
                    }

                    std::string packed_normal_data_string;
                    {
                        std::stringstream string_stream;
                        string_stream << packed_normal_data.v;
                        string_stream >> packed_normal_data_string;
                    }
                    normal_image_asset_file_name += std::move(packed_normal_data_string);

                    normal_image_asset_file_name_fake = (normal_image_asset_file_name + ".fake-normal");
                }
            }

            std::unordered_map<std::string, ID3D11ShaderResourceView *>::const_iterator found;
            if (mapped_textures.end() != (found = mapped_textures.find(normal_image_asset_file_name_fake)))
            {
                assert(NULL != found->second);
                out_mesh_subset.m_normal_texture_view = found->second;
            }
            else
            {
                D3D11_TEXTURE2D_DESC const texture2d_desc = {
                    1U,
                    1U,
                    1U,
                    1U,
                    fake_normal_texture_format,
                    {1U, 0U},
                    D3D11_USAGE_IMMUTABLE,
                    D3D11_BIND_SHADER_RESOURCE,
                    0U,
                    0U};

                D3D11_SUBRESOURCE_DATA const texture_2d_initial_data = {
                    &packed_normal_data,
                    sizeof(packed_normal_data),
                    sizeof(packed_normal_data)};

                ID3D11Texture2D *normal_texture = NULL;
                HRESULT res_device_create_texture_2d = device->CreateTexture2D(&texture2d_desc, &texture_2d_initial_data, &normal_texture);
                assert(SUCCEEDED(res_device_create_texture_2d));

                D3D11_SHADER_RESOURCE_VIEW_DESC const shader_resource_view_desc = {
                    .Format = fake_normal_texture_format,
                    .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
                    .Texture2D = {0U, 1U}};

                out_mesh_subset.m_normal_texture_view = NULL;
                HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(normal_texture, &shader_resource_view_desc, &out_mesh_subset.m_normal_texture_view);
                assert(SUCCEEDED(res_device_create_shader_resource_view));

                normal_texture->Release();

                mapped_textures.emplace_hint(found, normal_image_asset_file_name_fake, out_mesh_subset.m_normal_texture_view);
            }
        }

        if (!in_mesh_subset_data.m_base_color_texture_image_uri.empty())
        {
            std::string base_color_image_asset_file_name_dds;
            {
                std::string base_color_image_asset_file_name;

                size_t dir_name_pos = file_name.find_last_of("/\\");
                if (std::string::npos != dir_name_pos)
                {
                    base_color_image_asset_file_name = file_name.substr(0U, dir_name_pos + 1U);
                }
                else
                {
                    base_color_image_asset_file_name += "./";
                }

                size_t ext_name_pos = in_mesh_subset_data.m_base_color_texture_image_uri.find_last_of(".");
                if (std::string::npos != ext_name_pos)
                {
                    base_color_image_asset_file_name += in_mesh_subset_data.m_base_color_texture_image_uri.substr(0U, ext_name_pos + 1U);
                }
                else
                {
                    base_color_image_asset_file_name += in_mesh_subset_data.m_base_color_texture_image_uri;
                }

                base_color_image_asset_file_name_dds = (base_color_image_asset_file_name + "dds");
            }

            std::unordered_map<std::string, ID3D11ShaderResourceView *>::const_iterator found;
            if (mapped_textures.end() != (found = mapped_textures.find(base_color_image_asset_file_name_dds)))
            {
                assert(NULL != found->second);
                out_mesh_subset.m_base_color_texture_view = found->second;
            }
            else
            {
                std::wstring base_color_image_asset_file_name_dds_utf16;
                {
                    // llvm::convertUTF8ToUTF16String
                    assert(base_color_image_asset_file_name_dds_utf16.empty());

                    std::string src_utf8 = base_color_image_asset_file_name_dds;
                    std::wstring &dst_utf16 = base_color_image_asset_file_name_dds_utf16;

                    assert(dst_utf16.empty());

                    if (!src_utf8.empty())
                    {
                        llvm::UTF8 const *src = reinterpret_cast<const llvm::UTF8 *>(src_utf8.data());
                        llvm::UTF8 const *src_end = src + src_utf8.size();

                        // Allocate the same number of UTF-16 code units as UTF-8 code units. Encoding
                        // as UTF-16 should always require the same amount or less code units than the
                        // UTF-8 encoding.  Allocate one extra byte for the null terminator though,
                        // so that someone calling DstUTF16.data() gets a null terminated string.
                        // We resize down later so we don't have to worry that this over allocates.
                        dst_utf16.resize(src_utf8.size() + 1U);
                        llvm::UTF16 *dst = reinterpret_cast<llvm::UTF16 *>(dst_utf16.data());
                        llvm::UTF16 *dst_end = dst + dst_utf16.size();

                        llvm::ConversionResult c_r = llvm::ConvertUTF8toUTF16(&src, src_end, &dst, dst_end, llvm::strictConversion);
                        assert(llvm::conversionOK == c_r);

                        dst_utf16.resize(dst - reinterpret_cast<llvm::UTF16 *>(dst_utf16.data()));
                    }
                }

                out_mesh_subset.m_base_color_texture_view = NULL;
                HRESULT res_create_shader_resource_view_from_file = DXUTCreateShaderResourceViewFromFile(device, base_color_image_asset_file_name_dds_utf16.c_str(), &out_mesh_subset.m_base_color_texture_view);
                assert(SUCCEEDED(res_create_shader_resource_view_from_file));

                mapped_textures.emplace_hint(found, base_color_image_asset_file_name_dds, out_mesh_subset.m_base_color_texture_view);
            }
        }
        else
        {
            // avoid gamma correction and float error
            DXGI_FORMAT const fake_base_color_texture_format = DXGI_FORMAT_R8G8B8A8_UNORM;

            DirectX::XMFLOAT4 base_color_data(in_mesh_subset_data.m_base_color_factor.x, in_mesh_subset_data.m_base_color_factor.y, in_mesh_subset_data.m_base_color_factor.z, in_mesh_subset_data.m_base_color_factor.w);

            DirectX::PackedVector::XMUBYTEN4 packed_vector_base_color_data;
            DirectX::PackedVector::XMStoreUByteN4(&packed_vector_base_color_data, DirectX::XMLoadFloat4(&base_color_data));

            std::string base_color_image_asset_file_name_fake;
            {
                {
                    std::string base_color_image_asset_file_name;

                    size_t dir_name_pos = file_name.find_last_of("/\\");
                    if (std::string::npos != dir_name_pos)
                    {
                        base_color_image_asset_file_name = file_name.substr(0U, dir_name_pos + 1U);
                    }
                    else
                    {
                        base_color_image_asset_file_name += "./";
                    }

                    std::string packed_vector_base_color_data_string;
                    {
                        std::stringstream string_stream;
                        string_stream << packed_vector_base_color_data.v;
                        string_stream >> packed_vector_base_color_data_string;
                    }
                    base_color_image_asset_file_name += std::move(packed_vector_base_color_data_string);

                    base_color_image_asset_file_name_fake = (base_color_image_asset_file_name + ".fake-base-color");
                }
            }

            std::unordered_map<std::string, ID3D11ShaderResourceView *>::const_iterator found;
            if (mapped_textures.end() != (found = mapped_textures.find(base_color_image_asset_file_name_fake)))
            {
                assert(NULL != found->second);
                out_mesh_subset.m_base_color_texture_view = found->second;
            }
            else
            {
                D3D11_TEXTURE2D_DESC const texture2d_desc = {
                    1U,
                    1U,
                    1U,
                    1U,
                    fake_base_color_texture_format,
                    {1U, 0U},
                    D3D11_USAGE_IMMUTABLE,
                    D3D11_BIND_SHADER_RESOURCE,
                    0U,
                    0U};

                D3D11_SUBRESOURCE_DATA const texture_2d_initial_data = {
                    &packed_vector_base_color_data,
                    sizeof(packed_vector_base_color_data),
                    sizeof(packed_vector_base_color_data)};

                ID3D11Texture2D *base_color_texture = NULL;
                HRESULT res_device_create_texture_2d = device->CreateTexture2D(&texture2d_desc, &texture_2d_initial_data, &base_color_texture);
                assert(SUCCEEDED(res_device_create_texture_2d));

                D3D11_SHADER_RESOURCE_VIEW_DESC const shader_resource_view_desc = {
                    .Format = fake_base_color_texture_format,
                    .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
                    .Texture2D = {0U, 1U}};

                out_mesh_subset.m_base_color_texture_view = NULL;
                HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(base_color_texture, &shader_resource_view_desc, &out_mesh_subset.m_base_color_texture_view);
                assert(SUCCEEDED(res_device_create_shader_resource_view));

                base_color_texture->Release();

                mapped_textures.emplace_hint(found, base_color_image_asset_file_name_fake, out_mesh_subset.m_base_color_texture_view);
            }
        }

        if (!in_mesh_subset_data.m_metallic_roughness_texture_image_uri.empty())
        {
            std::string metallic_roughness_image_asset_file_name_dds;
            {
                std::string metallic_roughness_image_asset_file_name;

                size_t dir_name_pos = file_name.find_last_of("/\\");
                if (std::string::npos != dir_name_pos)
                {
                    metallic_roughness_image_asset_file_name = file_name.substr(0U, dir_name_pos + 1U);
                }
                else
                {
                    metallic_roughness_image_asset_file_name += "./";
                }

                size_t ext_name_pos = in_mesh_subset_data.m_metallic_roughness_texture_image_uri.find_last_of(".");
                if (std::string::npos != ext_name_pos)
                {
                    metallic_roughness_image_asset_file_name += in_mesh_subset_data.m_metallic_roughness_texture_image_uri.substr(0U, ext_name_pos + 1U);
                }
                else
                {
                    metallic_roughness_image_asset_file_name += in_mesh_subset_data.m_metallic_roughness_texture_image_uri;
                }

                metallic_roughness_image_asset_file_name_dds = (metallic_roughness_image_asset_file_name + "dds");
            }

            std::unordered_map<std::string, ID3D11ShaderResourceView *>::const_iterator found;
            if (mapped_textures.end() != (found = mapped_textures.find(metallic_roughness_image_asset_file_name_dds)))
            {
                assert(NULL != found->second);
                out_mesh_subset.m_metallic_roughness_texture_view = found->second;
            }
            else
            {
                std::wstring metallic_roughness_image_asset_file_name_dds_utf16;
                {
                    // llvm::convertUTF8ToUTF16String
                    assert(metallic_roughness_image_asset_file_name_dds_utf16.empty());

                    std::string src_utf8 = metallic_roughness_image_asset_file_name_dds;
                    std::wstring &dst_utf16 = metallic_roughness_image_asset_file_name_dds_utf16;

                    assert(dst_utf16.empty());

                    if (!src_utf8.empty())
                    {
                        llvm::UTF8 const *src = reinterpret_cast<const llvm::UTF8 *>(src_utf8.data());
                        llvm::UTF8 const *src_end = src + src_utf8.size();

                        // Allocate the same number of UTF-16 code units as UTF-8 code units. Encoding
                        // as UTF-16 should always require the same amount or less code units than the
                        // UTF-8 encoding.  Allocate one extra byte for the null terminator though,
                        // so that someone calling DstUTF16.data() gets a null terminated string.
                        // We resize down later so we don't have to worry that this over allocates.
                        dst_utf16.resize(src_utf8.size() + 1U);
                        llvm::UTF16 *dst = reinterpret_cast<llvm::UTF16 *>(dst_utf16.data());
                        llvm::UTF16 *dst_end = dst + dst_utf16.size();

                        llvm::ConversionResult c_r = llvm::ConvertUTF8toUTF16(&src, src_end, &dst, dst_end, llvm::strictConversion);
                        assert(llvm::conversionOK == c_r);

                        dst_utf16.resize(dst - reinterpret_cast<llvm::UTF16 *>(dst_utf16.data()));
                    }
                }

                out_mesh_subset.m_metallic_roughness_texture_view = NULL;
                HRESULT res_create_shader_resource_view_from_file = DXUTCreateShaderResourceViewFromFile(device, metallic_roughness_image_asset_file_name_dds_utf16.c_str(), &out_mesh_subset.m_metallic_roughness_texture_view);
                assert(SUCCEEDED(res_create_shader_resource_view_from_file));

                mapped_textures.emplace_hint(found, metallic_roughness_image_asset_file_name_dds, out_mesh_subset.m_metallic_roughness_texture_view);
            }
        }
        else
        {
            DXGI_FORMAT const fake_metallic_roughness_texture_format = DXGI_FORMAT_R8G8B8A8_UNORM;

            DirectX::XMFLOAT4 metallic_roughness_data(0.0F, in_mesh_subset_data.m_roughness_factor, in_mesh_subset_data.m_metallic_factor, 1.0F);

            DirectX::PackedVector::XMUBYTEN4 packed_vector_metallic_roughness_data;
            DirectX::PackedVector::XMStoreUByteN4(&packed_vector_metallic_roughness_data, DirectX::XMLoadFloat4(&metallic_roughness_data));

            std::string metallic_roughness_image_asset_file_name_fake;
            {
                {
                    std::string metallic_roughness_image_asset_file_name;

                    size_t dir_name_pos = file_name.find_last_of("/\\");
                    if (std::string::npos != dir_name_pos)
                    {
                        metallic_roughness_image_asset_file_name = file_name.substr(0U, dir_name_pos + 1U);
                    }
                    else
                    {
                        metallic_roughness_image_asset_file_name += "./";
                    }

                    std::string packed_vector_metallic_roughness_data_string;
                    {
                        std::stringstream string_stream;
                        string_stream << packed_vector_metallic_roughness_data.v;
                        string_stream >> packed_vector_metallic_roughness_data_string;
                    }
                    metallic_roughness_image_asset_file_name += std::move(packed_vector_metallic_roughness_data_string);

                    metallic_roughness_image_asset_file_name_fake = (metallic_roughness_image_asset_file_name + ".fake-metallic-roughness");
                }
            }

            std::unordered_map<std::string, ID3D11ShaderResourceView *>::const_iterator found;
            if (mapped_textures.end() != (found = mapped_textures.find(metallic_roughness_image_asset_file_name_fake)))
            {
                assert(NULL != found->second);
                out_mesh_subset.m_metallic_roughness_texture_view = found->second;
            }
            else
            {
                D3D11_TEXTURE2D_DESC const texture2d_desc = {
                    1U,
                    1U,
                    1U,
                    1U,
                    fake_metallic_roughness_texture_format,
                    {1U, 0U},
                    D3D11_USAGE_IMMUTABLE,
                    D3D11_BIND_SHADER_RESOURCE,
                    0U,
                    0U};

                D3D11_SUBRESOURCE_DATA const texture_2d_initial_data = {
                    &packed_vector_metallic_roughness_data,
                    sizeof(packed_vector_metallic_roughness_data),
                    sizeof(packed_vector_metallic_roughness_data)};

                ID3D11Texture2D *metallic_roughness_texture = NULL;
                HRESULT res_device_create_texture_2d = device->CreateTexture2D(&texture2d_desc, &texture_2d_initial_data, &metallic_roughness_texture);
                assert(SUCCEEDED(res_device_create_texture_2d));

                D3D11_SHADER_RESOURCE_VIEW_DESC const shader_resource_view_desc = {
                    .Format = fake_metallic_roughness_texture_format,
                    .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
                    .Texture2D = {0U, 1U}};

                out_mesh_subset.m_metallic_roughness_texture_view = NULL;
                HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(metallic_roughness_texture, &shader_resource_view_desc, &out_mesh_subset.m_metallic_roughness_texture_view);
                assert(SUCCEEDED(res_device_create_shader_resource_view));

                metallic_roughness_texture->Release();

                mapped_textures.emplace_hint(found, metallic_roughness_image_asset_file_name_fake, out_mesh_subset.m_metallic_roughness_texture_view);
            }
        }
    }

    this->m_scene_textures.reserve(mapped_textures.size());
    for (std::unordered_map<std::string, ID3D11ShaderResourceView *>::const_iterator material_texture_iterator = mapped_textures.begin(); material_texture_iterator != mapped_textures.end(); ++material_texture_iterator)
    {
        assert(NULL != material_texture_iterator->second);

        this->m_scene_textures.push_back(material_texture_iterator->second);
    }

    this->m_baked_normal_texture = NULL;
    this->m_baked_normal_rtv = NULL;
    this->m_baked_normal_srv = NULL;
    {
        DXGI_FORMAT const normal_texture_format = DXGI_FORMAT_R8G8B8A8_UNORM;

        D3D11_TEXTURE2D_DESC texture2d_desc;
        texture2d_desc.Width = baked_texture_width;
        texture2d_desc.Height = baked_texture_height;
        texture2d_desc.MipLevels = 1U;
        texture2d_desc.ArraySize = 1U;
        texture2d_desc.Format = normal_texture_format;
        texture2d_desc.SampleDesc.Count = 1U;
        texture2d_desc.SampleDesc.Quality = 0U;
        texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
        texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0U;
        texture2d_desc.MiscFlags = 0U;

        HRESULT res_device_create_texture_2d = device->CreateTexture2D(&texture2d_desc, NULL, &this->m_baked_normal_texture);
        assert(SUCCEEDED(res_device_create_texture_2d));

        WCHAR const debug_object_name[] = {L"Baked Normal"};
        HRESULT res_device_child_set_debug_object_name = this->m_baked_normal_texture->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(debug_object_name), debug_object_name);
        assert(SUCCEEDED(res_device_child_set_debug_object_name));

        D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
        render_target_view_desc.Format = normal_texture_format;
        render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        render_target_view_desc.Texture2D.MipSlice = 0U;

        HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(this->m_baked_normal_texture, &render_target_view_desc, &this->m_baked_normal_rtv);
        assert(SUCCEEDED(res_device_create_render_target_view));

        D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
        shader_resource_view_desc.Format = normal_texture_format;
        shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
        shader_resource_view_desc.Texture2D.MipLevels = 1U;

        HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(this->m_baked_normal_texture, &shader_resource_view_desc, &this->m_baked_normal_srv);
        assert(SUCCEEDED(res_device_create_shader_resource_view));
    }

    this->m_baked_base_color_texture = NULL;
    this->m_baked_base_color_rtv = NULL;
    this->m_baked_base_color_srv = NULL;
    {
        DXGI_FORMAT const base_color_texture_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        D3D11_TEXTURE2D_DESC texture2d_desc;
        texture2d_desc.Width = baked_texture_width;
        texture2d_desc.Height = baked_texture_height;
        texture2d_desc.MipLevels = 1U;
        texture2d_desc.ArraySize = 1U;
        texture2d_desc.Format = base_color_texture_format;
        texture2d_desc.SampleDesc.Count = 1U;
        texture2d_desc.SampleDesc.Quality = 0U;
        texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
        texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0U;
        texture2d_desc.MiscFlags = 0U;

        HRESULT res_device_create_texture_2d = device->CreateTexture2D(&texture2d_desc, NULL, &this->m_baked_base_color_texture);
        assert(SUCCEEDED(res_device_create_texture_2d));

        WCHAR const debug_object_name[] = {L"Baked Base Color"};
        HRESULT res_device_child_set_debug_object_name = this->m_baked_base_color_texture->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(debug_object_name), debug_object_name);
        assert(SUCCEEDED(res_device_child_set_debug_object_name));

        D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
        render_target_view_desc.Format = base_color_texture_format;
        render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        render_target_view_desc.Texture2D.MipSlice = 0U;

        HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(this->m_baked_base_color_texture, &render_target_view_desc, &this->m_baked_base_color_rtv);
        assert(SUCCEEDED(res_device_create_render_target_view));

        D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
        shader_resource_view_desc.Format = base_color_texture_format;
        shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
        shader_resource_view_desc.Texture2D.MipLevels = 1U;

        HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(this->m_baked_base_color_texture, &shader_resource_view_desc, &this->m_baked_base_color_srv);
        assert(SUCCEEDED(res_device_create_shader_resource_view));
    }

    this->m_baked_metallic_roughness_texture = NULL;
    this->m_baked_metallic_roughness_rtv = NULL;
    this->m_baked_metallic_roughness_srv = NULL;
    {
        DXGI_FORMAT const metallic_roughness_texture_format = DXGI_FORMAT_R8G8B8A8_UNORM;

        D3D11_TEXTURE2D_DESC texture2d_desc;
        texture2d_desc.Width = baked_texture_width;
        texture2d_desc.Height = baked_texture_height;
        texture2d_desc.MipLevels = 1U;
        texture2d_desc.ArraySize = 1U;
        texture2d_desc.Format = metallic_roughness_texture_format;
        texture2d_desc.SampleDesc.Count = 1U;
        texture2d_desc.SampleDesc.Quality = 0U;
        texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
        texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0U;
        texture2d_desc.MiscFlags = 0U;

        HRESULT res_device_create_texture_2d = device->CreateTexture2D(&texture2d_desc, NULL, &this->m_baked_metallic_roughness_texture);
        assert(SUCCEEDED(res_device_create_texture_2d));

        WCHAR const debug_object_name[] = {L"Baked Metallic Roughness"};
        HRESULT res_device_child_set_debug_object_name = this->m_baked_metallic_roughness_texture->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(debug_object_name), debug_object_name);
        assert(SUCCEEDED(res_device_child_set_debug_object_name));

        D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
        render_target_view_desc.Format = metallic_roughness_texture_format;
        render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        render_target_view_desc.Texture2D.MipSlice = 0U;

        HRESULT res_device_create_render_target_view = device->CreateRenderTargetView(this->m_baked_metallic_roughness_texture, &render_target_view_desc, &this->m_baked_metallic_roughness_rtv);
        assert(SUCCEEDED(res_device_create_render_target_view));

        D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
        shader_resource_view_desc.Format = metallic_roughness_texture_format;
        shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
        shader_resource_view_desc.Texture2D.MipLevels = 1U;

        HRESULT res_device_create_shader_resource_view = device->CreateShaderResourceView(this->m_baked_metallic_roughness_texture, &shader_resource_view_desc, &this->m_baked_metallic_roughness_srv);
        assert(SUCCEEDED(res_device_create_shader_resource_view));
    }

    this->m_baking_vs = NULL;
    {
#define g_vertex_varying_buffer_stride 16u
#define g_index_uint32_buffer_stride 4u
        static_assert(sizeof(scene_mesh_vertex_varying_binding) == g_vertex_varying_buffer_stride, "");
        static_assert(sizeof(uint32_t) == g_index_uint32_buffer_stride, "");
#undef g_vertex_varying_buffer_stride
#undef g_index_uint32_buffer_stride

        static
#include "_internal_baking_vertex.inl"
            HRESULT res_device_create_vertex_shader = device->CreateVertexShader(baking_vertex_shader_module_code, sizeof(baking_vertex_shader_module_code), NULL, &this->m_baking_vs);
        assert(SUCCEEDED(res_device_create_vertex_shader));
    }

    this->m_baking_rs = NULL;
    {
        D3D11_RASTERIZER_DESC2 rasterizer_desc;
        rasterizer_desc.FillMode = D3D11_FILL_SOLID;
        rasterizer_desc.CullMode = D3D11_CULL_NONE;
        rasterizer_desc.FrontCounterClockwise = TRUE;
        rasterizer_desc.DepthBias = 0;
        rasterizer_desc.SlopeScaledDepthBias = 0.0f;
        rasterizer_desc.DepthBiasClamp = 0.0f;
        rasterizer_desc.DepthClipEnable = TRUE;
        rasterizer_desc.ScissorEnable = FALSE;
        rasterizer_desc.MultisampleEnable = FALSE;
        rasterizer_desc.AntialiasedLineEnable = FALSE;
        rasterizer_desc.MultisampleEnable = FALSE;
        rasterizer_desc.ForcedSampleCount = 0U;
        rasterizer_desc.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON;

        HRESULT res_device_create_rasterizer_state = device->CreateRasterizerState2(&rasterizer_desc, &this->m_baking_rs);
        assert(SUCCEEDED(res_device_create_rasterizer_state));
    }

    this->m_baking_ps = NULL;
    {
        static
#include "_internal_baking_pixel.inl"
            HRESULT res_device_create_pixel_shader = device->CreatePixelShader(baking_pixel_shader_module_code, sizeof(baking_pixel_shader_module_code), NULL, &this->m_baking_ps);
        assert(SUCCEEDED(res_device_create_pixel_shader));
    }

    this->m_full_screen_vs = NULL;
    {
        static
#include "_internal_full_screen_vertex.inl"
            HRESULT res_device_create_vertex_shader = device->CreateVertexShader(full_screen_vertex_shader_module_code, sizeof(full_screen_vertex_shader_module_code), NULL, &this->m_full_screen_vs);
        assert(SUCCEEDED(res_device_create_vertex_shader));
    }

    this->m_full_screen_rs = NULL;
    {
        D3D11_RASTERIZER_DESC rasterizer_desc;
        rasterizer_desc.FillMode = D3D11_FILL_SOLID;
        rasterizer_desc.CullMode = D3D11_CULL_NONE;
        rasterizer_desc.FrontCounterClockwise = TRUE;
        rasterizer_desc.DepthBias = 0;
        rasterizer_desc.SlopeScaledDepthBias = 0.0f;
        rasterizer_desc.DepthBiasClamp = 0.0f;
        rasterizer_desc.DepthClipEnable = TRUE;
        rasterizer_desc.ScissorEnable = FALSE;
        rasterizer_desc.MultisampleEnable = FALSE;
        rasterizer_desc.AntialiasedLineEnable = FALSE;
        rasterizer_desc.MultisampleEnable = FALSE;
        HRESULT res_device_create_rasterizer_state = device->CreateRasterizerState(&rasterizer_desc, &this->m_full_screen_rs);
        assert(SUCCEEDED(res_device_create_rasterizer_state));
    }

    this->m_full_screen_clear_ps = NULL;
    {
        std::cout << "NOTE: The hard-coding color value in the shader \"full_screen_clear_pixel.hlsl\" should be changed!" << std::endl;

        static
#include "_internal_full_screen_clear_pixel.inl"
            HRESULT res_device_create_pixel_shader = device->CreatePixelShader(full_screen_clear_pixel_shader_module_code, sizeof(full_screen_clear_pixel_shader_module_code), NULL, &this->m_full_screen_clear_ps);
        assert(SUCCEEDED(res_device_create_pixel_shader));
    }

    this->m_full_screen_transfer_ps = NULL;
    {
        static
#include "_internal_full_screen_transfer_pixel.inl"
            HRESULT res_device_create_pixel_shader = device->CreatePixelShader(full_screen_transfer_pixel_shader_module_code, sizeof(full_screen_transfer_pixel_shader_module_code), NULL, &this->m_full_screen_transfer_ps);
        assert(SUCCEEDED(res_device_create_pixel_shader));
    }
}

void Demo::Tick(ID3D11Device3 *device, ID3D11DeviceContext *device_context, ID3D11RenderTargetView *swap_chain_back_buffer_rtv, uint32_t swap_chain_image_width, uint32_t swap_chain_image_height)
{
    // Baking
    {
        ID3D11RenderTargetView *const bind_rtvs[3] = {this->m_baked_normal_rtv, this->m_baked_base_color_rtv, this->m_baked_metallic_roughness_rtv};
        device_context->OMSetRenderTargets(sizeof(bind_rtvs) / sizeof(bind_rtvs[0]), bind_rtvs, NULL);

        D3D11_VIEWPORT viewport = {0.0F, 0.0F, static_cast<FLOAT>(baked_texture_width), static_cast<FLOAT>(baked_texture_height), 0.0F, 1.0F};
        device_context->RSSetViewports(1U, &viewport);

        // Clear
        {
            device_context->RSSetState(this->m_full_screen_rs);

            device_context->VSSetShader(this->m_full_screen_vs, NULL, 0U);
            device_context->PSSetShader(this->m_full_screen_clear_ps, NULL, 0U);

            device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            device_context->DrawInstanced(3U, 1U, 0U, 0U);
        }

        // UV Map
        {
            device_context->RSSetState(this->m_baking_rs);

            device_context->VSSetShader(this->m_baking_vs, NULL, 0U);
            device_context->PSSetShader(this->m_baking_ps, NULL, 0U);

            for (size_t mesh_subset_index = 0U; mesh_subset_index < this->m_scene_mesh_subsets.size(); ++mesh_subset_index)
            {
                Demo_Mesh_Subset const &scene_mesh_subset = this->m_scene_mesh_subsets[mesh_subset_index];

                ID3D11ShaderResourceView *const bind_vs_srvs[2] = {scene_mesh_subset.m_vertex_varying_buffer_view, scene_mesh_subset.m_index_buffer_view};
                device_context->VSSetShaderResources(0U, sizeof(bind_vs_srvs) / sizeof(bind_vs_srvs[0]), bind_vs_srvs);

                ID3D11ShaderResourceView *const bind_ps_srvs[3] = {scene_mesh_subset.m_normal_texture_view, scene_mesh_subset.m_base_color_texture_view, scene_mesh_subset.m_metallic_roughness_texture_view};
                device_context->PSSetShaderResources(0U, sizeof(bind_ps_srvs) / sizeof(bind_ps_srvs[0]), bind_ps_srvs);
                device_context->PSSetSamplers(0U, 1U, &this->m_sampler);

                device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                device_context->DrawInstanced(scene_mesh_subset.m_index_count, 1U, 0U, 0U);

                ID3D11ShaderResourceView *const unbind_vs_srvs[2] = {NULL, NULL};
                device_context->VSSetShaderResources(0U, sizeof(unbind_vs_srvs) / sizeof(unbind_vs_srvs[0]), unbind_vs_srvs);

                ID3D11ShaderResourceView *const unbind_ps_srvs[3] = {NULL, NULL, NULL};
                device_context->PSSetShaderResources(0U, sizeof(unbind_ps_srvs) / sizeof(unbind_ps_srvs[0]), unbind_ps_srvs);
            }

            ID3D11RenderTargetView *const unbind_rtvs[3] = {NULL, NULL, NULL};
            device_context->OMSetRenderTargets(sizeof(unbind_rtvs) / sizeof(unbind_rtvs[0]), unbind_rtvs, NULL);
        }
    }

    // Transfer
    {
        ID3D11RenderTargetView *const bind_rtvs[1] = {swap_chain_back_buffer_rtv};

        device_context->OMSetRenderTargets(sizeof(bind_rtvs) / sizeof(bind_rtvs[0]), bind_rtvs, NULL);

        D3D11_VIEWPORT viewport = {0.0F, 0.0F, static_cast<FLOAT>(swap_chain_image_width), static_cast<FLOAT>(swap_chain_image_height), 0.0F, 1.0F};
        device_context->RSSetViewports(1U, &viewport);

        device_context->RSSetState(this->m_full_screen_rs);

        device_context->VSSetShader(this->m_full_screen_vs, NULL, 0U);
        device_context->PSSetShader(this->m_full_screen_transfer_ps, NULL, 0U);

        ID3D11ShaderResourceView *const bind_srvs[3] = {this->m_baked_normal_srv, this->m_baked_base_color_srv, this->m_baked_metallic_roughness_srv};
        device_context->PSSetShaderResources(0U, sizeof(bind_srvs) / sizeof(bind_srvs[0]), bind_srvs);
        device_context->PSSetSamplers(0U, 1U, &this->m_sampler);

        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->DrawInstanced(3U, 1U, 0U, 0U);

        ID3D11ShaderResourceView *const unbind_srvs[3] = {NULL, NULL, NULL};
        device_context->PSSetShaderResources(0U, sizeof(unbind_srvs) / sizeof(unbind_srvs[0]), unbind_srvs);

        ID3D11RenderTargetView *const unbind_rtvs[1] = {NULL};
        device_context->OMSetRenderTargets(sizeof(unbind_rtvs) / sizeof(unbind_rtvs[0]), unbind_rtvs, NULL);
    }
}

void Demo::Destroy(ID3D11Device3 *device, ID3D11DeviceContext *device_context)
{
    this->m_sampler->Release();

    for (size_t mesh_subset_index = 0U; mesh_subset_index < this->m_scene_mesh_subsets.size(); ++mesh_subset_index)
    {
        Demo_Mesh_Subset &scene_mesh_subset = this->m_scene_mesh_subsets[mesh_subset_index];

        scene_mesh_subset.m_vertex_varying_buffer_view->Release();
        scene_mesh_subset.m_index_buffer_view->Release();
    }

    for (size_t texture_index = 0U; texture_index < this->m_scene_textures.size(); ++texture_index)
    {
        assert(NULL != this->m_scene_textures[texture_index]);

        this->m_scene_textures[texture_index]->Release();
    }

    this->m_baked_base_color_texture->Release();
    this->m_baked_base_color_rtv->Release();
    this->m_baked_base_color_srv->Release();

    this->m_baked_metallic_roughness_texture->Release();
    this->m_baked_metallic_roughness_rtv->Release();
    this->m_baked_metallic_roughness_srv->Release();

    this->m_baking_vs->Release();
    this->m_baking_rs->Release();
    this->m_baking_ps->Release();

    this->m_full_screen_vs->Release();
    this->m_full_screen_rs->Release();
    this->m_full_screen_clear_ps->Release();
    this->m_full_screen_transfer_ps->Release();
}

#include "../thirdparty/cgltf/cgltf.h"

static cgltf_result cgltf_custom_read_file(const struct cgltf_memory_options *memory_options, const struct cgltf_file_options *, const char *path, cgltf_size *size, void **data);

static void cgltf_custom_file_release(const struct cgltf_memory_options *memory_options, const struct cgltf_file_options *, void *data);

static void *cgltf_custom_alloc(void *, cgltf_size size);

static void cgltf_custom_free(void *, void *ptr);

static void import_mesh(std::vector<scene_mesh_subset_data> &out_mesh_subset_data, cgltf_data const *data, cgltf_mesh const *mesh);

static void import_scene(std::vector<scene_mesh_subset_data> &out_mesh_subset_data, char const *path)
{
    cgltf_data *data = NULL;
    {
        cgltf_options options = {};
        options.memory.alloc_func = cgltf_custom_alloc;
        options.memory.free_func = cgltf_custom_free;
        options.file.read = cgltf_custom_read_file;
        options.file.release = cgltf_custom_file_release;
        options.file.user_data = NULL;

        cgltf_result result_parse_file = cgltf_parse_file(&options, path, &data);
        assert(cgltf_result_success == result_parse_file);

        cgltf_result result_load_buffers = cgltf_load_buffers(&options, data, path);
        assert(cgltf_result_success == result_load_buffers);
    }

    std::cout << "TODO: At this moment, we merge all meshes together!" << std::endl;

    for (size_t mesh_index = 0U; mesh_index < data->meshes_count; ++mesh_index)
    {
        std::vector<scene_mesh_subset_data> current_mesh_subset_data;

        import_mesh(current_mesh_subset_data, data, &data->meshes[mesh_index]);

        size_t const mesh_subset_index_offset = out_mesh_subset_data.size();

        out_mesh_subset_data.resize(mesh_subset_index_offset + current_mesh_subset_data.size());

        for (size_t current_mesh_subset_index = 0U; current_mesh_subset_index < current_mesh_subset_data.size(); ++current_mesh_subset_index)
        {
            out_mesh_subset_data[mesh_subset_index_offset + current_mesh_subset_index] = std::move(current_mesh_subset_data[current_mesh_subset_index]);
        }
    }

    cgltf_free(data);
}

static void import_mesh(std::vector<scene_mesh_subset_data> &out_mesh_subset_data, cgltf_data const *data, cgltf_mesh const *mesh)
{
    std::unordered_map<size_t, size_t> subset_material_indices;
    subset_material_indices.reserve(mesh->primitives_count);

    out_mesh_subset_data.reserve(mesh->primitives_count);

    for (size_t primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
    {
        cgltf_primitive const *primitive = &mesh->primitives[primitive_index];

        if (cgltf_primitive_type_triangles == primitive->type)
        {
            cgltf_accessor const *texcoord_0_accessor = NULL;
            cgltf_accessor const *texcoord_1_accessor = NULL;
            {
                for (size_t vertex_attribute_index = 0; vertex_attribute_index < primitive->attributes_count; ++vertex_attribute_index)
                {
                    cgltf_attribute const *vertex_attribute = &primitive->attributes[vertex_attribute_index];

                    switch (vertex_attribute->type)
                    {
                    case cgltf_attribute_type_texcoord:
                    {
                        assert(cgltf_attribute_type_texcoord == vertex_attribute->type);

                        if (NULL == texcoord_0_accessor && 0 == vertex_attribute->index)
                        {
                            texcoord_0_accessor = vertex_attribute->data;
                        }
                        else if (NULL == texcoord_1_accessor && 1 == vertex_attribute->index)
                        {
                            texcoord_1_accessor = vertex_attribute->data;
                        }
                        else
                        {
                            assert(0);
                        }
                    }
                    break;
                    default:
                    {
                        // Do Nothing
                    }
                    }
                }
            }

            if (NULL != texcoord_0_accessor && NULL != texcoord_1_accessor)
            {
                cgltf_accessor const *const index_accessor = primitive->indices;

                size_t const vertex_count = texcoord_0_accessor->count;
                size_t const index_count = (NULL != index_accessor) ? index_accessor->count : vertex_count;

                std::vector<uint32_t> raw_indices(index_count);
                std::vector<DirectX::XMFLOAT2> raw_texcoord_0_s(vertex_count);
                std::vector<DirectX::XMFLOAT2> raw_texcoord_1_s(vertex_count);
                {
                    // TODO: support strip and fan
                    assert(0U == (index_count % 3U));
                    size_t const face_count = index_count / 3U;

                    if (NULL != index_accessor)
                    {
                        uintptr_t index_base = -1;
                        size_t index_stride = -1;
                        {
                            cgltf_buffer_view const *const index_buffer_view = index_accessor->buffer_view;
                            index_base = reinterpret_cast<uintptr_t>(index_buffer_view->buffer->data) + index_buffer_view->offset + index_accessor->offset;
                            index_stride = (0 != index_buffer_view->stride) ? index_buffer_view->stride : index_accessor->stride;
                        }

                        assert(cgltf_type_scalar == index_accessor->type);

                        switch (index_accessor->component_type)
                        {
                        case cgltf_component_type_r_8u:
                        {
                            assert(cgltf_component_type_r_8u == index_accessor->component_type);

                            for (size_t index_index = 0; index_index < index_accessor->count; ++index_index)
                            {
                                uint8_t const *const index_ubyte = reinterpret_cast<uint8_t const *>(index_base + index_stride * index_index);

                                uint32_t const raw_index = static_cast<uint32_t>(*index_ubyte);

                                raw_indices[index_index] = raw_index;
                            }
                        }
                        break;
                        case cgltf_component_type_r_16u:
                        {
                            assert(cgltf_component_type_r_16u == index_accessor->component_type);

                            for (size_t index_index = 0; index_index < index_accessor->count; ++index_index)
                            {
                                uint16_t const *const index_ushort = reinterpret_cast<uint16_t const *>(index_base + index_stride * index_index);

                                uint32_t const raw_index = static_cast<uint32_t>(*index_ushort);

                                raw_indices[index_index] = raw_index;
                            }
                        }
                        break;
                        case cgltf_component_type_r_32u:
                        {
                            assert(cgltf_component_type_r_32u == index_accessor->component_type);

                            for (size_t index_index = 0; index_index < index_accessor->count; ++index_index)
                            {
                                uint32_t const *const index_uint = reinterpret_cast<uint32_t const *>(index_base + index_stride * index_index);

                                uint32_t const raw_index = (*index_uint);

                                raw_indices[index_index] = raw_index;
                            }
                        }
                        break;
                        default:
                            assert(0);
                        }
                    }
                    else
                    {
                        for (size_t index_index = 0; index_index < index_count; ++index_index)
                        {
                            uint32_t const raw_index = static_cast<uint32_t>(index_index);

                            raw_indices[index_index] = raw_index;
                        }
                    }

                    assert(NULL != texcoord_0_accessor);
                    {
                        uintptr_t texcoord_0_base = -1;
                        size_t texcoord_0_stride = -1;
                        {
                            cgltf_buffer_view const *const texcoord_0_buffer_view = texcoord_0_accessor->buffer_view;
                            texcoord_0_base = reinterpret_cast<uintptr_t>(texcoord_0_buffer_view->buffer->data) + texcoord_0_buffer_view->offset + texcoord_0_accessor->offset;
                            texcoord_0_stride = (0 != texcoord_0_buffer_view->stride) ? texcoord_0_buffer_view->stride : texcoord_0_accessor->stride;
                        }

                        assert(texcoord_0_accessor->count == vertex_count);

                        assert(cgltf_type_vec2 == texcoord_0_accessor->type);

                        switch (texcoord_0_accessor->component_type)
                        {
                        case cgltf_component_type_r_8u:
                        {
                            assert(cgltf_component_type_r_8u == texcoord_0_accessor->component_type);

                            for (size_t vertex_index = 0; vertex_index < texcoord_0_accessor->count; ++vertex_index)
                            {
                                uint8_t const *const texcoord_ubyte2 = reinterpret_cast<uint8_t const *>(texcoord_0_base + texcoord_0_stride * vertex_index);

                                DirectX::PackedVector::XMUBYTEN2 packed_vector_ubyten2(texcoord_ubyte2[0], texcoord_ubyte2[1]);

                                DirectX::XMVECTOR unpacked_vector = DirectX::PackedVector::XMLoadUByteN2(&packed_vector_ubyten2);

                                DirectX::XMStoreFloat2(&raw_texcoord_0_s[vertex_index], unpacked_vector);
                            }
                        }
                        break;
                        case cgltf_component_type_r_16u:
                        {
                            assert(cgltf_component_type_r_16u == texcoord_0_accessor->component_type);

                            for (size_t vertex_index = 0; vertex_index < texcoord_0_accessor->count; ++vertex_index)
                            {
                                uint16_t const *const texcoord_ushortn2 = reinterpret_cast<uint16_t const *>(texcoord_0_base + texcoord_0_stride * vertex_index);

                                DirectX::PackedVector::XMUSHORTN2 packed_vector_ushortn2(texcoord_ushortn2[0], texcoord_ushortn2[1]);

                                DirectX::XMVECTOR unpacked_vector = DirectX::PackedVector::XMLoadUShortN2(&packed_vector_ushortn2);

                                DirectX::XMStoreFloat2(&raw_texcoord_0_s[vertex_index], unpacked_vector);
                            }
                        }
                        break;
                        case cgltf_component_type_r_32f:
                        {
                            assert(cgltf_component_type_r_32f == texcoord_0_accessor->component_type);

                            for (size_t vertex_index = 0; vertex_index < texcoord_0_accessor->count; ++vertex_index)
                            {
                                float const *const texcoord_float2 = reinterpret_cast<float const *>(texcoord_0_base + texcoord_0_stride * vertex_index);

                                raw_texcoord_0_s[vertex_index] = DirectX::XMFLOAT2(texcoord_float2[0], texcoord_float2[1]);
                            }
                        }
                        break;
                        default:
                            assert(0);
                        }
                    }

                    assert(NULL != texcoord_1_accessor);
                    {
                        uintptr_t texcoord_1_base = -1;
                        size_t texcoord_1_stride = -1;
                        {
                            cgltf_buffer_view const *const texcoord_1_buffer_view = texcoord_1_accessor->buffer_view;
                            texcoord_1_base = reinterpret_cast<uintptr_t>(texcoord_1_buffer_view->buffer->data) + texcoord_1_buffer_view->offset + texcoord_1_accessor->offset;
                            texcoord_1_stride = (0 != texcoord_1_buffer_view->stride) ? texcoord_1_buffer_view->stride : texcoord_1_accessor->stride;
                        }

                        assert(texcoord_1_accessor->count == vertex_count);

                        assert(cgltf_type_vec2 == texcoord_1_accessor->type);

                        switch (texcoord_1_accessor->component_type)
                        {
                        case cgltf_component_type_r_8u:
                        {
                            assert(cgltf_component_type_r_8u == texcoord_1_accessor->component_type);

                            for (size_t vertex_index = 0; vertex_index < texcoord_1_accessor->count; ++vertex_index)
                            {
                                uint8_t const *const texcoord_ubyte2 = reinterpret_cast<uint8_t const *>(texcoord_1_base + texcoord_1_stride * vertex_index);

                                DirectX::PackedVector::XMUBYTEN2 packed_vector_ubyten2(texcoord_ubyte2[0], texcoord_ubyte2[1]);

                                DirectX::XMVECTOR unpacked_vector = DirectX::PackedVector::XMLoadUByteN2(&packed_vector_ubyten2);

                                DirectX::XMStoreFloat2(&raw_texcoord_1_s[vertex_index], unpacked_vector);
                            }
                        }
                        break;
                        case cgltf_component_type_r_16u:
                        {
                            assert(cgltf_component_type_r_16u == texcoord_1_accessor->component_type);

                            for (size_t vertex_index = 0; vertex_index < texcoord_1_accessor->count; ++vertex_index)
                            {
                                uint16_t const *const texcoord_ushortn2 = reinterpret_cast<uint16_t const *>(texcoord_1_base + texcoord_1_stride * vertex_index);

                                DirectX::PackedVector::XMUSHORTN2 packed_vector_ushortn2(texcoord_ushortn2[0], texcoord_ushortn2[1]);

                                DirectX::XMVECTOR unpacked_vector = DirectX::PackedVector::XMLoadUShortN2(&packed_vector_ushortn2);

                                DirectX::XMStoreFloat2(&raw_texcoord_1_s[vertex_index], unpacked_vector);
                            }
                        }
                        break;
                        case cgltf_component_type_r_32f:
                        {
                            assert(cgltf_component_type_r_32f == texcoord_1_accessor->component_type);

                            for (size_t vertex_index = 0; vertex_index < texcoord_1_accessor->count; ++vertex_index)
                            {
                                float const *const texcoord_float2 = reinterpret_cast<float const *>(texcoord_1_base + texcoord_1_stride * vertex_index);

                                raw_texcoord_1_s[vertex_index] = DirectX::XMFLOAT2(texcoord_float2[0], texcoord_float2[1]);
                            }
                        }
                        break;
                        default:
                            assert(0);
                        }
                    }
                }

                // We merge the primitives, of which the material is the same, within the same mesh
                // TODO: shall we merge the primitives within different meshes (consider multiple instances)
                scene_mesh_subset_data *out_subset_data;
                uint32_t out_subset_vertex_index_offset;
                uint32_t out_subset_index_index_offset;
                {
                    cgltf_material const *const primitive_material = primitive->material;

                    size_t const primitive_material_index = cgltf_material_index(data, primitive_material);

                    auto found = subset_material_indices.find(primitive_material_index);
                    if (subset_material_indices.end() != found)
                    {
                        size_t const subset_data_index = found->second;
                        assert(subset_data_index < out_mesh_subset_data.size());
                        out_subset_data = &out_mesh_subset_data[subset_data_index];
                        out_subset_vertex_index_offset = static_cast<uint32_t>(out_subset_data->m_vertex_varying_binding.size());
                        out_subset_index_index_offset = static_cast<uint32_t>(out_subset_data->m_indices.size());
                    }
                    else
                    {
                        size_t const subset_data_index = out_mesh_subset_data.size();
                        out_mesh_subset_data.push_back({});
                        subset_material_indices.emplace_hint(found, primitive_material_index, subset_data_index);
                        out_subset_data = &out_mesh_subset_data.back();
                        out_subset_vertex_index_offset = 0U;
                        out_subset_index_index_offset = 0U;

                        if (NULL != primitive_material->normal_texture.texture)
                        {
                            cgltf_image const *const normal_texture_image = primitive_material->normal_texture.texture->image;
                            assert(NULL != normal_texture_image);
                            assert(NULL == normal_texture_image->buffer_view);
                            assert(NULL != normal_texture_image->uri);

                            out_subset_data->m_normal_texture_image_uri = normal_texture_image->uri;
                            cgltf_decode_uri(&out_subset_data->m_normal_texture_image_uri[0]);
                            size_t null_terminator_pos = out_subset_data->m_normal_texture_image_uri.find('\0');
                            if (std::string::npos != null_terminator_pos)
                            {
                                out_subset_data->m_normal_texture_image_uri.resize(null_terminator_pos);
                            }

                            // TODO: support normal scale
                            assert(1.0F == primitive_material->normal_texture.scale);
                        }

                        if (primitive_material->has_pbr_metallic_roughness)
                        {
                            out_subset_data->m_base_color_factor = DirectX::XMFLOAT4(primitive_material->pbr_metallic_roughness.base_color_factor[0], primitive_material->pbr_metallic_roughness.base_color_factor[1], primitive_material->pbr_metallic_roughness.base_color_factor[2], primitive_material->pbr_metallic_roughness.base_color_factor[3]);

                            if (NULL != primitive_material->pbr_metallic_roughness.base_color_texture.texture)
                            {
                                cgltf_image const *const base_color_texture_image = primitive_material->pbr_metallic_roughness.base_color_texture.texture->image;
                                assert(NULL != base_color_texture_image);
                                assert(NULL == base_color_texture_image->buffer_view);
                                assert(NULL != base_color_texture_image->uri);

                                out_subset_data->m_base_color_texture_image_uri = base_color_texture_image->uri;
                                cgltf_decode_uri(&out_subset_data->m_base_color_texture_image_uri[0]);
                                size_t null_terminator_pos = out_subset_data->m_base_color_texture_image_uri.find('\0');
                                if (std::string::npos != null_terminator_pos)
                                {
                                    out_subset_data->m_base_color_texture_image_uri.resize(null_terminator_pos);
                                }
                            }

                            out_subset_data->m_metallic_factor = primitive_material->pbr_metallic_roughness.metallic_factor;
                            out_subset_data->m_roughness_factor = primitive_material->pbr_metallic_roughness.roughness_factor;

                            if (NULL != primitive_material->pbr_metallic_roughness.metallic_roughness_texture.texture)
                            {
                                cgltf_image const *const metallic_roughness_texture_image = primitive_material->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
                                assert(NULL != metallic_roughness_texture_image);
                                assert(NULL == metallic_roughness_texture_image->buffer_view);
                                assert(NULL != metallic_roughness_texture_image->uri);

                                out_subset_data->m_metallic_roughness_texture_image_uri = metallic_roughness_texture_image->uri;
                                cgltf_decode_uri(&out_subset_data->m_metallic_roughness_texture_image_uri[0]);
                                size_t null_terminator_pos = out_subset_data->m_metallic_roughness_texture_image_uri.find('\0');
                                if (std::string::npos != null_terminator_pos)
                                {
                                    out_subset_data->m_metallic_roughness_texture_image_uri.resize(null_terminator_pos);
                                }
                            }
                        }
                        else
                        {
                            assert(0);
                        }
                    }
                }

                // Indices
                {
                    // The vertex buffer for each primitive is independent
                    // Index for each primitive is from zero
                    assert(out_subset_index_index_offset == out_subset_data->m_indices.size());
                    out_subset_data->m_indices.resize(out_subset_index_index_offset + index_count);

                    uint32_t *const out_indices = &out_subset_data->m_indices[out_subset_index_index_offset];

                    for (size_t index_index = 0; index_index < index_count; ++index_index)
                    {
                        out_indices[index_index] = (out_subset_vertex_index_offset + raw_indices[index_index]);
                    }
                }

                // Vertex Varying Binding
                {
                    assert(out_subset_vertex_index_offset == out_subset_data->m_vertex_varying_binding.size());
                    out_subset_data->m_vertex_varying_binding.resize(out_subset_vertex_index_offset + vertex_count);
                    scene_mesh_vertex_varying_binding *const out_vertex_varying_binding = &out_subset_data->m_vertex_varying_binding[out_subset_vertex_index_offset];

                    for (size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
                    {
                        out_vertex_varying_binding[vertex_index].m_texcoord_0 = raw_texcoord_0_s[vertex_index];

                        out_vertex_varying_binding[vertex_index].m_texcoord_1 = raw_texcoord_1_s[vertex_index];
                    }
                }
            }
            else
            {
                // TODO:
                assert(0);
            }
        }
        else
        {
            // TODO:
            assert(0);
        }
    }
}

static cgltf_result cgltf_custom_read_file(const struct cgltf_memory_options *memory_options, const struct cgltf_file_options *, const char *path, cgltf_size *size, void **data)
{
    void *(*const memory_alloc)(void *, cgltf_size) = memory_options->alloc_func;
    assert(NULL != memory_alloc);

    void (*const memory_free)(void *, void *) = memory_options->free_func;
    assert(NULL != memory_free);

    WCHAR path_utf16[4096];
    {
        std::wstring file_name_utf16;
        {
            // llvm::convertUTF8ToUTF16String
            assert(file_name_utf16.empty());

            std::string src_utf8 = path;
            std::wstring &dst_utf16 = file_name_utf16;

            assert(dst_utf16.empty());

            if (!src_utf8.empty())
            {
                llvm::UTF8 const *src = reinterpret_cast<const llvm::UTF8 *>(src_utf8.data());
                llvm::UTF8 const *src_end = src + src_utf8.size();

                // Allocate the same number of UTF-16 code units as UTF-8 code units. Encoding
                // as UTF-16 should always require the same amount or less code units than the
                // UTF-8 encoding.  Allocate one extra byte for the null terminator though,
                // so that someone calling DstUTF16.data() gets a null terminated string.
                // We resize down later so we don't have to worry that this over allocates.
                dst_utf16.resize(src_utf8.size() + 1U);
                llvm::UTF16 *dst = reinterpret_cast<llvm::UTF16 *>(dst_utf16.data());
                llvm::UTF16 *dst_end = dst + dst_utf16.size();

                llvm::ConversionResult c_r = llvm::ConvertUTF8toUTF16(&src, src_end, &dst, dst_end, llvm::strictConversion);
                assert(llvm::conversionOK == c_r);

                dst_utf16.resize(dst - reinterpret_cast<llvm::UTF16 *>(dst_utf16.data()));
            }
        }

        HRESULT res_find_dxsdk_media_file_cch = DXUTFindDXSDKMediaFileCch(path_utf16, sizeof(path_utf16) / sizeof(path_utf16[0]), file_name_utf16.c_str());
        assert(SUCCEEDED(res_find_dxsdk_media_file_cch));
    }

    HANDLE file = CreateFileW(path_utf16, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (INVALID_HANDLE_VALUE == file)
    {
        return cgltf_result_file_not_found;
    }

    cgltf_size file_size = (NULL != size) ? (*size) : 0;
    if (file_size == 0)
    {
        LARGE_INTEGER length;
        BOOL res_get_file_size_ex = GetFileSizeEx(file, &length);

        if (FALSE == res_get_file_size_ex)
        {
            BOOL res_close_handle = CloseHandle(file);
            assert(FALSE != res_close_handle);
            return cgltf_result_io_error;
        }

        file_size = static_cast<size_t>(length.QuadPart);
    }

    void *file_data = memory_alloc(memory_options->user_data, file_size);
    if (NULL == file_data)
    {
        BOOL res_close_handle = CloseHandle(file);
        assert(FALSE != res_close_handle);
        return cgltf_result_out_of_memory;
    }

    DWORD read_size;
    BOOL res_read_file = ReadFile(file, file_data, static_cast<DWORD>(file_size), &read_size, NULL);

    BOOL res_close_handle = CloseHandle(file);
    assert(FALSE != res_close_handle);

    if (FALSE == res_read_file || read_size != file_size)
    {
        memory_free(memory_options->user_data, file_data);
        return cgltf_result_io_error;
    }

    if (NULL != size)
    {
        *size = file_size;
    }
    if (NULL != data)
    {
        *data = file_data;
    }

    return cgltf_result_success;
}

static void cgltf_custom_file_release(const struct cgltf_memory_options *memory_options, const struct cgltf_file_options *, void *data)
{
    void (*const memory_free)(void *, void *) = memory_options->free_func;
    assert(NULL != memory_free);

    memory_free(memory_options->user_data, data);
}

static void *cgltf_custom_alloc(void *, cgltf_size size)
{
    return malloc(size);
}

static void cgltf_custom_free(void *, void *ptr)
{
    free(ptr);
}
