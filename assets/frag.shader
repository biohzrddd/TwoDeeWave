#version 400

in vec4 position;
out vec4 out_color;

void main() {
    float lerpValue = gl_FragCoord.y / 480.0f;
    
    out_color = mix(vec4(1.0f, 0.0f, 0.0f, 1.0f),
					vec4(0.0f, 1.0f, 0.0f, 1.0f), lerpValue);
}