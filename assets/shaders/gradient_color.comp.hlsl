[[vk::push_constant]] struct PushConstants {
  float4 data1; // top_color
  float4 data2; // bottom_color
  float4 data3;
  float4 data4;
} pc;

RWTexture2D<min16float4> image : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID,
          uint3 GTid : SV_GroupThreadID,
          uint3 GID  : SV_GroupID) {
  uint2 size;
  image.GetDimensions(size.x, size.y);
    
  float4 top_color = pc.data1;
  float4 bottom_color = pc.data2;

  if (DTid.x < size.x && DTid.y < size.y) {
    float blend = float(DTid.y) / float(size.y);
    float4 color = lerp(top_color, bottom_color, blend);
    image[DTid.xy] = min16float4(color);
  }
}
