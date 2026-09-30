#include "bake.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

namespace bk {

std::vector<unsigned> alpha_cull(const std::vector<float> &uv, const std::vector<unsigned> &idx, const std::vector<uint8_t> &alpha, int width, int height, int threshold) {
	auto sample = [&](float u, float v) {
		// Wrap tiled UVs, but keep 1.0 on the last texel rather than the first.
		if (u < 0.0f || u > 1.0f) u -= std::floor(u);
		if (v < 0.0f || v > 1.0f) v -= std::floor(v);
		const int x = std::min(width - 1, int(u * width)), y = std::min(height - 1, int(v * height));
		return int(alpha[size_t(y) * width + x]);
	};
	std::vector<unsigned> kept;
	for (size_t t = 0; t + 2 < idx.size(); t += 3) {
		float cu = 0, cv = 0;
		int sum = 0;
		for (int c = 0; c < 3; ++c) {
			const float u = uv[idx[t + c] * 2], v = uv[idx[t + c] * 2 + 1];
			sum += sample(u, v);
			cu += u / 3; cv += v / 3;
		}
		sum += sample(cu, cv);
		if (sum >= threshold * 4) kept.insert(kept.end(), { idx[t], idx[t + 1], idx[t + 2] });
	}
	return kept;
}

Atlas atlas(const std::vector<uint8_t> &pixels, const std::vector<int32_t> &sizes, int size) {
	const size_t n = sizes.size() / 2;
	std::vector<size_t> offset(n, 0);
	for (size_t i = 1; i < n; ++i) offset[i] = offset[i - 1] + size_t(sizes[(i - 1) * 2]) * sizes[(i - 1) * 2 + 1] * 4;
	const int pad = 4;
	Atlas out;
	std::vector<int> x(n), y(n), w(n), h(n);
	// Shelf packing, tallest first; halve everything until it fits.
	for (out.scale_down = 0;; ++out.scale_down) {
		for (size_t i = 0; i < n; ++i) { w[i] = std::max(1, sizes[i * 2] >> out.scale_down); h[i] = std::max(1, sizes[i * 2 + 1] >> out.scale_down); }
		std::vector<size_t> order(n);
		std::iota(order.begin(), order.end(), 0);
		std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return h[a] > h[b]; });
		int cx = 0, cy = 0, shelf = 0;
		bool fits = true;
		for (size_t i : order) {
			if (cx + w[i] + pad > size) { cx = 0; cy += shelf; shelf = 0; }
			if (w[i] + pad > size || cy + h[i] + pad > size) { fits = false; break; }
			x[i] = cx + pad / 2; y[i] = cy + pad / 2;
			cx += w[i] + pad;
			shelf = std::max(shelf, h[i] + pad);
		}
		if (fits) break;
	}
	out.rgba.assign(size_t(size) * size * 4, 0);
	for (size_t i = 0; i < n; ++i) {
		const int sw = sizes[i * 2], sh = sizes[i * 2 + 1];
		// Nearest-texel downscale, with the padding filled by the edge texels.
		for (int yy = -pad / 2; yy < h[i] + pad / 2; ++yy)
			for (int xx = -pad / 2; xx < w[i] + pad / 2; ++xx) {
				const int tx = x[i] + xx, ty = y[i] + yy;
				if (tx < 0 || ty < 0 || tx >= size || ty >= size) continue;
				const int cxs = std::min(w[i] - 1, std::max(0, xx)), cys = std::min(h[i] - 1, std::max(0, yy));
				const int sx = std::min(sw - 1, cxs * sw / w[i]), sy = std::min(sh - 1, cys * sh / h[i]);
				const uint8_t *src = &pixels[offset[i] + (size_t(sy) * sw + sx) * 4];
				uint8_t *dst = &out.rgba[(size_t(ty) * size + tx) * 4];
				dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = 255;
			}
		out.rects.insert(out.rects.end(), { float(x[i]) / size, float(y[i]) / size, float(w[i]) / size, float(h[i]) / size });
	}
	return out;
}

AtlasMesh atlas_mesh(const std::vector<float> &uv, const std::vector<unsigned> &indices, const std::vector<int32_t> &ends, const std::vector<int32_t> &texture_of_submesh, const std::vector<float> &rects) {
	AtlasMesh out;
	std::map<std::pair<size_t, unsigned>, unsigned> seen;
	for (size_t s = 0, start = 0; s < ends.size(); start = size_t(ends[s]), ++s) {
		const float *r = &rects[size_t(std::max(0, texture_of_submesh[s])) * 4];
		for (size_t k = start; k < size_t(ends[s]); ++k) {
			const unsigned v = indices[k];
			auto it = seen.find({ s, v });
			if (it == seen.end()) {
				float u = uv[v * 2], w = uv[v * 2 + 1];
				if (u < 0 || u > 1 || w < 0 || w > 1) ++out.wrapped;
				if (u < 0 || u > 1) u -= std::floor(u);
				if (w < 0 || w > 1) w -= std::floor(w);
				it = seen.insert({ { s, v }, unsigned(out.picks.size()) }).first;
				out.picks.push_back(int32_t(v));
				out.uv.push_back(r[0] + u * r[2]);
				out.uv.push_back(r[1] + w * r[3]);
			}
			out.indices.push_back(it->second);
		}
	}
	return out;
}

} // namespace bk
