// Bound UAV texture (rgba16f)
RWTexture2D<min16float4> image : register(u0);

// 16x16 thread group
[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID,
            uint3 GTid : SV_GroupThreadID,
            uint3 GID  : SV_GroupID) {
  uint2 size;
  image.GetDimensions(size.x, size.y);

   if (DTid.x < size.x && DTid.y < size.y) {
    float4 color = float4(0.0, 0.0, 0.0, 1.0);

    if (GTid.x != 0 && GTid.y != 0) {
      color.x = float(DTid.x) / size.x;
      color.y = float(DTid.y) / size.y;
    }
    image[DTid.xy] = min16float4(color);
  }
}
