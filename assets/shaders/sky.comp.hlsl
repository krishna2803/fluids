// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

[[vk::push_constant]] struct PushConstants {
  float4 data1;
  float4 data2;
  float4 data3;
  float4 data4;
} pc;

// [[vk::image_format("rgba16f")]]
RWTexture2D<min16float4> image : register(u0);

// return random noise in the range [0.0, 1.0], as a function of (x, y).
float noise2d(float2 v) {
  float xhash = cos(v.x * 37.0);
  float yhash = cos(v.y * 57.0);
  return frac(415.92653 * (xhash + yhash));
}

// convert noise2d(...) into a "star field" by stomping everthing below
// threshold to zero.
float star_field_noisy(float2 pos, float threshold) {
  float x = noise2d(pos);
  return x > threshold ? pow((x - threshold) / (1.0 - threshold), 6.0) : 0;
}

// stabilize star_field_noisy(...) by only sampling at integer values.
float star_field_stable(float2 pos, float threshold) {

  // linear interpolation between four samples.
  // note: This approach has some visual artifacts.
  // there must be a better way to "anti alias" the star field.
  float fractX = frac(pos.x);
  float fractY = frac(pos.y);
  float2 floor_sample = floor(pos);

  float v0 = star_field_noisy(floor_sample, threshold);
  float v1 = star_field_noisy(floor_sample + float2(0.0,1.0), threshold);
  float v2 = star_field_noisy(floor_sample + float2(1.0,0.0), threshold);
  float v3 = star_field_noisy(floor_sample + float2(1.0,1.0), threshold);

  return  v0 * (1.0 - fractX) * (1.0 - fractY)
        + v1 * (1.0 - fractX) * fractY
        + v2 * fractX * (1.0 - fractY)
        + v3 * fractX * fractY;
}


[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID,
          uint3 GTid : SV_GroupThreadID,
          uint3 GID  : SV_GroupID) {
  uint2 size;
  image.GetDimensions(size.x, size.y);

  if (DTid.x < size.x && DTid.y < size.y) {
    // sky background color
    // (0.1, 0.2, 0.4)
    float3 color = pc.data1.xyz * float(DTid.y) / float(size.y);

    // note: choose threshold in the range [0.99, 0.9999].
    float threshold = pc.data1.w; // 0.97;

    // stars with a slow crawl.
    float xrate = 0.2;
    float yrate = -0.06;
    float2 pos = DTid.xy + float2(xrate * 1.0, yrate * 1.0);
    float star_value = star_field_stable(pos, threshold);
    color += float3(star_value, star_value, star_value);

    image[DTid.xy] = min16float4(color, 1.0);
  }
}
