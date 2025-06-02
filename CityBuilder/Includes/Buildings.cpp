#include "buildings.hpp"
#include <cmath>
#include <iostream>

BuildingData Buildings::studioFlatData;
BuildingData Buildings::smallHouseData;
BuildingData Buildings::familyHouseData;
BuildingData Buildings::towerData;
BuildingData Buildings::apartmentBlockData;

void Buildings::Initialize()
{
    CreateStudioFlat();
    CreateSmallHouse();
    CreateFamilyHouse();
    CreateTower();
    CreateApartmentBlock();
}

void Buildings::Cleanup()
{
    GLuint vaos[] = {studioFlatData.vao, smallHouseData.vao,
                     familyHouseData.vao, towerData.vao, apartmentBlockData.vao};
    GLuint vbos[] = {studioFlatData.vbo, smallHouseData.vbo,
                     familyHouseData.vbo, towerData.vbo, apartmentBlockData.vbo};

    glDeleteVertexArrays(5, vaos);
    glDeleteBuffers(5, vbos);
}

const BuildingData &Buildings::GetBuildingData(BuildingType type)
{
    switch (type)
    {
    case STUDIO_FLAT:
        return studioFlatData;
    case SMALL_HOUSE:
        return smallHouseData;
    case FAMILY_HOUSE:
        return familyHouseData;
    case TOWER:
        return towerData;
    case APARTMENT_BLOCK:
        return apartmentBlockData;
    default:
        return studioFlatData;
    }
}

glm::vec2 Buildings::GetBuildingSize(BuildingType type)
{
    switch (type)
    {
    case STUDIO_FLAT:
        return glm::vec2(1.0f, 1.0f);
    case SMALL_HOUSE:
        return glm::vec2(1.0f, 1.0f);
    case FAMILY_HOUSE:
        return glm::vec2(1.5f, 1.0f);
    case TOWER:
        return glm::vec2(1.0f, 1.0f);
    case APARTMENT_BLOCK:
        return glm::vec2(1.5f, 0.75f);
    default:
        return glm::vec2(1.5f, 1.5f);
    }
}

void Buildings::SetupVAOVBO(const std::vector<Vertex> &vertices, BuildingData &data)
{
    glGenVertexArrays(1, &data.vao);
    glGenBuffers(1, &data.vbo);

    glBindVertexArray(data.vao);
    glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    data.vertexCount = vertices.size();
}

void Buildings::AddQuad(std::vector<Vertex> &vertices,
                        const glm::vec3 &a, const glm::vec3 &b,
                        const glm::vec3 &c, const glm::vec3 &d,
                        const glm::vec3 &normal, bool isRoof)
{

    float texLeft = isRoof ? 0.5f : 0.0f;
    float texRight = isRoof ? 1.0f : 0.49f;

    vertices.push_back({a, normal, {texLeft, 0.0f}});  // lower left
    vertices.push_back({b, normal, {texRight, 0.0f}}); // lower right
    vertices.push_back({c, normal, {texRight, 1.0f}}); // upper right

    vertices.push_back({a, normal, {texLeft, 0.0f}});  // lower left
    vertices.push_back({c, normal, {texRight, 1.0f}}); // upper right
    vertices.push_back({d, normal, {texLeft, 1.0f}});  // upper left
}

void Buildings::AddTriangle(std::vector<Vertex> &vertices,
                            const glm::vec3 &a, const glm::vec3 &b,
                            const glm::vec3 &c, const glm::vec3 &normal, bool isRoof)
{
    // Left half (0.0-0.5) for walls, right half (0.5-1.0) for roof
    float texLeft = isRoof ? 0.5f : 0.0f;
    float texRight = isRoof ? 1.0f : 0.5f;

    vertices.push_back({a, normal, {texLeft, 0.0f}});
    vertices.push_back({b, normal, {texRight, 0.0f}});
    vertices.push_back({c, normal, {texLeft + (texRight - texLeft) * 0.5f, 1.0f}});
}

void Buildings::CreateStudioFlat(float sizeXZ)
{
    float sizeY = sizeXZ / 2.0f;
    float halfXZ = sizeXZ / 2.0f;

    std::vector<Vertex> vertices;

    glm::vec3 positions[8] = {
        {-halfXZ, 0, halfXZ},     // 0 - front left bottom
        {halfXZ, 0, halfXZ},      // 1 - front right bottom
        {halfXZ, sizeY, halfXZ},  // 2 - front right top
        {-halfXZ, sizeY, halfXZ}, // 3 - front left top
        {-halfXZ, 0, -halfXZ},    // 4 - back left bottom
        {halfXZ, 0, -halfXZ},     // 5 - back right bottom
        {halfXZ, sizeY, -halfXZ}, // 6 - back right top
        {-halfXZ, sizeY, -halfXZ} // 7 - back left top
    };

    // Front face
    AddQuad(vertices, positions[0], positions[1], positions[2], positions[3], {0, 0, 1}, false);
    // Back face
    AddQuad(vertices, positions[7], positions[6], positions[5], positions[4], {0, 0, -1}, false);
    // Right face
    AddQuad(vertices, positions[1], positions[5], positions[6], positions[2], {1, 0, 0}, false);
    //// Left face
    AddQuad(vertices, positions[4], positions[0], positions[3], positions[7], {-1, 0, 0}, false);
    // Top face
    AddQuad(vertices, positions[3], positions[2], positions[6], positions[7], {0, 1, 0}, true);

    SetupVAOVBO(vertices, studioFlatData);
}

void Buildings::CreateSmallHouse(float sizeXZ)
{
    float baseHeight = sizeXZ / 2.0f;
    float roofHeight = baseHeight;
    std::vector<Vertex> vertices;

    // Base vertices (same as studio flat)
    glm::vec3 basePositions[8] = {
        {-sizeXZ / 2, 0, sizeXZ / 2},
        {sizeXZ / 2, 0, sizeXZ / 2},
        {sizeXZ / 2, baseHeight, sizeXZ / 2},
        {-sizeXZ / 2, baseHeight, sizeXZ / 2},
        {-sizeXZ / 2, 0, -sizeXZ / 2},
        {sizeXZ / 2, 0, -sizeXZ / 2},
        {sizeXZ / 2, baseHeight, -sizeXZ / 2},
        {-sizeXZ / 2, baseHeight, -sizeXZ / 2}};

    // Add base walls (without top)
    AddQuad(vertices, basePositions[0], basePositions[1], basePositions[2], basePositions[3], {0, 0, 1}, false);
    AddQuad(vertices, basePositions[5], basePositions[4], basePositions[7], basePositions[6], {0, 0, -1}, false);
    AddQuad(vertices, basePositions[1], basePositions[5], basePositions[6], basePositions[2], {1, 0, 0}, false);
    AddQuad(vertices, basePositions[4], basePositions[0], basePositions[3], basePositions[7], {-1, 0, 0}, false);

    // Roof vertices
    glm::vec3 roofTop = {0, baseHeight + roofHeight, 0};

    // Roof faces (pyramid)
    AddTriangle(vertices, basePositions[3], basePositions[2], roofTop,
                glm::normalize(glm::vec3{-1, roofHeight / (sizeXZ / 2), 0}), true);
    AddTriangle(vertices, basePositions[2], basePositions[6], roofTop,
                glm::normalize(glm::vec3{0, roofHeight / (sizeXZ / 2), -1}), true);
    AddTriangle(vertices, basePositions[6], basePositions[7], roofTop,
                glm::normalize(glm::vec3{1, roofHeight / (sizeXZ / 2), 0}), true);
    AddTriangle(vertices, basePositions[7], basePositions[3], roofTop,
                glm::normalize(glm::vec3{0, roofHeight / (sizeXZ / 2), 1}), true);

    SetupVAOVBO(vertices, smallHouseData);
}

void Buildings::CreateFamilyHouse(float sizeX, float sizeZ, float height)
{
    std::vector<Vertex> vertices;
    height *= 0.5f;

    // Main part of the house (longer part of the L)
    float mainSizeX = sizeX * 0.6f;
    float mainSizeZ = sizeZ * 0.6f;

    // Calculate total dimensions including extension
    float totalSizeX = mainSizeX + sizeX * 0.4f;
    float centerOffsetX = totalSizeX / 2 - mainSizeX / 2;

    glm::vec3 mainPositions[8] = {
        {-mainSizeX / 2 - centerOffsetX, 0, mainSizeZ / 2},      // Front-left-bottom
        {mainSizeX / 2 - centerOffsetX, 0, mainSizeZ / 2},       // Front-right-bottom
        {mainSizeX / 2 - centerOffsetX, height, mainSizeZ / 2},  // Front-right-top
        {-mainSizeX / 2 - centerOffsetX, height, mainSizeZ / 2}, // Front-left-top
        {-mainSizeX / 2 - centerOffsetX, 0, -mainSizeZ / 2},     // Back-left-bottom
        {mainSizeX / 2 - centerOffsetX, 0, -mainSizeZ / 2},      // Back-right-bottom
        {mainSizeX / 2 - centerOffsetX, height, -mainSizeZ / 2}, // Back-right-top
        {-mainSizeX / 2 - centerOffsetX, height, -mainSizeZ / 2} // Back-left-top
    };

    // Extension part (shorter part of the L)
    float extSizeX = sizeX * 0.4f;
    float extSizeZ = sizeZ * 0.4f;

    // Position the extension to form an L shape (attached to the back-right of the main part)
    float extOffsetX = mainSizeX / 2 - centerOffsetX; // Extend along X-axis (right side)
    float extOffsetZ = -mainSizeZ / 2;                // Extend along Z-axis (back side)

    glm::vec3 extPositions[8] = {
        {extOffsetX, 0, extOffsetZ + extSizeZ},                 // Front-left-bottom
        {extOffsetX + extSizeX, 0, extOffsetZ + extSizeZ},      // Front-right-bottom
        {extOffsetX + extSizeX, height, extOffsetZ + extSizeZ}, // Front-right-top
        {extOffsetX, height, extOffsetZ + extSizeZ},            // Front-left-top
        {extOffsetX, 0, extOffsetZ},                            // Back-left-bottom
        {extOffsetX + extSizeX, 0, extOffsetZ},                 // Back-right-bottom
        {extOffsetX + extSizeX, height, extOffsetZ},            // Back-right-top
        {extOffsetX, height, extOffsetZ}                        // Back-left-top
    };

    // Add main part walls
    AddQuad(vertices, mainPositions[0], mainPositions[1], mainPositions[2], mainPositions[3], {0, 0, 1}, false);  // Front
    AddQuad(vertices, mainPositions[5], mainPositions[4], mainPositions[7], mainPositions[6], {0, 0, -1}, false); // Back
    AddQuad(vertices, mainPositions[1], mainPositions[5], mainPositions[6], mainPositions[2], {1, 0, 0}, false);  // Right
    AddQuad(vertices, mainPositions[4], mainPositions[0], mainPositions[3], mainPositions[7], {-1, 0, 0}, false); // Left
    AddQuad(vertices, mainPositions[3], mainPositions[2], mainPositions[6], mainPositions[7], {0, 1, 0}, true);   // Top

    // Add extension walls (excluding the wall that connects to the main part)
    AddQuad(vertices, extPositions[0], extPositions[1], extPositions[2], extPositions[3], {0, 0, 1}, false);  // Front
    AddQuad(vertices, extPositions[5], extPositions[4], extPositions[7], extPositions[6], {0, 0, -1}, false); // Back
    AddQuad(vertices, extPositions[1], extPositions[5], extPositions[6], extPositions[2], {1, 0, 0}, false);  // Right
    // Left wall is not added (it connects to the main part)
    AddQuad(vertices, extPositions[3], extPositions[2], extPositions[6], extPositions[7], {0, 1, 0}, true); // Top

    // Add connecting wall between main and extension
    glm::vec3 conn1 = mainPositions[5]; // Main back-right-bottom
    glm::vec3 conn2 = extPositions[4];  // Ext back-left-bottom
    glm::vec3 conn3 = extPositions[7];  // Ext back-left-top
    glm::vec3 conn4 = mainPositions[6]; // Main back-right-top

    AddQuad(vertices, conn1, conn2, conn3, conn4,
            glm::normalize(glm::cross(conn2 - conn1, conn3 - conn1)), false);

    SetupVAOVBO(vertices, familyHouseData);
}

void Buildings::CreateTower(float radius, float height)
{
    std::vector<Vertex> vertices;
    const int segments = 8;
    const float pi = 3.14159265358979323846f;

    // Cylinder side (reverse vertex order to flip normals)
    for (int i = 0; i < segments; i++)
    {
        float angle1 = 2.0f * pi * float(i) / float(segments);
        float angle2 = 2.0f * pi * float(i + 1) / float(segments);

        glm::vec3 p1 = {radius * cosf(angle1), 0, radius * sinf(angle1)};
        glm::vec3 p2 = {radius * cosf(angle2), 0, radius * sinf(angle2)};
        glm::vec3 p3 = {radius * cosf(angle2), height, radius * sinf(angle2)};
        glm::vec3 p4 = {radius * cosf(angle1), height, radius * sinf(angle1)};

        glm::vec3 normal = {cosf(angle1), 0, sinf(angle1)};

        // Reverse vertex order to flip the face
        AddQuad(vertices, p4, p3, p2, p1, normal, false);
    }

    // Top circle (roof) - reverse vertex order
    glm::vec3 centerTop = {0, height, 0};
    for (int i = 0; i < segments; i++)
    {
        float angle1 = 2.0f * pi * float(i) / float(segments);
        float angle2 = 2.0f * pi * float(i + 1) / float(segments);

        glm::vec3 p1 = {radius * cosf(angle1), height, radius * sinf(angle1)};
        glm::vec3 p2 = {radius * cosf(angle2), height, radius * sinf(angle2)};

        // Reverse vertex order to flip the face
        AddTriangle(vertices, centerTop, p2, p1, {0, 1, 0}, true);
    }

    SetupVAOVBO(vertices, towerData);
}

void Buildings::CreateApartmentBlock(float sizeXY)
{
    float sizeZ = sizeXY / 2.0f;
    std::vector<Vertex> vertices;

    glm::vec3 positions[8] = {
        {-sizeXY / 2, 0, sizeZ / 2},
        {sizeXY / 2, 0, sizeZ / 2},
        {sizeXY / 2, sizeXY, sizeZ / 2},
        {-sizeXY / 2, sizeXY, sizeZ / 2},
        {-sizeXY / 2, 0, -sizeZ / 2},
        {sizeXY / 2, 0, -sizeZ / 2},
        {sizeXY / 2, sizeXY, -sizeZ / 2},
        {-sizeXY / 2, sizeXY, -sizeZ / 2}};

    // Front face
    AddQuad(vertices, positions[0], positions[1], positions[2], positions[3], {0, 0, 1}, false);
    // Back face
    AddQuad(vertices, positions[5], positions[4], positions[7], positions[6], {0, 0, -1}, false);
    // Right face
    AddQuad(vertices, positions[1], positions[5], positions[6], positions[2], {1, 0, 0}, false);
    // Left face
    AddQuad(vertices, positions[4], positions[0], positions[3], positions[7], {-1, 0, 0}, false);
    // Top face (roof)
    AddQuad(vertices, positions[3], positions[2], positions[6], positions[7], {0, 1, 0}, true);
    // Bottom face (not added - covered by terrain)

    SetupVAOVBO(vertices, apartmentBlockData);
}