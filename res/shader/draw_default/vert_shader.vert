#version 450

layout (binding = 0) uniform graphic_uniform {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;

layout (location = 3) in vec4 inModelMatrixCol0;
layout (location = 4) in vec4 inModelMatrixCol1;
layout (location = 5) in vec4 inModelMatrixCol2;
layout (location = 6) in vec4 inModelMatrixCol3;

layout (location = 0) out vec3 fragColor;

void main(){
    mat4 model = mat4(inModelMatrixCol0, inModelMatrixCol1, inModelMatrixCol2, inModelMatrixCol3);
    gl_Position =  ubo.projection * ubo.view * model * vec4(inPosition, 1.0);
    fragColor = inColor;
}