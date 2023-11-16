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

#ifndef _DEMO_H_
#define _DEMO_H_ 1

#include <sdkddkver.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>

#include <vector>

#include <dxgi.h>
#include <d3d11_3.h>

struct Demo_Mesh_Subset
{
    uint32_t m_vertex_count;
    uint32_t m_index_count;
    ID3D11ShaderResourceView *m_vertex_varying_buffer_view;
    ID3D11ShaderResourceView *m_index_buffer_view;
    ID3D11ShaderResourceView* m_normal_texture_view;
    ID3D11ShaderResourceView *m_base_color_texture_view;
    ID3D11ShaderResourceView* m_metallic_roughness_texture_view;
};

class Demo
{
    ID3D11SamplerState *m_sampler;

    std::vector<Demo_Mesh_Subset> m_scene_mesh_subsets;
    std::vector<ID3D11ShaderResourceView *> m_scene_textures;

    ID3D11Texture2D* m_baked_normal_texture;
    ID3D11RenderTargetView* m_baked_normal_rtv;
    ID3D11ShaderResourceView* m_baked_normal_srv;

    ID3D11Texture2D *m_baked_base_color_texture;
    ID3D11RenderTargetView *m_baked_base_color_rtv;
    ID3D11ShaderResourceView *m_baked_base_color_srv;

    ID3D11Texture2D* m_baked_metallic_roughness_texture;
    ID3D11RenderTargetView* m_baked_metallic_roughness_rtv;
    ID3D11ShaderResourceView* m_baked_metallic_roughness_srv;

    ID3D11VertexShader *m_baking_vs;
    ID3D11RasterizerState2 *m_baking_rs;
    ID3D11PixelShader *m_baking_ps;

    ID3D11VertexShader *m_full_screen_vs;
    ID3D11RasterizerState *m_full_screen_rs;
    ID3D11PixelShader *m_full_screen_clear_ps;
    ID3D11PixelShader *m_full_screen_transfer_ps;

public:
    void Init(ID3D11Device3 *device, ID3D11DeviceContext *device_context);
    void Tick(ID3D11Device3 *device, ID3D11DeviceContext *device_context, ID3D11RenderTargetView *swap_chain_back_buffer_rtv, uint32_t swap_chain_image_width, uint32_t swap_chain_image_height);
    void Destroy(ID3D11Device3 *device, ID3D11DeviceContext *device_context);
};

#endif