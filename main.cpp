// ============================================================================
// Controls:
//   W / S : translate up / down     (while held)
//   A / D : translate left / right  (while held)
//   Q / E : rotate 30 degrees anticlockwise / clockwise around the z axis
//   R / F : scale along the z axis by a factor s (grow / shrink)
//   ESC   : close the window
// ============================================================================

#include <GL/glew.h>        
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>   // needed for glm::value_ptr
#include <iostream>

// ---------------------------------------------------------------------------
// Configuration constants
// ---------------------------------------------------------------------------
const int   WINDOW_WIDTH  = 800;
const int   WINDOW_HEIGHT = 600;

const float MOVE_DISTANCE = 1.0f;   // "d": distance travelled per second while a key is held
const float ROTATION_STEP = 30.0f;  // degrees per key press
const float SCALE_FACTOR  = 1.1f;   // "s": multiplied / divided per key press

// ---------------------------------------------------------------------------
// Shader sources
// ---------------------------------------------------------------------------
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec3 vertexColor;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        vertexColor = aColor;
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 330 core
    in  vec3 vertexColor;
    out vec4 FragColor;

    void main() {
        FragColor = vec4(vertexColor, 1.0);
    }
)glsl";

// ---------------------------------------------------------------------------
// Holds the current state of the pyramid and builds its model matrix.
// Keeping the state in one place makes the render loop and the input handling
// easy to read, and guarantees the transformations are always composed in the
// same order (translate -> rotate -> scale).E
// ---------------------------------------------------------------------------
struct PyramidTransform {
    glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
    float     rotationZ   = 0.0f;                        // degrees around the z axis
    glm::vec3 scale       = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::mat4 modelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, translation);
        model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

// Keeps the rendering area in sync with the window if the user resizes it.
void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

// Compiles one shader stage and reports any compilation error.
// Returns 0 on failure.
unsigned int compileShader(GLenum shaderType, const char* source) {
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Compiles both stages and links them into a shader program.
// Returns 0 on failure.
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) return 0;

    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << "\n";
        glDeleteProgram(program);
        program = 0;
    }

    // The individual shader objects are no longer needed once linked.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// Returns true only on the frame where a key goes from released to pressed.
// Rotation and scaling need this: glfwGetKey() reports GLFW_PRESS on every
// frame the key is held down, so without this edge detection a single tap
// would apply 30 degrees (or a 1.1x scale) dozens of times in a fraction of
// a second.
bool keyJustPressed(GLFWwindow* window, int key) {
    static bool wasDown[GLFW_KEY_LAST + 1] = { false };

    bool isDown      = (glfwGetKey(window, key) == GLFW_PRESS);
    bool justPressed = isDown && !wasDown[key];
    wasDown[key]     = isDown;
    return justPressed;
}

// ---------------------------------------------------------------------------
// Step 4: keyboard input.
// Called once per frame. deltaTime is the duration of the previous frame in
// seconds, which makes the translation speed independent of the frame rate.
// ---------------------------------------------------------------------------
void processInput(GLFWwindow* window, PyramidTransform& pyramid, float deltaTime) {
    // ESC closes the window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- Translation: continuous while the key is held ---
    const float step = MOVE_DISTANCE * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pyramid.translation.y += step;   // up
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pyramid.translation.y -= step;   // down
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        pyramid.translation.x -= step;   // left
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        pyramid.translation.x += step;   // right

    // --- Rotation around the z axis: one 30 degree step per press ---
    if (keyJustPressed(window, GLFW_KEY_Q))
        pyramid.rotationZ += ROTATION_STEP;   // anticlockwise (positive angle, right-hand rule)
    if (keyJustPressed(window, GLFW_KEY_E))
        pyramid.rotationZ -= ROTATION_STEP;   // clockwise

    // Keep the angle in [0, 360) so it never grows without bound.
    if (pyramid.rotationZ >= 360.0f) pyramid.rotationZ -= 360.0f;
    if (pyramid.rotationZ <    0.0f) pyramid.rotationZ += 360.0f;

    // --- Scaling along the z axis: one factor s per press ---
    if (keyJustPressed(window, GLFW_KEY_R))
        pyramid.scale.z *= SCALE_FACTOR;      // stretch in the +z direction
    if (keyJustPressed(window, GLFW_KEY_F))
        pyramid.scale.z /= SCALE_FACTOR;      // shrink in the -z direction
}

// ---------------------------------------------------------------------------
int main() {
    // ---- Step 1: window and OpenGL context ----
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          "COMP 371 - A2", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);   // context must exist before glewInit
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // GLEW queries the driver for the OpenGL function pointers
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);   // so pyramid faces are drawn in the correct order

    // ---- Step 2: shaders ----
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (shaderProgram == 0) {
        glfwTerminate();
        return -1;
    }

    // ---- Step 2: pyramid geometry ----
    // Each vertex is: position (x, y, z) followed by colour (r, g, b).
    // Because the colour is interpolated across each triangle, the five
    // vertices are enough to give every face a distinct look.
    float pyramidVertices[] = {
        // position              // colour
         0.0f,  0.5f,  0.0f,     1.0f, 0.9f, 0.2f,   // 0 top corner (yellow)
        -0.5f, -0.5f,  0.5f,     0.9f, 0.2f, 0.2f,   // 1 bottom front left  (red)
         0.5f, -0.5f,  0.5f,     0.2f, 0.8f, 0.3f,   // 2 bottom front right (green)
         0.5f, -0.5f, -0.5f,     0.2f, 0.4f, 0.9f,   // 3 bottom back right  (blue)
        -0.5f, -0.5f, -0.5f,     0.8f, 0.3f, 0.8f    // 4 bottom back left   (purple)
    };

    // Winding order is counterclockwise seen from outside,
    // so every face normal points away from the pyramid's interior.
    unsigned int indices[] = {
        0, 1, 2,   // front face
        0, 2, 3,   // right face
        0, 3, 4,   // back face
        0, 4, 1,   // left face

        1, 3, 2,   // base, first triangle
        1, 4, 3    // base, second triangle
    };
    const int INDEX_COUNT = sizeof(indices) / sizeof(indices[0]);   // 6 faces * 3 = 18

    // ---- Step 2: VAO / VBO / EBO ----
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);   // 3 floats position + 3 floats colour

    // attribute 0 -> position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);

    // attribute 1 -> colour
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);   // unbind, good practice

    // ---- Camera and projection ----
    // The eye is offset slightly on x and y so the pyramid reads as a 3D solid
    // and so the z scaling (R / F) is clearly visible during the demo.
    glm::mat4 view = glm::lookAt(
        glm::vec3(1.0f, 0.8f, 3.0f),   // eye
        glm::vec3(0.0f, 0.0f, 0.0f),   // looking at the origin
        glm::vec3(0.0f, 1.0f, 0.0f)    // +y is up
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),                                        // field of view
        static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT),
        0.1f,                                                       // near plane
        100.0f                                                      // far plane
    );

    // ---- Uniform locations (they never change, so look them up once) ----
    unsigned int modelLoc      = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc       = glGetUniformLocation(shaderProgram, "view");
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // The view and projection matrices are constant, so upload them once.
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // ---- Live state driven by the keyboard ----
    PyramidTransform pyramid;
    float lastFrameTime = static_cast<float>(glfwGetTime());

    // ---- Render loop ----
    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime   = currentTime - lastFrameTime;
        lastFrameTime     = currentTime;

        // Step 4: read the keyboard and update the pyramid's state
        processInput(window, pyramid, deltaTime);

        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Step 3: build the model matrix from the current state and send it
        // to the "model" uniform in the vertex shader
        glm::mat4 model = pyramid.modelMatrix();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);   // double buffering
        glfwPollEvents();          // process input events
    }

    // ---- Cleanup ----
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}