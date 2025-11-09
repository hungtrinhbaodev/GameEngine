#version 450

layout (binding = 0) uniform graphic_uniform {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

vec3 positions[3] = vec3[](
    vec3(0.0, -0.5, 0.0),
    vec3(0.5, 0.5, 0.0),
    vec3(-0.5, 0.5, 0.0)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;

layout (location = 3) in vec4 inModelMatrixCol0;
layout (location = 4) in vec4 inModelMatrixCol1;
layout (location = 5) in vec4 inModelMatrixCol2;
layout (location = 6) in vec4 inModelMatrixCol3;

layout (location = 0) out vec3 fragColor;

void main(){
    vec3 inPosition2 = positions[gl_VertexIndex];
    vec3 inColor2 = vec3(colors[gl_VertexIndex]);
    mat4 model = mat4(inModelMatrixCol0, inModelMatrixCol1, inModelMatrixCol2, inModelMatrixCol3);
    gl_Position =  model * vec4(inPosition2, 1.0);
    fragColor = inColor2;
}