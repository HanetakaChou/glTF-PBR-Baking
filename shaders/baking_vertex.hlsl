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

#define g_vertex_varying_buffer_stride 16u
#define g_index_uint32_buffer_stride 4u

ByteAddressBuffer g_mesh_subset_vertex_varying_buffer : register(t0);
ByteAddressBuffer g_mesh_subset_index_buffer : register(t1);

void main(
    in uint in_vertex_id : SV_VERTEXID,
    out float4 out_position : SV_POSITION,
    out float2 out_uv : TEXCOORD0)
{
    uint vertex_index;
    {
        const uint index_buffer_offset = g_index_uint32_buffer_stride * in_vertex_id;
        vertex_index = g_mesh_subset_index_buffer.Load(index_buffer_offset);
    }

    float2 vertex_texcoord_0;
    float2 vertex_texcoord_1;
    {
        const uint vertex_varying_buffer_offset = g_vertex_varying_buffer_stride * vertex_index;
        const uint4 packed_vector_vertex_varying_binding = g_mesh_subset_vertex_varying_buffer.Load4(vertex_varying_buffer_offset);
        vertex_texcoord_0 = asfloat(packed_vector_vertex_varying_binding.xy);
        vertex_texcoord_1 = asfloat(packed_vector_vertex_varying_binding.zw);
    }

    float4 surface_position_ndc_space = float4(vertex_texcoord_1 * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.5, 1.0);

    out_position = surface_position_ndc_space;
    out_uv = vertex_texcoord_0;
}