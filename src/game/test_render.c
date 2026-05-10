/**
 * test_render.c - OpenGL Rendering Test
 * 
 * Tests the modern OpenGL rendering system.
 */

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include "render.h"

static void error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main() {
    printf("Motor City Online - OpenGL Rendering Test\n");
    printf("========================================\n\n");
    
    // Initialize GLFW
    if (!glfwInit()) {
        printf("ERROR: Failed to initialize GLFW\n");
        return 1;
    }
    
    glfwSetErrorCallback(error_callback);
    
    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "MCO Renderer Test", NULL, NULL);
    if (!window) {
        printf("ERROR: Failed to create window\n");
        glfwTerminate();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize renderer
    Renderer render;
    if (!Renderer_Init(&render, 1280, 720)) {
        printf("ERROR: Failed to initialize renderer\n");
        glfwTerminate();
        return 1;
    }
    
    printf("Renderer initialized successfully\n");
    printf("  OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("  GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("  Vendor: %s\n\n", glGetString(GL_VENDOR));
    
    // Create camera
    Camera camera;
    Camera_Init(&camera);
    Camera_LookAt(&camera, (Vec3){3, 3, 5}, (Vec3){0, 0, 0}, (Vec3){0, 1, 0});
    Camera_SetFOV(&camera, 60.0f * 3.14159f / 180.0f);
    
    // Create lights
    Light sun;
    Light_Init(&sun);
    Light_SetDirectional(&sun, (Vec3){-0.5f, -1.0f, -0.5f}, Color_RGB(1, 1, 0.95f), 2.0f);
    Renderer_AddLight(&render, &sun);
    
    Light point1;
    Light_Init(&point1);
    Light_SetPoint(&point1, (Vec3){2, 2, 2}, Color_RGB(1, 0.8f, 0.6f), 1.0f, 1.0f, 0.1f, 0.01f);
    Renderer_AddLight(&render, &point1);
    
    // Create materials
    Material* matCar = Material_Create("CarBody");
    Material_SetAlbedo(matCar, Color_RGB(0.8f, 0.1f, 0.1f));  // Red
    Material_SetRoughness(matCar, 0.3f);
    Material_SetMetallic(matCar, 0.8f);
    
    Material* matChrome = Material_Create("Chrome");
    Material_SetAlbedo(matChrome, Color_RGB(0.9f, 0.9f, 0.9f));
    Material_SetRoughness(matChrome, 0.1f);
    Material_SetMetallic(matChrome, 1.0f);
    
    Material* matTrack = Material_Create("Asphalt");
    Material_SetAlbedo(matTrack, Color_RGB(0.2f, 0.2f, 0.2f));
    Material_SetRoughness(matTrack, 0.8f);
    Material_SetMetallic(matTrack, 0.0f);
    
    Material* matSky = Material_Create("Sky");
    Material_SetAlbedo(matSky, Color_RGB(0.5f, 0.7f, 1.0f));
    Material_SetEmission(matSky, Color_RGB(0.3f, 0.5f, 0.8f));
    
    // Create meshes
    Mesh* meshCar = Mesh_CreateCube(1.0f);
    Mesh_Build(meshCar);
    
    Mesh* meshSphere = Mesh_CreateSphere(0.5f, 16);
    
    Mesh* meshGround = Mesh_CreateQuad(20.0f, 20.0f);
    
    // Create test meshes
    Mesh* testSphere = Mesh_CreateSphere(1.0f, 32);
    
    printf("Creating scene...\n");
    
    // Test animation
    float time = 0.0f;
    
    printf("\nRunning render loop...\n");
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        time += 0.016f;
        
        // Begin frame
        Renderer_BeginFrame(&render);
        
        // Update camera (orbit around origin)
        float camAngle = time * 0.2f;
        Vec3 camPos = {
            sinf(camAngle) * 8.0f,
            3.0f + sinf(time * 0.5f) * 0.5f,
            cosf(camAngle) * 8.0f
        };
        Camera_LookAt(&camera, camPos, (Vec3){0, 0, 0}, (Vec3){0, 1, 0});
        Renderer_SetCamera(&render, &camera);
        
        // Draw ground
        Mat4 groundTransform = Mat4_Translate(0, -1, 0);
        Renderer_DrawMesh(&render, meshGround, matTrack, groundTransform);
        
        // Draw car body (rotating cube)
        Mat4 carTransform = Mat4_FromEuler(0, time, 0);
        carTransform = Mat4_Mul(Mat4_Translate(0, 0.5f, 0), carTransform);
        Renderer_DrawMesh(&render, meshCar, matCar, carTransform);
        
        // Draw chrome sphere
        Mat4 chromeTransform = Mat4_Translate(
            sinf(time * 2.0f) * 2.0f,
            0.5f + cosf(time * 1.5f) * 0.5f,
            cosf(time * 2.0f) * 2.0f
        );
        Renderer_DrawMesh(&render, meshSphere, matChrome, chromeTransform);
        
        // Draw test sphere
        Mat4 sphereTransform = Mat4_Translate(
            cosf(time * 0.7f) * 3.0f,
            1.0f + sinf(time) * 0.3f,
            sinf(time * 0.7f) * 3.0f
        );
        Renderer_DrawMesh(&render, testSphere, matChrome, sphereTransform);
        
        // End frame
        Renderer_EndFrame(&render);
        
        // Print stats every second
        if ((int)(time * 10) % 10 == 0) {
            printf("Frame: %d draw calls, %d triangles, %.2f ms\n",
                   Renderer_GetDrawCalls(&render),
                   Renderer_GetTriangleCount(&render),
                   Renderer_GetFrameTime(&render) * 1000.0f);
        }
        
        // Handle events
        glfwPollEvents();
        
        // ESC to quit
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }
    }
    
    printf("\nCleaning up...\n");
    
    // Cleanup
    Material_Destroy(matCar);
    Material_Destroy(matChrome);
    Material_Destroy(matTrack);
    Material_Destroy(matSky);
    
    Mesh_Destroy(meshCar);
    Mesh_Destroy(meshSphere);
    Mesh_Destroy(meshGround);
    Mesh_Destroy(testSphere);
    
    Renderer_Shutdown(&render);
    glfwTerminate();
    
    printf("Test complete!\n");
    return 0;
}
