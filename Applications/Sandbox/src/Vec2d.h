#pragma once 
#include <cmath> 
#include <cassert> 
#include <cstdio> 
#include "Float.h" 
 
namespace NkMath { 
 
    struct Vec2d { 
        double x, y; 
    
        // Constructeurs 
        Vec2d() : x(0.0), y(0.0) {} 
        Vec2d(double x, double y) : x(x), y(y) {} 
        explicit Vec2d(double s) : x(s), y(s) {}  // fill constructor 
    
        // Accès par index — & pour permettre l'écriture 
        double& operator[](int i) { 
            assert(i >= 0 && i < 2 && "Vec2d index out of bounds"); 
            return (&x)[i];  // garanti contigu en C++17 
        } 
        const double& operator[](int i) const { 
            assert(i >= 0 && i < 2 && "Vec2d index out of bounds"); 
            return (&x)[i]; 
        } 
    
        // Opérateurs arithmétiques 
        Vec2d operator+(const Vec2d& o) const { return {x+o.x, y+o.y}; } 
        Vec2d operator-(const Vec2d& o) const { return {x-o.x, y-o.y}; } 
        Vec2d operator*(double s)        const { return {x*s, y*s}; } 
        Vec2d operator/(double s)        const { assert(!nearlyZero(s)); return {x/s, y/s}; } 
        Vec2d operator-()                const { return {-x, -y}; } 
    
        // Opérateurs composés 
        Vec2d& operator+=(const Vec2d& o) { x+=o.x; y+=o.y; return *this; } 
        Vec2d& operator-=(const Vec2d& o) { x-=o.x; y-=o.y; return *this; } 
        Vec2d& operator*=(double s)       { x*=s; y*=s; return *this; } 
    
        // Norme et normalisation 
        double Norm2() const { return x*x + y*y; } 
        double Norm()  const { return std::sqrt(Norm2()); } 
    
        Vec2d Normalized() const { 
            double n = Norm(); 
            if(nearlyZero(n)) return Vec2d(0.0);  // vecteur nul → retourner zéro 
            return {x/n, y/n}; 
        } 
    
        bool IsNormalized(double eps = kEps) const { 
            return approxEq(Norm2(), 1.0, eps); 
        } 
    
        void Print() const { printf("Vec2d(%.6f, %.6f)\n", x, y); } 
    }; 
    
    // Dot product : a·b = |a||b|cos(θ) 
    inline double Dot(const Vec2d& a, const Vec2d& b) { 
        return a.x*b.x + a.y*b.y; 
    } 
    
    // Cross 2D (scalaire) : composante z du produit vectoriel 3D 
    // Positif si b est à gauche de a, négatif si à droite 
    inline double Cross2D(const Vec2d& a, const Vec2d& b) { 
        return a.x*b.y - a.y*b.x; 
    } 
    
    // Lerp linéaire 
    inline Vec2d Lerp(const Vec2d& a, const Vec2d& b, double t) { 
        return {a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t}; 
    } 
    
    // Opérateur externe scalaire * Vec2d 
    inline Vec2d operator*(double s, const Vec2d& v) { return v * s; } 
    
    // Garantie de layout mémoire pour glVertexAttribPointer 
    static_assert(sizeof(Vec2d) == 16, "Vec2d must be 16 bytes (2 doubles)"); 
    static_assert(offsetof(Vec2d, x) == 0, "x must be first"); 
    static_assert(offsetof(Vec2d, y) == 8, "y must be at offset 8"); 
 
} // namespace NkMath 
