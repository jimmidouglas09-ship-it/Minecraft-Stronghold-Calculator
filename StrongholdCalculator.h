#pragma once
#include "Common.h"

// Structure to represent a chunk candidate within a stronghold cell
struct ChunkCandidate {
    int chunkX;           // Chunk X coordinate
    int chunkZ;           // Chunk Z coordinate
    int strongholdX;      // World X coordinate of stronghold (chunk*16 + 2)
    int strongholdZ;      // World Z coordinate of stronghold (chunk*16 + 2)
    double distanceFromRay; // Distance from the eye of ender ray
    double probability;   // Probability based on distance from ray
};

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

    // New methods for chunk-level precision
    static ChunkCandidate findBestChunkInCell(const StrongholdCell& cell,
        double rayX, double rayZ,
        double directionX, double directionZ,
        double standardDeviation);
    static double calculateDistanceFromPointToRay(double pointX, double pointZ,
        double rayX, double rayZ,
        double directionX, double directionZ);
    static double calculateGaussianProbability(double distance, double standardDeviation);
};
