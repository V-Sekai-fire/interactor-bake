// bake.elf's core: what a mobile avatar variant's texture side needs from
// the host's materials, host-neutral. Plain C++ with no sandbox API, so it
// runs in any godot-sandbox host or natively. RFD 2284's bake stage, CPU part.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace bk {

// Keep the triangles whose alpha, sampled at the three corners and the
// centroid with UV wrapping, averages at least threshold (0..255). uv holds
// u, v per vertex; alpha is width x height bytes, row 0 at v = 0.
std::vector<unsigned> alpha_cull(const std::vector<float> &uv, const std::vector<unsigned> &idx, const std::vector<uint8_t> &alpha, int width, int height, int threshold);

// Pack RGBA8 textures into one size x size atlas (shelf packing, halving
// every texture until they fit, 4 px padding with edges extended). rects
// holds x, y, w, h per texture in 0..1 atlas UV, row 0 at v = 0.
struct Atlas {
	std::vector<uint8_t> rgba;
	std::vector<float> rects;
	int scale_down = 0; // times every texture was halved to fit
};
Atlas atlas(const std::vector<uint8_t> &pixels, const std::vector<int32_t> &sizes, int size);

// Move each submesh's UVs into its texture's rect (tiled UVs wrap) and merge
// the submeshes into one. A vertex used by two submeshes is duplicated:
// picks[new] is the source vertex. Submesh s spans [ends[s-1], ends[s]).
struct AtlasMesh {
	std::vector<int32_t> picks;
	std::vector<float> uv;
	std::vector<unsigned> indices;
	size_t wrapped = 0;
};
AtlasMesh atlas_mesh(const std::vector<float> &uv, const std::vector<unsigned> &indices, const std::vector<int32_t> &ends, const std::vector<int32_t> &texture_of_submesh, const std::vector<float> &rects);

} // namespace bk
