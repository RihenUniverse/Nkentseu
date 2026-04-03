// NkMath/Float.h — fonctions utilitaires obligatoires 
#pragma once 

#include <cmath> 
#include <limits> 
#include <cstdint> 
#include <cstring> 
#include <vector>
 
namespace NkMath { 
 
    // Constantes 
    constexpr double kEps  = 1e-9;            // epsilon double pour tests 
    constexpr float  kFEps = 1e-6f;           // epsilon float pour tests 
    
    // Valide : ni NaN, ni Inf 
    inline bool isFiniteValid(double x) { return std::isfinite(x); } 
    inline bool isFiniteValid(float  x) { return std::isfinite(x); } 
    
    // Proche de zéro 
    inline bool nearlyZero(double x, double eps = kEps)  { return std::abs(x) < eps; } 
    inline bool nearlyZero(float  x, float  eps = kFEps) { return std::abs(x) < eps; } 
    
    // Égalité relative 
    inline bool approxEq(double a, double b, double eps = kEps) { 
        if(a == b) return true; 
        double maxAB = std::max(std::abs(a), std::abs(b)); 
        return std::abs(a - b) <= eps * std::max(1.0, maxAB); 
    } 

    // Sommation de Kahan 
    float kahanSum(std::vector<float>& data);
    float kahanSum(std::vector<double>& data);

    // Inspecter un float, écrit en binaire
    //Norme IEEE 754 : valeur = (-1)^signe × 1.mantisse × 2^(exposant - 127)
    void inspectFloat(float x);
    //Norme IEEE 754 : valeur = (-1)^s × 1.mantisse × 2^(exposant - 1023)
    void inspectDouble(double x);

    // Variance naïve : moyenne des carrés - carré de la moyenne
    float varianceNaive(const std::vector<float>& data);
    // Formule de Welford
    float varianceWelford(const std::vector<float>& data);

    // Epsilon machine par boucle
    float epsilonMachine();

} // namespace NkMath
