#pragma once

#include "Common.h"

class StrongholdCalculator {
public:
    static void generateStrongholdCells();
    static void calculateStrongholdFromAngle(double playerX, double playerZ, double angle);
    static void calculateStrongholdFromTriangulation(const ThrowData& throw1, const ThrowData& throw2);
    static void initializeDistanceProbabilities();

private:
    static double calculateProbability(const StrongholdCandidate& candidate);
    static double getDistanceProbability(int distanceFromOrigin);
    static bool linesIntersect(double x1, double z1, double dx1, double dz1,
        double x2, double z2, double dx2, double dz2,
        double& intersectionX, double& intersectionZ);
};