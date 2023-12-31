#pragma once

#include "utilities.h"
#include "hittable.h"
#include "ray.h"
#include "color.h"
#include "texture.h"

class hit_info;

class Material
{
public:
    virtual ~Material() = default;

    virtual bool scatter(const Ray &ray_in, hit_info &info, Color &attenuation, Ray &scattered) const = 0;

    virtual Color emitted(double u, double v, const Point3 &p) const
    {
        return Color(0, 0, 0);
    }
};

class Lambertian : public Material
{
public:
    Lambertian(const Color &col) : albedo(std::make_shared<SolidColor>(col)) {}
    Lambertian(std::shared_ptr<Texture> tex) : albedo(tex) {}
    bool scatter(const Ray &ray_in, hit_info &info, Color &attenuation, Ray &scattered) const override
    {
        // always scatters
        // the math for this Lambertian diffusion is beautiful, read up again.
        auto scatter_direction = info.normal + randomUnitVec();

        // to check for case when randomUnitVec() is exactly opposite info.normal
        if (scatter_direction.near_zero())
        {
            scatter_direction = info.normal;
        }
        scattered = Ray(info.p, scatter_direction, ray_in.time());
        attenuation = albedo->value(info.u, info.v, info.p);
        return true;
    }

private:
    // albedo = proportion of incident light reflected.
    std::shared_ptr<Texture> albedo;
};
class Metal : public Material
{
public:
    Metal(const Color &col, double f) : albedo(col), fuzz(f < 1 ? f : 1) {}
    bool scatter(const Ray &ray_in, hit_info &info, Color &attenuation, Ray &scattered) const override
    {
        vec3 dir = normalize(ray_in.direction());
        vec3 reflected_direction = reflect(dir, info.normal);
        scattered = Ray(info.p, reflected_direction + fuzz * randomUnitVec(), ray_in.time());
        attenuation = albedo;
        // to count out reflections below the surface
        return info.normal.dot(scattered.direction());
    }

private:
    // albedo = proportion of incident light reflected.
    Color albedo;
    double fuzz;
};

class Dielectric : public Material
{
public:
    Dielectric(double refractive_index) : eta(refractive_index) {}
    bool scatter(const Ray &ray_in, hit_info &info, Color &attenuation, Ray &scattered) const override
    {
        // always refracts
        attenuation = Color(1.0, 1.0, 1.0);
        double refractive_ibyr = info.front_face ? 1.0 / eta : eta;
        // calculations expect unit vector
        vec3 unit_dir = normalize(ray_in.direction());
        // to check for TIR
        double cos_theta = fmin(-unit_dir.dot(info.normal), 1.0);
        double sin_theta = sqrt(1 - cos_theta * cos_theta);

        bool cannot_refract = refractive_ibyr * sin_theta > 1.0;
        vec3 scattered_direction;
        if (cannot_refract || reflectance(cos_theta, refractive_ibyr) > randomDouble())
        {
            scattered_direction = reflect(unit_dir, info.normal);
        }
        else
        {
            scattered_direction = refract(unit_dir, info.normal, refractive_ibyr);
        }
        scattered = Ray(info.p, scattered_direction, ray_in.time());
        return true;
    }

private:
    double eta;

    static double reflectance(double cos, double ri)
    {
        auto r0 = (1 - ri) / (1 + ri);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cos), 5);
    }
};

class DiffuseLight : public Material
{
public:
    DiffuseLight(std::shared_ptr<Texture> _emit) : emit(_emit) {}
    bool scatter(const Ray &ray_in, hit_info &info, Color &attenuation, Ray &scattered) const override
    {
        return false;
    }
    Color emitted(double u, double v, const Point3 &p) const override
    {
        return emit->value(u, v, p);
    }

private:
    std::shared_ptr<Texture> emit;
};

class Isotropic : public Material {
public:
    Isotropic(Color c) : albedo(std::make_shared<SolidColor>(c)) {}
    Isotropic(std::shared_ptr<Texture> a) : albedo(a) {}

    bool scatter(const Ray &ray_in, hit_info &info, Color &attenuation, Ray &scattered) const override
    {
        //random unit vector scattered in any direction from point of contact
        scattered = Ray(info.p, randomUnitVec(), ray_in.time());
        attenuation = albedo -> value(info.u, info.v, info.p);
        return true;
    }
    private:
    std::shared_ptr<Texture> albedo;
};