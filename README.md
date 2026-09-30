# interactor-bake

Alpha culling and texture atlases for avatar meshes as a godot-sandbox guest, the CPU part of RFD 2284's bake stage.

`bake.elf` takes UVs, indices and RGBA8 pixels and returns arrays; it has no
filesystem, sockets or host objects. It runs unmodified in Godot and in the
sandbox host of any other game engine's editor. Positions never cross this
stage, so it is frame-free; the mesh stages exchange geometry in
`contract-guest-common`'s `mesh_wire` format.

| Function | Returns |
|---|---|
| `version()` | `bake.elf 1` |
| `alpha_cull(uvs, indices, alpha, width, height, threshold)` | the indices of triangles whose texture is not mostly transparent (corners and centroid averaged; tiled UVs wrap) |
| `atlas(pixels, sizes, size)` | `[rgba, rects, scale_down]`: RGBA8 textures shelf-packed into one atlas, halved until they fit, 4 px edge-extended padding |
| `atlas_mesh(uv, indices, ends, texture_of_submesh, rects)` | `[picks, uv, indices, wrapped]`: UVs moved into the atlas, submeshes merged, shared vertices split |
| `gate_alpha`, `gate_atlas` | each gate's verdict line |

Split out of `interactor-remesh` at `610e7eb` (one ELF per stage); the
GPU bake of RFD 2284 is not here yet.

## Gates

Measured on macOS arm64 through `sandbox_host.dylib`. Each planted defect
must fail.

| Gate | Clean | Planted |
|---|---|---|
| Alpha cull, a 32 x 32 plane transparent for u < 0.5 | PASS, 1024 of 2048 kept | opaque texture: FAIL, 2048 kept |
| Atlas, three solid textures in 64 px: rects apart, each colour at its centre | PASS | rect moved onto another: FAIL |

`tests/bake/build.sh` builds the guest and runs all four.

## Pins

| Dependency | Revision |
|---|---|
| `contract-guest-runtime` (`vendor/sandbox-api`) | `22cdad11236c28fadb30c856271e2efb8a774395` (tree `5ec3b43904`) |
