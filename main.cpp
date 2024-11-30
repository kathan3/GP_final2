#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>  // For std::atoi
#include <chrono>   // For high-resolution timing
#include <algorithm> // For std::min and std::max
#include <set>
#include <cassert>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Vertex {
    int indexValue;    // First column (index)
    int rowIdentifier; // Row identifier (sequential)
};

void checkGLError(const char* functionName) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error after " << functionName << ": ";
        switch (error) {
            case GL_INVALID_ENUM:
                std::cerr << "GL_INVALID_ENUM\n";
                break;
            case GL_INVALID_VALUE:
                std::cerr << "GL_INVALID_VALUE\n";
                break;
            case GL_INVALID_OPERATION:
                std::cerr << "GL_INVALID_OPERATION\n";
                break;
            case GL_STACK_OVERFLOW:
                std::cerr << "GL_STACK_OVERFLOW\n";
                break;
            case GL_STACK_UNDERFLOW:
                std::cerr << "GL_STACK_UNDERFLOW\n";
                break;
            case GL_OUT_OF_MEMORY:
                std::cerr << "GL_OUT_OF_MEMORY\n";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION\n";
                break;
            default:
                std::cerr << "Unknown error\n";
                break;
        }
    }
}

// Function to load CSV data and create vertices
std::vector<Vertex> loadTable(const char* filename) {
    std::vector<Vertex> vertices;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return vertices;
    }

    std::string line;
    int row = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;

        if (std::getline(ss, cell, '|')) {
            Vertex vertex;
            vertex.indexValue = std::stoi(cell);
            vertex.rowIdentifier = row;  // Assign row identifier sequentially
            vertices.push_back(vertex);
        }
        row++;
    }
    file.close();
    return vertices;
}

// Function to load shader code from a file
std::string loadShaderCode(const char* filePath) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }
    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    return shaderStream.str();
}

// Utility function to compile shader program with error checking
GLuint compileShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Create shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    // Set shader sources
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);

    GLint success;
    GLchar infoLog[512];

    // Compile vertex shader
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Error compiling vertex shader:\n" << infoLog << std::endl;
        return 0;
    }

    // Compile fragment shader
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Error compiling fragment shader:\n" << infoLog << std::endl;
        return 0;
    }

    // Link shaders into a program
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    // Check for linking errors
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        std::cerr << "Error linking shader program:\n" << infoLog << std::endl;
        return 0;
    }

    // Clean up shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return programID;
}

void APIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                              GLsizei length, const GLchar* message, const void* userParam) {
    std::cerr << "GL CALLBACK: " << message << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <table_file> <query_x1> <query_x2>" << std::endl;
        return -1;
    }

    const char* tableFile = argv[1];
    std::vector<Vertex> vertices = loadTable(tableFile);
    if (vertices.empty()) {
        return -1;
    }

    // Find range_min and range_max
    int range_min = vertices[0].indexValue;
    int range_max = vertices[0].indexValue;

    for (const auto& vertex : vertices) {
        range_min = std::min(range_min, vertex.indexValue);
        range_max = std::max(range_max, vertex.indexValue);
    }
    // so that range_min and range_max are not out of the bounds. 
    range_min -= 1;
    range_max += 1;

    std::cout << "Range: [" << range_min << ", " << range_max << "]" << std::endl;
    int textureSize = range_max - range_min + 1;

    // Create textureData vector initialized to -1
    std::vector<int> textureData(textureSize, -1);
    for (const auto& vertex : vertices) {
        int index = vertex.indexValue;
        textureData[index] = vertex.rowIdentifier;
    }

    // Initialize OpenGL context (using GLFW)
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed" << std::endl;
        return -1;
    }

    // Request OpenGL version 4.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "OpenGL Line-Point Intersection", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW after context creation
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    // Enable OpenGL debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);


    GLint maxBufferTextureSize = 0; 
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxBufferTextureSize);
    std::cout << "Maximum texture buffer size: " << maxBufferTextureSize << std::endl;

    // Create and bind a 1D texture as TBO.
    // Generate and bind a buffer object for the texture buffer
    GLuint tbo;
    glGenBuffers(1, &tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo);

    // Upload your data to the buffer
    glBufferData(GL_TEXTURE_BUFFER, textureSize * sizeof(int), textureData.data(), GL_STATIC_DRAW);

    // Generate the texture buffer object
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_BUFFER, textureID);

    // Associate the buffer with the texture buffer
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, tbo);

    GLint maxRenderbufferSize;
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    std::cout << "Maximum renderbuffer size: " << maxRenderbufferSize << std::endl;

    // Set viewport
    // glViewport(0, 0, 8192, 8192);
    // checkGLError("glViewport");
    // Pass the projection matrix to your shader
    
    // Query the viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float viewportWidth = static_cast<float>(viewport[2]);
    float viewportHeight = static_cast<float>(viewport[3]);

    // Create line vertices
    struct LineVertex {
        float x;
        float y;
    };

    // get query_x1 and query_x2 floats from cmd args.
    float query_x1 = std::stof(argv[2]);    
    float query_x2 = std::stof(argv[3]);

    // modify the query_x1 and query_x2 so that point lookup can happen.
    assert(query_x1 <= query_x2);
    query_x1 = query_x1 - 0.5;
    query_x2 = query_x2 + 0.5;

    auto query_range = query_x2 - query_x1;
    if(query_range < 0.0) {
        std::cerr << "query_x2 should be greater than query_x1" << std::endl;
        return -1;
    }
    if(query_range >= viewportWidth * viewportHeight) {
        std::cerr << "query range should be less than viewportWidth * viewportHeight" << std::endl;
        return -1;
    }

    // Calculate starting point
    int start_y = static_cast<int>(query_x1 / viewportWidth);
    int start_x = static_cast<int>(query_x1 - start_y * viewportWidth);

    // Calculate ending point
    int end_y = static_cast<int>(query_x2 / viewportWidth);
    int end_x = static_cast<int>(query_x2 - end_y * viewportWidth);

    // Allocate line vertices array based on the viewport height
    std::vector<LineVertex> lineVertices;

    // Iterate from start_y to end_y to create lines
    for (int y = start_y; y <= end_y; ++y) {
        LineVertex startVertex, endVertex;

        // Determine the x-coordinates for the current line
        if (y == start_y) {
            startVertex.x = static_cast<float>(start_x);
            endVertex.x = static_cast<float>((y == end_y) ? end_x : viewportWidth);
        } else if (y == end_y) {
            startVertex.x = 0.0f;
            endVertex.x = static_cast<float>(end_x);
        } else {
            startVertex.x = 0.0f;
            endVertex.x = static_cast<float>(viewportWidth);
        }

        // Assign y-coordinates
        startVertex.y = static_cast<float>(y);
        endVertex.y = static_cast<float>(y);

        // Add the vertices to the lineVertices array
        lineVertices.push_back(startVertex);
        lineVertices.push_back(endVertex);
    }   

    // Generate and bind a Vertex Array Object (VAO) for the line
    GLuint lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertex) * lineVertices.size(), lineVertices.data(), GL_STATIC_DRAW);

    // Specify the layout of the vertex data
    glEnableVertexAttribArray(0);  // For data_x
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, x));

    glEnableVertexAttribArray(1);  // For data_y
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, y));

    GLuint dataSSBO;
    glGenBuffers(1, &dataSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO);
    
    int arr[(1 << 20)];
    std::fill_n(arr, (1 << 20), -1);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ((1 << 20)) * sizeof(int),arr,GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, dataSSBO);

    GLuint atomicCounterBuffer;
    glGenBuffers(1, &atomicCounterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterBuffer);

    // Allocate storage for the atomic counter (initialize to zero)
    GLuint zero = 0;
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    // Bind the atomic counter buffer to binding point 1 (matching the shader)
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, atomicCounterBuffer);
    // Load shaders from separate files
    std::string vertexShaderCode = loadShaderCode("shader.vs");
    std::string fragmentShaderCode = loadShaderCode("shader.fs");

    // Compile shader program using the loaded shader code
    GLuint shaderProgram = compileShaderProgram(vertexShaderCode.c_str(), fragmentShaderCode.c_str());
    if (!shaderProgram) {
        std::cerr << "Failed to compile shader program" << std::endl;
        return -1;
    }
    glUseProgram(shaderProgram);

    // Set uniform variables
    GLint rangeMinLocation = glGetUniformLocation(shaderProgram, "range_min");
    glUniform1f(rangeMinLocation, static_cast<float>(range_min));

    GLint rangeMaxLocation = glGetUniformLocation(shaderProgram, "range_max");
    glUniform1f(rangeMaxLocation, static_cast<float>(range_max));

    GLint textureSizeLocation = glGetUniformLocation(shaderProgram, "textureSize");
    glUniform1i(textureSizeLocation, textureSize);
    

    // Pass the viewport width to the shader
    GLint viewportWidthLocation = glGetUniformLocation(shaderProgram, "viewportWidth");
    glUniform1f(viewportWidthLocation, viewportWidth);

   glm::mat4 projectionMatrix = glm::ortho(
    static_cast<float>(0),
    static_cast<float>(viewportWidth),
    static_cast<float>(0), 
    static_cast<float>(viewportHeight), 
    -1.0f, 1.0f  // Near and Far planes
    );

    GLint projMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
    glUniformMatrix4fv(projMatrixLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    // Bind the texture buffer to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, textureID);

    // Set the sampler uniform in your shader to use texture unit 0
    GLuint dataTextureBufferLocation = glGetUniformLocation(shaderProgram, "dataTextureBuffer");
    glUniform1i(dataTextureBufferLocation, 0);

    // Disable depth testing and face culling to ensure fragment shader execution
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);


    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);


    // Bind the atomic counter buffer
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterBuffer);

    // Render the line
    glBindVertexArray(lineVAO);

    // adding framebuffer

    // unsigned int framebuffer;
    // glGenFramebuffers(1, &framebuffer);
    // glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // unsigned int colorTexture;
    // glGenTextures(1, &colorTexture);
    // glBindTexture(GL_TEXTURE_2D, colorTexture);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8192, 8192, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    // if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    //     std::cerr << "Framebuffer is not complete!" << std::endl;
    // } else {
    //     std::cout << "Framebuffer is complete!" << std::endl;
    // }
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Start timing before rendering
    auto startTime = std::chrono::high_resolution_clock::now();

        

    // // // Main loop (optional, since we're rendering only once)
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glDrawArrays(GL_LINES, 0, 2 * viewportHeight);

        // Ensure all rendering commands have completed
        glFinish();

        // Swap buffers (if window is displayed)
        glfwSwapBuffers(window);

    }


    // Stop timing after rendering
    auto endTime = std::chrono::high_resolution_clock::now();

    // Calculate duration
    std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
    std::cout << "Total Time: " << elapsed.count() << " ms" << std::endl;

    // Bind the atomic counter buffer
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterBuffer);

    // Map the buffer to read the counter value
    GLuint* counterValue = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    GLuint totalEntries = 0;
    if (counterValue) {
        totalEntries = *counterValue;
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    } else {
        // Handle error
        std::cerr << "Failed to map atomic counter buffer for reading." << std::endl;
    }

    // Bind the SSBO for reading
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO);

    // Map the buffer to read the data
    int* ssboData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (ssboData) {
        // Unmap the buffer when done
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    } else {
        // Handle error
        std::cerr << "Failed to map SSBO for reading." << std::endl;
    }

    // Create a set to store unique values from SSBO
    std::set<int> uniqueValues;

    // Process the data
    for (GLuint i = 0; i < (1 << 20); ++i) {
        int value = ssboData[i];
        if (value != -1) {
            uniqueValues.insert(value);
        }
    }

    // Print the unique values
    // std::cout << "Unique values in SSBO:" << std::endl;
    // for (const int& val : uniqueValues) {
    //     if(val > 0 && val < vertices.size())
    //         std::cout << vertices[val].indexValue<< std::endl;
    // }
    std::cout << "total entries: " << uniqueValues.size() << std::endl;
    std::cout << "viewport width: " << viewportWidth << std::endl;
    std::cout << "query_size: " << query_x2 - query_x1 << std::endl;

    // Clean up and exit
    glDeleteBuffers(1, &lineVBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &dataSSBO);
    glDeleteTextures(1, &textureID);
    // glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
