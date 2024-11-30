#version 460

layout(location = 0) in float data_x;
layout(location = 1) in float data_y;

uniform mat4 projectionMatrix;

void main() {
    gl_Position = projectionMatrix * vec4(data_x, data_y, 0.0, 1.0);
}

// #version 460

// layout(location = 0) in float data_x;
// layout(location = 1) in float data_y;

// out float frag_data_x; 
// out float orig_data_x;

// uniform float range_min;
// uniform float range_max;

// void main() {
//     // Map data_x to Normalized Device Coordinates (NDC)
//     float ndc_x = 2.0 * (data_x - range_min) / (range_max - range_min) - 1.0;
//     float ndc_y = data_y; // Assuming data_y is already in NDC (0.0)

//     gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);
//     frag_data_x = data_x; // Pass the data_x to the fragment shader
//     orig_data_x = data_x; // Pass the original data_x to the fragment shader
    
// }
