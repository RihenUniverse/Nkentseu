#pragma once 
#include "Vec4d.h" 
#include "Mat3d.h"
#include <array> 
 
namespace NkMath { 
    
    struct Mat4d { 
        // Stockage column-major : data[col*4 + row] 
        double data[16];     
        Mat4d() { std::fill(data, data+16, 0.0); } 
    
        // Accès row,col 
        double& operator()(int row, int col) { 
            assert(row>=0 && row<4 && col>=0 && col<4); 
            return data[col*4 + row];  // column-major 
        } 
        const double& operator()(int row, int col) const { 
            return data[col*4 + row]; 
        } 
    
        // Matrice identité 
        static Mat4d Identity() { 
            Mat4d m; 
            m(0,0) = m(1,1) = m(2,2) = m(3,3) = 1.0; 
            return m; 
        } 
    
        // Produit matriciel O(64 multiplications pour 4×4) 
        Mat4d operator*(const Mat4d& o) const { 
            Mat4d result; 
            for(int row = 0; row < 4; row++) 
                for(int col = 0; col < 4; col++) { 
                    double sum = 0.0; 
                    for(int k = 0; k < 4; k++) 
                        sum += (*this)(row, k) * o(k, col); 
                    result(row, col) = sum; 
                } 
            return result; 
        } 
    
        // Multiplication Mat4d × Vec4d 
        Vec4d operator*(const Vec4d& v) const { 
            return { 
                (*this)(0,0)*v.x + (*this)(0,1)*v.y + (*this)(0,2)*v.z + (*this)(0,3)*v.w, 
                (*this)(1,0)*v.x + (*this)(1,1)*v.y + (*this)(1,2)*v.z + (*this)(1,3)*v.w, 
                (*this)(2,0)*v.x + (*this)(2,1)*v.y + (*this)(2,2)*v.z + (*this)(2,3)*v.w, 
                (*this)(3,0)*v.x + (*this)(3,1)*v.y + (*this)(3,2)*v.z + (*this)(3,3)*v.w 
            }; 
        } 
    
        // Transposée 
        Mat4d Transposed() const { 
            Mat4d t; 
            for(int r = 0; r < 4; r++) 
                for(int c = 0; c < 4; c++) 
                    t(r,c) = (*this)(c,r); 
            return t; 
        } 
    
        // Pointeur vers les données pour upload GPU 
        const double* DataPtr() const { return data; } 

        // Pour upload float GPU (convertir en float): 
        void ToFloat(float out[16]) const { 
            for(int i=0; i<16; i++) out[i] = (float)data[i]; 
        } 
    }; 

    // Inverse par Gauss-Jordan avec pivot partiel 
    // Retourne false si la matrice est singulière (non inversible) 
    bool Inverse(const Mat4d& m, Mat4d& out) { 
        // Matrice augmentée [m | I] 
        double aug[4][8]; 
        for(int r = 0; r < 4; r++) 
            for(int c = 0; c < 4; c++) { 
                aug[r][c]   = m(r,c); 
                aug[r][c+4] = (r == c) ? 1.0 : 0.0; 
            } 
    
        for(int col = 0; col < 4; col++) { 
            // Pivot partiel : trouver la ligne avec le max absolu dans cette colonne 
            int pivotRow = col; 
            for(int r = col+1; r < 4; r++) 
                if(std::abs(aug[r][col]) > std::abs(aug[pivotRow][col])) 
                    pivotRow = r; 
    
            // Swap des lignes col et pivotRow 
            if(pivotRow != col) 
                for(int c = 0; c < 8; c++) 
                    std::swap(aug[col][c], aug[pivotRow][c]); 
    
            // Vérification singularité 
            if(nearlyZero(aug[col][col])) return false; 
    
            // Normaliser la ligne pivot 
            double inv = 1.0 / aug[col][col]; 
            for(int c = 0; c < 8; c++) aug[col][c] *= inv; 
    
            // Élimination sur les autres lignes 
            for(int r = 0; r < 4; r++) { 
                if(r == col) continue; 
                double factor = aug[r][col]; 
                for(int c = 0; c < 8; c++) 
                    aug[r][c] -= factor * aug[col][c]; 
            } 
        } 
    
        // Extraire la partie droite [I | A⁻¹] 
        for(int r = 0; r < 4; r++) 
            for(int c = 0; c < 4; c++) 
                out(r,c) = aug[r][c+4]; 
        return true; 
    } 

    inline bool ApproxMat(const Mat4d& mat1, const Mat4d& mat2, double eps = kEps) {
        for(int i=0;i<4;i++)
            for(int j=0;j<4;j++)
                if(!approxEq(mat1(i, j), mat2(i, j), eps))
                    return false;
        return true;
    }

    // 3.3 Matrice de rotation — formule de Rodrigues 
    // La rotation d'angle θ autour d'un axe unitaire n = (nx, ny, nz) est donnée par la formule de Rodrigues : 
    Mat4d RotateAxis(const Vec3d& axis, double angleRad) { 
        Vec3d n = axis.Normalized(); 
        double c = std::cos(angleRad); 
        double s = std::sin(angleRad); 
        double t = 1.0 - c; 
    
        Mat4d R = Mat4d::Identity(); 
        // Formule de Rodrigues 
        R(0,0) = t*n.x*n.x + c; 
        R(0,1) = t*n.x*n.y - s*n.z; 
        R(0,2) = t*n.x*n.z + s*n.y; 
    
        R(1,0) = t*n.x*n.y + s*n.z; 
        R(1,1) = t*n.y*n.y + c; 
        R(1,2) = t*n.y*n.z - s*n.x; 
    
        R(2,0) = t*n.x*n.z - s*n.y; 
        R(2,1) = t*n.y*n.z + s*n.x; 
        R(2,2) = t*n.z*n.z + c; 
    
        return R; 
    } 
    
    // LookAt — matrice View 
    // Construire la matrice View (caméra regarde target depuis eye) 
    Mat4d LookAt(const Vec3d& eye, const Vec3d& target, const Vec3d& worldUp) { 
        // Axe forward = direction regardée = normalize(target - eye) 
        Vec3d f = (target - eye).Normalized(); 
    
        // Axe right = perpendiculaire à forward et worldUp 
        Vec3d r = Cross(f, worldUp).Normalized(); 
    
        // Axe up corrigé = perpendiculaire à right et forward 
        Vec3d u = Cross(r, f); 
    
        // View = R * T (rotater puis translater) 
        Mat4d V = Mat4d::Identity(); 
        V(0,0) =  r.x; V(0,1) =  r.y; V(0,2) =  r.z; 
        V(1,0) =  u.x; V(1,1) =  u.y; V(1,2) =  u.z; 
        V(2,0) = -f.x; V(2,1) = -f.y; V(2,2) = -f.z; 
        // Translation : t = -R × eye 
        V(0,3) = -Dot(r, eye); 
        V(1,3) = -Dot(u, eye); 
        V(2,3) =  Dot(f, eye); 
        return V; 
    } 

    Mat4d Perspective(double fov_deg, double aspect, double nearPlane, double farPlane){
        double f = 1.0 / std::tan(fov_deg * 0.5 * nkentseu::math::DEG_TO_RAD);
        Mat4d P{};
        P(0, 0) = f/aspect;
        P(1, 1) = f;
        P(2, 2) = (farPlane+nearPlane)/(nearPlane-farPlane);
        P(2, 3) = (2*farPlane*nearPlane)/(nearPlane-farPlane);
        P(3, 2) = -1.0;
        return P;
    }
    
    // Construire Mat4d depuis [R3×3 | t3×1] de solvePnP
    // Mat4d FromRT(const Mat3d& R, const Vec3d& t) { 
    //     Mat4d M = Mat4d::Identity(); 
    //     for(int r=0; r<3; r++) 
    //         for(int c=0; c<3; c++) 
    //             M(r,c) = R(r,c); 
    //     M(0,3) = t.x; M(1,3) = t.y; M(2,3) = t.z; 
    //     return M; 
    // } 

    // TRS — Translation, Rotation, Scale 
    Mat4d Translate(const Vec3d& t) { 
        Mat4d M = Mat4d::Identity(); 
        M(0,3) = t.x; M(1,3) = t.y; M(2,3) = t.z; 
        return M; 
    } 
    
    Mat4d Scale(const Vec3d& s) { 
        Mat4d M = Mat4d::Identity(); 
        M(0,0) = s.x; M(1,1) = s.y; M(2,2) = s.z; 
        return M; 
    } 

    // TRS = T × R × S — L'ORDRE EST CRUCIAL ! 
    // 1. Mettre à l'échelle l'objet (autour de son origine locale) 
    // 2. Tourner l'objet mis à l'échelle 
    // 3. Translater vers la position monde 
    Mat4d TRS(const Vec3d& t, const Vec3d& axis, double angle, const Vec3d& s) { 
        return Translate(t) * RotateAxis(axis, angle) * Scale(s); 
    } 

    Mat4d TRS(const Vec3d& t, const Vec3d& r, const Vec3d& s){
        Mat4d rotation = RotateAxis({0, 0, 1}, r.z) * RotateAxis({0, 1, 0}, r.y) * RotateAxis({1, 0, 0}, r.x);
        return Translate(t) * rotation * Scale(s);
    }

    void DecomposeTRS(const Mat4d& m, Vec3d& outT, Vec3d& outR, Vec3d& outS){
        // 1. translation
        outT = {m(0, 3), m(1, 3), m(2, 3)};

        // 2. scale = norme des colonnes
        Vec3d col0{m(0, 0), m(1, 0), m(2, 0)};
        Vec3d col1{m(0, 1), m(1, 1), m(2, 1)};
        Vec3d col2{m(0, 2), m(1, 2), m(2, 2)};
        outS = {col0.Norm(), col1.Norm(), col2.Norm()};

        // 3. matrice rotation pure
        double r00 = m(0, 0) / outS.x;
        double r01 = m(0, 1) / outS.y;
        double r02 = m(0, 2) / outS.z;

        double r10 = m(1, 0) / outS.x;
        double r11 = m(1, 1) / outS.y;
        double r12 = m(1, 2) / outS.z;

        double r20 = m(2, 0) / outS.x;
        double r21 = m(2, 1) / outS.y;
        double r22 = m(2, 2) / outS.z;

        // 4. extraction Euler (XYZ)
        outR.y = std::asin(-r20);

        if(std::cos(outR.y) > 1e-6) {
            outR.x = std::atan2(r21, r22);
            outR.z = std::atan2(r10, r00);
        } else {
            // gimbal lock
            outR.x = std::atan2(-r12, r11);
            outR.z = 0;
        }
    }

}
