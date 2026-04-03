#pragma once 
#include "Vec3d.h"

namespace NkMath { 

    // Orthogonalisation de Gram-Schmidt 
    // Entrée : 3 vecteurs (peuvent être non orthogonaux) 
    // Sortie : 3 vecteurs orthonormaux (base orthonormale) 
    struct OrthoBasis { 
        Vec3d u, v, w;  // garantis orthogonaux et normalisés 
    }; 
    
    inline OrthoBasis GramSchmidt(Vec3d a, Vec3d b, Vec3d c) { 
        // Étape 1 : normaliser a -> u 
        Vec3d u = a.Normalized(); 
        assert(!nearlyZero(u.Norm()) && "First vector is zero"); 
    
        // Étape 2 : ôter la composante u de b -> v 
        Vec3d v = (b - Project(b, u)).Normalized(); 
        assert(!nearlyZero(v.Norm()) && "Vectors are collinear"); 
    
        // Étape 3 : ôter les composantes u et v de c -> w 
        Vec3d w = (c - Project(c, u) - Project(c, v)).Normalized(); 
        assert(!nearlyZero(w.Norm()) && "Vectors are coplanar"); 
    
        return {u, v, w}; 
    }

}
