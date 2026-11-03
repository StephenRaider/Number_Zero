#pragma once
#include <array>
namespace highway_assets {
struct PropDefinition {const char *name;int canvas;float anchorX,anchorY,depthRange;};
inline constexpr std::array<PropDefinition,5> Props={{
{"lamp_right",256,128.00000000f,232.00000000f,32.00000000f},
{"lamp_left",256,128.00000000f,232.00000000f,32.00000000f},
{"rail_right",128,64.00000000f,104.00000000f,32.00000000f},
{"rail_left",128,64.00000000f,104.00000000f,32.00000000f},
{"gantry",512,256.00000000f,400.00000000f,32.00000000f},
}};
struct LampHead {float forward,lateral,height;};
inline constexpr LampHead RightHead={-0.00000240f,-3.72792387f,7.13879681f};
inline constexpr LampHead LeftHead={0.00000251f,3.72792387f,7.13879681f};
inline constexpr float RoadRepeat=27.f,RailRepeat=4.f,MountLateral=8.3f;
}
