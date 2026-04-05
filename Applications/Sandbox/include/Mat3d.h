#pragma once 
#include "Vec3d.h" 
#include <array> 
 
namespace NkMath { 
    
    struct Mat3d { 
        // Stockage column-major : data[col*3 + row] 
        double data[9];     
        Mat3d() { std::fill(data, data+9, 0.0); } 
    
        // Accès row,col 
        double& operator()(int row, int col) { 
            assert(row>=0 && row<3 && col>=0 && col<3); 
            return data[col*3 + row];  // column-major 
        } 
        const double& operator()(int row, int col) const { 
            return data[col*3 + row]; 
        } 
    
        // Matrice identité 
        static Mat3d Identity() { 
            Mat3d m; 
            m(0,0) = m(1,1) = m(2,2) = 1.0; 
            return m; 
        } 
    
        // Produit matriciel O(64 multiplications pour 4×4) 
        Mat3d operator*(const Mat3d& o) const { 
            Mat3d result; 
            for(int row = 0; row < 3; row++) 
                for(int col = 0; col < 3; col++) { 
                    double sum = 0.0; 
                    for(int k = 0; k < 3; k++) 
                        sum += (*this)(row, k) * o(k, col); 
                    result(row, col) = sum; 
                } 
            return result; 
        } 
    
        // Multiplication Mat3d × Vec3d 
        Vec3d operator*(const Vec3d& v) const { 
            return { 
                (*this)(0,0)*v.x + (*this)(0,1)*v.y + (*this)(0,2)*v.z, 
                (*this)(1,0)*v.x + (*this)(1,1)*v.y + (*this)(1,2)*v.z, 
                (*this)(2,0)*v.x + (*this)(2,1)*v.y + (*this)(2,2)*v.z 
            }; 
        } 
    
        // Transposée 
        Mat3d Transposed() const { 
            Mat3d t; 
            for(int r = 0; r < 3; r++) 
                for(int c = 0; c < 3; c++) 
                    t(r,c) = (*this)(c,r); 
            return t; 
        } 
    
        // ========================
        // Déterminant
        // ========================
        double Det() const {
            return
                (*this)(0,0)*((*this)(1,1)*(*this)(2,2) - (*this)(1,2)*(*this)(2,1)) -
                (*this)(0,1)*((*this)(1,0)*(*this)(2,2) - (*this)(1,2)*(*this)(2,0)) +
                (*this)(0,2)*((*this)(1,0)*(*this)(2,1) - (*this)(1,1)*(*this)(2,0));
        }

        // Pointeur vers les données pour upload GPU 
        const double* DataPtr() const { return data; } 

        // Pour upload float GPU (convertir en float): 
        void ToFloat(float out[9]) const { 
            for(int i=0; i<9; i++) out[i] = (float)data[i]; 
        } 
        
        // Matrice de rotation — formule de Rodrigues 
        // La rotation d'angle θ autour d'un axe unitaire n = (nx, ny, nz) est donnée par la formule de Rodrigues : 
        static Mat3d RotateAxis(const Vec3d& axis, double angleRad) { 
            Vec3d n = axis.Normalized(); 
            double c = std::cos(angleRad); 
            double s = std::sin(angleRad); 
            double t = 1.0 - c; 
        
            Mat3d R = Mat3d::Identity(); 
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
    }; 

    // Inverse par Gauss-Jordan avec pivot partiel 
    // Retourne false si la matrice est singulière (non inversible) 
    bool Inverse(const Mat3d& m, Mat3d& out) { 
        // Matrice augmentée [m | I] 
        double aug[3][6]; 
        for(int r = 0; r < 3; r++) 
            for(int c = 0; c < 3; c++) { 
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

    inline bool ApproxMat(const Mat3d& mat1, const Mat3d& mat2, double eps = kEps) {
        for(int i=0;i<3;i++)
            for(int j=0;j<3;j++)
                if(!approxEq(mat1(i, j), mat2(i, j), eps))
                    return false;
        return true;
    }

    // ========================
    // Matrice intrinsèque (caméra AR)
    // ========================
    static Mat3d Intrinsics(double fx, double fy, double cx, double cy){
        Mat3d K = Mat3d::Identity();
        K(0, 0) = fx;
        K(1, 1) = fy;
        K(0, 2) = cx;
        K(1, 2) = cy;
        return K;
    }
    
    static Mat3d Translate(const Vec2d& t){
        Mat3d T = Mat3d::Identity();
        T(0, 2) = t.x;
        T(1, 2) = t.y;
        return T;
    }

    static Mat3d Scale(const Vec2d& s){
        Mat3d S = Mat3d::Identity();
        S(0, 0) = s.x;
        S(1, 1) = s.y;
        return S;
    }

    // ========================
    // Transformations 2D (TRS)
    // ========================
    static Mat3d TRS(const Vec2d& t, const Vec3d& axis, double angle, const Vec2d& s) { 
        return Translate(t) * Mat3d::RotateAxis(axis, angle) * Scale(s); 
    } 

    static Mat3d TRS(const Vec2d& t, const Vec3d& r, const Vec2d& s){
        Mat3d rotation = Mat3d::RotateAxis({0, 0, 1}, r.z) * Mat3d::RotateAxis({0, 1, 0}, r.y) * Mat3d::RotateAxis({1, 0, 0}, r.x);
        return Translate(t) * rotation * Scale(s);
    }

    // ========================
    // Décomposition TRS 2D
    // ========================
    void DecomposeTRS(const Mat3d& m, Vec2d& outT, double& outR, Vec2d& outS){
        // 1. translation
        outT = { m(0,2), m(1,2) };

        // 2. scale = norme des colonnes
        Vec2d col0{ m(0,0), m(1,0) };
        Vec2d col1{ m(0,1), m(1,1) };

        double sx = col0.Norm();
        double sy = col1.Norm();

        outS = { sx, sy };

        // 3. matrice rotation pure (normalisation)
        double r00 = m(0,0) / sx;
        double r01 = m(0,1) / sy;

        double r10 = m(1,0) / sx;
        double r11 = m(1,1) / sy;

        // 4. extraction angle (équivalent Euler 2D)
        outR = std::atan2(r10, r00);
    }
}
