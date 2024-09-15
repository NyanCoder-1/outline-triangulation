#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

//void main() {
	// outColor = vec4(fragTexCoord, 1.0, 1.0);
	// quadratic bezier
	// if (fragTexCoord.x*fragTexCoord.x - fragTexCoord.y > 0.0)
	// 	outColor = vec4(fragColor, 1.0);
	// else
	// 	outColor = vec4(0.0, 0.0, 0.0, 0.0);
void main() {
    // Gradients
    vec2 px = dFdx(fragTexCoord);
    vec2 py = dFdy(fragTexCoord);

    // Chain rule
    float fx = (2.0 * fragTexCoord.x) * px.x - px.y;
    float fy = (2.0 * fragTexCoord.x) * py.x - py.y;

    // Signed distance  

    float sd = (fragTexCoord.x * fragTexCoord.x - fragTexCoord.y) / sqrt(fx * fx + fy * fy);  


    // Linear alpha
    float alpha = 0.5 - sd;

    if (alpha > 1.0) {
        // Inside
        outColor.a = 1.0;
    } else if (alpha < 0.0) {
        // Outside
        discard; // Equivalent to clip(-1) in HLSL
    } else {
        // Near boundary
        outColor.a = alpha;
    }
    // Multiply alpha by input color alpha
    outColor.a *= fragColor.a;

    outColor.rgb = fragColor.rgb; // Assuming color is intended for RGB
}
