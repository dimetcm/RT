#pragma once

#include <cstdint>

#include <glm/glm.hpp>

enum class MaterialType : uint32_t
{
    Lambertian,
    Metal,
    Dielectric
};

struct LambertianMaterialProperties 
{
    glm::vec3 albedo;
    float dummy;
};

struct MetalMaterialProperties 
{
    glm::vec3 albedo;
    float fuzz = 0.5f;
};

struct DielectricMaterialProperties
{
    float refractionIndex = 1.0f;
    float _dummy1;
    float _dummy2;
    float _dummy3;
};

struct MaterialInfo
{
    MaterialType type;
    uint32_t propertiesIdx;
    uint32_t _dummy1;
    uint32_t _dummy2;

private:
    MaterialInfo(MaterialType inType, uint32_t inPropertiesIdx) : type(inType), propertiesIdx(inPropertiesIdx) {}
    friend struct MaterialManager;
};

struct MaterialManager
{
    MaterialInfo CreateMaterial(const LambertianMaterialProperties& propertis)
    {
        MaterialInfo info(MaterialType::Lambertian, static_cast<uint32_t>(lambertianMaterials.size()));
        lambertianMaterials.push_back(propertis);
        return info;
    }

    MaterialInfo CreateMaterial(const MetalMaterialProperties& propertis)
    {
        MaterialInfo info(MaterialType::Metal, static_cast<uint32_t>(metalMaterials.size()));
        metalMaterials.push_back(propertis);
        return info;
    }

    MaterialInfo CreateMaterial(const DielectricMaterialProperties& propertis)
    {
        MaterialInfo info(MaterialType::Dielectric, static_cast<uint32_t>(dielectricMaterials.size()));
        dielectricMaterials.push_back(propertis);
        return info;
    }

    std::vector<LambertianMaterialProperties> lambertianMaterials;
    std::vector<MetalMaterialProperties> metalMaterials;
    std::vector<DielectricMaterialProperties> dielectricMaterials;
};