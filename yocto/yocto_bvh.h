//
// YOCTO_BVH: ray-intersection and closet-point routines supporting points,
// lines, triangles accelerated by a two-level bounding volume
// hierarchy (BVH)
//

//
// USAGE:
//
// 0. include this file (more compilation options below)
// 1. create the scene bvh
//      bvh = yb_init_scene_bvh(mnshapes, heuristic)
// 2. for each shape, add shape data
//     for(int i = 0; i < nshapes; i ++)
//         yb_set_shape_bvh(bvh, pass shape data)
// 3. build the bvh with yb_build_scene_bvh
// 4.a. perform ray-interseciton tests
//     - use yb_intersect_bvh if you want to know the hit point
//         hit = yb_intersect_bvh(bvh, ray data, out primitive intersection)
//     - use yb_hit_bvh if you only need to know whether there is a hit
//         hit = yb_hit_bvh(bvh, ray data)
// 4.b. perform closet-point tests
//     - use yb_neightbor_bvh to get the closet element to a point
//       bounded by a radius
//         hit = yb_neightbor_bvh(bvh, pos, radius, out primitive)
// 5. use yb_interpolate_vertex to get interpolated vertex values from the
//    intersection data
//    yb_interpolate_vertex(intersection data, shape data, out interpolated val)
// 6. use yb_refit_bvh to recompute the bvh bounds if objects move (you should
//    rebuild the bvh for large changes)
// 7. cleanup with yb_free_bvh
//   - you have to do this for each shape bvh and the scene bvh
//   yb_free_bvh(bvh)
//   for(int i = 0; i < nshapes; i ++) yb_free_bvh(shape_bvhs[i])
//
// The interface for each function is described in details in the interface
// section of this file.
//
// You can also just use one untransformed shape by calling yb_init_shape_bvh
// and yb_build_shape_bvh, etc.
//
// Shapes are indexed meshes and are described by their
// number of elements, an array of vertex indices,
// the primitive type (points, lines, triangles),
// an array of vertex positions, and an array of vertex radius
// (for points and lines).

//
// COMPILATION:
//
// The library has two APIs. The default one is usable directly from C++,
// while the other is usable from both C and C++. To use from C, compile the
// library into a static or dynamic lib using a C++ and then include/link from
// C using the C API.
//
// All functions in this library are inlined by default for ease of use in C++.
// To use the library as a .h/.cpp pair do the following:
// - to use as a .h, just #define YGL_DECLARATION before including this file
// - to build as a .cpp, just #define YGL_IMPLEMENTATION before including this
// file into only one file that you can either link directly or pack as a lib.
//
// This file depends on yocto_math.h.
//

//
// HISTORY:
// - v 0.1: C++ implementation
// - v 0.0: initial release in C99
//

//
// LICENSE:
//
// Copyright (c) 2016 Fabio Pellacini
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef _YB_H_
#define _YB_H_

// compilation options
#ifdef __cplusplus
#ifndef YGL_DECLARATION
#define YGL_API inline
#define YGLC_API inline
#else
#define YGL_API
#define YGLC_API extern "C"
#endif
#include "yocto_math.h"
#endif

#ifndef __cplusplus
#define YGLC_API extern
#include <stdbool.h>
#endif

// -----------------------------------------------------------------------------
// C++ INTERFACE
// -----------------------------------------------------------------------------

#ifdef __cplusplus

//
// Element types for shapes
//
enum {
    yb_etype_point = 1,     // points
    yb_etype_line = 2,      // lines
    yb_etype_triangle = 3,  // triangles
};

//
// Heuristic stratigy for bvh build
//
enum {
    yb_htype_default = 0,  // default strategy (use this for ray-casting)
    yb_htype_equalnum,     // balanced binary tree
    yb_htype_sah,          // surface area heuristic
    yb_htype_max           // total number of strategies
};

//
// Scene BVH data structure. Implementation details hidden.
//
struct yb_scene_bvh;

//
// Shape BVH data structure. Implementation details hidden.
//
struct yb_shape_bvh;

//
// Create a BVH for a collection of transformed shapes (scene).
//
// Shapes' BVHs can be transformed with transformations matrices. The code
// supports only affine transforms.
//
// Parameters:
// - nshapes: number of shape bvhs
// - heuristic: heristic use to build the bvh (0 for deafult)
//
YGL_API yb_scene_bvh* yb_init_scene_bvh(int nshapes, int heuristic);

//
// Sets shape data for scene BVH. Equivalent to calling init_shape_bvh.
//
// Parameters:
// - bvh: scene bvh
// - sid: shape id
// - xform: shape transform
// - nelems: number of elements
// - elem: array of vertex indices
// - etype: shape element type (as per previous enum)
// - pos: array of 3D vertex positions
// - radius: array of vertex radius
// - heuristic: heristic use to build the bvh (0 for deafult)
//
YGL_API void yb_set_shape_bvh(yb_scene_bvh* bvh, int sid,
                              const ym_affine3f& xform, int nelems,
                              const int* elem, int etype, int nverts,
                              const ym_vec3f* pos, const float* radius,
                              int heuristic);

//
// Builds a shape BVH.
//
// Parameters:
// - bvh: bvh to build
// - heuristic: heristic use to build the bvh (0 for deafult)
//
YGL_API void yb_build_scene_bvh(yb_scene_bvh* bvh);

//
// Clears BVH memory.
//
// This will only clear the memory for the given bvh. For scene bvhs, you are
// still responsible for clearing all shape bvhs.
//
// Parameters:
// - bvh: bvh to clean
//
YGL_API void yb_free_scene_bvh(yb_scene_bvh* bvh);

//
// Create a BVH for a given shape.
//
// Shapes are indexed meshes with 1, 2, 3, 4 indices respectively for points,
// lines, triangles and quads. Vertices have positions and radius, the latter
// required only for points and lines.
//
// Parameters:
// - nelems: number of elements
// - elem: array of vertex indices
// - etype: shape element type (as per previous enum)
// - pos: array of 3D vertex positions
// - radius: array of vertex radius
// - heuristic: heristic use to build the bvh (0 for deafult)
//
// Return:
// - shape bvh
//
YGL_API yb_shape_bvh* yb_init_shape_bvh(int nelems, const int* elem, int etype,
                                        int nverts, const ym_vec3f* pos,
                                        const float* radius, int heuristic);

//
// Builds a shape BVH.
//
// Parameters:
// - bvh: bvh to build
// - heuristic: heristic use to build the bvh (0 for deafult)
//
YGL_API void yb_build_shape_bvh(yb_shape_bvh* bvh);

//
// Clears BVH memory.
//
// This will only clear the memory for the given bvh. For scene bvhs, you are
// still responsible for clearing all shape bvhs.
//
// Parameters:
// - bvh: bvh to clean
//
YGL_API void yb_free_shape_bvh(yb_shape_bvh* bvh);

//
// Refit the bounds of each shape for moving objects. Use this only to avoid
// a rebuild, but note that queries are likely slow if objects move a lot.
//
// Parameters:
// - bvh: bvh to refit
// - bvhs: array of pointers to shape bvhs
//
YGL_API void yb_refit_scene_bvh(yb_scene_bvh* bvh, const ym_affine3f* xforms);

//
// Intersect the scene with a ray finding the closest intersection.
//
// Parameters:
// - bvh: bvh to intersect (scene or shape)
// - ray_o: ray origin
// - ray_d: ray direction
// - ray_o: minimal distance along the ray to consider (0 for all)
// - ray_o: maximal distance along the ray to consider (HUGE_VALF for all)
//
// Out Parameters:
// - ray_t: hit distance
// - sid: hit shape index
// - eid: hit element index
// - euv: hit element parameters
//
// Return:
// - whether we intersect or not
//
YGL_API bool yb_intersect_scene_bvh(const yb_scene_bvh* bvh,
                                    const ym_ray3f& ray, float* ray_t, int* sid,
                                    int* eid, ym_vec2f* euv);

//
// Intersect the scene with a ray finding any intersection.
//
// Parameters:
// - bvh: bvh to intersect (scene or shape)
// - ray_o: ray origin
// - ray_d: ray direction
// - ray_o: minimal distance along the ray to consider (0 for all)
// - ray_o: maximal distance along the ray to consider (HUGE_VALF for all)
//
// Return:
// - whether we intersect or not
//
YGL_API bool yb_hit_scene_bvh(const yb_scene_bvh* bvh, const ym_ray3f& ray);

//
// Returns a list of shape pairs that can possibly overlap by checking only they
// axis aligned bouds. This is only a conservative check useful for collision
// detection.
//
// Parameters:
// - bvh: scene bvh
// - exclude_self: whether to exlude self intersections
// - overlap_cb: function called for each overlap
//
// Return:
// - number of overlaps
//
// Notes:
// - intersections are duplicated, so if (i,j) overlaps than both (i,j) and
// (j,i)
//   will be present; this makes it easier to apply asymmetric checks
// - to remove symmetric checks, just skip all pairs with i > j
//
YGL_API int yb_overlap_shape_bounds(const yb_scene_bvh* bvh, bool exclude_self,
                                    void* overlap_ctx,
                                    void (*overlap_cb)(const ym_vec2i& v));

//
// Returns a list of shape pairs that can possibly overlap by checking only they
// axis aligned bouds. This is only a conservative check useful for collision
// detection.
//
// Parameters:
// - bvh: scene bvh
// - exclude_self: whether to exlude self intersections
// - overlaps: possible shape overlaps
//
// Return:
// - number of overlaps
//
// Notes: See above.
//
YGL_API int yb_overlap_shape_bounds(const yb_scene_bvh* bvh, bool exclude_self,
                                    ym_vector<ym_vec2i>* overlaps);

//
// Finds the closest element to a point wihtin a given radius.
//
// Parameters:
// - bvh: bvh to intersect (scene or shape)
// - pt: ray origin
// - max_dist: max point distance
// - req_sid: required shape (-1 for all)
//
// Out Parameters:
// - dist: distance
// - sid: shape index
// - eid: element index
// - euv: element parameters
//
// Return:
// - whether we intersect or not
//
YGL_API bool yb_neighbour_scene_bvh(const yb_scene_bvh* bvh, const ym_vec3f& pt,
                                    float max_dist, int req_sid, float* dist,
                                    int* sid, int* eid, ym_vec2f* euv);

//
// Interpolates a vertex property from the given intersection data. Uses
// linear interpolation for lines, baricentric for triangles and quads
// (to correctly treat as two triangles) and copies values for points.
//
// Parameters:
// - elem: array of vertex indices
// - etype: shape element type (as per previous enum)
// - eid: hit element index
// - euv: hit element parameters
// - vsize: number of floats in the vertex property
// - vert: shape vertex data (contigous array of vsize-sized values)
//
// Out Parameters:
// - v: interpolated vertex (array of vsize size)
//
YGL_API void yb_interpolate_vert(const int* elem, int etype, int eid,
                                 const ym_vec2f& euv, int vsize,
                                 const float* vert, float* v);

//
// Print stats for the given BVH. Mostly useful for debugging or performance
// analysis. Ignore for general purpose.
//
// Parameters:
// - bvh: bvh to print stats for
// - whether to print the tree structure
//
YGL_API void yb_print_scene_bvh_stats(const yb_scene_bvh* bvh, bool print_tree);

//
// Print stats for the given BVH. Mostly useful for debugging or performance
// analysis. Ignore for general purpose.
//
// Parameters:
// - bvh: bvh to print stats for
// - whether to print the tree structure
//
YGL_API void yb_print_shape_bvh_stats(const yb_shape_bvh* bvh, bool print_tree);

#endif  // __cplusplus

// -----------------------------------------------------------------------------
// C/C++ INTERFACE
// -----------------------------------------------------------------------------

//
// Scene BVH data structure. Implementation details hidden.
//
typedef struct yb_scene_bvh yb_scene_bvh;

//
// Shape BVH data structure. Implementation details hidden.
//
typedef struct yb_shape_bvh yb_shape_bvh;

//
// Create a BVH for a given shape.
//
// Shapes are indexed meshes with 1, 2, 3, 4 indices respectively for points,
// lines, triangles and quads. Vertices have positions and radius, the latter
// required only for points and lines.
//
// Parameters:
// - nelems: number of elements
// - elem: array of vertex indices
// - etype: shape element type (as per previous enum)
// - pos: array of 3D vertex positions
// - radius: array of vertex radius
// - heuristic: heristic use to build the bvh (0 for deafult)
//
// Return:
// - shape bvh
//
YGLC_API yb_shape_bvh* ybc_init_shape_bvh(int nelems, int* elem, int etype,
                                          int nverts, float* pos, float* radius,
                                          int heuristic);

//
// Create a BVH for a collection of transformed shapes (scene).
//
// Shapes' BVHs can be transformed with transformations matrices. The code
// supports only affine transforms.
//
// Parameters:
// - nbvhs: number of shape bvhs
// - heuristic: heristic use to build the bvh (0 for deafult)
//
YGLC_API yb_scene_bvh* ybc_init_scene_bvh(int nbvhs, int heuristic);

//
// Sets shape data for scene BVH. Equivalent to calling init_shape_bvh.
//
// Parameters:
// - bvh: scene bvh
// - sid: shape id
// - xform: shape transform
// - nelems: number of elements
// - elem: array of vertex indices
// - etype: shape element type (as per previous enum)
// - pos: array of 3D vertex positions
// - radius: array of vertex radius
// - heuristic: heristic use to build the bvh (0 for deafult)
//
YGL_API void ybc_set_shape_bvh(yb_scene_bvh* bvh, int sid,
                               const float xform[16], int nelems,
                               const int* elem, int etype, int nverts,
                               const ym_vec3f* pos, const float* radius,
                               int heuristic);

//
// Clears BVH memory.
//
// This will only clear the memory for the given bvh. For scene bvhs, you are
// still responsible for clearing all shape bvhs.
//
// Parameters:
// - bvh: bvh to clean
//
YGLC_API void ybc_free_scene_bvh(yb_scene_bvh* bvh);

//
// Clears BVH memory.
//
// This will only clear the memory for the given bvh. For scene bvhs, you are
// still responsible for clearing all shape bvhs.
//
// Parameters:
// - bvh: bvh to clean
//
YGLC_API void ybc_free_shape_bvh(yb_shape_bvh* bvh);

//
// Refit the bounds of each shape for moving objects. Use this only to avoid
// a rebuild, but note that queries are likely slow if objects move a lot.
//
// Parameters:
// - bvh: bvh to refit
// - bvhs: array of pointers to shape bvhs
//
YGLC_API void ybc_refit_scene_bvh(yb_scene_bvh* bvh, const float* xforms[16]);

//
// Intersect the scene with a ray finding the closest intersection.
//
// Parameters:
// - bvh: bvh to intersect (scene or shape)
// - ray_o: ray origin
// - ray_d: ray direction
// - ray_o: minimal distance along the ray to consider (0 for all)
// - ray_o: maximal distance along the ray to consider (HUGE_VALF for all)
//
// Out Parameters:
// - ray_t: hit distance
// - sid: hit shape index
// - eid: hit element index
// - euv: hit element parameters
//
// Return:
// - whether we intersect or not
//
YGLC_API bool ybc_intersect_scene_bvh(const yb_scene_bvh* bvh,
                                      const float ray_o[3],
                                      const float ray_d[3], float ray_tmin,
                                      float ray_tmax, float* ray_t, int* sid,
                                      int* eid, float euv[2]);

//
// Intersect the scene with a ray finding any intersection.
//
// Parameters:
// - bvh: bvh to intersect (scene or shape)
// - ray_o: ray origin
// - ray_d: ray direction
// - ray_o: minimal distance along the ray to consider (0 for all)
// - ray_o: maximal distance along the ray to consider (HUGE_VALF for all)
//
// Return:
// - whether we intersect or not
//
YGLC_API bool ybc_hit_scene_bvh(const yb_scene_bvh* bvh, const float ray_o[3],
                                const float ray_d[3], float ray_tmin,
                                float ray_tmax);

//
// Returns a list of shape pairs that can possibly overlap by checking only they
// axis aligned bouds. This is only a conservative check useful for collision
// detection.
//
// Parameters:
// - bvh: scene bvh
// - exclude_self: whether to exlude self intersections
// - overlaps: pointer to an array of shape pairs (consecutive ints)
// - ocapacity: capacity of the overlap array to avoid reallocations
//
// Return:
// - number of intersections
//
// Notes:
// - intersections are duplicated, so if (i,j) overlaps than both (i,j) and
// (j,i)
//   will be present; this makes it easier to apply asymmetric checks
// - to remove symmetric checks, just skip all pairs with i > j
//
YGLC_API int ybc_overlap_shape_bounds(const yb_scene_bvh* bvh,
                                      bool exclude_self, void* overlap_ctx,
                                      void (*overlap_cb)(void* ctx,
                                                         const int ij[2]));

//
// Finds the closest element to a point wihtin a given radius.
//
// Parameters:
// - bvh: bvh to intersect (scene or shape)
// - pt: ray origin
// - max_dist: max point distance
// - req_sid: required shape (-1 for all)
//
// Out Parameters:
// - dist: distance
// - sid: shape index
// - eid: element index
// - euv: element parameters
//
// Return:
// - whether we intersect or not
//
YGLC_API bool ybc_neighbour_scene_bvh(const yb_scene_bvh* bvh,
                                      const float pt[3], float max_dist,
                                      int req_sid, float* dist, int* sid,
                                      int* eid, float euv[2]);

//
// Interpolates a vertex property from the given intersection data. Uses
// linear interpolation for lines, baricentric for triangles and quads
// (to correctly treat as two triangles) and copies values for points.
//
// Parameters:
// - elem: array of vertex indices
// - etype: shape element type (as per previous enum)
// - eid: hit element index
// - euv: hit element parameters
// - vsize: number of floats in the vertex property
// - vert: shape vertex data (contigous array of vsize-sized values)
//
// Out Parameters:
// - v: interpolated vertex (array of vsize size)
//
YGLC_API void ybc_interpolate_vert(const int* elem, int etype, int eid,
                                   const float euv[2], int vsize,
                                   const float* vert, float* v);

//
// Print stats for the given BVH. Mostly useful for debugging or performance
// analysis. Ignore for general purpose.
//
// Parameters:
// - bvh: bvh to print stats for
// - whether to print the tree structure
//
YGLC_API void ybc_print_scene_bvh_stats(const yb_scene_bvh* bvh,
                                        bool print_tree);

//
// Print stats for the given BVH. Mostly useful for debugging or performance
// analysis. Ignore for general purpose.
//
// Parameters:
// - bvh: bvh to print stats for
// - whether to print the tree structure
//
YGLC_API void ybc_print_shape_bvh_stats(const yb_shape_bvh* bvh,
                                        bool print_tree);

// -----------------------------------------------------------------------------
// IMPLEMENTATION
// -----------------------------------------------------------------------------

// handles compilation options
#if defined(__cplusplus) &&                                                    \
    (!defined(YGL_DECLARATION) || defined(YGL_IMPLEMENTATION))

#include <cstdio>

// -----------------------------------------------------------------------------
// MATH FUNCTIONS SUPPORT
// -----------------------------------------------------------------------------

//
// Transforms a bbox via an affine matrix
//
static inline ym_range3f yb__transform_bbox(const ym_affine3f& a,
                                            const ym_range3f& bbox) {
    ym_vec3f corners[8] = {
        {bbox.min.x, bbox.min.y, bbox.min.z},
        {bbox.min.x, bbox.min.y, bbox.max.z},
        {bbox.min.x, bbox.max.y, bbox.min.z},
        {bbox.min.x, bbox.max.y, bbox.max.z},
        {bbox.max.x, bbox.min.y, bbox.min.z},
        {bbox.max.x, bbox.min.y, bbox.max.z},
        {bbox.max.x, bbox.max.y, bbox.min.z},
        {bbox.max.x, bbox.max.y, bbox.max.z},
    };
    ym_range3f xformed = ym_invalid_range3f;
    for (int j = 0; j < 8; j++) {
        xformed = ym_rexpand(xformed, ym_transform_point(a, corners[j]));
    }
    return xformed;
}

//
// Compute the point on the ray ray_o, ray_d at distance t
//
static inline ym_vec3f yb__eval_ray(const ym_vec3f ray_o, const ym_vec3f ray_d,
                                    float t) {
    return {ray_o.x + ray_d.x * t, ray_o.y + ray_d.y * t,
            ray_o.z + ray_d.z * t};
}

// -----------------------------------------------------------------------------
// ELEMENT-WISE INTERSECTION FUNCTIONS
// -----------------------------------------------------------------------------

//
// Intersect a ray with a point (approximate)
//
// Parameters:
// - ray: ray origin and direction, parameter min, max range
// - p: point position
// - r: point radius
//
// Out Parameters:
// - ray_t: ray parameter at the intersection point
// - euv: primitive uv ( {0,0} for points )
//
// Returns:
// - whether the intersection occurred
//
// Iplementation Notes:
// - out parameters and only writtent o if an intersection occurs
// - algorithm finds the closest point on the ray segment to the point and
//    test their distance with the point radius
// - based on http://geomalgorithms.com/a02-lines.html.
//
static inline bool yb__intersect_point(const ym_ray3f& ray, const ym_vec3f& p,
                                       float r, float* ray_t, ym_vec2f* euv) {
    // find parameter for line-point minimum distance
    ym_vec3f w = p - ray.o;
    float t = ym_dot(w, ray.d) / ym_dot(ray.d, ray.d);

    // exit if not within bounds
    if (t < ray.tmin || t > ray.tmax) return false;

    // test for line-point distance vs point radius
    ym_vec3f rp = ray.eval(t);
    ym_vec3f prp = p - rp;
    if (ym_dot(prp, prp) > r * r) return false;

    // intersection occurred: set params and exit
    *ray_t = t;
    *euv = ym_vec2f{0, 0};

    return true;
}

//
// Intersect a ray with a line
//
// Parameters:
// - ray: ray origin and direction, parameter min, max range
// - v0, v1: line segment points
// - r0, r1: line segment radia
//
// Out Parameters:
// - ray_t: ray parameter at the intersection point
// - euv: euv[0] is the line parameter at the intersection ( euv[1] is zero )
//
// Returns:
// - whether the intersection occurred
//
// Notes:
// - out parameters and only writtent o if an intersection occurs
// - algorithm find the closest points on line and ray segment and test
//   their distance with the line radius at that location
// - based on http://geomalgorithms.com/a05-intersect-1.html
// - based on http://geomalgorithms.com/a07-distance.html#
//     dist3D_Segment_to_Segment
//
static inline bool yb__intersect_line(const ym_ray3f& ray, const ym_vec3f& v0,
                                      const ym_vec3f& v1, float r0, float r1,
                                      float* ray_t, ym_vec2f* euv) {
    // setup intersection params
    ym_vec3f u = ray.d;
    ym_vec3f v = v1 - v0;
    ym_vec3f w = ray.o - v0;

    // compute values to solve a linear system
    float a = ym_dot(u, u);
    float b = ym_dot(u, v);
    float c = ym_dot(v, v);
    float d = ym_dot(u, w);
    float e = ym_dot(v, w);
    float det = a * c - b * b;

    // check determinant and exit if lines are parallel
    // (could use EPSILONS if desired)
    if (det == 0) return false;

    // compute parameters on both ray and segment
    float t = (b * e - c * d) / det;
    float s = (a * e - b * d) / det;

    // exit if not within bounds
    if (t < ray.tmin || t > ray.tmax) return false;

    // clamp segment param to segment corners
    s = ym_clamp(s, 0, 1);

    // compute segment-segment distance on the closest points
    ym_vec3f p0 = yb__eval_ray(ray.o, ray.d, t);
    ym_vec3f p1 = yb__eval_ray(v0, v1 - v0, s);
    ym_vec3f p01 = p0 - p1;

    // check with the line radius at the same point
    float r = r0 * (1 - s) + r1 * s;
    if (ym_dot(p01, p01) > r * r) return false;

    // intersection occurred: set params and exit
    *ray_t = t;
    *euv = ym_vec2f{s, 0};

    return true;
}

//
// Intersect a ray with a triangle
//
// Parameters:
// - ray: ray origin and direction, parameter min, max range
// - v0, v1, v2: triangle vertices
//
// Out Parameters:
// - ray_t: ray parameter at the intersection point
// - euv: baricentric coordinates of the intersection
//
// Returns:
// - whether the intersection occurred
//
// Notes:
// - out parameters and only writtent o if an intersection occurs
// - algorithm based on Muller-Trombone intersection test
//
static inline bool yb__intersect_triangle(const ym_ray3f& ray,
                                          const ym_vec3f& v0,
                                          const ym_vec3f& v1,
                                          const ym_vec3f& v2, float* ray_t,
                                          ym_vec2f* euv) {
    // compute triangle edges
    ym_vec3f edge1 = v1 - v0;
    ym_vec3f edge2 = v2 - v0;

    // compute determinant to solve a linear system
    ym_vec3f pvec = ym_cross(ray.d, edge2);
    float det = ym_dot(edge1, pvec);

    // check determinant and exit if triangle and ray are parallel
    // (could use EPSILONS if desired)
    if (det == 0) return false;
    float inv_det = 1.0f / det;

    // compute and check first bricentric coordinated
    ym_vec3f tvec = ray.o - v0;
    float u = ym_dot(tvec, pvec) * inv_det;
    if (u < 0 || u > 1) return false;

    // compute and check second bricentric coordinated
    ym_vec3f qvec = ym_cross(tvec, edge1);
    float v = ym_dot(ray.d, qvec) * inv_det;
    if (v < 0.0 || u + v > 1.0) return false;

    // compute and check ray parameter
    float t = ym_dot(edge2, qvec) * inv_det;
    if (t < ray.tmin || t > ray.tmax) return false;

    // intersection occurred: set params and exit
    *ray_t = t;
    *euv = ym_vec2f{u, v};

    return true;
}

//
// Intersect a ray with a quad, represented as two triangles
//
// Parameters:
// - ray: ray origin and direction, parameter min, max range
// - v0, v1, v2, v3: quad vertices
//
// Out Parameters:
// - ray_t: ray parameter at the intersection point
// - euv: baricentric coordinates (as described above)
//
// Returns:
// - whether the intersection occurred
//
// Notes:
// - out parameters and only writtent o if an intersection occurs
// - might become deprecated in the future
//
static inline bool yb__intersect_quad(const ym_ray3f& ray, const ym_vec3f& v0,
                                      const ym_vec3f& v1, const ym_vec3f& v2,
                                      const ym_vec3f& v3, float* ray_t,
                                      ym_vec2f* euv) {
    ym_ray3f r = ray;
    // test first triangle
    bool hit = false;
    if (yb__intersect_triangle(r, v0, v1, v3, ray_t, euv)) {
        hit = true;
        r.tmax = *ray_t;
    }

    // test second triangle
    if (yb__intersect_triangle(r, v2, v3, v1, ray_t, euv)) {
        hit = true;
        // flip coordinates to map to [0,0]x[1,1]
        *euv = ym_vec2f{1 - euv->x, 1 - euv->y};
    }

    return hit;
}

//
// Intersect a ray with a axis-aligned bounding box
//
// Parameters:
// - ray_o, ray_d: ray origin and direction
// - ray_tmin, ray_tmax: ray parameter min, max range
// - bbox_min, bbox_max: bounding box min/max bounds
//
// Returns:
// - whether the intersection occurred
//
static inline bool yb__intersect_check_bbox(const ym_ray3f& ray,
                                            const ym_range3f& bbox) {
    // set up convenient pointers for looping over axes
    const float* _ray_o = &ray.o.x;
    const float* _ray_d = &ray.d.x;
    const float* _bbox_min = &bbox.min.x;
    const float* _bbox_max = &bbox.max.x;

    float tmin = ray.tmin, tmax = ray.tmax;

    // for each axis, clip intersection against the bounding planes
    for (int i = 0; i < 3; i++) {
        // determine intersection ranges
        float invd = 1.0f / _ray_d[i];
        float t0 = (_bbox_min[i] - _ray_o[i]) * invd;
        float t1 = (_bbox_max[i] - _ray_o[i]) * invd;
        // flip based on range directions
        if (invd < 0.0f) {
            float a = t0;
            t0 = t1;
            t1 = a;
        }
        // clip intersection
        tmin = t0 > tmin ? t0 : tmin;
        tmax = t1 < tmax ? t1 : tmax;
        // if intersection is empty, exit
        if (tmin > tmax) return false;
    }

    // passed all planes, then intersection occurred
    return true;
}

// -----------------------------------------------------------------------------
// ELEMENT-WISE DISTANCE FUNCTIONS
// -----------------------------------------------------------------------------

// TODO: documentation
static inline bool yb__distance_point(const ym_vec3f& pos, float dist_max,
                                      const ym_vec3f& p, float r, float* dist,
                                      ym_vec2f* euv) {
    float d2 = ym_distsqr(pos, p);
    if (d2 > (dist_max + r) * (dist_max + r)) return false;
    *dist = sqrtf(d2);
    *euv = ym_vec2f{0, 0};
    return true;
}

// TODO: documentation
static inline float yb__closestuv_line(const ym_vec3f& pos, const ym_vec3f& v0,
                                       const ym_vec3f& v1) {
    ym_vec3f ab = v1 - v0;
    float d = ym_dot(ab, ab);
    // Project c onto ab, computing parameterized position d(t) = a + t*(b – a)
    float u = ym_dot(pos - v0, ab) / d;
    u = ym_clamp(u, 0, 1);
    return u;
}

// TODO: documentation
static inline bool yb__distance_line(const ym_vec3f& pos, float dist_max,
                                     const ym_vec3f& v0, const ym_vec3f& v1,
                                     float r0, float r1, float* dist,
                                     ym_vec2f* euv) {
    float u = yb__closestuv_line(pos, v0, v1);
    // Compute projected position from the clamped t d = a + t * ab;
    ym_vec3f p = ym_lerp(v0, v1, u);
    float r = ym_lerp(r0, r1, u);
    float d2 = ym_distsqr(pos, p);
    // check distance
    if (d2 > (dist_max + r) * (dist_max + r)) return false;
    // done
    *dist = sqrtf(d2);
    *euv = ym_vec2f{u, 0};
    return true;
}

// TODO: documentation
// this is a complicated test -> I probably prefer to use a sequence of test
// (triangle body, and 3 edges)
static inline ym_vec2f yb__closestuv_triangle(const ym_vec3f& pos,
                                              const ym_vec3f& v0,
                                              const ym_vec3f& v1,
                                              const ym_vec3f& v2) {
    ym_vec3f ab = v1 - v0;
    ym_vec3f ac = v2 - v0;
    ym_vec3f ap = pos - v0;

    float d1 = ym_dot(ab, ap);
    float d2 = ym_dot(ac, ap);

    // corner and edge cases
    if (d1 <= 0 && d2 <= 0) return ym_vec2f{0, 0};

    ym_vec3f bp = pos - v1;
    float d3 = ym_dot(ab, bp);
    float d4 = ym_dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return ym_vec2f{1, 0};

    float vc = d1 * d4 - d3 * d2;
    if ((vc <= 0) && (d1 >= 0) && (d3 <= 0)) return ym_vec2f{d1 / (d1 - d3), 0};

    ym_vec3f cp = pos - v2;
    float d5 = ym_dot(ab, cp);
    float d6 = ym_dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return ym_vec2f{0, 1};

    float vb = d5 * d2 - d1 * d6;
    if ((vb <= 0) && (d2 >= 0) && (d6 <= 0)) return ym_vec2f{0, d2 / (d2 - d6)};

    float va = d3 * d6 - d5 * d4;
    if ((va <= 0) && (d4 - d3 >= 0) && (d5 - d6 >= 0)) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return ym_vec2f{1 - w, w};
    }

    // face case
    float denom = 1 / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return ym_vec2f{v, w};
}

// TODO: documentation
static inline bool yb__distance_triangle(const ym_vec3f& pos, float dist_max,
                                         const ym_vec3f& v0, const ym_vec3f& v1,
                                         const ym_vec3f& v2, float r0, float r1,
                                         float r2, float* dist, ym_vec2f* euv) {
    ym_vec2f uv = yb__closestuv_triangle(pos, v0, v1, v2);
    ym_vec3f p = ym_blerp(v0, v1, v2, uv.x, uv.y);
    float r = ym_blerp(r0, r1, r2, uv.x, uv.y);
    float dd = ym_distsqr(p, pos);
    if (dd > (dist_max + r) * (dist_max + r)) return false;
    *dist = sqrtf(dd);
    *euv = uv;
    return true;
}

// TODO: documentation
// TODO: radius
static inline bool yb__distance_quad(const ym_vec3f& pos, float dist_max,
                                     const ym_vec3f& v0, const ym_vec3f& v1,
                                     const ym_vec3f& v2, const ym_vec3f& v3,
                                     float r0, float r1, float r2, float r3,
                                     float* dist, ym_vec2f* euv) {
    // test first triangle
    bool hit = false;
    if (yb__distance_triangle(pos, dist_max, v0, v1, v3, r0, r1, r3, dist,
                              euv)) {
        hit = true;
        dist_max = *dist;
    }

    // test second triangle
    if (yb__distance_triangle(pos, dist_max, v2, v3, v1, r2, r3, r1, dist,
                              euv)) {
        hit = true;
        // flip coordinates to map to [0,0]x[1,1]
        *euv = ym_vec2f{1 - euv->x, 1 - euv->y};
    }

    return hit;
}

// TODO: documentation
static inline bool yb__distance_check_bbox(const ym_vec3f pos, float dist_max,
                                           const ym_vec3f bbox_min,
                                           const ym_vec3f bbox_max) {
    // set up convenient pointers for looping over axes
    const float* _pos = &pos.x;
    const float* _bbox_min = &bbox_min.x;
    const float* _bbox_max = &bbox_max.x;

    // computing distance
    float dd = 0.0f;
    // For each axis count any excess distance outside box extents
    for (int i = 0; i < 3; i++) {
        float v = _pos[i];
        if (v < _bbox_min[i]) dd += (_bbox_min[i] - v) * (_bbox_min[i] - v);
        if (v > _bbox_max[i]) dd += (v - _bbox_max[i]) * (v - _bbox_max[i]);
    }

    // check distance
    return dd < dist_max * dist_max;
}

// TODO: doc
static inline bool yb__overlap_bbox(const ym_vec3f bbox1_min,
                                    const ym_vec3f bbox1_max,
                                    const ym_vec3f bbox2_min,
                                    const ym_vec3f bbox2_max) {
    if (bbox1_max.x < bbox2_min.x || bbox1_min.x > bbox2_max.x) return false;
    if (bbox1_max.y < bbox2_min.y || bbox1_min.y > bbox2_max.y) return false;
    if (bbox1_max.z < bbox2_min.z || bbox1_min.z > bbox2_max.z) return false;
    return true;
}

// -----------------------------------------------------------------------------
// BVH DATA STRUCTURE
// -----------------------------------------------------------------------------

// number of primitives to avoid splitting on
#define YB__BVH_MINPRIMS 4

//
// Types of primitives contained in the entire bvh or its nodes.
//
enum {
    yb__ntype_internal = 0,  // node is internal (not leaf)
    yb__ntype_point = 1,     // points
    yb__ntype_line = 2,      // lines
    yb__ntype_triangle = 3,  // triangles
    yb__ntype_bvh = 9,       // pointers to other bvhs
};

//
// BVH tree node containing its bounds, indices to the BVH arrays of either
// sorted primitives or internal nodes, whether its a leaf or an internal node,
// and the split axis. Leaf and internal nodes are identical, except that
// indices refer to primitives for leaf nodes or other nodes for internal
// nodes. See yb_bvh for more details.
//
// Implemenetation Notes:
// - Padded to 32 bytes for cache fiendly access
//
struct yb__bvhn {
    ym_range3f bbox;  // bounding box
    uint32_t start;   // index to the first sorted primitive/node
    uint16_t count;   // number of primitives/nodes
    uint8_t isleaf;   // whether it is a leaf
    uint8_t axis;     // plit axis
};

//
// BVH tree, stored as a node array. The tree structure is encoded using array
// indices instead of pointers, both for speed but also to simplify code.
// BVH nodes indices refer to either the node array, for internal nodes,
// or a primitive array, for leaf nodes. BVH trees may contain only one type
// of geometric primitive, like points, lines, triangle or shape other BVHs.
// We handle multiple primitive types and transformed primitices by building
// a two-level hierarchy with the outer BVH, the scene BVH, containing inner
// BVHs, shape BVHs, each of which of a uniform primitive type.
//
// Implementation Notes:
// - we handle three types of BVHs, shape, shape and scene, using a union
// - shape BVHs contain sorted geometric primitives for fast intersection
// - shape BVHs contain pointers to external geometric data, to save memory
// - scene BVHs contain other BVHs to build the two level hiearchy
// - all BVH types shares the same type of internal nodes
//
struct yb__bvh {
    // bvh data
    int ntype;                   // type of primitive data contained by bvh
    int heuristic;               // heuristic used to build the bvh
    ym_vector<yb__bvhn> nodes;   // sorted array of internal nodes
    ym_vector<int> sorted_prim;  // sorted elements
};

//
// Shape BVH
//
struct yb_shape_bvh : yb__bvh {
    int nelems;  // shape elements
    int etype;   // shape element type
    union {
        const int* elem;  // shape elements
        const int* point;
        const ym_vec2i* line;
        const ym_vec3i* triangle;
    };
    const ym_vec3f* pos;  // vertex pos
    const float* radius;  // vertex radius
};

//
// Scene BVH
//
struct yb_scene_bvh : yb__bvh {
    int nshapes;                      // number of shapes
    ym_vector<yb_shape_bvh*> shapes;  // array of shape BVHs
    ym_vector<ym_affine3f> xforms;
    ym_vector<ym_affine3f> inv_xforms;

    ~yb_scene_bvh() {
        for (int i = 0; i < shapes.size(); i++) {
            if (shapes[i]) delete shapes[i];
        }
    }
};

// -----------------------------------------------------------------------------
// BVH BUILD FUNCTIONS
// -----------------------------------------------------------------------------

//
// Struct that pack a bounding box, its associate primitive index, and other
// data for faster hierarchy build.
//
struct yb__bound_prim {
    ym_range3f bbox;       // bounding box
    ym_vec3f center;       // bounding box center (for faster sort)
    int pid;               // primitive id
    float sah_cost_left;   // buffer for sah heuristic costs
    float sah_cost_right;  // buffer for sah heuristic costs
};

//
// Shell sort BVH array a of length n along the axis axis. Implementation based
// on http://rosettacode.org/wiki/Sorting_algorithms/Shell_sort
//
static inline void yb__shellsort_bound_prim(yb__bound_prim* a, int n,
                                            int axis) {
    int h, i, j;
    yb__bound_prim t;
    for (h = n; h /= 2;) {
        for (i = h; i < n; i++) {
            t = a[i];
            for (j = i;
                 j >= h && (&t.center.x)[axis] < (&a[j - h].center.x)[axis];
                 j -= h) {
                a[j] = a[j - h];
            }
            a[j] = t;
        }
    }
}

//
// Heap sort BVH array a of length n along the axis axis. Implementation based
// on https://en.wikipedia.org/wiki/Heapsort
//
static inline void yb__heapsort_bound_prim(yb__bound_prim* arr, int count,
                                           int axis) {
    if (!count) return;

    yb__bound_prim t;
    unsigned int n = count, parent = count / 2, index, child;
    while (true) {
        if (parent > 0) {
            t = arr[--parent];
        } else {
            n--;
            if (n == 0) return;
            t = arr[n];
            arr[n] = arr[0];
        }
        index = parent;
        child = index * 2 + 1;
        while (child < n) {
            if (child + 1 < n &&
                (&arr[child + 1].center.x)[axis] > (&arr[child].center.x)[axis])
                child++;
            if ((&arr[child].center.x)[axis] > (&t.center.x)[axis]) {
                arr[index] = arr[child];
                index = child;
                child = index * 2 + 1;
            } else
                break;
        }
        arr[index] = t;
    }
}

// choose a sort algorithm
#define yb__sort_bound_prim yb__heapsort_bound_prim

//
// Given an array sorted_prim of primitives to split between the elements
// start and end, determines the split axis axis, split primitive index mid
// based on the heuristic heuristic. Supports balanced tree (equalnum) and
// Surface-Area Heuristic.
//
static inline void yb__split_axis(yb__bound_prim* sorted_prim, int start,
                                  int end, int* axis, int* mid, int heuristic) {
    *axis = -1;
    *mid = -1;
    switch (heuristic) {
        // balanced tree split: find the largest axis of the bounding box
        // and split along this one right in the middle
        case yb_htype_equalnum: {
            // compute primintive bounds
            ym_range3f bbox = ym_invalid_range3f;
            for (int i = start; i < end; i++) {
                bbox = ym_rexpand(bbox, sorted_prim[i].center);
            }
            // split along largest
            ym_vec3f bvh_size = ym_rsize(bbox);
            if (bvh_size.x >= bvh_size.y && bvh_size.x >= bvh_size.z)
                *axis = 0;
            else if (bvh_size.y >= bvh_size.x && bvh_size.y >= bvh_size.z)
                *axis = 1;
            else if (bvh_size.z >= bvh_size.x && bvh_size.z >= bvh_size.y)
                *axis = 2;
            else
                assert(false);
            *mid = (start + end) / 2;
        } break;
        case yb_htype_default:
        case yb_htype_sah: {
            // surface area heuristic: estimate the cost of splitting
            // along each of the axis and pick the one with best expected
            // performance
            float min_cost = HUGE_VALF;
            int count = end - start;
            for (int a = 0; a < 3; a++) {
                yb__sort_bound_prim(sorted_prim + start, end - start, a);
                ym_range3f sbbox = ym_invalid_range3f;
                // to avoid an O(n^2) computation, use sweaps to compute the
                // cost,
                // first smallest to largest, then largest to smallest
                for (int i = 0; i < count; i++) {
                    sbbox = ym_rexpand(sbbox, sorted_prim[start + i].bbox);
                    ym_vec3f sbbox_size = ym_rsize(sbbox);
                    sorted_prim[start + i].sah_cost_left =
                        sbbox_size.x * sbbox_size.y +
                        sbbox_size.x * sbbox_size.z +
                        sbbox_size.y * sbbox_size.z;
                    sorted_prim[start + i].sah_cost_left *= i + 1;
                }
                // the other sweep
                sbbox = ym_invalid_range3f;
                for (int i = 0; i < count; i++) {
                    sbbox = ym_rexpand(sbbox, sorted_prim[end - 1 - i].bbox);
                    ym_vec3f sbbox_size = ym_rsize(sbbox);
                    sorted_prim[end - 1 - i].sah_cost_right =
                        sbbox_size.x * sbbox_size.y +
                        sbbox_size.x * sbbox_size.z +
                        sbbox_size.y * sbbox_size.z;
                    sorted_prim[end - 1 - i].sah_cost_right *= i + 1;
                }
                // find the minimum cost
                for (int i = start + 2; i <= end - 2; i++) {
                    float cost = sorted_prim[i - 1].sah_cost_left +
                                 sorted_prim[i].sah_cost_right;
                    if (min_cost > cost) {
                        min_cost = cost;
                        *axis = a;
                        *mid = i;
                    }
                }
            }
        } break;
        default: assert(false); break;
    }
    assert(*axis >= 0 && *mid > 0);
}

//
// Initializes the BVH node node that contains the primitives sorted_prims
// from start to end, by either splitting it into two other nodes,
// or initializing it as a leaf. When splitting, the heuristic heuristic is
// used and nodes added sequentially in the preallocated nodes array and
// the number of nodes nnodes is updated.
//
static inline void yb__make_node(yb__bvhn* node, int* nnodes, yb__bvhn* nodes,
                                 yb__bound_prim* sorted_prims, int start,
                                 int end, int heuristic) {
    // compute node bounds
    node->bbox = ym_invalid_range3f;
    for (int i = start; i < end; i++) {
        node->bbox = ym_rexpand(node->bbox, sorted_prims[i].bbox);
    }

    // decide whether to create a leaf
    if (end - start <= YB__BVH_MINPRIMS) {
        // makes a leaf node
        node->isleaf = true;
        node->start = start;
        node->count = end - start;
    } else {
        // makes an internal node
        node->isleaf = false;
        // choose the split axis and position
        int axis, mid;
        yb__split_axis(sorted_prims, start, end, &axis, &mid, heuristic);
        // sort primitives along the given axis
        yb__sort_bound_prim(sorted_prims + start, end - start, axis);
        // perform the splits by preallocating the child nodes and recurring
        node->axis = axis;
        node->start = *nnodes;
        node->count = 2;
        *nnodes += 2;
        // build child nodes
        yb__make_node(nodes + node->start, nnodes, nodes, sorted_prims, start,
                      mid, heuristic);
        yb__make_node(nodes + node->start + 1, nnodes, nodes, sorted_prims, mid,
                      end, heuristic);
    }
}

//
// Makes a shape BVH. Public function whose interface is described above.
//
YGL_API yb_shape_bvh* yb_init_shape_bvh(int nelems, const int* elem, int etype,
                                        int nverts, const ym_vec3f* pos,
                                        const float* radius, int heuristic) {
    // allocates the BVH and set its primitive type
    yb_shape_bvh* bvh = new yb_shape_bvh();
    bvh->nelems = nelems;
    bvh->etype = etype;
    bvh->elem = elem;
    bvh->ntype = etype;
    bvh->pos = (ym_vec3f*)pos;
    bvh->radius = radius;
    bvh->heuristic = heuristic;
    return bvh;
}

//
// Build a shape BVH. Public function whose interface is described above.
//
YGL_API void yb_build_shape_bvh(yb_shape_bvh* bvh) {
    // create bounded primitivrs used in BVH build
    ym_vector<yb__bound_prim> bound_prims =
        ym_vector<yb__bound_prim>(bvh->nelems);
    assert(bvh->pos && bvh->elem);
    // compute bounds for each primitive type
    switch (bvh->etype) {
        case yb_etype_point: {
            // point bounds are computed as small spheres
            for (int i = 0; i < bvh->nelems; i++) {
                const int& f = bvh->point[i];
                yb__bound_prim* prim = &bound_prims[i];
                prim->pid = i;
                prim->bbox = ym_invalid_range3f;
                float r0 = (bvh->radius) ? bvh->radius[f] : 0;
                prim->bbox = ym_rexpand(prim->bbox, bvh->pos[f] - ym_vec3f(r0));
                prim->bbox = ym_rexpand(prim->bbox, bvh->pos[f] + ym_vec3f(r0));
                prim->center = ym_rcenter(prim->bbox);
            }
        } break;
        case yb_etype_line: {
            // line bounds are computed as thick rods
            assert(bvh->radius);
            for (int i = 0; i < bvh->nelems; i++) {
                const ym_vec2i& f = bvh->line[i];
                yb__bound_prim* prim = &bound_prims[i];
                prim->pid = i;
                prim->bbox = ym_invalid_range3f;
                float r0 = (bvh->radius) ? bvh->radius[f.x] : 0;
                float r1 = (bvh->radius) ? bvh->radius[f.y] : 0;
                prim->bbox =
                    ym_rexpand(prim->bbox, bvh->pos[f.x] - ym_vec3f(r0));
                prim->bbox =
                    ym_rexpand(prim->bbox, bvh->pos[f.x] + ym_vec3f(r0));
                prim->bbox =
                    ym_rexpand(prim->bbox, bvh->pos[f.y] - ym_vec3f(r1));
                prim->bbox =
                    ym_rexpand(prim->bbox, bvh->pos[f.y] + ym_vec3f(r1));
                prim->center = ym_rcenter(prim->bbox);
            }
        } break;
        case yb_etype_triangle: {
            // triangle bounds are computed by including their vertices
            for (int i = 0; i < bvh->nelems; i++) {
                const ym_vec3i& f = bvh->triangle[i];
                yb__bound_prim* prim = &bound_prims[i];
                prim->pid = i;
                prim->bbox = ym_invalid_range3f;
                prim->bbox = ym_rexpand(prim->bbox, bvh->pos[f.x]);
                prim->bbox = ym_rexpand(prim->bbox, bvh->pos[f.y]);
                prim->bbox = ym_rexpand(prim->bbox, bvh->pos[f.z]);
                prim->center = ym_rcenter(prim->bbox);
            }
        } break;
        default: assert(false);
    }

    // allocate nodes (over-allocate now then shrink)
    int nnodes = 0;
    bvh->nodes.resize(bvh->nelems * 2);

    // start recursive splitting
    nnodes = 1;
    yb__make_node(bvh->nodes.data(), &nnodes, bvh->nodes.data(),
                  bound_prims.data(), 0, bvh->nelems, bvh->heuristic);

    // shrink back
    bvh->nodes.resize(nnodes);
    bvh->nodes.shrink_to_fit();

    // init sorted element arrays
    // for shared memory, stored pointer to the external data
    // store the sorted primitive order for BVH walk
    bvh->sorted_prim.resize(bvh->nelems);
    for (int i = 0; i < bvh->nelems; i++) {
        bvh->sorted_prim[i] = bound_prims[i].pid;
    }
    bvh->sorted_prim.shrink_to_fit();
}

//
// Makes a scene BVH. Public function whose interface is described above.
//
YGL_API yb_scene_bvh* yb_init_scene_bvh(int nshapes, int heuristic) {
    // allocates the BVH and set its primitive type
    yb_scene_bvh* bvh = new yb_scene_bvh();
    bvh->nshapes = nshapes;
    bvh->ntype = yb__ntype_bvh;
    bvh->shapes.assign(nshapes, nullptr);
    bvh->xforms.assign(nshapes, ym_identity_affine3f);
    bvh->inv_xforms.assign(nshapes, ym_identity_affine3f);
    return bvh;
}

//
// Sets shape data for scene BVH. Equivalent to calling init_shape_bvh.
//
YGL_API void yb_set_shape_bvh(yb_scene_bvh* bvh, int sid,
                              const ym_affine3f& xform, int nelems,
                              const int* elem, int etype, int nverts,
                              const ym_vec3f* pos, const float* radius,
                              int heuristic) {
    if (bvh->shapes[sid]) delete bvh->shapes[sid];
    bvh->shapes[sid] =
        yb_init_shape_bvh(nelems, elem, etype, nverts, pos, radius, heuristic);
    bvh->xforms[sid] = xform;
    bvh->inv_xforms[sid] = ym_inverse(xform);
}

//
// Build a scene BVH. Public function whose interface is described above.
//
YGL_API void yb_build_scene_bvh(yb_scene_bvh* bvh) {
    // recursively build
    for (int sid = 0; sid < bvh->nshapes; sid++) {
        yb_build_shape_bvh((yb_shape_bvh*)bvh->shapes[sid]);
    }

    // compute bounds for all transformed shape bvhs
    ym_vector<yb__bound_prim> bound_prims =
        ym_vector<yb__bound_prim>(bvh->nshapes);
    for (int i = 0; i < bvh->nshapes; i++) {
        yb__bound_prim* prim = &bound_prims[i];
        const yb_shape_bvh* sbvh = bvh->shapes[i];
        ym_affine3f xform = bvh->xforms[i];
        prim->pid = i;
        prim->bbox = sbvh->nodes[0].bbox;
        // apply transformations if needed (estimate trasform BVH bounds as
        // corners bounds, overestimate, but best effort for now)
        ym_vec3f corners[8] = {
            {prim->bbox.min.x, prim->bbox.min.y, prim->bbox.min.z},
            {prim->bbox.min.x, prim->bbox.min.y, prim->bbox.max.z},
            {prim->bbox.min.x, prim->bbox.max.y, prim->bbox.min.z},
            {prim->bbox.min.x, prim->bbox.max.y, prim->bbox.max.z},
            {prim->bbox.max.x, prim->bbox.min.y, prim->bbox.min.z},
            {prim->bbox.max.x, prim->bbox.min.y, prim->bbox.max.z},
            {prim->bbox.max.x, prim->bbox.max.y, prim->bbox.min.z},
            {prim->bbox.max.x, prim->bbox.max.y, prim->bbox.max.z},
        };
        prim->bbox = ym_invalid_range3f;
        for (int j = 0; j < 8; j++) {
            prim->bbox =
                ym_rexpand(prim->bbox, ym_transform_point(xform, corners[j]));
        }
        prim->center = ym_rcenter(prim->bbox);
    }

    // allocate nodes (over-allocate now then shrink)
    int nnodes = 0;
    bvh->nodes.resize(bvh->nshapes * 2);

    // start recursive splitting
    nnodes = 1;
    yb__make_node(bvh->nodes.data(), &nnodes, bvh->nodes.data(),
                  bound_prims.data(), 0, bvh->nshapes, bvh->heuristic);

    // shrink back
    bvh->nodes.resize(nnodes);
    bvh->nodes.shrink_to_fit();

    // pack sorted primimitives in the internal bvhs array
    bvh->sorted_prim.resize(bvh->nshapes);
    for (int i = 0; i < bvh->nshapes; i++) {
        bvh->sorted_prim[i] = bound_prims[i].pid;
    }
}

//
// Recursively recomputes the node bounds for a scene bvh
//
static inline void yb__recompute_scene_bounds(yb_scene_bvh* bvh, int nodeid) {
    yb__bvhn* node = &bvh->nodes[nodeid];
    if (node->isleaf) {
        node->bbox = ym_invalid_range3f;
        for (int i = 0; i < node->count; i++) {
            int idx = bvh->sorted_prim[node->start + i];
            ym_range3f bbox = bvh->shapes[0]->nodes[0].bbox;
            ym_affine3f xform = bvh->xforms[idx];
            ym_vec3f corners[8] = {
                {bbox.min.x, bbox.min.y, bbox.min.z},
                {bbox.min.x, bbox.min.y, bbox.max.z},
                {bbox.min.x, bbox.max.y, bbox.min.z},
                {bbox.min.x, bbox.max.y, bbox.max.z},
                {bbox.max.x, bbox.min.y, bbox.min.z},
                {bbox.max.x, bbox.min.y, bbox.max.z},
                {bbox.max.x, bbox.max.y, bbox.min.z},
                {bbox.max.x, bbox.max.y, bbox.max.z},
            };
            for (int j = 0; j < 8; j++) {
                node->bbox = ym_rexpand(node->bbox,
                                        ym_transform_point(xform, corners[j]));
            }
        }
    } else {
        node->bbox = ym_invalid_range3f;
        for (int i = 0; i < node->count; i++) {
            yb__recompute_scene_bounds(bvh, node->start + i);
            node->bbox =
                ym_rexpand(node->bbox, bvh->nodes[node->start + i].bbox);
        }
    }
}

//
// Refits a scene BVH. Public function whose interface is described above.
//
YGL_API void yb_refit_scene_bvh(yb_scene_bvh* bvh, const ym_affine3f* xforms) {
    assert(bvh->ntype == yb__ntype_bvh);

    // updated xforms
    for (int i = 0; i < bvh->nshapes; i++) {
        bvh->xforms[i] = xforms[i];
        bvh->inv_xforms[i] = ym_inverse(xforms[i]);
    }

    // recompute bvh bounds
    yb__recompute_scene_bounds(bvh, 0);
}

//
// FreeBVH data.
//
YGL_API void yb_free_scene_bvh(yb_scene_bvh* bvh) { delete bvh; }

//
// FreeBVH data.
//
YGL_API void yb_free_shape_bvh(yb_shape_bvh* bvh) { delete bvh; }

// -----------------------------------------------------------------------------
// BVH INTERSECTION FUNCTIONS
// -----------------------------------------------------------------------------

//
// Intersect ray with a bvh. Similar to the generic public function whose
// interface is described above. See yb_intersect_bvh for parameter docs.
// With respect to that, only adds early_exit to decide whether we exit
// at the first primitive hit or we find the closest hit.
//
// Implementation Notes:
// - Walks the BVH using an internal stack to avoid the slowness of recursive
// calls; this follows general conventions and stragely makes the code shorter
// - The walk is simplified for first hit by obeserving that if we update
// the ray_tmax liminit with the closest intersection distance during
// traversal, we will speed up computation significantly while simplifying
// the code; note in fact that all subsequence farthest iterations will be
// rejected in the tmax tests
//
static inline bool yb__intersect_bvh(const yb__bvh* bvh, bool early_exit,
                                     const ym_ray3f& ray_, float* ray_t,
                                     int* sid, int* eid, ym_vec2f* euv) {
    // node stack
    int node_stack[64];
    int node_cur = 0;
    node_stack[node_cur++] = 0;

    // shared variables
    bool hit = false;

    // init ray
    ym_ray3f ray = ray_;

    // walking stack
    while (node_cur && !(early_exit && hit)) {
        // exit if needed
        if (early_exit && hit) return hit;

        // grab node
        const yb__bvhn* node = &bvh->nodes[node_stack[--node_cur]];

        // intersect bbox
        if (!yb__intersect_check_bbox(ray, node->bbox)) continue;

        // intersect node, switching based on node type
        // for each type, iterate over the the primitive list
        int ntype = (node->isleaf) ? bvh->ntype : yb__ntype_internal;
        switch (ntype) {
            // internal node
            case yb__ntype_internal: {
                // for internal nodes, attempts to proceed along the
                // split axis from smallest to largest nodes
                if ((&ray.d.x)[node->axis] >= 0) {
                    for (int i = 0; i < node->count; i++) {
                        int idx = node->start + i;
                        node_stack[node_cur++] = idx;
                        assert(node_cur < 64);
                    }
                } else {
                    for (int i = node->count - 1; i >= 0; i--) {
                        int idx = node->start + i;
                        node_stack[node_cur++] = idx;
                        assert(node_cur < 64);
                    }
                }
            } break;
            case yb__ntype_point: {
                const yb_shape_bvh* sbvh = (const yb_shape_bvh*)bvh;
                for (int i = 0; i < node->count; i++) {
                    int idx = bvh->sorted_prim[node->start + i];
                    const int& f = sbvh->point[idx];
                    if (yb__intersect_point(ray, sbvh->pos[f], sbvh->radius[f],
                                            &ray.tmax, euv)) {
                        hit = true;
                        *eid = idx;
                    }
                }
            } break;
            case yb__ntype_line: {
                const yb_shape_bvh* sbvh = (const yb_shape_bvh*)bvh;
                for (int i = 0; i < node->count; i++) {
                    int idx = bvh->sorted_prim[node->start + i];
                    const ym_vec2i& f = sbvh->line[idx];
                    if (yb__intersect_line(ray, sbvh->pos[f.x], sbvh->pos[f.y],
                                           sbvh->radius[f.x], sbvh->radius[f.y],
                                           &ray.tmax, euv)) {
                        hit = true;
                        *eid = idx;
                    }
                }
            } break;
            case yb__ntype_triangle: {
                const yb_shape_bvh* sbvh = (const yb_shape_bvh*)bvh;
                for (int i = 0; i < node->count; i++) {
                    int idx = bvh->sorted_prim[node->start + i];
                    const ym_vec3i& f = sbvh->triangle[idx];
                    if (yb__intersect_triangle(ray, sbvh->pos[f.x],
                                               sbvh->pos[f.y], sbvh->pos[f.z],
                                               &ray.tmax, euv)) {
                        hit = true;
                        *eid = idx;
                    }
                }
            } break;
            case yb__ntype_bvh: {
                const yb_scene_bvh* sbvh = (const yb_scene_bvh*)bvh;
                for (int i = 0; i < node->count; i++) {
                    int idx = bvh->sorted_prim[node->start + i];
                    ym_ray3f tray = ray;
                    tray.o = ym_transform_point(sbvh->inv_xforms[idx], ray.o);
                    tray.d = ym_transform_vector(sbvh->inv_xforms[idx], ray.d);
                    if (yb__intersect_bvh(sbvh->shapes[idx], early_exit, tray,
                                          &ray.tmax, sid, eid, euv)) {
                        hit = true;
                        *sid = idx;
                    }
                }
            } break;
            default: { assert(false); } break;
        }
    }

    // update ray distance for outside
    if (hit) *ray_t = ray.tmax;

    return hit;
}

//
// Find closest ray-scene intersection. A convenient wrapper for the above func.
// Public function whose interface is described above.
//
YGL_API bool yb_intersect_scene_bvh(const yb_scene_bvh* bvh,
                                    const ym_ray3f& ray, float* ray_t, int* sid,
                                    int* eid, ym_vec2f* euv) {
    return yb__intersect_bvh(bvh, false, ray, ray_t, sid, eid, (ym_vec2f*)euv);
}

//
// Find any ray-scene intersection. A convenient wrapper for the above func.
// Public function whose interface is described above.
//
YGL_API bool yb_hit_scene_bvh(const yb_scene_bvh* bvh, const ym_ray3f& ray) {
    // check intersection
    int sid, eid;
    float ray_t;
    ym_vec2f euv;
    return yb__intersect_bvh(bvh, true, ray, &ray_t, &sid, &eid, &euv);
}

// -----------------------------------------------------------------------------
// BVH CLOSEST ELEMENT LOOKUP
// -----------------------------------------------------------------------------

//
// Finds the closest element with a bvh. Similar to the generic public function
// whose
// interface is described above. See yb_nightbor_bvh for parameter docs.
// With respect to that, only adds early_exit to decide whether we exit
// at the first primitive hit or we find the closest hit.
//
// Implementation Notes:
// - Walks the BVH using an internal stack to avoid the slowness of recursive
// calls; this follows general conventions and stragely makes the code shorter
// - The walk is simplified for first hit by obeserving that if we update
// the dist_max limit with the closest intersection distance during
// traversal, we will speed up computation significantly while simplifying
// the code; note in fact that all subsequent farthest iterations will be
// rejected in the tmax tests
//
static inline bool yb__neighbour_bvh(const yb__bvh* bvh, bool early_exit,
                                     const ym_vec3f pt, float dist_max,
                                     float* dist, int* sid, int* eid,
                                     ym_vec2f* euv) {
    // node stack
    int node_stack[64];
    int node_cur = 0;
    node_stack[node_cur++] = 0;

    // shared variables
    bool hit = false;

    // walking stack
    while (node_cur && !(early_exit && hit)) {
        // exit if needed
        if (early_exit && hit) return hit;

        // grab node
        const yb__bvhn* node = &bvh->nodes[node_stack[--node_cur]];

        // intersect bbox
        if (!yb__distance_check_bbox(pt, dist_max, node->bbox.min,
                                     node->bbox.max))
            continue;

        // intersect node, switching based on node type
        // for each type, iterate over the the primitive list
        int ntype = (node->isleaf) ? bvh->ntype : yb__ntype_internal;
        switch (ntype) {
            // internal node
            case yb__ntype_internal: {
                for (int i = 0; i < node->count; i++) {
                    int idx = node->start + i;
                    node_stack[node_cur++] = idx;
                    assert(node_cur < 64);
                }
            } break;
            case yb__ntype_point: {
                const yb_shape_bvh* sbvh = (const yb_shape_bvh*)bvh;
                for (int i = 0; i < node->count; i++) {
                    int idx = bvh->sorted_prim[node->start + i];
                    const int& f = sbvh->point[idx];
                    if (yb__distance_point(pt, dist_max, sbvh->pos[f],
                                           (sbvh->radius) ? sbvh->radius[f] : 0,
                                           &dist_max, euv)) {
                        hit = true;
                        *eid = idx;
                    }
                }
            } break;
            case yb__ntype_line: {
                const yb_shape_bvh* sbvh = (const yb_shape_bvh*)bvh;
                for (int i = 0; i < node->count; i++) {
                    int idx = bvh->sorted_prim[node->start + i];
                    const ym_vec2i& f = sbvh->line[idx];
                    if (yb__distance_line(
                            pt, dist_max, sbvh->pos[f.x], sbvh->pos[f.y],
                            (sbvh->radius) ? sbvh->radius[f.x] : 0,
                            (sbvh->radius) ? sbvh->radius[f.y] : 0, &dist_max,
                            euv)) {
                        hit = true;
                        *eid = idx;
                    }
                }
            } break;
            case yb__ntype_triangle: {
                const yb_shape_bvh* sbvh = (const yb_shape_bvh*)bvh;
                for (int i = 0; i < node->count; i++) {
                    int idx = bvh->sorted_prim[node->start + i];
                    const ym_vec3i& f = sbvh->triangle[idx];
                    if (yb__distance_triangle(
                            pt, dist_max, sbvh->pos[f.x], sbvh->pos[f.y],
                            sbvh->pos[f.z],
                            (sbvh->radius) ? sbvh->radius[f.x] : 0,
                            (sbvh->radius) ? sbvh->radius[f.y] : 0,
                            (sbvh->radius) ? sbvh->radius[f.z] : 0, &dist_max,
                            euv)) {
                        hit = true;
                        *eid = idx;
                    }
                }
            } break;
            case yb__ntype_bvh: {
                const yb_scene_bvh* sbvh = (const yb_scene_bvh*)bvh;
                for (int i = 0; i < node->count; i++) {
                    int idx = bvh->sorted_prim[node->start + i];
                    ym_vec3f tpt =
                        ym_transform_point(sbvh->inv_xforms[idx], pt);
                    if (yb__neighbour_bvh(sbvh->shapes[idx], early_exit, tpt,
                                          dist_max, &dist_max, sid, eid, euv)) {
                        hit = true;
                        *sid = idx;
                    }
                }
            } break;
            default: { assert(false); } break;
        }
    }

    // update ray distance for outside
    if (hit) *dist = dist_max;

    return hit;
}

//
// Find the closest element to a given point within a maximum distance.
// A convenient wrapper for the above func.
// Public function whose interface is described above.
//
YGL_API bool yb_neighbour_scene_bvh(const yb_scene_bvh* bvh, const ym_vec3f& pt,
                                    float max_dist, int req_sid, float* dist,
                                    int* sid, int* eid, ym_vec2f* euv) {
    // check correctness
    assert(bvh->ntype == yb__ntype_bvh);

    // handle specialized shapes
    if (req_sid >= 0) {
        ym_vec3f tpt = ym_transform_point(bvh->inv_xforms[req_sid], pt);
        bool hit = yb__neighbour_bvh(bvh->shapes[req_sid], false, tpt, max_dist,
                                     dist, sid, eid, euv);
        if (hit) *sid = req_sid;
        return hit;
    } else {
        return yb__neighbour_bvh(bvh, false, pt, max_dist, dist, sid, eid, euv);
    }
}

//
// Find the list of overlaps between shape bounds.
// Public function whose interface is described above.
//
YGL_API int yb_overlap_shape_bounds(const yb_scene_bvh* bvh, bool exclude_self,
                                    void* overlap_ctx,
                                    void (*overlap_cb)(void* ctx,
                                                       const ym_vec2i& v)) {
    // node stack
    int node_stack[256];
    int node_cur = 0;
    node_stack[node_cur++] = 0;
    node_stack[node_cur++] = 0;

    // shared variables
    int hits = 0;

    // walking stack
    while (node_cur) {
        // grab node
        int node1_idx = node_stack[--node_cur];
        int node2_idx = node_stack[--node_cur];
        const yb__bvhn* node1 = &bvh->nodes[node1_idx];
        const yb__bvhn* node2 = &bvh->nodes[node2_idx];

        // intersect bbox
        if (!yb__overlap_bbox(node1->bbox.min, node1->bbox.max, node2->bbox.min,
                              node2->bbox.max))
            continue;

        // check for leaves
        if (node1->isleaf && node2->isleaf) {
            // collide primitives
            for (int i1 = 0; i1 < node1->count; i1++) {
                for (int i2 = 0; i2 < node2->count; i2++) {
                    int idx1 = bvh->sorted_prim[node1->start + i1];
                    int idx2 = bvh->sorted_prim[node2->start + i2];
                    if (exclude_self && idx1 == idx2) continue;
                    const ym_range3f bbox1 = yb__transform_bbox(
                        bvh->xforms[idx1], bvh->shapes[idx1]->nodes[0].bbox);
                    const ym_range3f bbox2 = yb__transform_bbox(
                        bvh->xforms[idx2], bvh->shapes[idx1]->nodes[0].bbox);
                    if (!yb__overlap_bbox(bbox1.min, bbox1.max, bbox2.min,
                                          bbox2.max))
                        continue;
                    hits += 1;
                    overlap_cb(overlap_ctx, {idx1, idx2});
                }
            }
        } else {
            // descend
            if (node1->isleaf) {
                for (int i = 0; i < node2->count; i++) {
                    int idx = node2->start + i;
                    node_stack[node_cur++] = node1_idx;
                    node_stack[node_cur++] = idx;
                    assert(node_cur < 256);
                }
            } else {
                for (int i = 0; i < node1->count; i++) {
                    int idx = node1->start + i;
                    node_stack[node_cur++] = idx;
                    node_stack[node_cur++] = node2_idx;
                    assert(node_cur < 256);
                }
            }
        }
    }

    // done
    return hits;
}

//
// Overlap callback for ym_vector
//
static inline void yb__overlap_cb_vector(void* ctx, const ym_vec2i& v) {
    ym_vector<ym_vec2i>* vec = (ym_vector<ym_vec2i>*)ctx;
    vec->push_back(v);
}

//
// Find the list of overlaps between shape bounds.
// Public function whose interface is described above.
//
YGL_API int yb_overlap_shape_bounds(const yb_scene_bvh* bvh, bool exclude_self,
                                    ym_vector<ym_vec2i>* overlaps) {
    overlaps->clear();
    return yb_overlap_shape_bounds(bvh, exclude_self, overlaps,
                                   yb__overlap_cb_vector);
}

// -----------------------------------------------------------------------------
// VERTEX PROPERTY INTERPOLATION
// -----------------------------------------------------------------------------

//
// Interpolate vertex properties at the intersection.
// Public function whose interface is described above.
//
YGL_API void yb_interpolate_vert(const int* elem, int etype, int eid,
                                 const ym_vec2f& euv, int vsize,
                                 const float* vert, float* v) {
    for (int c = 0; c < vsize; c++) v[c] = 0;
    switch (etype) {
        case yb_etype_point: {
            const int* f = elem + eid;
            const float* vf = vert + f[0] * vsize;
            for (int c = 0; c < vsize; c++) v[c] += vf[c];
        } break;
        case yb_etype_line: {
            const int* f = elem + eid * 2;
            const float* vf[] = {vert + f[0] * vsize, vert + f[1] * vsize};
            const float w[2] = {1 - euv[0], euv[0]};
            for (int i = 0; i < 2; i++) {
                for (int c = 0; c < vsize; c++) v[c] += w[i] * vf[i][c];
            }
        } break;
        case yb_etype_triangle: {
            const int* f = elem + eid * 3;
            const float* vf[] = {vert + f[0] * vsize, vert + f[1] * vsize,
                                 vert + f[2] * vsize};
            const float w[3] = {1 - euv[0] - euv[1], euv[0], euv[1]};
            for (int i = 0; i < 3; i++) {
                for (int c = 0; c < vsize; c++) v[c] += w[i] * vf[i][c];
            }
        } break;
        default: assert(false);
    }
}

// -----------------------------------------------------------------------------
// STATISTICS FOR DEBUGGING (probably not helpful to all)
// -----------------------------------------------------------------------------

//
// Implementation of the function below
//
static inline void yb__print_bvh_stats_rec(const yb__bvh* bvh,
                                           const yb__bvhn* n, int d) {
    for (int i = 0; i < d; i++) printf(" ");
    printf("%s\n", (!n->isleaf) ? "-" : "*");
    if (!n->isleaf) {
        for (int i = 0; i < n->count; i++) {
            int idx = n->start + i;
            yb__print_bvh_stats_rec(bvh, &bvh->nodes[idx], d + 1);
        }
    }
}

//
// Implementation of the function below
//
static inline void yb__collect_bvh_stats(const yb__bvh* bvh, const yb__bvhn* n,
                                         int d, int* nl, int* ni, int* sump,
                                         int* mind, int* maxd, int* sumd) {
    if (n->isleaf) {
        *mind = (*mind < d) ? *mind : d;
        *maxd = (*maxd > d) ? *maxd : d;
        *sumd += d;
        (*nl)++;
        (*sump) += n->count;
    } else {
        (*ni)++;
        for (int i = 0; i < n->count; i++) {
            int idx = n->start + i;
            yb__collect_bvh_stats(bvh, &bvh->nodes[idx], d + 1, nl, ni, sump,
                                  mind, maxd, sumd);
        }
    }
}

//
// Print bvh for debugging
//
YGL_API void yb__print_bvh_stats(const yb__bvh* bvh, bool print_tree) {
    printf("nodes:  %d\nprims: %d\n", (int)bvh->nodes.size(),
           (int)bvh->sorted_prim.size());
    int nl = 0, ni = 0, sump = 0, mind = 0, maxd = 0, sumd = 0;
    yb__collect_bvh_stats(bvh, bvh->nodes.data(), 0, &nl, &ni, &sump, &mind,
                          &maxd, &sumd);
    printf("leaves:  %d\ninternal: %d\nprims per leaf: %f\n", nl, ni,
           (float)sump / (float)nl);
    printf("min depth:  %d\nmax depth: %d\navg depth: %f\n", mind, maxd,
           (float)sumd / (float)nl);
    yb__print_bvh_stats_rec(bvh, bvh->nodes.data(), 0);
    if (bvh->ntype == yb__ntype_bvh) {
        const yb_scene_bvh* sbvh = (const yb_scene_bvh*)bvh;
        for (int i = 0; i < bvh->sorted_prim.size(); i++)
            yb__print_bvh_stats(sbvh->shapes[i], print_tree);
    }
}

//
// Print bvh for debugging
//
YGL_API void yb_print_scene_bvh_stats(const yb_scene_bvh* bvh,
                                      bool print_tree) {
    yb__print_bvh_stats(bvh, print_tree);
}

//
// Print bvh for debugging
//
YGL_API void yb_print_scene_bvh_stats(const yb_shape_bvh* bvh,
                                      bool print_tree) {
    yb__print_bvh_stats(bvh, print_tree);
}

// -----------------------------------------------------------------------------
// C/C++ INTERFACE
// -----------------------------------------------------------------------------

//
// Create a BVH for a given shape.
//
YGLC_API yb_shape_bvh* ybc_init_shape_bvh(int nelems, int* elem, int etype,
                                          int nverts, float* pos, float* radius,
                                          int heuristic) {
    return yb_init_shape_bvh(nelems, elem, etype, nverts, (ym_vec3f*)pos,
                             radius, heuristic);
}

//
// Create a BVH for a collection of transformed shapes (scene).
//
YGLC_API yb_scene_bvh* ybc_init_scene_bvh(int nbvhs, int heuristic) {
    return yb_init_scene_bvh(nbvhs, heuristic);
}

//
// Sets the BVH for a given shape.
//
YGLC_API void ybc_set_shape_bvh(yb_scene_bvh* bvh, int sid, float xform[16],
                                int nelems, int* elem, int etype, int nverts,
                                float* pos, float* radius, int heuristic) {
    return yb_set_shape_bvh(bvh, sid, ym_affine3f(ym_mat4f(xform)), nelems,
                            elem, etype, nverts, (ym_vec3f*)pos, radius,
                            heuristic);
}

//
// Builds a shape BVH.
//
YGLC_API void ybc_build_shape_bvh(yb_shape_bvh* bvh) {
    return yb_build_shape_bvh(bvh);
}

//
// Builds a scene BVH.
//
YGLC_API void ybc_build_scene_bvh(yb_scene_bvh* bvh) {
    return yb_build_scene_bvh(bvh);
}

//
// Clears BVH memory.
//
YGLC_API void ybc_free_scene_bvh(yb_scene_bvh* bvh) {
    return yb_free_scene_bvh(bvh);
}

//
// Clears BVH memory.
//
YGLC_API void ybc_free_shape_bvh(yb_shape_bvh* bvh) {
    return yb_free_shape_bvh(bvh);
}

//
// Refit the bounds of each shape for moving objects. Use this only to avoid
// a rebuild, but note that queries are likely slow if objects move a lot.
//
YGLC_API void ybc_refit_scene_bvh(yb_scene_bvh* bvh, const float* xforms[16]) {
    ym_vector<ym_affine3f> affines;
    if (xforms) {
        affines.resize(bvh->nshapes);
        for (int i = 0; i < bvh->nshapes; i++)
            affines[i] = ym_affine3f(ym_mat4f(xforms[i]));
    }
    yb_refit_scene_bvh(bvh, affines.data());
}

//
// Intersect the scene with a ray finding the closest intersection.
//
YGLC_API bool ybc_intersect_scene_bvh(const yb_scene_bvh* bvh,
                                      const float ray_o[3],
                                      const float ray_d[3], float ray_tmin,
                                      float ray_tmax, float* ray_t, int* sid,
                                      int* eid, float euv[2]) {
    return yb_intersect_scene_bvh(
        bvh, ym_ray3f(ym_vec3f(ray_o), ym_vec3f(ray_d), ray_tmin, ray_tmax),
        ray_t, sid, eid, (ym_vec2f*)euv);
}

//
// Intersect the scene with a ray finding any intersection.
//
YGLC_API bool ybc_hit_scene_bvh(const yb_scene_bvh* bvh, const float ray_o[3],
                                const float ray_d[3], float ray_tmin,
                                float ray_tmax) {
    return yb_hit_scene_bvh(
        bvh, ym_ray3f(ym_vec3f(ray_o), ym_vec3f(ray_d), ray_tmin, ray_tmax));
}

//
// Returns a list of shape pairs that can possibly overlap by checking only they
// axis aligned bouds. This is only a conservative check useful for collision
// detection.
//
YGLC_API int ybc_overlap_shape_bounds(const yb_scene_bvh* bvh,
                                      bool exclude_self, void* overlap_ctx,
                                      void (*overlap_cb)(void* ctx,
                                                         const int ij[2])) {
    return 0;
// TODO: fixme
#if 0
    return yb_overlap_shape_bounds(bvh, exclude_self, overlap_ctx, (void*)overlap_cb);
#endif
}

//
// Finds the closest element to a point wihtin a given radius.
//
YGLC_API bool ybc_neighbour_scene_bvh(const yb_scene_bvh* bvh,
                                      const float pt[3], float max_dist,
                                      int req_sid, float* dist, int* sid,
                                      int* eid, float euv[2]) {
    return yb_neighbour_scene_bvh(bvh, ym_vec3f(pt), max_dist, req_sid, dist,
                                  sid, eid, (ym_vec2f*)euv);
}

//
// Interpolates a vertex property from the given intersection data. Uses
// linear interpolation for lines, baricentric for triangles and quads
// (to correctly treat as two triangles) and copies values for points.
//
YGLC_API void ybc_interpolate_vert(const int* elem, int etype, int eid,
                                   const float euv[2], int vsize,
                                   const float* vert, float* v) {
    return yb_interpolate_vert(elem, etype, eid, ym_vec2f(euv), vsize, vert, v);
}

//
// Print stats for the given BVH. Mostly useful for debugging or performance
// analysis. Ignore for general purpose.
//
YGLC_API void ybc_print_scene_bvh_stats(const yb_scene_bvh* bvh,
                                        bool print_tree);

#endif

#endif
