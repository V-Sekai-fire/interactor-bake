// bake.elf: RFD 2284's bake stage, the CPU part: triangles a cutout or
// blended material would hide, and one atlas per mesh. The guest reads only
// its arguments and returns arrays; it has no file, socket or host-object access.
#include <api.hpp>
#include <cstdio>
#include <string>
#include <vector>

#include "bake.h"

static std::vector<unsigned> indices_of(const PackedInt32Array &a) {
	std::vector<unsigned> out;
	for (int32_t i : a.fetch()) out.push_back(unsigned(i));
	return out;
}

static PackedInt32Array packed(const std::vector<unsigned> &ix) {
	return PackedInt32Array(std::vector<int32_t>(ix.begin(), ix.end()));
}

static Variant version() { return Variant(String("bake.elf 1")); }

// The kept indices. uvs as u, v pairs; alpha holds width * height bytes, row 0 at v = 0.
static Variant alpha_cull(PackedFloat32Array uvs, PackedInt32Array indices, PackedByteArray alpha, int64_t width, int64_t height, int64_t threshold) {
	return Variant(packed(bk::alpha_cull(uvs.fetch(), indices_of(indices), alpha.fetch(), int(width), int(height), int(threshold))));
}

// RGBA8 textures back to back, sizes as w, h pairs. [rgba, rects, scale_down]
static Variant atlas(PackedByteArray pixels, PackedInt32Array sizes, int64_t size) {
	const bk::Atlas a = bk::atlas(pixels.fetch(), sizes.fetch(), int(size));
	return Variant(Array::make(Variant(PackedArray<uint8_t>(a.rgba)), Variant(PackedFloat32Array(a.rects)), Variant(int64_t(a.scale_down))));
}

// [picks, uv, indices, wrapped]
static Variant atlas_mesh(PackedFloat32Array uv, PackedInt32Array indices, PackedInt32Array ends, PackedInt32Array texture_of_submesh, PackedFloat32Array rects) {
	const bk::AtlasMesh m = bk::atlas_mesh(uv.fetch(), indices_of(indices), ends.fetch(), texture_of_submesh.fetch(), rects.fetch());
	return Variant(Array::make(Variant(PackedInt32Array(m.picks)), Variant(PackedFloat32Array(m.uv)), Variant(packed(m.indices)), Variant(int64_t(m.wrapped))));
}

// Alpha gate: a 32 x 32 unit plane whose texture is transparent for u < 0.5
// must keep exactly half its triangles. planted != 0 makes the texture opaque.
static Variant gate_alpha(int64_t planted) {
	const int n = 32;
	std::vector<float> uv;
	std::vector<unsigned> ix;
	for (int j = 0; j <= n; ++j)
		for (int i = 0; i <= n; ++i) { uv.push_back(float(i) / n); uv.push_back(float(j) / n); }
	for (int j = 0; j < n; ++j)
		for (int i = 0; i < n; ++i) {
			const unsigned a = j * (n + 1) + i, b = a + n + 1;
			ix.insert(ix.end(), { a, b, a + 1, a + 1, b, b + 1 });
		}
	const int w = 64, h = 64;
	std::vector<uint8_t> alpha(w * h);
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < w; ++x) alpha[y * w + x] = (planted || x >= w / 2) ? 255 : 0;
	const size_t kept = bk::alpha_cull(uv, ix, alpha, w, h, 128).size() / 3, want = size_t(n * n);
	char line[160];
	std::snprintf(line, sizeof line, "%s alpha kept %zu of %zu triangles; want %zu; planted %lld", kept == want ? "PASS" : "FAIL", kept, ix.size() / 3, want, (long long)planted);
	return Variant(String(line));
}

// Atlas gate: three solid textures must land apart, each with its own colour
// at its rect's centre. planted != 0 moves rect 1 onto rect 0.
static Variant gate_atlas(int64_t planted) {
	const int sz[3][2] = { { 32, 32 }, { 16, 48 }, { 24, 8 } };
	const uint8_t col[3][3] = { { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 } };
	std::vector<uint8_t> px;
	std::vector<int32_t> sizes;
	for (int i = 0; i < 3; ++i) {
		sizes.push_back(sz[i][0]); sizes.push_back(sz[i][1]);
		for (int k = 0; k < sz[i][0] * sz[i][1]; ++k) px.insert(px.end(), { col[i][0], col[i][1], col[i][2], 255 });
	}
	bk::Atlas a = bk::atlas(px, sizes, 64);
	if (planted) { a.rects[4] = a.rects[0]; a.rects[5] = a.rects[1]; }
	bool apart = true, colours = true;
	for (int i = 0; i < 3; ++i) {
		const float *r = &a.rects[i * 4];
		for (int j = 0; j < i; ++j) {
			const float *q = &a.rects[j * 4];
			if (r[0] < q[0] + q[2] && q[0] < r[0] + r[2] && r[1] < q[1] + q[3] && q[1] < r[1] + r[3]) apart = false;
		}
		const int cx = int((r[0] + r[2] / 2) * 64), cy = int((r[1] + r[3] / 2) * 64);
		const uint8_t *p = &a.rgba[(size_t(cy) * 64 + cx) * 4];
		if (p[0] != col[i][0] || p[1] != col[i][1] || p[2] != col[i][2]) colours = false;
	}
	char line[200];
	std::snprintf(line, sizeof line, "%s atlas 3 textures in 64 px, halved %d times, rects apart %d, colours at centres %d; planted %lld", apart && colours ? "PASS" : "FAIL", a.scale_down, apart, colours, (long long)planted);
	return Variant(String(line));
}

int main() {
	ADD_API_FUNCTION(version, "String", "", "bake.elf version");
	ADD_API_FUNCTION(alpha_cull, "PackedInt32Array", "PackedFloat32Array uvs, PackedInt32Array indices, PackedByteArray alpha, int width, int height, int threshold", "drop mostly transparent triangles; the kept indices");
	ADD_API_FUNCTION(atlas, "Array", "PackedByteArray pixels, PackedInt32Array sizes, int size", "[rgba, rects, scale_down]");
	ADD_API_FUNCTION(atlas_mesh, "Array", "PackedFloat32Array uv, PackedInt32Array indices, PackedInt32Array ends, PackedInt32Array texture_of_submesh, PackedFloat32Array rects", "[picks, uv, indices, wrapped]");
	ADD_API_FUNCTION(gate_alpha, "String", "int planted", "alpha gate; planted != 0 must FAIL");
	ADD_API_FUNCTION(gate_atlas, "String", "int planted", "atlas gate; planted != 0 must FAIL");
	halt();
}
