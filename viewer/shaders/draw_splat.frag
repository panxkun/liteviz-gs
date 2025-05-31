#version 430 core

in vec3 color;
in float alpha;
in vec3 conic;
in vec2 coordxy;  // local coordinate in quad, unit in pixel

uniform int render_mod;  // > 0 render 0-ith SH dim, -1 depth, -2 bill board, -3 flat ball, -4 gaussian ball

out vec4 FragColor;

void main(){

    // render_mod == 0
    float power = -0.5f * (conic.x * coordxy.x * coordxy.x + conic.z * coordxy.y * coordxy.y) - conic.y * coordxy.x * coordxy.y;
    if (power > 0.f)
        discard;
    float opacity = min(0.99f, alpha * exp(power));
    if (opacity < 1.f / 255.f)
        discard;
    FragColor = vec4(color, opacity);

    if (render_mod == 5){
        FragColor.a = FragColor.a > 0.22 ? 1 : 0;
        FragColor.rgb = FragColor.rgb * exp(power);
    }
}
