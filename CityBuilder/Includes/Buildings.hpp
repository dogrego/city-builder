#ifndef BUILDINGS_HPP
#define BUILDINGS_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

enum BuildingType
{
    STUDIO_FLAT,
    SMALL_HOUSE,
    FAMILY_HOUSE,
    TOWER,
    APARTMENT_BLOCK
};

struct BuildingData
{
    GLuint vao;
    GLuint vbo;
    int vertexCount;
};

class Buildings
{
public:
    static void Initialize();
    static void Cleanup();
    static const BuildingData &GetBuildingData(BuildingType type);
    static glm::vec2 GetBuildingSize(BuildingType type);

private:
    static void CreateStudioFlat(float sizeXZ = 2.0f);
    static void CreateSmallHouse(float sizeXZ = 2.0f);
    static void CreateFamilyHouse(float sizeX = 3.0f, float sizeZ = 4.0f, float height = 2.5f);
    static void CreateTower(float radius = 1.0f, float height = 4.0f);
    static void CreateApartmentBlock(float sizeXY = 3.0f);

    static BuildingData studioFlatData;
    static BuildingData smallHouseData;
    static BuildingData familyHouseData;
    static BuildingData towerData;
    static BuildingData apartmentBlockData;

    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

    static void SetupVAOVBO(const std::vector<Vertex> &vertices, BuildingData &data);
    static void AddQuad(std::vector<Vertex> &vertices,
                        const glm::vec3 &a, const glm::vec3 &b,
                        const glm::vec3 &c, const glm::vec3 &d,
                        const glm::vec3 &normal, bool isRoof);
    static void AddTriangle(std::vector<Vertex> &vertices,
                            const glm::vec3 &a, const glm::vec3 &b,
                            const glm::vec3 &c, const glm::vec3 &normal, bool isRoof);
};

#endif