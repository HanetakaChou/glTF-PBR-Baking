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
#include <d3d11.h>

struct Demo_Mesh_Subset
{
	uint32_t m_vertex_count;
	uint32_t m_index_count;
	ID3D11ShaderResourceView *m_vertex_varying_buffer_view;
	ID3D11ShaderResourceView *m_index_buffer_view;
	ID3D11ShaderResourceView *m_base_color_texture_view;
};

class Demo
{
	ID3D11SamplerState *m_sampler;

	std::vector<Demo_Mesh_Subset> m_scene_mesh_subsets;
	std::vector<ID3D11ShaderResourceView *> m_scene_textures;

	ID3D11Texture2D *m_baked_base_color_texture;
	ID3D11RenderTargetView *m_baked_base_color_rtv;
	ID3D11ShaderResourceView *m_baked_base_color_srv;

	ID3D11VertexShader *m_baking_vs;
	ID3D11RasterizerState *m_baking_rs;
	ID3D11PixelShader *m_baking_fs;

	ID3D11VertexShader *m_full_screen_vs;
	ID3D11RasterizerState *m_full_screen_rs;
	ID3D11PixelShader *m_full_screen_fs;

public:
	void Init(ID3D11Device *device, ID3D11DeviceContext *device_context);
	void Tick(ID3D11Device *device, ID3D11DeviceContext *device_context, ID3D11RenderTargetView *swap_chain_back_buffer_rtv, uint32_t swap_chain_image_width, uint32_t swap_chain_image_height);
	void Destroy(ID3D11Device *device, ID3D11DeviceContext *device_context);
};

#endif