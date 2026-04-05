// NkMath/Float.h — fonctions utilitaires obligatoires 
#pragma once 

#include <cmath> 
#include <limits> 
#include <cstdint> 
#include <cstring> 
#include <vector>
#include <bitset>

#include "NKLogger/NkLog.h"
#include "NKContainers/String/NkStringUtils.h"

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
    float kahanSum(std::vector<float>& data){
        float sum  = 0.0f; 
        float comp = 0.0f;  // compensation des erreurs perdues 
    
        for(int i = 0; i < data.size(); i++) { 
            float y = data[i] - comp;    // compenser l'erreur précédente 
            float t = sum + y;           // t est grand, y est petit → perte de bits 
            comp    = (t - sum) - y;     // capture les bits perdus dans y 
            sum     = t; 
        } 
        return sum; 
    }

    // Sommation de Kahan 
    float kahanSum(std::vector<double>& data){
        float sum  = 0.0f; 
        float comp = 0.0f;  // compensation des erreurs perdues 
    
        for(int i = 0; i < data.size(); i++) { 
            float y = data[i] - comp;    // compenser l'erreur précédente 
            float t = sum + y;           // t est grand, y est petit → perte de bits 
            comp    = (t - sum) - y;     // capture les bits perdus dans y 
            sum     = t; 
        } 
        return sum; 
    }

    // Inspecter un float, écrit en binaire
    //Norme IEEE 754 : valeur = (-1)^signe × 1.mantisse × 2^(exposant - 127)
    void inspectFloat(float x){
        // Interpréter les bits du float comme un entier 32 bits
        uint32_t bits;
        std::memcpy(&bits, &x, sizeof(bits));
        
        // Extraire les champs
        uint32_t sign = (bits >> 31) & 0x1;      // Le dernier bit après décalage
        uint32_t exponent = (bits >> 23) & 0xFF; // Les 8 derniers bits après décalage
        uint32_t mantissa = bits & 0x7FFFFF;     // Les 23 derniers bits après décalage
        
        // Affichage en binaire
        logger.Info("Affichage du float {0} en format binaire :\n- Signe    : {1}\n- Exposant : {2}\n- Mantisse : {3}",
            x,
            sign,
            std::bitset<8>(exponent).to_string().c_str(),
            std::bitset<23>(mantissa).to_string().c_str()
        );
    }

    //Norme IEEE 754 : valeur = (-1)^s × 1.mantisse × 2^(exposant - 1023)
    void inspectDouble(double x){
        // Interpréter les bits du double comme un entier 64 bits
        uint64_t bits;
        std::memcpy(&bits, &x, sizeof(bits));

        uint64_t sign = (bits >> 63) & 0x1;         // Le dernier bit après décalage
        uint64_t exponent = (bits >> 52) & 0x7FF;   // Les 11 derniers bits après décalage
        uint64_t mantissa = bits & 0xFFFFFFFFFFFFF; // Les 52 derniers bits après décalage
        
        // Affichage en binaire
        logger.Info("Affichage du double {0} en format binaire :\n- Signe    : {1}\n- Exposant : {2}\n- Mantisse : {3}",
            x,
            sign,        
            std::bitset<11>(exponent).to_string().c_str(),
            std::bitset<52>(mantissa).to_string().c_str()
        );
    }

    // Variance naïve : moyenne des carrés - carré de la moyenne
    float varianceNaive(const std::vector<float>& data){
        float sum = 0.0f, sumSq = 0.0f;

        for (float x : data) {
            sum += x;
            sumSq += x * x;
        }

        float mean = sum / data.size();
        return (sumSq / data.size()) - (mean * mean);
    }

    // Formule de Welford
    float varianceWelford(const std::vector<float>& data){
        float mean = 0.0f;
        float M2 = 0.0f;
        int n = 0;

        for (float x : data) {
            n++;
            float delta = x - mean;
            mean += delta / n;
            float delta2 = x - mean;
            M2 += delta * delta2;
        }

        return M2 / n;
    }

    // Epsilon machine par boucle
    float epsilonMachine(){
        float eps = 1.0f;

        while ((1.0f + eps / 2.0f) > 1.0f) {
            eps /= 2.0f;
        }

        return eps;
    }

} // namespace NkMath
