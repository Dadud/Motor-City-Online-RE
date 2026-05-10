/**
 * render.h - Modern OpenGL Rendering System
 * 
 * Motor City Online - Rendering System
 * 
 * Modern OpenGL 3.3+ with:
 * - GLSL shaders
 * - PBR-inspired materials
 * - Multiple light sources
 * - Camera system
 * - Simple post-processing
 * - Mesh loading
 * 
 * Not overly complex - practical modern rendering.
 */

#ifndef RENDER_H
#define RENDER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define RENDER_MAX_LIGHTS        8
#define RENDER_MAX_MESHES        256
#define RENDER_MAX_TEXTURES       128
#define RENDER_SHADOW_MAP_SIZE    1024

// ============================================================================
// VECTOR/MATRIX MATH (no external dependencies)
// ============================================================================

typedef struct {
    float x, y, z, w;
} Vec4;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y;
} Vec2;

// 4x4 Matrix (column-major, OpenGL style)
typedef float Mat4[16];

// Create identity matrix
static inline Mat4 Mat4_Identity(void) {
    Mat4 m = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    return m;
}

// Create perspective matrix
static inline Mat4 Mat4_Perspective(float fov, float aspect, float near, float far) {
    float tanHalfFov = tanf(fov * 0.5f);
    Mat4 m = {0};
    m[0] = 1.0f / (aspect * tanHalfFov);
    m[5] = 1.0f / tanHalfFov;
    m[10] = -(far + near) / (far - near);
    m[11] = -1.0f;
    m[14] = -(2.0f * far * near) / (far - near);
    return m;
}

// Create look-at view matrix
static inline Mat4 Mat4_LookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = {center.x - eye.x, center.y - eye.y, center.z - eye.z};
    float fl = sqrtf(f.x*f.x + f.y*f.y + f.z*f.z);
    f.x /= fl; f.y /= fl; f.z /= fl;
    
    Vec3 s = {f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x};
    float sl = sqrtf(s.x*s.x + s.y*s.y + s.z*s.z);
    s.x /= sl; s.y /= sl; s.z /= sl;
    
    Vec3 u = {s.y*f.z - s.z*f.y, s.z*f.x - s.x*f.z, s.x*f.y - s.y*f.x};
    
    Mat4 m = Mat4_Identity();
    m[0] = s.x; m[4] = s.y; m[8] = s.z;
    m[1] = u.x; m[5] = u.y; m[9] = u.z;
    m[2] = -f.x; m[6] = -f.y; m[10] = -f.z;
    m[12] = -s.x*eye.x - s.y*eye.y - s.z*eye.z;
    m[13] = -u.x*eye.x - u.y*eye.y - u.z*eye.z;
    m[14] = f.x*eye.x + f.y*eye.y + f.z*eye.z;
    return m;
}

// Create translation matrix
static inline Mat4 Mat4_Translate(float x, float y, float z) {
    Mat4 m = Mat4_Identity();
    m[12] = x; m[13] = y; m[14] = z;
    return m;
}

// Create scale matrix
static inline Mat4 Mat4_Scale(float x, float y, float z) {
    Mat4 m = Mat4_Identity();
    m[0] = x; m[5] = y; m[10] = z;
    return m;
}

// Create rotation matrix (angle in radians)
static inline Mat4 Mat4_Rotate(Vec3 axis, float angle) {
    float c = cosf(angle), s = sinf(angle);
    float t = 1.0f - c;
    float x = axis.x, y = axis.y, z = axis.z;
    
    Mat4 m = Mat4_Identity();
    m[0] = t*x*x + c;   m[4] = t*x*y - s*z; m[8] = t*x*z + s*y;
    m[1] = t*x*y + s*z;   m[5] = t*y*y + c;   m[9] = t*y*z - s*x;
    m[2] = t*x*z - s*y;   m[6] = t*y*z + s*x; m[10] = t*z*z + c;
    return m;
}

// Multiply two matrices
static inline Mat4 Mat4_Mul(Mat4 a, Mat4 b) {
    Mat4 result = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result[i*4+j] += a[k*4+j] * b[i*4+k];
            }
        }
    }
    return result;
}

// Matrix from euler angles (radians)
static inline Mat4 Mat4_FromEuler(float pitch, float yaw, float roll) {
    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw), sy = sinf(yaw);
    float cr = cosf(roll), sr = sinf(roll);
    
    Mat4 m = Mat4_Identity();
    m[0] = cy*cr + sy*sp*sr;
    m[4] = -cy*sr + sy*sp*cr;
    m[8] = sy*cp;
    m[1] = sr*cp;
    m[5] = cr*cp;
    m[9] = -sp;
    m[2] = -sy*cr + cy*sp*sr;
    m[6] = sr*sy + cy*sp*cr;
    m[10] = cy*cp;
    return m;
}

// ============================================================================
// COLOR
// ============================================================================

typedef struct {
    float r, g, b, a;
} Color;

static inline Color Color_RGB(float r, float g, float b) {
    return (Color){r, g, b, 1.0f};
}

static inline Color Color_RGBA(float r, float g, float b, float a) {
    return (Color){r, g, b, a};
}

static inline Color Color_White(void) { return Color_RGB(1, 1, 1); }
static inline Color Color_Black(void) { return Color_RGB(0, 0, 0); }
static inline Color Color_Red(void) { return Color_RGB(1, 0, 0); }
static inline Color Color_Green(void) { return Color_RGB(0, 1, 0); }
static inline Color Color_Blue(void) { return Color_RGB(0, 0, 1); }
static inline Color Color_Gray(float v) { return Color_RGB(v, v, v); }

// ============================================================================
// TEXTURES
// ============================================================================

typedef struct {
    GLuint id;
    int width;
    int height;
    char name[64];
} Texture;

Texture* Texture_Load(const char* filename);
Texture* Texture_Create(int width, int height, const void* data, GLenum format);
void Texture_Bind(Texture* tex, int slot);
void Texture_Destroy(Texture* tex);

// ============================================================================
// MATERIALS (PBR-inspired)
// ============================================================================

typedef struct {
    // Base color
    Color albedo;
    
    // Roughness (0=pure mirror, 1=fully diffuse)
    float roughness;
    
    // Metallic (0=non-metal, 1=metal)
    float metallic;
    
    // Emission for glowing materials
    Color emission;
    
    // Normal map
    Texture* normalMap;
    
    // Texture maps
    Texture* albedoMap;
    Texture* roughnessMap;
    Texture* metallicMap;
    Texture* emissionMap;
    
    // Shader to use (-1 = use default)
    int shaderId;
    
    char name[64];
} Material;

Material* Material_Create(const char* name);
void Material_SetAlbedo(Material* mat, Color color);
void Material_SetRoughness(Material* mat, float roughness);
void Material_SetMetallic(Material* mat, float metallic);
void Material_SetEmission(Material* mat, Color color);
void Material_SetAlbedoMap(Material* mat, Texture* tex);
void Material_SetNormalMap(Material* mat, Texture* tex);
void Material_Destroy(Material* mat);

// ============================================================================
// MESH
// ============================================================================

typedef struct {
    // Vertex data
    float* positions;   // xyz (3 floats per vertex)
    float* normals;     // xyz (3 floats per vertex)
    float* texCoords;   // uv (2 floats per vertex)
    float* tangents;    // xyzw (4 floats per vertex)
    float* colors;      // rgba (4 floats per vertex)
    
    int vertexCount;
    
    // Index data
    unsigned int* indices;
    int indexCount;
    
    // OpenGL handles
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint indexCountGL;
    
    // Bounding box
    Vec3 minBound;
    Vec3 maxBound;
    
    char name[64];
} Mesh;

Mesh* Mesh_Create(const char* name);
void Mesh_SetPositions(Mesh* mesh, float* positions, int count);
void Mesh_SetNormals(Mesh* mesh, float* normals, int count);
void Mesh_SetTexCoords(Mesh* mesh, float* texCoords, int count);
void Mesh_SetIndices(Mesh* mesh, unsigned int* indices, int count);
void Mesh_CalculateTangents(Mesh* mesh);
void Mesh_CalculateBounds(Mesh* mesh);
void Mesh_Build(Mesh* mesh);
void Mesh_Draw(Mesh* mesh);
void Mesh_Destroy(Mesh* mesh);

// Built-in meshes
Mesh* Mesh_CreateQuad(float width, float height);
Mesh* Mesh_CreateCube(float size);
Mesh* Mesh_CreateSphere(float radius, int segments);

// ============================================================================
// CAMERA
// ============================================================================

typedef struct {
    Vec3 position;
    Vec3 target;
    Vec3 up;
    
    float fov;          // Field of view (radians)
    float aspectRatio;
    float nearPlane;
    float farPlane;
    
    Mat4 viewMatrix;
    Mat4 projectionMatrix;
    Mat4 viewProjection;
    
    // First-person camera support
    float pitch;         // Vertical angle
    float yaw;           // Horizontal angle
    float roll;          // Roll
    
} Camera;

void Camera_Init(Camera* cam);
void Camera_SetPosition(Camera* cam, Vec3 pos);
void Camera_SetTarget(Camera* cam, Vec3 target);
void Camera_LookAt(Camera* cam, Vec3 eye, Vec3 target, Vec3 up);
void Camera_SetFOV(Camera* cam, float fov);
void Camera_SetAspect(Camera* cam, float aspect);
void Camera_SetPlanes(Camera* cam, float near, float far);
void Camera_Update(Camera* cam);

// First-person camera controls
void Camera_MoveForward(Camera* cam, float distance);
void Camera_MoveRight(Camera* cam, float distance);
void Camera_MoveUp(Camera* cam, float distance);
void Camera_Pitch(Camera* cam, float angle);
void Camera_Yaw(Camera* cam, float angle);

// ============================================================================
// LIGHTS
// ============================================================================

typedef enum {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT,
    LIGHT_SPOT
} LightType;

typedef struct {
    LightType type;
    
    // Position/direction
    Vec3 position;
    Vec3 direction;
    
    // Color and intensity
    Color color;
    float intensity;
    
    // Attenuation (for point/spot)
    float constant;
    float linear;
    float quadratic;
    
    // Spot light angle
    float innerCone;    // Radians
    float outerCone;     // Radians
    
    // Shadow casting
    BOOL castsShadows;
    GLuint shadowMap;
    Mat4 shadowMatrix;
    
    BOOL enabled;
    char name[32];
} Light;

void Light_Init(Light* light);
void Light_SetDirectional(Light* light, Vec3 direction, Color color, float intensity);
void Light_SetPoint(Light* light, Vec3 position, Color color, float intensity,
                   float constant, float linear, float quadratic);
void Light_SetSpot(Light* light, Vec3 position, Vec3 direction, Color color,
                  float intensity, float innerCone, float outerCone);
void Light_SetShadow(Light* light, BOOL castShadows);

// ============================================================================
// SHADERS
// ============================================================================

typedef struct {
    GLuint program;
    
    // Attribute locations
    GLint aPosition;
    GLint aNormal;
    GLint aTexCoord;
    GLint aTangent;
    GLint aColor;
    
    // Uniform locations
    GLint uModelMatrix;
    GLint uViewMatrix;
    GLint uProjectionMatrix;
    GLint uNormalMatrix;
    GLint uAlbedo;
    GLint uRoughness;
    GLint uMetallic;
    GLint uEmission;
    GLint uAlbedoMap;
    GLint uNormalMap;
    GLint uRoughnessMap;
    GLint uMetallicMap;
    GLint uEmissionMap;
    GLint uUseNormalMap;
    GLint uUseAlbedoMap;
    GLint uUseRoughnessMap;
    GLint uUseMetallicMap;
    GLint uCameraPosition;
    GLint uLightPosition[RENDER_MAX_LIGHTS];
    GLint uLightDirection[RENDER_MAX_LIGHTS];
    GLint uLightColor[RENDER_MAX_LIGHTS];
    GLint uLightIntensity[RENDER_MAX_LIGHTS];
    GLint uLightType[RENDER_MAX_LIGHTS];
    GLint uLightCount;
    GLint uLightShadow[RENDER_MAX_LIGHTS];
    GLint uShadowMap[RENDER_MAX_LIGHTS];
    GLint uShadowMatrix[RENDER_MAX_LIGHTS];
    GLint uAmbientColor;
    GLint uTime;
    
    char name[64];
} Shader;

Shader* Shader_Create(const char* name);
BOOL Shader_Compile(Shader* shader, const char* vertexSource, const char* fragmentSource);
BOOL Shader_LoadFromFiles(Shader* shader, const char* vertexFile, const char* fragmentFile);
void Shader_Bind(Shader* shader);
void Shader_SetMatrix(Shader* shader, const char* name, Mat4 matrix);
void Shader_SetVec3(Shader* shader, const char* name, Vec3 vec);
void Shader_SetFloat(Shader* shader, const char* name, float value);
void Shader_SetColor(Shader* shader, const char* name, Color color);
void Shader_SetInt(Shader* shader, const char* name, int value);
void Shader_SetBool(Shader* shader, const char* name, BOOL value);
void Shader_Destroy(Shader* shader);

// ============================================================================
// RENDERER
// ============================================================================

typedef struct {
    GLFWwindow* window;
    int width;
    int height;
    
    // Current camera
    Camera* camera;
    
    // Current shader
    Shader* currentShader;
    
    // Lights
    Light lights[RENDER_MAX_LIGHTS];
    int lightCount;
    
    // Global ambient
    Color ambientColor;
    
    // Default shaders
    Shader* defaultShader;
    Shader* skyboxShader;
    Shader* postShader;
    
    // Render targets
    GLuint framebuffer;
    Texture* renderTexture;
    GLuint depthBuffer;
    
    // Post-processing
    Mesh* postQuad;
    Shader* postProcessShader;
    
    // Statistics
    int drawCalls;
    int triangles;
    float frameTime;
    
    BOOL initialized;
} Renderer;

BOOL Renderer_Init(Renderer* render, int width, int height);
void Renderer_Shutdown(Renderer* render);
void Renderer_BeginFrame(Renderer* render);
void Renderer_EndFrame(Renderer* render);
void Renderer_Resize(Renderer* render, int width, int height);

// Render passes
void Renderer_SetCamera(Renderer* render, Camera* cam);
void Renderer_SetAmbient(Renderer* render, Color ambient);
void Renderer_AddLight(Renderer* render, Light* light);
void Renderer_ClearLights(Renderer* render);

// Mesh rendering
void Renderer_DrawMesh(Renderer* render, Mesh* mesh, Material* material, Mat4 transform);
void Renderer_DrawMeshInstanced(Renderer* render, Mesh* mesh, Material* material, 
                               Mat4* transforms, int count);

// Shape rendering helpers
void Renderer_DrawLine(Renderer* render, Vec3 start, Vec3 end, Color color);
void Renderer_DrawBox(Renderer* render, Vec3 center, Vec3 size, Color color, Mat4 transform);
void Renderer_DrawSphere(Renderer* render, Vec3 center, float radius, Color color);

// Text rendering (simple bitmap font)
void Renderer_DrawText(Renderer* render, const char* text, Vec2 position, float size, Color color);

// Post-processing
void Renderer_EnablePostProcess(Renderer* render, BOOL enable);
void Renderer_SetPostEffect(Renderer* render, const char* effect);

// Statistics
int Renderer_GetDrawCalls(Renderer* render);
int Renderer_GetTriangleCount(Renderer* render);
float Renderer_GetFrameTime(Renderer* render);

// ============================================================================
// SCENE GRAPH (simple)
// ============================================================================

typedef struct SceneNode SceneNode;

typedef void (*SceneNodeUpdateFunc)(SceneNode* node, float dt);

struct SceneNode {
    char name[64];
    
    Mat4 transform;
    Vec3 position;
    Vec3 rotation;       // Euler angles
    Vec3 scale;
    
    Mesh* mesh;
    Material* material;
    
    SceneNode* parent;
    SceneNode* firstChild;
    SceneNode* nextSibling;
    
    SceneNodeUpdateFunc updateFunc;
    void* userData;
    
    BOOL visible;
};

SceneNode* SceneNode_Create(const char* name);
void SceneNode_Destroy(SceneNode* node);
void SceneNode_SetMesh(SceneNode* node, Mesh* mesh);
void SceneNode_SetMaterial(SceneNode* node, Material* material);
void SceneNode_SetPosition(SceneNode* node, Vec3 pos);
void SceneNode_SetRotation(SceneNode* node, Vec3 euler);
void SceneNode_SetScale(SceneNode* node, Vec3 scale);
void SceneNode_UpdateTransform(SceneNode* node);
void SceneNode_AddChild(SceneNode* parent, SceneNode* child);
void SceneNode_RemoveChild(SceneNode* parent, SceneNode* child);
void SceneNode_Update(SceneNode* node, float dt);
void SceneNode_Draw(SceneNode* node, Renderer* render, Mat4 parentTransform);

// ============================================================================
// GLOBALS (for simple usage)
// ============================================================================

extern Renderer g_render;

void Render_InitDefaults(void);
void Render_Shutdown(void);

#endif // RENDER_H
