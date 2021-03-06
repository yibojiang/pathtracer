#pragma once
#include "transform.h"
#include "BVH.h"
#include "material.h"


struct Ray {
    vec3 origin;
    vec3 dir;
    vec2 uv;
    vec3 normal;
    Ray(vec3 ro) { origin = ro; dir = vec3(1,0,0); }
    Ray(vec3 ro, vec3 rd){ origin = ro; dir = rd; }

};


enum Refl_t { DIFF, SPEC, REFR };  // material types, used in radiance()




class Object {
protected:
    Extents bounds;
    Material *material;

public:
    std::string name;
    bool isMesh;
    Object(){}
    virtual ~Object(){}
    virtual vec3 getNormal(const vec3 &) const{ return vec3(1);}
    virtual double intersect(Ray &){ return 0;}


    virtual double getProjectAngleToSphere(){
        return M_PI;
    }


    virtual void setMaterial(Material* material){
        this->material = material;
        this->material->init();

    }

    Material* getMaterial(){
        return this->material;
    }

    virtual vec3 getCentriod() const{ return vec3(); }


    virtual void updateTransformMatrix(const mat4&){}

    virtual void computebounds(){}
    
    Extents getBounds(){
        return bounds;
    }
    // virtual vec3 debug(vec3 _pos) const{return vec3(0);}
};



class Plane: public Object{
private:
    vec3 normal;
public:
    
    double off;
    Plane(vec3 _nor, double _off){
        normal = _nor.normalize();
        off = _off;
        isMesh = false;
    }

    double intersect(Ray &r) {
        return (-r.origin.dot(normal) - off) / (normal).dot(r.dir);
    }

    vec3 getNormal(const vec3 &) const{
        return normal;
    }

    void updateTransformMatrix(const mat4& m){
        vec4 normalDir = vec4(normal, 0);
        normalDir = m * normalDir;
        normalDir.normalize();
        normal = vec3(normalDir.x, normalDir.y, normalDir.z);

        vec4 origin = vec4(normal*off, 1);
        origin = m * origin;
        vec3 origin3 = vec3(origin.x, origin.y, origin.z);
        off = origin3.dot(normal);
    }

    // virtual vec3 debug(vec3 _pos) const{return vec3(1);}
};



class Sphere: public Object {
public:
    double rad;
    vec3 center;

    
    mat4 m;

    Sphere(double _rad){
        rad = _rad;
        isMesh = false;
        center = vec3(0,0,0);
    }

    Sphere(double rad, vec3 pos) { this->rad = rad; this->center = pos; }
    double intersect(Ray &r) { // returns distance, 0 if nohit
        vec3 op = center - r.origin; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
        double t, eps = 1e-4, b = op.dot(r.dir), det = b * b - op.dot(op) + rad * rad;
        if (det < 0){
            return 0;  
        }  
        else{
            det = sqrt(det);  
        } 

        t = (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);

        // double t0 = b + det;
        // double t1 = b - det;
        // if (t0 > t1) std::swap(t0, t1); 

        // if (t0 < 0) { 
        //     t0 = t1; // if t0 is negative, let's use t1 instead 
        //     if (t0 < 0) return 0; // both t0 and t1 are negative 
        // } 
 
        // t = t0;
        
        vec3 hit = r.origin + r.dir * t;
        vec3 sp = (this->m * (hit - center)).normalized();

        // float u = (atan( sp.x, sp.z)) / ( 2.0 * M_PI ) + 0.5;
        // float v = asin( sp.y ) / M_PI + 0.5;
        r.uv = vec2((atan2(sp.x, sp.z)) / ( 2.0 * M_PI ) + 0.5, 
                    asin(sp.y) / M_PI + 0.5) ;

        r.normal = getNormal(hit);
        // return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
        return t;
    }

    vec3 getNormal(const vec3 &_pos) const{
        return (_pos - center)/rad;
    }

    void updateTransformMatrix(const mat4& m){
        this->m = m;
        vec4 pos = vec4(center, 1);
        pos = m * pos;
        center = vec3(pos.x, pos.y, pos.z);

        vec3 dir = m * vec3(1, 0, 0);
        rad = dir.length();
    }

    void computebounds(){
        // bounds = new Extents();
        for (uint8_t i = 0; i < SLABCOUNT; ++i){
            vec3 slabN = BVH::normals[i];
            double d = center.dot(slabN);
            bounds.dnear[i] = -rad - d;
            bounds.dfar[i] = rad - d;
        }
    }

    vec3 getCentriod() const{
        return center;
    }

    
    // virtual vec3 debug(vec3 _pos) const{return vec3(1);}
};


class Box: public Object{ 
private:
    double dnear[3];
    double dfar[3];
public: 
    vec3 p[8]; 
    vec3 center;
    vec3 size;
    vec3 normals[3];

    Box(const vec3 &_size){ 
        size = _size;
        normals[0] = vec3(1, 0, 0);
        normals[1] = vec3(0, 1, 0);
        normals[2] = vec3(0, 0, 1);

        vec3 half_size = size * 0.5;
        p[0] = center + vec3(half_size.x, half_size.y, half_size.z);
        p[1] = center + vec3(-half_size.x, half_size.y, half_size.z);
        p[2] = center + vec3(half_size.x, -half_size.y, half_size.z);
        p[3] = center + vec3(half_size.x, half_size.y, -half_size.z);
        p[4] = center + vec3(-half_size.x, -half_size.y, half_size.z);
        p[5] = center + vec3(half_size.x, -half_size.y, -half_size.z);
        p[6] = center + vec3(-half_size.x, half_size.y, -half_size.z);
        p[7] = center + vec3(-half_size.x, -half_size.y, -half_size.z);
        updatePlane();

        isMesh = false;
        
    }

    void updatePlane(){
        
        for (int i = 0; i < 3; ++i){
            dnear[i] = inf;
            dfar[i] = -inf;
            
            for (int j = 0; j < 8; ++j){
                double d = -p[j].dot(normals[i]);
                if (d > dfar[i]){
                    dfar[i] = d;
                }

                if (d < dnear[i]){
                    dnear[i] = d;
                }
            }

        }
    }

    // AABB
    // double intersect(const Ray &r) { // returns distance, 0 if nohit
    //     vec3 invdir(1.0 / r.dir.x, 1.0 / r.dir.y, 1.0 / r.dir.z);
        
    //     // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
    //     // r.org is origin of ray
    //     double t1 = (bounds[0].x - r.origin.x) * invdir.x;
    //     double t2 = (bounds[1].x - r.origin.x) * invdir.x;
    //     double t3 = (bounds[0].y - r.origin.y) * invdir.y;
    //     double t4 = (bounds[1].y - r.origin.y) * invdir.y;
    //     double t5 = (bounds[0].z - r.origin.z) * invdir.z;
    //     double t6 = (bounds[1].z - r.origin.z) * invdir.z;

    //     double tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6)); // max tnear 
    //     double tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6)); // min tfar

    //     // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    //     if (tmax < 0){
    //         return 0;
    //     }

    //     // if tmin > tmax, ray doesn't intersect AABB
    //     if (tmin > tmax){
    //         return 0;
    //     }
    //     return tmin;
    // } 


    double intersect(Ray &r) { // returns distance, 0 if nohit
        double t1 = (-dnear[0] - r.origin.dot(normals[0])) / r.dir.dot(normals[0]);
        double t2 = (-dfar[0] - r.origin.dot(normals[0])) / r.dir.dot(normals[0]);
        double t3 = (-dnear[1] - r.origin.dot(normals[1])) / r.dir.dot(normals[1]);
        double t4 = (-dfar[1] - r.origin.dot(normals[1])) / r.dir.dot(normals[1]);
        double t5 = (-dnear[2] - r.origin.dot(normals[2])) / r.dir.dot(normals[2]);
        double t6 = (-dfar[2] - r.origin.dot(normals[2])) / r.dir.dot(normals[2]);
        double tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6)); // max t near 
        double tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6)); // min t far
        // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
        if (tmax < 0){
            return 0;
        }

        // if tmin > tmax, ray doesn't intersect AABB
        if (tmin > tmax){
            return 0;
        }

        vec3 hit = r.origin + r.dir*tmin;
        r.normal = getNormal(hit);

        return tmin;
    } 
    
    vec3 getNormal(const vec3 &_pos) const{
        for (int i = 0; i < 3; ++i){
            if (fabs((_pos.dot(normals[i]) + dnear[i])) < eps){
                return normals[i];
            }

            if (fabs((_pos.dot(normals[i]) + dfar[i])) < eps){
                return -normals[i];
            }

            if (fabs((_pos.dot(-normals[i]) + dnear[i])) < eps){
                return -normals[i];
            }

            if (fabs((_pos.dot(-normals[i]) + dfar[i])) < eps){
                return normals[i];
            }
        }
        return vec3();
    }

    void updateTransformMatrix(const mat4& m){
        for (int i = 0; i < 3; ++i){
            // vec4 n = m*  vec4(normals[i], 0);
            normals[i] = m * normals[i];   
            normals[i].normalize();
        }

        for (int i = 0; i < 8; ++i){
            vec4 p4 = vec4(p[i], 1);
            p4 = m * p4;
            p[i] = vec3(p4.x, p4.y, p4.z);
        }

        // size = m * size;
        vec4 center4 = m * vec4(center, 1);
        center = vec3(center4.x, center4.y, center4.z);
        updatePlane();
    }

    void computebounds(){

        for (uint8_t i = 0; i < SLABCOUNT; ++i){
            bounds.dnear[i] = inf;
            bounds.dfar[i] = -inf;
            for (int j = 0; j < 8; ++j){
                // qDebug()<<p[j];
                double d =  -p[j].dot(BVH::normals[i]);
                if (d < bounds.dnear[i]){
                    bounds.dnear[i] = d;
                }
                if (d > bounds.dfar[i]){
                    bounds.dfar[i] = d;
                }
            }
        }
    }

    vec3 getCentriod() const{
        return center;
    }
};
 
class Triangle : public Object{
private:
    
    
    

public:
    vec2 uv1, uv2, uv3;
    vec3 n1, n2, n3;
    vec3 normal, u, v;
    vec3 p1, p2, p3;
    double  s, t;
    // bool normalAtVertex;

    

    // Triangle(){}
    Triangle(vec3 p1, vec3 p2, vec3 p3){
        this->p1 = p1;
        this->p2 = p2;
        this->p3 = p3;
        u = this->p2 - this->p1;
        v = this->p3 - this->p1;
        normal = u.cross(v).normalize();              // cross product
        isMesh = false;
        // normalAtVertex = false;
    }

    ~Triangle(){

    }

    void setupVertices(const vec3 p1,const vec3 p2,const vec3 p3){
        this->p1 = p1;
        this->p2 = p2;
        this->p3 = p3;
        u = this->p2 - this->p1;
        v = this->p3 - this->p1;
        normal = u.cross(v).normalize();              // cross product
        isMesh = false;
    }

    void setupNormals(vec3 _n1, vec3 _n2, vec3 _n3) {
        // normal = _normal.normalize();
        n1 = _n1.normalize();
        n2 = _n2.normalize();
        n3 = _n3.normalize();
        // normalAtVertex = true;
    }

    void setupUVs(vec2 uv1, vec2 uv2, vec2 uv3){
        this->uv1 = uv1;
        this->uv2 = uv2;
        this->uv3 = uv3;
    }

    vec3 getNormal(const vec3 &) const{    

        return normal;
        // return n1;
        // return n1 * (1 - s -t) + n2 * s + n3 * t;
    }

    double intersect(Ray &r) { // returns distance, 0 if nohit        
        // vec3 nl = r.dir.dot(normal) > 0 ? normal : normal * -1;
        // double dist = -r.origin.dot(nl) + center.dot(nl);
        // double tt = dist / r.dir.dot(nl);

        // if (fabs(r.dir.dot(normal)) < eps){
        //     return 0;
        // }

        
        // if (normal == vec3(0,0,0)){             // triangle is degenerate
        //     return 0; 
        // }

        double dn = r.dir.dot(normal);
        if (fabs(dn) < eps) {     // ray is  parallel to triangle plane
            return 0;
        }

        vec3 center = (p1 + p2 + p3) / 3;
        double dist = -r.origin.dot(normal) + center.dot(normal);
        double tt = dist / dn;
        vec3 hit = r.origin + r.dir * tt;


        u = p2 - p1;
        v = p3 - p1;

         // is I inside T?
        double    uu, uv, vv, wu, wv, D;
        uu = u.dot(u);
        uv = u.dot(v);
        vv = v.dot(v);
        
        vec3 w = hit - p1;
        wu = w.dot(u);
        wv = w.dot(v);
        D = uv * uv - uu * vv;

        // get and test parametric coords
        // double s, t;
        s = (uv * wv - vv * wu) / D;
        if (s < 0.0 || s > 1.0){         // I is outside T
            return 0;
        }

        t = (uv * wu - uu * wv) / D;
        if (t < 0.0 || (s + t) > 1.0){  // I is outside T
            return 0;
        }

        // r.uv = (uv1 * s +  uv2 * (1 - s));
        r.uv = uv1 * (1 - s -t) + uv2 * s + uv3 * t;

        r.normal = getNormal(hit);
        // r.normal = n1 * (1 - s -t) + n2 * s + n3 * t;
        // r.uv = vec2(drand48(), drand48());

        // r.uv.x = fmin(1, r.uv.x);
        // r.uv.y = fmin(1, r.uv.y);
        // r.uv = uv1 * s +  uv2 * (1 - s);
        return tt;
    }

    void computebounds(){
        for (uint8_t i = 0; i < SLABCOUNT; ++i){
            vec3 slabN = BVH::normals[i];
            double d1 =  -p1.dot(slabN);
            double d2 =  -p2.dot(slabN);
            double d3 =  -p3.dot(slabN);
            bounds.dnear[i] = fmin(d1, fmin(d2, d3));
            bounds.dfar[i] = fmax(d1, fmax(d2, d3));
            
            // bounds.dnear[i]-=eps;
            // bounds.dfar[i]+=eps;
            // qDebug() << this->name.c_str() << "near" << bounds.dnear[i];
            // qDebug() << this->name.c_str() << "far" << bounds.dfar[i];
            // if ( fabs(bounds.dfar[i] - bounds.dnear[i]) < eps){
            //     bounds.dnear[i]-=eps;
            //     bounds.dfar[i]+=eps;
            // }

            
            
            
        
        }
    }

    vec3 getCentriod() const{
        return (p1 + p2 + p3) / 3.0;
    }


    void updateTransformMatrix(const mat4& m){
        // qDebug()<<"update mesh matrix";
    
        vec4 p1(this->p1, 1.0);
        vec4 p2(this->p2, 1.0);
        vec4 p3(this->p3, 1.0);
        // qDebug() << v1 << v2 << v3;
        p1 = m * p1;
        p2 = m * p2;
        p3 = m * p3;
        // qDebug() << "after: " << p1 << p2 << p3;
        this->p1 = vec3(p1.x, p1.y, p1.z);
        this->p2 = vec3(p2.x, p2.y, p2.z);
        this->p3 = vec3(p3.x, p3.y, p3.z);

        this->n1 = m * this->n1;
        this->n2 = m * this->n2;
        this->n3 = m * this->n3;
    
    }
};

// class Face
// {

// public:
//     vec3 v1, v2, v3;
//     vec3 n1, n2, n3;
//     vec2 uv1, uv2, uv3;

//     void setupUVs(const vec2 uv1,const vec2 uv2,const vec2 uv3){
//         this->uv1 = uv1;
//         this->uv2 = uv2;
//         this->uv3 = uv3;
        
//     }

//     void setupVertices(const vec3 v1,const vec3 v2,const vec3 v3){
//         this->v1 = v1;
//         this->v2 = v2;
//         this->v3 = v3;
//     }

//     void setupNormals(const vec3 n1,const vec3 n2,const vec3 n3){
//         this->n1 = n1;
//         this->n2 = n2;
//         this->n3 = n3;
//     }


//     Face() {};
//     ~Face() {};
    
// };

class Mesh: public Object{

public:
    // std::vector<Face*> faces;
    std::vector<Triangle*> faces;
    
    Mesh() {
        isMesh = true;
    }

    ~Mesh(){
        for (uint32_t i = 0; i < faces.size(); ++i){
            delete faces[i];
        }
    }

    void updateTransformMatrix(const mat4& m){
        // qDebug()<<"update mesh matrix";
        for (uint32_t i = 0; i < faces.size(); ++i){
            vec4 p1(faces[i]->p1, 1.0);
            vec4 p2(faces[i]->p2, 1.0);
            vec4 p3(faces[i]->p3, 1.0);
            // qDebug() << v1 << v2 << v3;
            p1 = m * p1;
            p2 = m * p2;
            p3 = m * p3;

            
            // qDebug() << "after: " << p1 << p2 << p3;
            faces[i]->p1 = vec3(p1.x, p1.y, p1.z);
            faces[i]->p2 = vec3(p2.x, p2.y, p2.z);
            faces[i]->p3 = vec3(p3.x, p3.y, p3.z);

            faces[i]->u = faces[i]->p2 - faces[i]->p1;
            faces[i]->v = faces[i]->p3 - faces[i]->p1;
            faces[i]->normal = faces[i]->u.cross(faces[i]->v).normalize();

            faces[i]->n1 = m * faces[i]->n1;
            faces[i]->n2 = m * faces[i]->n2;
            faces[i]->n3 = m * faces[i]->n3;
        }
    }

    void addFace(Triangle* face){
        faces.push_back(face);
    }

};