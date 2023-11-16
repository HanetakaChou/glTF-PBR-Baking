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

Texture2D g_base_color_texture : register(t0);
SamplerState g_sampler : register(s0);

void main(
	in float4 in_position : SV_POSITION,
	in float2 in_uv : TEXCOORD0,
	out float4 out_base_color : SV_TARGET0)
{
	float4 base_color_and_alpha = g_base_color_texture.SampleLevel(g_sampler, in_uv, 0.0);

	out_base_color = float4(base_color_and_alpha.rgb * base_color_and_alpha.a, 1.0);
}