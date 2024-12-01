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
    // std::cerr << "GL CALLBACK: " << message << std::endl;
}

struct ResultData {
    int queryIndex;
    int rowIdentifier;
};


class KKIndex {
public: 
    GLuint shaderProgram;
    std::vector<Vertex> vertices;
    std::vector<int> textureData;
    int viewPortWidth;
    int viewPortHeight;
    GLuint atomicCounterBuffer;
    GLuint dataSSBO;

    void loadTableData(const char* filename) {
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
        this->vertices = loadTable(filename);
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        std::cout << "table_load_time: " << elapsed.count() << " ms" << std::endl;
    }

    void compileShaders(const char* vertexShaderCode, const char* fragmentShaderCode) {
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
        this->shaderProgram = compileShaderProgram(vertexShaderCode, fragmentShaderCode);
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;

        glUseProgram(this->shaderProgram);
        std::cout << "shader_compile_time: " << elapsed.count() << " ms" << std::endl;
    }

    void setUpTexture() {
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
        int range_min = this->vertices[0].indexValue;
        int range_max = this->vertices[0].indexValue;

        for (const auto& vertex : this->vertices) {
            range_min = std::min(range_min, vertex.indexValue);
            range_max = std::max(range_max, vertex.indexValue);
        }
        // so that range_min and range_max are not out of the bounds. 
        range_min -= 1;
        range_max += 1;

        std::cout << "Range: [" << range_min << ", " << range_max << "]" << std::endl;
        int textureSize = range_max - range_min + 1;

        // Create textureData vector initialized to -1
        this->textureData.resize(textureSize, -1);
        for (const auto& vertex : this->vertices) {
            int index = vertex.indexValue;
            this->textureData[index] = vertex.rowIdentifier;
        }
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        std::cout << "texture_setup_time (cpu): " << elapsed.count() << " ms" << std::endl;
        // Create and bind a 1D texture as TBO.
        // Generate and bind a buffer object for the texture buffer
        
        GLuint tbo;
        glGenBuffers(1, &tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, tbo);

        // Upload your data to the buffer
        glBufferData(GL_TEXTURE_BUFFER, textureSize * sizeof(int), this->textureData.data(), GL_STATIC_DRAW);

        // Generate the texture buffer object
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_BUFFER, textureID);

        // Associate the buffer with the texture buffer
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, tbo);

        // Set uniform variables
        GLint rangeMinLocation = glGetUniformLocation(this->shaderProgram, "range_min");
        glUniform1f(rangeMinLocation, static_cast<float>(range_min));

        GLint rangeMaxLocation = glGetUniformLocation(this->shaderProgram, "range_max");
        glUniform1f(rangeMaxLocation, static_cast<float>(range_max));

        GLint textureSizeLocation = glGetUniformLocation(this->shaderProgram, "textureSize");
        glUniform1i(textureSizeLocation, textureSize);

        // Bind the texture buffer to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, textureID);

        // Set the sampler uniform in your shader to use texture unit 0
        GLuint dataTextureBufferLocation = glGetUniformLocation(shaderProgram, "dataTextureBuffer");
        glUniform1i(dataTextureBufferLocation, 0);

        std::chrono::high_resolution_clock::time_point endTime2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed2 = endTime2 - endTime;

        std::cout << "texture size in elements: " << textureSize << std::endl;
        std::cout << "texture size in bytes: " << textureSize * sizeof(int) << std::endl;
        std::cout << "texture_setup_time (gpu upload + binding): " << elapsed2.count() << " ms" << std::endl;
    }

    void setuptFrameBuffersAndViewPort(int width, int height, bool useFBO) {
        this->viewPortWidth = width;
        this->viewPortHeight = height;

        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

        // Set viewport
        glViewport(0, 0, width, height);
        checkGLError("glViewport");
        // Pass the projection matrix to your shader
        glm::mat4 projectionMatrix = glm::ortho(
            static_cast<float>(0),
            static_cast<float>(width),
            static_cast<float>(0), 
            static_cast<float>(height), 
            -1.0f, 1.0f  // Near and Far planes
        );


        GLint projMatrixLocation = glGetUniformLocation(this->shaderProgram, "projectionMatrix");
        glUniformMatrix4fv(projMatrixLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        
        GLint viewportWidthLocation = glGetUniformLocation(shaderProgram, "viewportWidth");
        glUniform1i(viewportWidthLocation, width);


        // Optionally set FBO.
        if(useFBO) {
            unsigned int framebuffer;
            glGenFramebuffers(1, &framebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

            unsigned int colorTexture;
            glGenTextures(1, &colorTexture);
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        }
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        std::cout << "framebuffer_setup_time: " << elapsed.count() << " ms" << std::endl;
    }  

int createLinesForQueries(const std::vector<std::pair<int, int>>& queries) {
    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
    struct LineVertex {
        float x;
        float y;
        int queryIndex;
    };

    std::vector<LineVertex> lineVertices;
    int queryCount = 0;

    for (const auto& query : queries) {
        int query_x1 = query.first;
        int query_x2 = query.second;
        int queryIndex = queryCount++;

        // Modify the query_x1 and query_x2 so that point lookup can happen.
        assert(query_x1 <= query_x2);
        auto query_range = query_x2 - query_x1;
        if (query_range < 0.0) {
            std::cerr << "query_x2 should be greater than query_x1" << std::endl;
            return -1;
        }

        // Calculate starting and ending points
        int start_y = static_cast<int>(query_x1 / this->viewPortWidth) + 1;
        int start_x = static_cast<int>(query_x1 - (start_y - 1) * this->viewPortWidth);
        int end_y = static_cast<int>(query_x2 / this->viewPortWidth) + 1;
        int end_x = static_cast<int>(query_x2 - (end_y - 1) * this->viewPortWidth);

        // Iterate from start_y to end_y to create lines
        for (int y = start_y; y <= end_y; ++y) {
            LineVertex startVertex, endVertex;

            // Determine the x-coordinates for the current line
            if (y == start_y) {
                startVertex.x = static_cast<float>(start_x);
                endVertex.x = static_cast<float>((y == end_y) ? end_x : this->viewPortWidth);
            } else if (y == end_y) {
                startVertex.x = 0.0f;
                endVertex.x = static_cast<float>(end_x);
            } else {
                startVertex.x = 0.0f;
                endVertex.x = static_cast<float>(this->viewPortWidth);
            }

            // Assign y-coordinates and query index
            startVertex.y = static_cast<float>(y);
            endVertex.y = static_cast<float>(y);
            startVertex.queryIndex = queryIndex;
            endVertex.queryIndex = queryIndex;

            // Add the vertices to the lineVertices array
            lineVertices.push_back(startVertex);
            lineVertices.push_back(endVertex);
        }
    }

    // Generate and bind a Vertex Array Object (VAO) for the lines
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

    glEnableVertexAttribArray(2);  // For queryIndex
    glVertexAttribIPointer(2, 1, GL_INT, sizeof(LineVertex), (void*)offsetof(LineVertex, queryIndex));

    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
    std::cout << "line_creation_time: " << elapsed.count() << " ms" << std::endl;
    std::cout << "no. of lines: " << lineVertices.size() << std::endl;

    glBindVertexArray(lineVAO);
    return lineVertices.size();
}


    // int createLinesForQuery(int query_x1, int query_x2) {
    //     std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
    //     struct LineVertex {
    //         float x;
    //         float y;
    //     };


    //     // modify the query_x1 and query_x2 so that point lookup can happen.
    //     assert(query_x1 <= query_x2);
    //     // query_x2 += 0.5;
    //     auto query_range = query_x2 - query_x1;
    //     if(query_range < 0.0) {
    //         std::cerr << "query_x2 should be greater than query_x1" << std::endl;
    //         return -1;
    //     }
    //     std::cout << "query_range: " << query_range << std::endl;
    //     std::cout << "viewportSize: " << this->viewPortWidth * this->viewPortHeight << std::endl;
    //     if(query_range >= this->viewPortWidth * this->viewPortHeight) {
    //         std::cerr << "query range should be less than viewportWidth * viewportHeight" << std::endl;
    //         return -1;
    //     }

    //     // Calculate starting point
    //     int start_y = static_cast<int>(query_x1 / this->viewPortWidth) + 1;
    //     int start_x = static_cast<int>(query_x1 - (start_y - 1) * this->viewPortWidth);

    //     // Calculate ending point
    //     int end_y = static_cast<int>(query_x2 / this->viewPortWidth) + 1;
    //     int end_x = static_cast<int>(query_x2 - (end_y - 1) * this->viewPortWidth);

    //     // Allocate line vertices array based on the viewport height
    //     std::vector<LineVertex> lineVertices;

    //     // Iterate from start_y to end_y to create lines
    //     for (int y = start_y; y <= end_y; ++y) {
    //         LineVertex startVertex, endVertex;

    //         // Determine the x-coordinates for the current line
    //         if (y == start_y) {
    //             startVertex.x = static_cast<float>(start_x);
    //             endVertex.x = static_cast<float>((y == end_y) ? end_x : this->viewPortWidth);
    //         } else if (y == end_y) {
    //             startVertex.x = 0.0f;
    //             endVertex.x = static_cast<float>(end_x);
    //         } else {
    //             startVertex.x = 0.0f;
    //             endVertex.x = static_cast<float>(this->viewPortWidth);
    //         }

    //         // Assign y-coordinates
    //         startVertex.y = static_cast<float>(y);
    //         endVertex.y = static_cast<float>(y);

    //         // Add the vertices to the lineVertices array
    //         lineVertices.push_back(startVertex);
    //         lineVertices.push_back(endVertex);
    //         // std::cout << "Line: (" << startVertex.x << ", " << startVertex.y << ") -> (" << endVertex.x << ", " << endVertex.y << ")" << std::endl;
    //     }   

    //     // Generate and bind a Vertex Array Object (VAO) for the line
    //     GLuint lineVAO, lineVBO;
    //     glGenVertexArrays(1, &lineVAO);
    //     glGenBuffers(1, &lineVBO);

    //     glBindVertexArray(lineVAO);
    //     glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    //     glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertex) * lineVertices.size(), lineVertices.data(), GL_STATIC_DRAW);

    //     // Specify the layout of the vertex data
    //     glEnableVertexAttribArray(0);  // For data_x
    //     glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, x));

    //     glEnableVertexAttribArray(1);  // For data_y
    //     glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, y));
    //     std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    //     std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
    //     std::cout << "line_creation_time: " << elapsed.count() << " ms" << std::endl;
    //     std::cout << "no. of lines: " << lineVertices.size() << std::endl;

    //     glBindVertexArray(lineVAO);
    //     return lineVertices.size();
    // }

void setupDataSSBO(int size) {
    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
    glGenBuffers(1, &dataSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO); 

    glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(ResultData), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, dataSSBO);
    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = endTime - startTime;

    glGenBuffers(1, &atomicCounterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterBuffer);

    // Allocate storage for the atomic counter (initialize to zero)
    GLuint zero = 0;
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    // Bind the atomic counter buffer to binding point 1 (matching the shader)
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, atomicCounterBuffer);

    std::cout << "data_ssbo_setup_time: " << elapsed.count() << " ms" << std::endl;
}


    // void setupDataSSBO(int size) {
    //     std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
    //     glGenBuffers(1, &dataSSBO);
    //     glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO); 
        
    //     std::cout << "here1" << std::endl;
    //     std::vector<int> arr(size, -1);
    //     std::cout << "here3" << std::endl;
    //     glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(int), arr.data(), GL_DYNAMIC_COPY);
    //     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, dataSSBO);
    //     std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    //     std::chrono::duration<double, std::milli> elapsed = endTime - startTime;


    //     glGenBuffers(1, &atomicCounterBuffer);
    //     glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterBuffer);

    //     // Allocate storage for the atomic counter (initialize to zero)
    //     GLuint zero = 0;
    //     glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    //     // Bind the atomic counter buffer to binding point 1 (matching the shader)
    //     glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, atomicCounterBuffer);
        
    //     std::cout << "data_ssbo_setup_time: " << elapsed.count() << " ms" << std::endl;
    // }

    int query(float query_x1, float query_x2) {
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
        int lines = createLinesForQuery(query_x1, query_x2);

        glDrawArrays(GL_LINES, 0, lines);

        glFinish();
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        std::cout << "query_time: " << elapsed.count() << " ms" << std::endl;

        // Bind the atomic counter buffer
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterBuffer);
        int totalEntries = 0;
        // Map the buffer to read the counter value
        GLuint* counterValue = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
        if (counterValue) {
            totalEntries = *counterValue;
            glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        } else {
            // Handle error
            std::cerr << "Failed to map atomic counter buffer for reading." << std::endl;
        }

        return totalEntries;
    }


    int* getSSBOData() {
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSBO);
        int* ssboData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (ssboData) {
            return ssboData;
        } else {
            // Handle error
            std::cerr << "Failed to map SSBO for reading." << std::endl;
            return nullptr;
        }
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        std::cout << "ssbo_read_time: " << elapsed.count() << " ms" << std::endl;
    }


    void check(const std::set<int>& uniqueValues, int query_x1, int query_x2) {
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
        std::set<int> correctValues;
        for (const auto& vertex : this->vertices) {
            if (vertex.indexValue >= query_x1 && vertex.indexValue < query_x2) {
                correctValues.insert(vertex.rowIdentifier);
            }
        }
        std::cout << "correct values: " << correctValues.size() << std::endl;
        if (correctValues == uniqueValues) {
            std::cout << "All values are correct!" << std::endl;
        } else {
            std::cerr << "Some values are incorrect!" << std::endl;

            // print the query results.
            // std::cout << "Unique values in SSBO:" << std::endl;
            // for (const int& val : uniqueValues) {
            //     std::cout << vertices[val].indexValue<< std::endl;
            // }
            // Find the incorrect values
            std::set<int> incorrectValues;
            std::set_difference(correctValues.begin(), correctValues.end(), uniqueValues.begin(), uniqueValues.end(), std::inserter(incorrectValues, incorrectValues.begin()));
            std::set_difference(uniqueValues.begin(), uniqueValues.end(), correctValues.begin(), correctValues.end(), std::inserter(incorrectValues, incorrectValues.begin()));
            for (const int& value : incorrectValues) {
                std::cerr << "Incorrect value: " << value;
                if(correctValues.find(value) != correctValues.end()) {
                    std::cerr << ", Value is present in correct values i.e not in result." << std::endl;
                } else {
                    std::cerr << ", Value is not present in correct values i.e in result." << std::endl;
             }
            }
            std::cout << "Number of incorrect values: " << incorrectValues.size() << std::endl;
        }
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        std::cout << "check_time: " << elapsed.count() << " ms" << std::endl;
    }
};





// TOOD: think what to do when range_min is not 0.

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <table_file> <query_x1> <query_x2>" << std::endl;
        return -1;
    }

    const char* tableFile = argv[1];

    // Initialize OpenGL context (using GLFW)
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed" << std::endl;
        return -1;
    }

    // Request OpenGL version 4.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int windowWidth = 8192;
    int windowHeight = 8192;

    // NOTE: change the window size from 10x10 to windowWidth x windowHeight when useFBO is false
    // Create window
    GLFWwindow* window = glfwCreateWindow(10, 10, "OpenGL Line-Point Intersection", NULL, NULL);
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


    GLint maxBufferTextureSize = 0; 
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxBufferTextureSize);
    std::cout << "Maximum texture buffer size: " << maxBufferTextureSize << std::endl;

    // Enable OpenGL debug output
    glEnable(GL_DEBUG_OUTPUT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);


    glClear(GL_COLOR_BUFFER_BIT);

    glDebugMessageCallback(MessageCallback, 0);

    KKIndex kkIndex;

    kkIndex.compileShaders(loadShaderCode("shader.vs").c_str(), loadShaderCode("shader.fs").c_str());


    kkIndex.loadTableData(tableFile);

    kkIndex.setUpTexture();


    kkIndex.setuptFrameBuffersAndViewPort(windowWidth, windowHeight, true);

    int ssboDataSize = windowWidth * windowHeight;
    kkIndex.setupDataSSBO(ssboDataSize);

    int query_x1 = std::atoi(argv[2]);
    int query_x2 = std::atoi(argv[3]);

    int totalEntries = kkIndex.query(query_x1, query_x2);

    // int width, height;
    // glfwGetWindowSize(window, &width, &height);
    // std::cout << "window width: " << width << std::endl;
    // std::cout << "window height: " << height << std::endl;
    
    // main loop.
    // while (!glfwWindowShouldClose(window)) {
    //     glfwSwapBuffers(window);
    //     glfwPollEvents();
    // }

    int* ssboData = kkIndex.getSSBOData();
    
    // Create a set to store unique values from SSBO
    std::set<int> uniqueValues;

    // Process the data
    for (GLuint i = 0; i < std::min(ssboDataSize, totalEntries+1); ++i) {
        int value = ssboData[i];
        if (value != -1) {
            uniqueValues.insert(value);
        }
    }
    // verify the unique values.
    // manually run the query on the table(Vertices) and check if the values are correct.
    kkIndex.check(uniqueValues, query_x1, query_x2);  

    // Print the unique values
    // std::cout << "Unique values in SSBO:" << std::endl;
    // for (const int& val : uniqueValues) {
    //     if(val > 0 && val < vertices.size())
    //         std::cout << vertices[val].indexValue<< std::endl;
    // }
    std::cout << "total (index) entries: " << totalEntries << std::endl;
    std::cout << "total entries: " << uniqueValues.size() << std::endl;
    std::cout << "viewport width: " << windowWidth << std::endl;
    std::cout << "query_size: " << query_x2 - query_x1 << std::endl;

    // Clean up and exit
    glfwTerminate();
    return 0;
}
