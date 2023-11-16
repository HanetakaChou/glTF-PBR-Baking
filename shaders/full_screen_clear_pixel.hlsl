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

Texture2D g_base_color : register(t0);
SamplerState g_sampler : register(s0);

void main(
    in float4 in_position : SV_POSITION,
    in float2 in_uv : TEXCOORD0,
    out float4 out_normal_texture : SV_TARGET0,
    out float4 out_base_color : SV_TARGET1,
    out float4 out_metallic_roughness : SV_TARGET2)
{
    [branch]
    if (in_uv.x < 0.5 && in_uv.y < 0.5)
    {
        out_normal_texture = float4(0.5, 0.5, 1.0, 1.0);
        out_base_color = float4(0.98438, 0.85547, 0.76953, 1.0);
        out_metallic_roughness = float4(0.0, 0.0, 0.0, 1.0);
    }
    else if (in_uv.y < 0.5)
    {
        out_normal_texture = float4(0.5, 0.5, 1.0, 1.0);
        out_base_color = float4(0.28711, 0.19434, 0.31836, 1.0);
        out_metallic_roughness = float4(0.0, 0.0, 0.0, 1.0);
    }
    else
    {
        out_normal_texture = float4(0.5, 0.5, 1.0, 1.0);
        out_base_color = float4(0.0, 0.0, 0.0, 1.0);
        out_metallic_roughness = float4(0.0, 0.0, 0.0, 1.0);
    }
}