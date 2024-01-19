
//! \file tuto_rayons.cpp

#include <vector>
#include <cfloat>
#include <chrono>
#include <random>

#include "vec.h"
#include "mat.h"
#include "color.h"
#include "image.h"
#include "image_io.h"   
#include "image_hdr.h"
#include "orbiter.h"
#include "mesh.h"
#include "wavefront.h"


struct Ray
{
    Point o;                // origine
    Vector d;               // direction
    float tmax;             // position de l'extremite, si elle existe. le rayon est un intervalle [0 tmax]

    // le rayon est un segment, on connait origine et extremite, et tmax= 1
    Ray(const Point& origine, const Point& extremite) : o(origine), d(Vector(origine, extremite)), tmax(1) {}

    // le rayon est une demi droite, on connait origine et direction, et tmax= \inf
    Ray(const Point& origine, const Vector& direction) : o(origine), d(direction), tmax(FLT_MAX) {}

    // renvoie le point sur le rayon pour t
    Point point(const float t) const { return o + t * d; }
};


struct Hit
{
    float t;            // p(t)= o + td, position du point d'intersection sur le rayon
    float u, v;         // p(u, v), position du point d'intersection sur le triangle
    int triangle_id;    // indice du triangle dans le mesh

    Hit() : t(FLT_MAX), u(), v(), triangle_id(-1) {}
    Hit(const float _t, const float _u, const float _v, const int _id) : t(_t), u(_u), v(_v), triangle_id(_id) {}
    operator bool() { return (triangle_id != -1); }
};

struct Triangle
{
    Point p;            // sommet a du triangle
    Vector e1, e2;      // aretes ab, ac du triangle
    int id;

    Triangle(const TriangleData& data, const int _id) : p(data.a), e1(Vector(data.a, data.b)), e2(Vector(data.a, data.c)), id(_id) {}

    /* calcule l'intersection ray/triangle
        cf "fast, minimum storage ray-triangle intersection"

        renvoie faux s'il n'y a pas d'intersection valide (une intersection peut exister mais peut ne pas se trouver dans l'intervalle [0 tmax] du rayon.)
        renvoie vrai + les coordonnees barycentriques (u, v) du point d'intersection + sa position le long du rayon (t).
        convention barycentrique : p(u, v)= (1 - u - v) * a + u * b + v * c
    */
    Hit intersect(const Ray& ray, const float tmax) const
    {
        Vector pvec = cross(ray.d, e2);
        float det = dot(e1, pvec);

        float inv_det = 1 / det;
        Vector tvec(p, ray.o);

        float u = dot(tvec, pvec) * inv_det;
        if (u < 0 || u > 1) return Hit();

        Vector qvec = cross(tvec, e1);
        float v = dot(ray.d, qvec) * inv_det;
        if (v < 0 || u + v > 1) return Hit();

        float t = dot(e2, qvec) * inv_det;
        if (t > tmax || t < 0) return Hit();

        return Hit(t, u, v, id);           // p(u, v)= (1 - u - v) * a + u * b + v * c
    }
};


Vector normal(const Mesh& mesh, const Hit& hit)
{
    // recuperer le triangle complet dans le mesh
    const TriangleData& data = mesh.triangle(hit.triangle_id);
    // interpoler la normale avec les coordonnées barycentriques du point d'intersection
    float w = 1 - hit.u - hit.v;
    Vector n = w * Vector(data.na) + hit.u * Vector(data.nb) + hit.v * Vector(data.nc);
    return normalize(n);
}

Color diffuse_color(const Mesh& mesh, const Hit& hit)
{
    const Material& material = mesh.triangle_material(hit.triangle_id);
    return material.diffuse;
}

struct Source
{
    Triangle triangle;
    Color emission;
    Vector n;
    float area;
};

Hit intersect(const Ray& ray, const std::vector<Triangle>& triangles) {
    // calculer les intersections avec tous les triangles
    Hit hit;
    float tmax = ray.tmax;
    for (int i = 0; i < int(triangles.size()); i++)
    {
        if (Hit h = triangles[i].intersect(ray, tmax)) {
            // ne conserve que l'intersection la plus proche de l'origine du rayon
            assert(h.t >= 0);
            hit = h;
            tmax = h.t;
        }
    }
    return hit;
}
int main(const int argc, const char** argv)
{
    
    const char* mesh_filename = "data/cornell.obj";
    if (argc > 1)
        mesh_filename = argv[1];

    const char* orbiter_filename = "data/cornell_orbiter.txt";
    if (argc > 2)
        orbiter_filename = argv[2];

    Orbiter camera;
    if (camera.read_orbiter(orbiter_filename) < 0)
        return 1;

    Mesh mesh = read_mesh(mesh_filename);

    // recupere les triangles
    std::vector<Triangle> triangles;
    {
        int n = mesh.triangle_count();
        for (int i = 0; i < n; i++)
            triangles.emplace_back(mesh.triangle(i), i);
    }

    // recupere les sources
    std::vector<Source> sources;
    {
        int n = mesh.triangle_count();
        for (int i = 0; i < n; i++)
        {
            const Material& material = mesh.triangle_material(i);
            if (material.emission.r + material.emission.g + material.emission.b > 0)
            {
                // utiliser le centre du triangle comme source de lumière
                const TriangleData& data = mesh.triangle(i);
                float area = length(cross(Vector(data.a, data.b), Vector(data.a, data.c))) / 2;
                Vector normal = normalize(cross(Vector(data.a, data.c), Vector(data.a, data.b)));
                sources.push_back({ triangles[i], material.emission, normal, area });
            }
        }

        printf("%d sources\n", int(sources.size()));
        assert(sources.size() > 0);
    }

    Image image(1024, 768);

    // recupere les transformations
    camera.projection(image.width(), image.height(), 45);
    Transform model = Identity();
    Transform view = camera.view();
    Transform projection = camera.projection();
    Transform viewport = camera.viewport();
    Transform inv = Inverse(viewport * projection * view * model);

    auto start = std::chrono::high_resolution_clock::now();

    // c'est parti, parcours tous les pixels de l'image
#pragma omp parallel for //schedule(dynamic, 1)
    for (int y = 0; y < image.height(); y++) {
        std::random_device hwseed;
        std::default_random_engine rng(hwseed());
        std::uniform_real_distribution<float> uniform(0, 1);

        for (int x = 0; x < image.width(); x++)
        {
            // generer le rayon
            Point origine = inv(Point(x + float(0.5), y + float(0.5), 0));
            Point extremite = inv(Point(x + float(0.5), y + float(0.5), 1));
            Ray ray(origine, extremite);

            Color color;
            const int N = 128;
            Color diffuse;

            if (Hit hit = intersect(ray, triangles))
            {
                Point p = ray.o + hit.t * ray.d;
                Vector pn = normal(mesh, hit);
                diffuse = mesh.triangle_material(hit.triangle_id).diffuse;

                for (int i = 0; i < N; i++)
                {
                    int sInd = uniform(rng) * sources.size();   // uniforme entre 0 et n
                    Source& source = sources[sInd];
                    float pdf = 1 / float(sources.size()) * 1 / source.area;

                    float u2 = uniform(rng);
                    float u3 = uniform(rng);

                    //si aleatoire marche pas prendre barycentre
                    float squ = sqrt(u2);
                    Point bary(1 - squ, squ * (1 - u3), squ * u3);
                    Point b = source.triangle.p + source.triangle.e1;
                    Point c = source.triangle.p + source.triangle.e2;
                    Point sPoint = bary.x * source.triangle.p + bary.y * b + bary.z * c;

                    Ray ray2(p, sPoint);
                    ray2.o = ray2.point(0.001);

                    //Version 2
                    Color calcul;
                    if (Hit hit = intersect(ray2, triangles))
                    {
                        const Material& material = mesh.triangle_material(hit.triangle_id);    // cf la doc de Mesh
                        Color emission = material.emission;                                    // cf la doc de Material

                        // utiliser l'emission du triangle touche par le rayon dans la direction l
                        float cos_theta = dot(pn, normalize(sPoint - p));
                        float cos_theta2 = dot(source.n, normalize(ray2.d));
                        float distanceCarre = distance2(p, sPoint);
                        calcul = (diffuse / M_PI) * (emission * cos_theta * cos_theta2 / distanceCarre * 1 / pdf);

                    }
                    color = color + calcul;
                }
                color = color / float(N);
                image(x, y) = Color(color, 1);
            }
        }
        std::cout << y << std::endl;
    }


    auto stop = std::chrono::high_resolution_clock::now();
    int cpu = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    printf("%dms\n", cpu);

    write_image(image, "render.png");
    write_image_hdr(image, "shadow.hdr");
    return 0;
}
