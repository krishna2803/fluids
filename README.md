# Toy fluid sim (using vulkan)

`vk_enum_string_helper.h`

always pull to latest change: `https://github.com/KhronosGroup/Vulkan-Utility-Libraries`

read ts: https://github.com/KhronosGroup/Vulkan-Utility-Libraries/blob/main/docs/generated_code.md

run ts: `scripts/generate_source.py /usr/share/vulkan/registry/ --target vk_enum_string_helper.h`
C+c, C+v ts file: `./include/vulkan/vk_enum_string_helper.h` to `/usr/include/vulkan/`

```
$ doas mkdir -p /usr/local/include/vulkan/
$ doas cp -av ./include/vulkan/vk_enum_string_helper.h /usr/local/include/vulkan/vk_enum_string_helper.h
```
