# interactor-bake

Alpha culling and texture atlases for avatar meshes as a `godot-sandbox` guest, the CPU part of RFD 2284's bake stage.

## Use

`bake.elf` takes UVs, indices and RGBA8 pixels and returns arrays. It drops triangles whose texture is mostly transparent, packs a mesh's textures into one atlas, and moves the UVs into it. It has no filesystem, sockets or host objects, so it runs unmodified in any sandbox host. [RFD 2284](https://github.com/V-Sekai-fire/manuals-weftspun/tree/main/rfd/2284-a-quest-variant-by-sandbox-guests) owns the bake stage, and `bake.h` declares the functions.

## Build and run

```sh
tests/bake/build.sh
```

It builds the guest and, given a sandbox host, runs each gate against a clean input and a planted defect that must fail. The script's header names the toolchain and checkouts it reads.

## Licence

MIT. See `LICENSE`.
