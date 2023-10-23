
//! \file tuto_rayons.cpp

#include <vector>
#include <cfloat>
#include <chrono>

#include "vec.h"
#include "mat.h"
#include "color.h"
#include "image.h"
#include "image_io.h"
#include "image_hdr.h"
#include "orbiter.h"
#include "mesh.h"
#include "wavefront.h"
#include <random>


struct Ray
{
    Point o;            // origine
    Vector d;           // direction
    float tmax;		 // distance max d'intersection


    Ray( const Point& _o, const Point& _e, const float _tmax = 100) :  o(_o), d(Vector(_o, _e)), tmax(_tmax) {}
};

struct Hit
{
    float t;            // p(t)= o + td, position du point d'intersection sur le rayon
    float u, v;         // p(u, v), position du point d'intersection sur le triangle
    int triangle_id;    // indice du triangle dans le mesh

    Hit( ) : t(FLT_MAX), u(), v(), triangle_id(-1) {}
    Hit( const float _t, const float _u, const float _v, const int _id ) : t(_t), u(_u), v(_v), triangle_id(_id) {}
    operator bool ( ) { return (triangle_id != -1); }
};

struct Triangle
{
    Point p;            // sommet a du triangle
    Vector e1, e2;      // aretes ab, ac du triangle
    int id;

    Triangle( const TriangleData& data, const int _id ) : p(data.a), e1(Vector(data.a, data.b)), e2(Vector(data.a, data.c)), id(_id) {}

    /* calcule l'intersection ray/triangle
        cf "fast, minimum storage ray-triangle intersection"

        renvoie faux s'il n'y a pas d'intersection valide (une intersection peut exister mais peut ne pas se trouver dans l'intervalle [0 tmax] du rayon.)
        renvoie vrai + les coordonnees barycentriques (u, v) du point d'intersection + sa position le long du rayon (t).
        convention barycentrique : p(u, v)= (1 - u - v) * a + u * b + v * c
    */
    Hit intersect( const Ray &ray, const float tmax ) const
    {
        Vector pvec= cross(ray.d, e2);
        float det= dot(e1, pvec);

        float inv_det= 1 / det;
        Vector tvec(p, ray.o);

        float u= dot(tvec, pvec) * inv_det;
        if(u < 0 || u > 1) return Hit();

        Vector qvec= cross(tvec, e1);
        float v= dot(ray.d, qvec) * inv_det;
        if(v < 0 || u + v > 1) return Hit();

        float t= dot(e2, qvec) * inv_det;
        if(t > tmax || t < 0) return Hit();

        return Hit(t, u, v, id);           // p(u, v)= (1 - u - v) * a + u * b + v * c
    }
};


Vector normal( const Mesh& mesh, const Hit& hit )
{
    // recuperer le triangle complet dans le mesh
    const TriangleData& data= mesh.triangle(hit.triangle_id);
    // interpoler la normale avec les coordonnées barycentriques du point d'intersection
    float w= 1 - hit.u - hit.v;
    Vector n= w * Vector(data.na) + hit.u * Vector(data.nb) + hit.v * Vector(data.nc);
    return normalize(n);
}

Color diffuse_color( const Mesh& mesh, const Hit& hit )
{
    const Material& material= mesh.triangle_material(hit.triangle_id);
    return material.diffuse;
}

struct Source
{
    Point s;
    Color emission;
};

Hit intersect(const Ray& ray, const float rtmax, const std::vector<Triangle>& triangles) {
    // calculer les intersections avec tous les triangles
    Hit hit;
    float tmax = rtmax;
    for (int i = 0; i < int(triangles.size()); i++)
    {
        if (Hit h = triangles[i].intersect(ray, tmax)) {
            // ne conserve que l'intersection la plus proche de l'origine du rayon
            assert(h.t > 0);
            hit = h;
            tmax = h.t;
        }
    }
    return hit;
}
int main( const int argc, const char **argv )
{
    const char *mesh_filename= "../data/cornell.obj";
    if(argc > 1)
        mesh_filename= argv[1];

    const char *orbiter_filename= "../data/cornell_orbiter.txt";
    if(argc > 2)
        orbiter_filename= argv[2];

    Orbiter camera;
    if(camera.read_orbiter(orbiter_filename) < 0)
        return 1;

    Mesh mesh= read_mesh(mesh_filename);

    // recupere les triangles
    std::vector<Triangle> triangles;
    {
        int n= mesh.triangle_count();
        for(int i= 0; i < n; i++)
            triangles.emplace_back(mesh.triangle(i), i);
    }

    // recupere les sources
    std::vector<Source> sources;
    {
        int n= mesh.triangle_count();
        for(int i= 0; i < n; i++)
        {
            const Material& material= mesh.triangle_material(i);
            if(material.emission.r + material.emission.g + material.emission.b > 0)
            {
                // utiliser le centre du triangle comme source de lumière
                const TriangleData& data= mesh.triangle(i);
                Point p= (Point(data.a) + Point(data.b) + Point(data.c)) / 3;

                sources.push_back( { p, material.emission } );
            }
        }

        printf("%d sources\n", int(sources.size()));
        assert(sources.size() > 0);
    }

    Image image(1024, 768);

    // recupere les transformations
    camera.projection(image.width(), image.height(), 45);
    Transform model= Identity();
    Transform view= camera.view();
    Transform projection= camera.projection();
    Transform viewport= camera.viewport();
    Transform inv= Inverse(viewport * projection * view * model);

    auto start= std::chrono::high_resolution_clock::now();

    // c'est parti, parcours tous les pixels de l'image
    for (int y = 0; y < image.height(); y++) {
        std::random_device hwseed;
        std::default_random_engine rng(hwseed());
        std::uniform_real_distribution<float> uniform(0, 1);

        float u = uniform(rng);

        for (int x = 0; x < image.width(); x++)
        {
            // generer le rayon
            Point origine = inv(Point(x + .5f, y + .5f, 0));
            Point extremite = inv(Point(x + .5f, y + .5f, 1));
            Ray ray(origine, extremite);

            Color color;
            const int N = 64;
            if (Hit hit = intersect(ray, ray.tmax, triangles))
            {
                Point p = ray.o + hit.t * ray.d;
                Vector pn = normal(mesh, hit);
                const Material& material = mesh.triangle_material(hit.triangle_id);
                Color diffuse = material.diffuse / float(M_PI);
                for (int i = 0; i < N; i++)
                {
                    float u1 = uniform(rng);
                    float u2 = uniform(rng);
                    Vector l;
                    float pdf;
                    {
                        float cos_theta = u1;
                        float sin_theta = std::sqrt(1 - cos_theta * cos_theta);
                        float phi = float(2 * M_PI) * u2;
                        l = Vector(std::cos(phi) * sin_theta, cos_theta, sin_theta * std::sin(phi));
                        pdf = 1 / float(2 * M_PI);
                    }

                    float V = 1;
                    Point p2 = p + 0.0001 * pn;
                    Ray ray(p2, Point(l));    //???
                    if (intersect(ray, ray.tmax, triangles))
                        V = 0;
                    float cos_theta = dot(pn, l);
                    color = color + diffuse * V * cos_theta / pdf;
                }
                color = color / float(N);
                image(x, y) = Color(color, 1);
            }

        }
        std::cout << y << std::endl;
    }


    auto stop= std::chrono::high_resolution_clock::now();
    int cpu= std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    printf("%dms\n", cpu);

    write_image(image, "render.png");
    write_image_hdr(image, "shadow.hdr");
    return 0;
}