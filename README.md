# Toy fluid sim (using vulkan)

## `vk_enum_string_helper.h`

always pull to latest change: `https://github.com/KhronosGroup/Vulkan-Utility-Libraries`

read ts: https://github.com/KhronosGroup/Vulkan-Utility-Libraries/blob/main/docs/generated_code.md

run ts:
```bash
scripts/generate_source.py /usr/share/vulkan/registry/ --target vk_enum_string_helper.h
```
C+c, C+v ts file: `./include/vulkan/vk_enum_string_helper.h` to `/usr/include/vulkan/`

```bash
sudo true
mkdir -p /usr/local/include/vulkan/
cp -av ./include/vulkan/vk_enum_string_helper.h /usr/local/include/vulkan/vk_enum_string_helper.h
```

requirements:
- wayland, linux, x86  
- dxc (https://github.com/microsoft/DirectXShaderCompiler)  
- glslang (https://github.com/KhronosGroup/glslang)  
- cmake 4.1  
- ccache (optional)  


## Build

```bash
cmake -B build -G Ninja
ninja -C build build
ninja -C build run
ninja -C build clean-project
```

## Shader format
**GLSL:** `.vert.glsl`, `.frag.glsl`, `.comp.glsl`, `.geom.glsl`, `.tesc.glsl`, `.tese.glsl`

**HLSL:** `.vert.hlsl`, `.frag.hlsl`, `.comp.hlsl`, `.geom.hlsl`, `.tesc.hlsl`, `.tese.hlsl`
