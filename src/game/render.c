/**
 * render.c - Modern OpenGL Rendering Implementation
 * 
 * Motor City Online - Rendering System
 */

#include "render.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ============================================================================
// SHADER SOURCES (built-in default shader)
// ============================================================================

static const char* DEFAULT_VERTEX_SHADER = R"(
#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vTangent;

void main()
{
    vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vTexCoord = aTexCoord;
    vTangent = aTangent.xyz;
    
    gl_Position = uProjectionMatrix * uViewMatrix * worldPos;
}
)";

static const char* DEFAULT_FRAGMENT_SHADER = R"(
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vTangent;

uniform vec3 uCameraPosition;

// Material
uniform vec3 uAlbedo;
uniform float uRoughness;
uniform float uMetallic;
uniform vec3 uEmission;

uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uEmissionMap;

uniform int uUseAlbedoMap;
uniform int uUseNormalMap;
uniform int uUseRoughnessMap;
uniform int uUseMetallicMap;

uniform vec3 uLightPosition[8];
uniform vec3 uLightDirection[8];
uniform vec3 uLightColor[8];
uniform float uLightIntensity[8];
uniform int uLightType[8];
uniform int uLightCount;
uniform int uLightShadow[8];
uniform sampler2D uShadowMap[8];
uniform mat4 uShadowMatrix[8];

uniform vec3 uAmbientColor;
uniform float uTime;

out vec4 fragColor;

// Normal mapping
vec3 getNormalFromMap(vec3 normal, vec3 tangent, vec2 texCoord)
{
    if (uUseNormalMap == 0) return normal;
    
    vec3 mapNormal = texture(uNormalMap, texCoord).xyz * 2.0 - 1.0;
    
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    return normalize(TBN * mapNormal);
}

// Fresnel-Schlick
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Distribution GGX
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    vec3 N_cross_H = cross(N, H);
    float N_cross_H_len = length(N_cross_H);
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;
    
    return nom / denom;
}

// Geometry Smith
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    
    return ggx1 * ggx2;
}

// Shadow calculation
float calculateShadow(int lightIndex, vec3 fragPos)
{
    if (uLightShadow[lightIndex] == 0) return 1.0;
    
    vec4 shadowCoord = uShadowMatrix[lightIndex] * vec4(fragPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    
    float shadow = 0.0;
    if (shadowCoord.z > 0.0 && shadowCoord.z < 1.0) {
        float bias = 0.005;
        shadow = texture(uShadowMap[lightIndex], shadowCoord.xy).r;
        if (shadow < shadowCoord.z - bias) shadow = 0.2;
        else shadow = 1.0;
    }
    return shadow;
}

void main()
{
    // Material properties
    vec3 albedo = uUseAlbedoMap != 0 ? texture(uAlbedoMap, vTexCoord).rgb : uAlbedo;
    float roughness = uUseRoughnessMap != 0 ? texture(uRoughnessMap, vTexCoord).r : uRoughness;
    float metallic = uUseMetallicMap != 0 ? texture(uMetallicMap, vTexCoord).r : uMetallic;
    vec3 emission = uUseMetallicMap != 0 ? texture(uEmissionMap, vTexCoord).rgb : uEmission;
    
    // Normal
    vec3 N = getNormalFromMap(normalize(vNormal), normalize(vTangent), vTexCoord);
    vec3 V = normalize(uCameraPosition - vWorldPos);
    
    // F0 for metallic/non-metallic
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Lighting
    vec3 Lo = vec3(0.0);
    
    for (int i = 0; i < uLightCount; i++)
    {
        vec3 L;
        float attenuation = 1.0;
        
        if (uLightType[i] == 0) {  // Directional
            L = normalize(-uLightDirection[i]);
        } else {  // Point/Spot
            L = normalize(uLightPosition[i] - vWorldPos);
            float distance = length(uLightPosition[i] - vWorldPos);
            attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        }
        
        vec3 H = normalize(V + L);
        
        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / 3.14159 + specular) * uLightColor[i] * uLightIntensity[i] * NdotL * attenuation;
        
        // Spotlight cone
        if (uLightType[i] == 2) {  // Spot
            float theta = dot(L, normalize(-uLightDirection[i]));
            float epsilon = 0.3 - 0.1;  // Inner - outer cone
            float intensity = clamp((theta - 0.1) / epsilon, 0.0, 1.0);
            Lo *= intensity;
        }
        
        // Shadow
        Lo *= calculateShadow(i, vWorldPos);
    }
    
    // Ambient
    vec3 ambient = uAmbientColor * albedo;
    
    vec3 color = ambient + Lo + emission;
    
    // Simple tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    
    fragColor = vec4(color, 1.0);
}
)";

static const char* POST_VERTEX_SHADER = R"(
#version 330 core

in vec2 aPosition;
out vec2 vTexCoord;

void main()
{
    vTexCoord = aPosition * 0.5 + 0.5;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)";

static const char* POST_FRAGMENT_SHADER = R"(
#version 330 core

in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform float uTime;

out vec4 fragColor;

void main()
{
    vec3 color = texture(uTexture, vTexCoord).rgb;
    
    // Vignette
    vec2 uv = vTexCoord * (1.0 - vTexCoord);
    float vignette = uv.x * uv.y * 15.0;
    vignette = pow(vignette, 0.2);
    color *= vignette;
    
    // Subtle film grain
    float grain = (fract(sin(dot(vTexCoord * uTime, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) * 0.03;
    color += grain;
    
    fragColor = vec4(color, 1.0);
}
)";

// ============================================================================
// TEXTURE IMPLEMENTATION
// ============================================================================

Texture* Texture_Load(const char* filename) {
    // This would use stb_image or similar to load images
    // For now, return a placeholder
    Texture* tex = (Texture*)malloc(sizeof(Texture));
    memset(tex, 0, sizeof(Texture));
    
    // Create a simple 1x1 white texture as placeholder
    unsigned char data[4] = {255, 255, 255, 255};
    
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    tex->width = 1;
    tex->height = 1;
    strncpy(tex->name, filename, sizeof(tex->name) - 1);
    
    return tex;
}

Texture* Texture_Create(int width, int height, const void* data, GLenum format) {
    Texture* tex = (Texture*)malloc(sizeof(Texture));
    memset(tex, 0, sizeof(Texture));
    
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    tex->width = width;
    tex->height = height;
    
    return tex;
}

void Texture_Bind(Texture* tex, int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, tex ? tex->id : 0);
}

void Texture_Destroy(Texture* tex) {
    if (tex) {
        if (tex->id) glDeleteTextures(1, &tex->id);
        free(tex);
    }
}

// ============================================================================
// MATERIAL IMPLEMENTATION
// ============================================================================

Material* Material_Create(const char* name) {
    Material* mat = (Material*)malloc(sizeof(Material));
    memset(mat, 0, sizeof(Material));
    
    mat->albedo = Color_White();
    mat->roughness = 0.5f;
    mat->metallic = 0.0f;
    mat->emission = Color_Black();
    mat->shaderId = -1;
    strncpy(mat->name, name, sizeof(mat->name) - 1);
    
    return mat;
}

void Material_SetAlbedo(Material* mat, Color color) {
    mat->albedo = color;
}

void Material_SetRoughness(Material* mat, float roughness) {
    mat->roughness = roughness > 1.0f ? 1.0f : roughness < 0.0f ? 0.0f : roughness;
}

void Material_SetMetallic(Material* mat, float metallic) {
    mat->metallic = metallic > 1.0f ? 1.0f : metallic < 0.0f ? 0.0f : metallic;
}

void Material_SetEmission(Material* mat, Color color) {
    mat->emission = color;
}

void Material_SetAlbedoMap(Material* mat, Texture* tex) {
    mat->albedoMap = tex;
}

void Material_SetNormalMap(Material* mat, Texture* tex) {
    mat->normalMap = tex;
}

void Material_Destroy(Material* mat) {
    if (mat) {
        // Don't free textures here - they're shared
        free(mat);
    }
}

// ============================================================================
// MESH IMPLEMENTATION
// ============================================================================

Mesh* Mesh_Create(const char* name) {
    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    memset(mesh, 0, sizeof(Mesh));
    strncpy(mesh->name, name, sizeof(mesh->name) - 1);
    return mesh;
}

void Mesh_SetPositions(Mesh* mesh, float* positions, int count) {
    mesh->positions = positions;
    mesh->vertexCount = count / 3;
}

void Mesh_SetNormals(Mesh* mesh, float* normals, int count) {
    mesh->normals = normals;
}

void Mesh_SetTexCoords(Mesh* mesh, float* texCoords, int count) {
    mesh->texCoords = texCoords;
}

void Mesh_SetIndices(Mesh* mesh, unsigned int* indices, int count) {
    mesh->indices = indices;
    mesh->indexCount = count;
}

void Mesh_CalculateTangents(Mesh* mesh) {
    if (!mesh->positions || !mesh->texCoords || !mesh->normals) return;
    
    mesh->tangents = (float*)malloc(mesh->vertexCount * 4 * sizeof(float));
    
    for (int i = 0; i < mesh->vertexCount; i++) {
        Vec3* v = (Vec3*)mesh->positions;
        Vec2* uv = (Vec2*)mesh->texCoords;
        Vec3* n = (Vec3*)mesh->normals;
        
        Vec3 tangent;
        // Simplified tangent calculation
        tangent.x = 1.0f;
        tangent.y = 0.0f;
        tangent.z = 0.0f;
        
        mesh->tangents[i*4] = tangent.x;
        mesh->tangents[i*4+1] = tangent.y;
        mesh->tangents[i*4+2] = tangent.z;
        mesh->tangents[i*4+3] = 1.0f;
    }
}

void Mesh_CalculateBounds(Mesh* mesh) {
    if (!mesh->positions) return;
    
    mesh->minBound = (Vec3){1e10f, 1e10f, 1e10f};
    mesh->maxBound = (Vec3){-1e10f, -1e10f, -1e10f};
    
    for (int i = 0; i < mesh->vertexCount; i++) {
        Vec3* p = ((Vec3*)mesh->positions) + i;
        if (p->x < mesh->minBound.x) mesh->minBound.x = p->x;
        if (p->y < mesh->minBound.y) mesh->minBound.y = p->y;
        if (p->z < mesh->minBound.z) mesh->minBound.z = p->z;
        if (p->x > mesh->maxBound.x) mesh->maxBound.x = p->x;
        if (p->y > mesh->maxBound.y) mesh->maxBound.y = p->y;
        if (p->z > mesh->maxBound.z) mesh->maxBound.z = p->z;
    }
}

void Mesh_Build(Mesh* mesh) {
    if (mesh->vao) return;  // Already built
    
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);
    
    glBindVertexArray(mesh->vao);
    
    // Calculate strides
    int stride = 0;
    int posOffset = 0;
    int normOffset = 0;
    int uvOffset = 0;
    int tanOffset = 0;
    int colOffset = 0;
    
    if (mesh->positions) { stride += 3; posOffset = 0; }
    if (mesh->normals) { stride += 3; normOffset = posOffset + 3; }
    if (mesh->texCoords) { stride += 2; uvOffset = normOffset + 3; }
    if (mesh->tangents) { stride += 4; tanOffset = uvOffset + 2; }
    if (mesh->colors) { stride += 4; colOffset = tanOffset + 4; }
    
    int bufferSize = mesh->vertexCount * stride * sizeof(float);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
    
    float* ptr = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    for (int i = 0; i < mesh->vertexCount; i++) {
        if (mesh->positions) {
            memcpy(ptr, ((float*)mesh->positions) + i * 3, 3 * sizeof(float));
            ptr += 3;
        }
        if (mesh->normals) {
            memcpy(ptr, ((float*)mesh->normals) + i * 3, 3 * sizeof(float));
            ptr += 3;
        }
        if (mesh->texCoords) {
            memcpy(ptr, ((float*)mesh->texCoords) + i * 2, 2 * sizeof(float));
            ptr += 2;
        }
        if (mesh->tangents) {
            memcpy(ptr, ((float*)mesh->tangents) + i * 4, 4 * sizeof(float));
            ptr += 4;
        }
        if (mesh->colors) {
            memcpy(ptr, ((float*)mesh->colors) + i * 4, 4 * sizeof(float));
            ptr += 4;
        }
    }
    
    glUnmapBuffer(GL_ARRAY_BUFFER);
    
    // Set up attributes
    int attrOffset = 0;
    
    if (mesh->positions) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(attrOffset * sizeof(float)));
        attrOffset += 3;
    }
    
    if (mesh->normals) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(attrOffset * sizeof(float)));
        attrOffset += 3;
    }
    
    if (mesh->texCoords) {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(attrOffset * sizeof(float)));
        attrOffset += 2;
    }
    
    if (mesh->tangents) {
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(attrOffset * sizeof(float)));
        attrOffset += 4;
    }
    
    if (mesh->colors) {
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(attrOffset * sizeof(float)));
    }
    
    // Indices
    if (mesh->indices && mesh->indexCount > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indexCount * sizeof(unsigned int), mesh->indices, GL_STATIC_DRAW);
        mesh->indexCountGL = mesh->indexCount;
    } else {
        mesh->indexCountGL = 0;
    }
    
    glBindVertexArray(0);
}

void Mesh_Draw(Mesh* mesh) {
    if (!mesh->vao) Mesh_Build(mesh);
    
    glBindVertexArray(mesh->vao);
    
    if (mesh->indexCountGL > 0) {
        glDrawElements(GL_TRIANGLES, mesh->indexCountGL, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, mesh->vertexCount);
    }
    
    glBindVertexArray(0);
}

void Mesh_Destroy(Mesh* mesh) {
    if (mesh) {
        if (mesh->vao) glDeleteVertexArrays(1, &mesh->vao);
        if (mesh->vbo) glDeleteBuffers(1, &mesh->vbo);
        if (mesh->ebo) glDeleteBuffers(1, &mesh->ebo);
        if (mesh->positions) free(mesh->positions);
        if (mesh->normals) free(mesh->normals);
        if (mesh->texCoords) free(mesh->texCoords);
        if (mesh->tangents) free(mesh->tangents);
        if (mesh->colors) free(mesh->colors);
        if (mesh->indices) free(mesh->indices);
        free(mesh);
    }
}

// Built-in meshes
Mesh* Mesh_CreateQuad(float width, float height) {
    Mesh* mesh = Mesh_Create("Quad");
    
    float positions[] = {
        -width/2, -height/2, 0,
         width/2, -height/2, 0,
         width/2,  height/2, 0,
        -width/2,  height/2, 0
    };
    
    float normals[] = {
        0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1
    };
    
    float uvs[] = {
        0, 0, 1, 0, 1, 1, 0, 1
    };
    
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
    
    Mesh_SetPositions(mesh, positions, 12);
    Mesh_SetNormals(mesh, normals, 12);
    Mesh_SetTexCoords(mesh, uvs, 8);
    Mesh_SetIndices(mesh, indices, 6);
    Mesh_CalculateTangents(mesh);
    Mesh_Build(mesh);
    
    return mesh;
}

Mesh* Mesh_CreateCube(float size) {
    Mesh* mesh = Mesh_Create("Cube");
    float s = size / 2.0f;
    
    float positions[] = {
        // Front
        -s, -s,  s,   s, -s,  s,   s,  s,  s,  -s,  s,  s,
        // Back
         s, -s, -s,  -s, -s, -s,  -s,  s, -s,   s,  s, -s,
        // Top
        -s,  s,  s,   s,  s,  s,   s,  s, -s,  -s,  s, -s,
        // Bottom
        -s, -s, -s,   s, -s, -s,   s, -s,  s,  -s, -s,  s,
        // Right
         s, -s,  s,   s, -s, -s,   s,  s, -s,   s,  s,  s,
        // Left
        -s, -s, -s,  -s, -s,  s,  -s,  s,  s,  -s,  s, -s
    };
    
    float normals[] = {
        0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
        0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
        0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
        0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,
        1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
        -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0
    };
    
    float uvs[] = {
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1
    };
    
    unsigned int indices[] = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };
    
    Mesh_SetPositions(mesh, positions, 72);
    Mesh_SetNormals(mesh, normals, 72);
    Mesh_SetTexCoords(mesh, uvs, 48);
    Mesh_SetIndices(mesh, indices, 36);
    Mesh_CalculateTangents(mesh);
    Mesh_Build(mesh);
    
    return mesh;
}

Mesh* Mesh_CreateSphere(float radius, int segments) {
    Mesh* mesh = Mesh_Create("Sphere");
    
    int vertCount = (segments + 1) * (segments + 1);
    float* positions = (float*)malloc(vertCount * 3 * sizeof(float));
    float* normals = (float*)malloc(vertCount * 3 * sizeof(float));
    float* uvs = (float*)malloc(vertCount * 2 * sizeof(float));
    
    for (int lat = 0; lat <= segments; lat++) {
        float theta = lat * 3.14159f / segments;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);
        
        for (int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2.0f * 3.14159f / segments;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            int i = lat * (segments + 1) + lon;
            
            positions[i*3] = x * radius;
            positions[i*3+1] = y * radius;
            positions[i*3+2] = z * radius;
            
            normals[i*3] = x;
            normals[i*3+1] = y;
            normals[i*3+2] = z;
            
            uvs[i*2] = (float)lon / segments;
            uvs[i*2+1] = (float)lat / segments;
        }
    }
    
    int indexCount = segments * segments * 6;
    unsigned int* indices = (unsigned int*)malloc(indexCount * sizeof(unsigned int));
    
    for (int lat = 0; lat < segments; lat++) {
        for (int lon = 0; lon < segments; lon++) {
            int i = lat * (segments + 1) + lon;
            
            indices[(lat*segments + lon)*6 + 0] = i;
            indices[(lat*segments + lon)*6 + 1] = i + segments + 1;
            indices[(lat*segments + lon)*6 + 2] = i + segments + 2;
            indices[(lat*segments + lon)*6 + 3] = i;
            indices[(lat*segments + lon)*6 + 4] = i + segments + 2;
            indices[(lat*segments + lon)*6 + 5] = i + 1;
        }
    }
    
    Mesh_SetPositions(mesh, positions, vertCount * 3);
    Mesh_SetNormals(mesh, normals, vertCount * 3);
    Mesh_SetTexCoords(mesh, uvs, vertCount * 2);
    Mesh_SetIndices(mesh, indices, indexCount);
    Mesh_CalculateTangents(mesh);
    Mesh_Build(mesh);
    
    return mesh;
}

// ============================================================================
// CAMERA IMPLEMENTATION
// ============================================================================

void Camera_Init(Camera* cam) {
    memset(cam, 0, sizeof(Camera));
    cam->position = (Vec3){0, 1.7f, 5};  // 1.7m eye height (driver)
    cam->target = (Vec3){0, 1.7f, 0};
    cam->up = (Vec3){0, 1, 0};
    cam->fov = 60.0f * 3.14159f / 180.0f;  // 60 degrees
    cam->aspectRatio = 16.0f / 9.0f;
    cam->nearPlane = 0.1f;
    cam->farPlane = 1000.0f;
    cam->pitch = 0;
    cam->yaw = 0;
    cam->roll = 0;
    Camera_Update(cam);
}

void Camera_SetPosition(Camera* cam, Vec3 pos) {
    cam->position = pos;
    Camera_Update(cam);
}

void Camera_SetTarget(Camera* cam, Vec3 target) {
    cam->target = target;
    Camera_Update(cam);
}

void Camera_LookAt(Camera* cam, Vec3 eye, Vec3 target, Vec3 up) {
    cam->position = eye;
    cam->target = target;
    cam->up = up;
    Camera_Update(cam);
}

void Camera_SetFOV(Camera* cam, float fov) {
    cam->fov = fov;
    Camera_Update(cam);
}

void Camera_SetAspect(Camera* cam, float aspect) {
    cam->aspectRatio = aspect;
    Camera_Update(cam);
}

void Camera_SetPlanes(Camera* cam, float near, float far) {
    cam->nearPlane = near;
    cam->farPlane = far;
    Camera_Update(cam);
}

void Camera_Update(Camera* cam) {
    cam->viewMatrix = Mat4_LookAt(cam->position, cam->target, cam->up);
    cam->projectionMatrix = Mat4_Perspective(cam->fov, cam->aspectRatio, cam->nearPlane, cam->farPlane);
    cam->viewProjection = Mat4_Mul(cam->projectionMatrix, cam->viewMatrix);
}

void Camera_MoveForward(Camera* cam, float distance) {
    Vec3 forward = {cam->target.x - cam->position.x, 0, cam->target.z - cam->position.z};
    float fl = sqrtf(forward.x*forward.x + forward.z*forward.z);
    forward.x /= fl; forward.z /= fl;
    
    cam->position.x += forward.x * distance;
    cam->position.z += forward.z * distance;
    cam->target.x += forward.x * distance;
    cam->target.z += forward.z * distance;
}

void Camera_MoveRight(Camera* cam, float distance) {
    Vec3 forward = {cam->target.x - cam->position.x, 0, cam->target.z - cam->position.z};
    float fl = sqrtf(forward.x*forward.x + forward.z*forward.z);
    forward.x /= fl; forward.z /= fl;
    
    Vec3 right = {-forward.z, 0, forward.x};
    
    cam->position.x += right.x * distance;
    cam->position.z += right.z * distance;
    cam->target.x += right.x * distance;
    cam->target.z += right.z * distance;
}

void Camera_MoveUp(Camera* cam, float distance) {
    cam->position.y += distance;
    cam->target.y += distance;
}

void Camera_Pitch(Camera* cam, float angle) {
    cam->pitch += angle;
    // Clamp pitch
    if (cam->pitch > 1.5f) cam->pitch = 1.5f;
    if (cam->pitch < -1.5f) cam->pitch = -1.5f;
    
    // Update target based on pitch/yaw
    cam->target.x = cam->position.x + sinf(cam->yaw) * cosf(cam->pitch);
    cam->target.y = cam->position.y + sinf(cam->pitch);
    cam->target.z = cam->position.z + cosf(cam->yaw) * cosf(cam->pitch);
}

void Camera_Yaw(Camera* cam, float angle) {
    cam->yaw += angle;
    
    // Update target
    cam->target.x = cam->position.x + sinf(cam->yaw) * cosf(cam->pitch);
    cam->target.z = cam->position.z + cosf(cam->yaw) * cosf(cam->pitch);
}

// ============================================================================
// LIGHT IMPLEMENTATION
// ============================================================================

void Light_Init(Light* light) {
    memset(light, 0, sizeof(Light));
    light->enabled = TRUE;
    light->castsShadows = FALSE;
    light->intensity = 1.0f;
}

void Light_SetDirectional(Light* light, Vec3 direction, Color color, float intensity) {
    light->type = LIGHT_DIRECTIONAL;
    light->direction = direction;
    light->color = color;
    light->intensity = intensity;
}

void Light_SetPoint(Light* light, Vec3 position, Color color, float intensity,
                   float constant, float linear, float quadratic) {
    light->type = LIGHT_POINT;
    light->position = position;
    light->color = color;
    light->intensity = intensity;
    light->constant = constant;
    light->linear = linear;
    light->quadratic = quadratic;
}

void Light_SetSpot(Light* light, Vec3 position, Vec3 direction, Color color,
                  float intensity, float innerCone, float outerCone) {
    light->type = LIGHT_SPOT;
    light->position = position;
    light->direction = direction;
    light->color = color;
    light->intensity = intensity;
    light->innerCone = innerCone;
    light->outerCone = outerCone;
}

void Light_SetShadow(Light* light, BOOL castShadows) {
    light->castsShadows = castShadows;
}

// ============================================================================
// SHADER IMPLEMENTATION
// ============================================================================

static GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

Shader* Shader_Create(const char* name) {
    Shader* shader = (Shader*)malloc(sizeof(Shader));
    memset(shader, 0, sizeof(Shader));
    strncpy(shader->name, name, sizeof(shader->name) - 1);
    return shader;
}

BOOL Shader_Compile(Shader* shader, const char* vertexSource, const char* fragmentSource) {
    GLuint vertShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    if (!vertShader || !fragShader) return FALSE;
    
    shader->program = glCreateProgram();
    glAttachShader(shader->program, vertShader);
    glAttachShader(shader->program, fragShader);
    glLinkProgram(shader->program);
    
    GLint linked;
    glGetProgramiv(shader->program, GL_LINK_STATUS, &linked);
    
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(shader->program, sizeof(log), NULL, log);
        fprintf(stderr, "Shader link error: %s\n", log);
        return FALSE;
    }
    
    // Get attribute locations
    shader->aPosition = glGetAttribLocation(shader->program, "aPosition");
    shader->aNormal = glGetAttribLocation(shader->program, "aNormal");
    shader->aTexCoord = glGetAttribLocation(shader->program, "aTexCoord");
    shader->aTangent = glGetAttribLocation(shader->program, "aTangent");
    shader->aColor = glGetAttribLocation(shader->program, "aColor");
    
    // Get uniform locations
    shader->uModelMatrix = glGetUniformLocation(shader->program, "uModelMatrix");
    shader->uViewMatrix = glGetUniformLocation(shader->program, "uViewMatrix");
    shader->uProjectionMatrix = glGetUniformLocation(shader->program, "uProjectionMatrix");
    shader->uNormalMatrix = glGetUniformLocation(shader->program, "uNormalMatrix");
    shader->uAlbedo = glGetUniformLocation(shader->program, "uAlbedo");
    shader->uRoughness = glGetUniformLocation(shader->program, "uRoughness");
    shader->uMetallic = glGetUniformLocation(shader->program, "uMetallic");
    shader->uEmission = glGetUniformLocation(shader->program, "uEmission");
    shader->uAlbedoMap = glGetUniformLocation(shader->program, "uAlbedoMap");
    shader->uNormalMap = glGetUniformLocation(shader->program, "uNormalMap");
    shader->uRoughnessMap = glGetUniformLocation(shader->program, "uRoughnessMap");
    shader->uMetallicMap = glGetUniformLocation(shader->program, "uMetallicMap");
    shader->uEmissionMap = glGetUniformLocation(shader->program, "uEmissionMap");
    shader->uUseNormalMap = glGetUniformLocation(shader->program, "uUseNormalMap");
    shader->uUseAlbedoMap = glGetUniformLocation(shader->program, "uUseAlbedoMap");
    shader->uUseRoughnessMap = glGetUniformLocation(shader->program, "uUseRoughnessMap");
    shader->uUseMetallicMap = glGetUniformLocation(shader->program, "uUseMetallicMap");
    shader->uCameraPosition = glGetUniformLocation(shader->program, "uCameraPosition");
    
    for (int i = 0; i < RENDER_MAX_LIGHTS; i++) {
        char name[64];
        snprintf(name, sizeof(name), "uLightPosition[%d]", i);
        shader->uLightPosition[i] = glGetUniformLocation(shader->program, name);
        snprintf(name, sizeof(name), "uLightDirection[%d]", i);
        shader->uLightDirection[i] = glGetUniformLocation(shader->program, name);
        snprintf(name, sizeof(name), "uLightColor[%d]", i);
        shader->uLightColor[i] = glGetUniformLocation(shader->program, name);
        snprintf(name, sizeof(name), "uLightIntensity[%d]", i);
        shader->uLightIntensity[i] = glGetUniformLocation(shader->program, name);
        snprintf(name, sizeof(name), "uLightType[%d]", i);
        shader->uLightType[i] = glGetUniformLocation(shader->program, name);
        snprintf(name, sizeof(name), "uLightShadow[%d]", i);
        shader->uLightShadow[i] = glGetUniformLocation(shader->program, name);
        snprintf(name, sizeof(name), "uShadowMap[%d]", i);
        shader->uShadowMap[i] = glGetUniformLocation(shader->program, name);
        snprintf(name, sizeof(name), "uShadowMatrix[%d]", i);
        shader->uShadowMatrix[i] = glGetUniformLocation(shader->program, name);
    }
    
    shader->uLightCount = glGetUniformLocation(shader->program, "uLightCount");
    shader->uAmbientColor = glGetUniformLocation(shader->program, "uAmbientColor");
    shader->uTime = glGetUniformLocation(shader->program, "uTime");
    
    return TRUE;
}

BOOL Shader_LoadFromFiles(Shader* shader, const char* vertexFile, const char* fragmentFile) {
    // Would read files and call Shader_Compile
    // For now, use built-in shaders
    return Shader_Compile(shader, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
}

void Shader_Bind(Shader* shader) {
    glUseProgram(shader ? shader->program : 0);
}

void Shader_SetMatrix(Shader* shader, const char* name, Mat4 matrix) {
    GLint loc = glGetUniformLocation(shader->program, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, matrix);
}

void Shader_SetVec3(Shader* shader, const char* name, Vec3 vec) {
    GLint loc = glGetUniformLocation(shader->program, name);
    if (loc >= 0) glUniform3f(loc, vec.x, vec.y, vec.z);
}

void Shader_SetFloat(Shader* shader, const char* name, float value) {
    GLint loc = glGetUniformLocation(shader->program, name);
    if (loc >= 0) glUniform1f(loc, value);
}

void Shader_SetColor(Shader* shader, const char* name, Color color) {
    GLint loc = glGetUniformLocation(shader->program, name);
    if (loc >= 0) glUniform4f(loc, color.r, color.g, color.b, color.a);
}

void Shader_SetInt(Shader* shader, const char* name, int value) {
    GLint loc = glGetUniformLocation(shader->program, name);
    if (loc >= 0) glUniform1i(loc, value);
}

void Shader_SetBool(Shader* shader, const char* name, BOOL value) {
    Shader_SetInt(shader, name, value ? 1 : 0);
}

void Shader_Destroy(Shader* shader) {
    if (shader) {
        if (shader->program) glDeleteProgram(shader->program);
        free(shader);
    }
}

// ============================================================================
// RENDERER IMPLEMENTATION
// ============================================================================

Renderer g_render;

static void Renderer_CreateDefaultResources(Renderer* render) {
    // Default shader
    render->defaultShader = Shader_Create("Default");
    Shader_Compile(render->defaultShader, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    
    // Post-processing shader
    render->postShader = Shader_Create("PostProcess");
    Shader_Compile(render->postShader, POST_VERTEX_SHADER, POST_FRAGMENT_SHADER);
    
    // Post-processing quad
    render->postQuad = Mesh_CreateQuad(2.0f, 2.0f);
}

BOOL Renderer_Init(Renderer* render, int width, int height) {
    memset(render, 0, sizeof(Renderer));
    render->width = width;
    render->height = height;
    render->ambientColor = Color_RGB(0.03f, 0.03f, 0.03f);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW init error: %s\n", glewGetErrorString(err));
        return FALSE;
    }
    
    // Create default resources
    Renderer_CreateDefaultResources(render);
    
    // Set some GL defaults
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    render->initialized = TRUE;
    return TRUE;
}

void Renderer_Shutdown(Renderer* render) {
    if (render->defaultShader) Shader_Destroy(render->defaultShader);
    if (render->postShader) Shader_Destroy(render->postShader);
    if (render->postQuad) Mesh_Destroy(render->postQuad);
    
    memset(render, 0, sizeof(Renderer));
}

void Renderer_BeginFrame(Renderer* render) {
    glViewport(0, 0, render->width, render->height);
    glClearColor(
        render->ambientColor.r,
        render->ambientColor.g,
        render->ambientColor.b,
        1.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    render->drawCalls = 0;
    render->triangles = 0;
}

void Renderer_EndFrame(Renderer* render) {
    // Could do post-processing here
    glfwSwapBuffers(glfwGetCurrentContext());
}

void Renderer_Resize(Renderer* render, int width, int height) {
    render->width = width;
    render->height = height;
    if (render->camera) {
        render->camera->aspectRatio = (float)width / (float)height;
        Camera_Update(render->camera);
    }
}

void Renderer_SetCamera(Renderer* render, Camera* cam) {
    render->camera = cam;
}

void Renderer_SetAmbient(Renderer* render, Color ambient) {
    render->ambientColor = ambient;
}

void Renderer_AddLight(Renderer* render, Light* light) {
    if (render->lightCount < RENDER_MAX_LIGHTS) {
        render->lights[render->lightCount++] = *light;
    }
}

void Renderer_ClearLights(Renderer* render) {
    render->lightCount = 0;
}

void Renderer_DrawMesh(Renderer* render, Mesh* mesh, Material* material, Mat4 transform) {
    if (!mesh || !material) return;
    
    Shader* shader = render->currentShader ? render->currentShader : render->defaultShader;
    Shader_Bind(shader);
    
    // Matrices
    if (shader->uModelMatrix >= 0) {
        glUniformMatrix4fv(shader->uModelMatrix, 1, GL_FALSE, transform);
    }
    
    if (render->camera) {
        if (shader->uViewMatrix >= 0) {
            glUniformMatrix4fv(shader->uViewMatrix, 1, GL_FALSE, render->camera->viewMatrix);
        }
        if (shader->uProjectionMatrix >= 0) {
            glUniformMatrix4fv(shader->uProjectionMatrix, 1, GL_FALSE, render->camera->projectionMatrix);
        }
        if (shader->uCameraPosition >= 0) {
            glUniform3f(shader->uCameraPosition, 
                       render->camera->position.x,
                       render->camera->position.y,
                       render->camera->position.z);
        }
        
        // Normal matrix (inverse transpose of model matrix 3x3)
        // Simplified: just use model matrix for now
        if (shader->uNormalMatrix >= 0) {
            glUniformMatrix3fv(shader->uNormalMatrix, 1, GL_FALSE, transform);
        }
    }
    
    // Material
    if (shader->uAlbedo >= 0) {
        glUniform4f(shader->uAlbedo, material->albedo.r, material->albedo.g, 
                   material->albedo.b, material->albedo.a);
    }
    if (shader->uRoughness >= 0) glUniform1f(shader->uRoughness, material->roughness);
    if (shader->uMetallic >= 0) glUniform1f(shader->uMetallic, material->metallic);
    if (shader->uEmission >= 0) {
        glUniform4f(shader->uEmission, material->emission.r, material->emission.g,
                   material->emission.b, material->emission.a);
    }
    
    // Textures
    int texSlot = 0;
    if (material->albedoMap && shader->uAlbedoMap >= 0) {
        Texture_Bind(material->albedoMap, texSlot);
        glUniform1i(shader->uAlbedoMap, texSlot);
        glUniform1i(shader->uUseAlbedoMap, 1);
        texSlot++;
    } else if (shader->uUseAlbedoMap >= 0) {
        glUniform1i(shader->uUseAlbedoMap, 0);
    }
    
    if (material->normalMap && shader->uNormalMap >= 0) {
        Texture_Bind(material->normalMap, texSlot);
        glUniform1i(shader->uNormalMap, texSlot);
        glUniform1i(shader->uUseNormalMap, 1);
        texSlot++;
    } else if (shader->uUseNormalMap >= 0) {
        glUniform1i(shader->uUseNormalMap, 0);
    }
    
    // Lights
    if (shader->uLightCount >= 0) {
        glUniform1i(shader->uLightCount, render->lightCount);
    }
    
    for (int i = 0; i < render->lightCount; i++) {
        Light* light = &render->lights[i];
        if (!light->enabled) continue;
        
        if (shader->uLightPosition[i] >= 0) {
            glUniform3f(shader->uLightPosition[i], light->position.x, light->position.y, light->position.z);
        }
        if (shader->uLightDirection[i] >= 0) {
            glUniform3f(shader->uLightDirection[i], light->direction.x, light->direction.y, light->direction.z);
        }
        if (shader->uLightColor[i] >= 0) {
            glUniform3f(shader->uLightColor[i], light->color.r, light->color.g, light->color.b);
        }
        if (shader->uLightIntensity[i] >= 0) {
            glUniform1f(shader->uLightIntensity[i], light->intensity);
        }
        if (shader->uLightType[i] >= 0) {
            glUniform1i(shader->uLightType[i], light->type);
        }
        if (shader->uLightShadow[i] >= 0) {
            glUniform1i(shader->uLightShadow[i], light->castsShadows ? 1 : 0);
        }
    }
    
    if (shader->uAmbientColor >= 0) {
        glUniform4f(shader->uAmbientColor, render->ambientColor.r, render->ambientColor.g,
                   render->ambientColor.b, render->ambientColor.a);
    }
    
    // Draw
    Mesh_Draw(mesh);
    
    render->drawCalls++;
    render->triangles += mesh->indexCountGL > 0 ? mesh->indexCountGL / 3 : mesh->vertexCount / 3;
}

void Renderer_DrawMeshInstanced(Renderer* render, Mesh* mesh, Material* material,
                               Mat4* transforms, int count) {
    // Would use glDrawElementsInstanced
    // For simplicity, just draw multiple times
    for (int i = 0; i < count; i++) {
        Renderer_DrawMesh(render, mesh, material, transforms[i]);
    }
}

int Renderer_GetDrawCalls(Renderer* render) {
    return render->drawCalls;
}

int Renderer_GetTriangleCount(Renderer* render) {
    return render->triangles;
}

float Renderer_GetFrameTime(Renderer* render) {
    return render->frameTime;
}

// ============================================================================
// SCENE NODE IMPLEMENTATION
// ============================================================================

SceneNode* SceneNode_Create(const char* name) {
    SceneNode* node = (SceneNode*)malloc(sizeof(SceneNode));
    memset(node, 0, sizeof(SceneNode));
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->transform = Mat4_Identity();
    node->scale = (Vec3){1, 1, 1};
    node->visible = TRUE;
    return node;
}

void SceneNode_Destroy(SceneNode* node) {
    if (node->firstChild) SceneNode_Destroy(node->firstChild);
    if (node->nextSibling) SceneNode_Destroy(node->nextSibling);
    free(node);
}

void SceneNode_SetMesh(SceneNode* node, Mesh* mesh) {
    node->mesh = mesh;
}

void SceneNode_SetMaterial(SceneNode* node, Material* material) {
    node->material = material;
}

void SceneNode_SetPosition(SceneNode* node, Vec3 pos) {
    node->position = pos;
    SceneNode_UpdateTransform(node);
}

void SceneNode_SetRotation(SceneNode* node, Vec3 euler) {
    node->rotation = euler;
    SceneNode_UpdateTransform(node);
}

void SceneNode_SetScale(SceneNode* node, Vec3 scale) {
    node->scale = scale;
    SceneNode_UpdateTransform(node);
}

void SceneNode_UpdateTransform(SceneNode* node) {
    Mat4 t = Mat4_Translate(node->position.x, node->position.y, node->position.z);
    Mat4 r = Mat4_FromEuler(node->rotation.x, node->rotation.y, node->rotation.z);
    Mat4 s = Mat4_Scale(node->scale.x, node->scale.y, node->scale.z);
    node->transform = Mat4_Mul(Mat4_Mul(t, r), s);
}

void SceneNode_AddChild(SceneNode* parent, SceneNode* child) {
    child->parent = parent;
    child->nextSibling = parent->firstChild;
    parent->firstChild = child;
}

void SceneNode_RemoveChild(SceneNode* parent, SceneNode* child) {
    SceneNode** current = &parent->firstChild;
    while (*current) {
        if (*current == child) {
            *current = child->nextSibling;
            child->parent = NULL;
            child->nextSibling = NULL;
            return;
        }
        current = &(*current)->nextSibling;
    }
}

void SceneNode_Update(SceneNode* node, float dt) {
    if (node->updateFunc) {
        node->updateFunc(node, dt);
    }
    
    SceneNode* child = node->firstChild;
    while (child) {
        SceneNode_Update(child, dt);
        child = child->nextSibling;
    }
}

void SceneNode_Draw(SceneNode* node, Renderer* render, Mat4 parentTransform) {
    if (!node->visible) return;
    
    Mat4 worldTransform = Mat4_Mul(parentTransform, node->transform);
    
    if (node->mesh && node->material) {
        Renderer_DrawMesh(render, node->mesh, node->material, worldTransform);
    }
    
    SceneNode* child = node->firstChild;
    while (child) {
        SceneNode_Draw(child, render, worldTransform);
        child = child->nextSibling;
    }
}

// ============================================================================
// GLOBALS / SHORTCUTS
// ============================================================================

Renderer* Renderer_GetGlobal(void) {
    return &g_render;
}

void Render_InitDefaults(void) {
    Renderer_Init(&g_render, 1280, 720);
}

void Render_Shutdown(void) {
    Renderer_Shutdown(&g_render);
}
