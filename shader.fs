#version 460

out vec4 FragColor;

uniform isamplerBuffer dataTextureBuffer;
uniform float range_min;
uniform float range_max;
uniform int textureSize;
uniform int viewportWidth;

// Declare the SSBO to store results
layout(std430, binding = 0) buffer MySSBO {
    int data[];
};

// Declare the atomic counter
layout(binding = 1, offset = 0) uniform atomic_uint atomicCounter;

void main() {
    // Reconstruct orig_data_x from gl_FragCoord.x
    float x_coord = gl_FragCoord.x;
    float y_coord = gl_FragCoord.y;

    x_coord = int(x_coord - fract(x_coord));
    y_coord = int(y_coord - fract(y_coord));

    int orig_data_x = int(x_coord - fract(x_coord)); 
    orig_data_x += int(y_coord - fract(y_coord)) * viewportWidth;
    
    
    // FragColor = vec4(0.0, fract(gl_FragCoord.y-0.5), 0.0, 1.0); // For visualization
    // return;
    // Map orig_data_x to buffer coordinate index
    int index = orig_data_x;
    // Ensure bufferCoord is in range
    if (index < 0.0 || index >= textureSize) {
        FragColor = vec4(0.0, 1.0, 1.0, 1.0); // For visualization
        return;
    }
  
    // if(fract(bufferCoord) <= 0.5) {
        // FragColor = vec4(0.0, fract(bufferCoord), 0.0, 1.0); // For visualization
        // return;
    // }


    // Sample the texture buffer
    int rowIdentifier = texelFetch(dataTextureBuffer, index).r;

    if (rowIdentifier == -1) {
        discard; // No data point at this position
    } else {
        // Output the fragment and use the rowIdentifier for further processing
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // For visualization

        // Atomically increment the counter and get a unique index
        uint dataIndex = atomicCounterIncrement(atomicCounter);
        
        // Write the rowIdentifier into the SSBO at the unique index
        data[dataIndex] = rowIdentifier;
    }
}

// #version 460

// in float frag_data_x;
// in float orig_data_x;

// out vec4 FragColor;

// uniform isamplerBuffer dataTextureBuffer;
// uniform float range_min;
// uniform float range_max;
// uniform int textureSize;

// // Declare the SSBO to store results
// layout(std430, binding = 0) buffer MySSBO {
//     int data[];
// };

// // Declare the atomic counter
// layout(binding = 1, offset = 0) uniform atomic_uint atomicCounter;

// void main() {
//     // Map frag_data_x to texture coordinate index
//     // Map frag_data_x to buffer coordinate index
//     if(fract(frag_data_x) >= 0.5) {
//         FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Render green pixels where data points don't exist
//         return;
//     }

//     float bufferCoord = (frag_data_x - range_min) / (range_max - range_min) * float(textureSize - 1);

//     // Ensure bufferCoord is in range
//     if (bufferCoord < 0.0 || bufferCoord >= float(textureSize)) {
//         discard;
//     }

//     // TODO: 
//     if (fract(bufferCoord) >= 0.05) {
//         FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Render green pixels where data points don't exist
//         return;
//     }
//     // Round to the nearest integer index
//     int index = int(bufferCoord + 0.5);

//     // Sample the texture buffer
//     int rowIdentifier = texelFetch(dataTextureBuffer, index).r;

//     if (rowIdentifier == -1) {
//         FragColor = vec4(0.0, 0.0, 1.0, 1.0); // Render blue pixels where data points don't exist        
//     } else {
//         FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Render red pixels where data points exist

//         // Atomically increment the counter and get a unique index
//         uint index = atomicCounterIncrement(atomicCounter);

//         // Write the rowIdentifier into the SSBO at the unique index
//         data[index] = rowIdentifier;

//         if (index >= 40) {
//             FragColor = vec4(0.0, 1.0, 1.0, 1.0); // Render green pixels where data points don't exist
//         }
//     }
// }
