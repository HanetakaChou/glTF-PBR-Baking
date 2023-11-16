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

Texture2D g_normal_texture : register(t0);
Texture2D g_base_color_texture : register(t1);
Texture2D g_metallic_roughness_texture : register(t2);
SamplerState g_sampler : register(s0);

void main(
    in float4 in_position : SV_POSITION,
    in float2 in_uv : TEXCOORD0,
    out float4 out_normal_texture : SV_TARGET0,
    out float4 out_base_color_texture : SV_TARGET1,
    out float4 out_metallic_roughness_texture : SV_TARGET2)
{
    float4 normal = g_normal_texture.SampleLevel(g_sampler, in_uv, 0.0);
    float4 base_color = g_base_color_texture.SampleLevel(g_sampler, in_uv, 0.0);
    float4 metallic_roughness = g_metallic_roughness_texture.SampleLevel(g_sampler, in_uv, 0.0);

    out_normal_texture = normal;
    out_base_color_texture = base_color;
    out_metallic_roughness_texture = metallic_roughness;
}