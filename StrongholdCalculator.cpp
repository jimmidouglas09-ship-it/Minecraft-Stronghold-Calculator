#include "StrongholdCalculator.h"
#include <random>

void StrongholdCalculator::initializeDistanceProbabilities() {
    distanceProbabilities = {
        {500, 0.0262}, {600, 0.0639}, {800, 0.1705}, {900, 0.1582}, {1000, 0.1427},
        {1100, 0.1204}, {1200, 0.0919}, {1300, 0.1133}, {1400, 0.1139}, {1500, 0.1228},
        {1700, 0.0586}, {1800, 0.0535}, {1900, 0.0610}, {2100, 0.0590}, {2200, 0.0431},
        {2300, 0.0375}, {2400, 0.0292}, {2500, 0.0493}, {2600, 0.0382}, {2700, 0.0347},
        {2800, 0.0258}, {3000, 0.0171}, {3100, 0.0169}, {3200, 0.0189}
    };
}

void StrongholdCalculator::generateStrongholdCells() {
    strongholdCells.clear();
    int cellSize = 272;
    int gap = 160;
    int totalStep = cellSize + gap;

    for (int xIndex = -15; xIndex <= 15; xIndex++) {
        for (int zIndex = -15; zIndex <= 15; zIndex++) {
            double xMin, xMax, zMin, zMax;

            if (xIndex >= 0) {
                xMin = xIndex * totalStep;
                xMax = xMin + cellSize;
            }
            else {
                xMax = xIndex * totalStep - gap;
                xMin = xMax - cellSize;
            }

            if (zIndex >= 0) {
                zMin = zIndex * totalStep;
                zMax = zMin + cellSize;
            }
            else {
                zMax = zIndex * totalStep - gap;
                zMin = zMax - cellSize;
            }

            double centerX = (xMin + xMax) / 2.0;
            double centerZ = (zMin + zMax) / 2.0;
            double distanceFromOrigin = sqrt(centerX * centerX + centerZ * centerZ);

            if (distanceFromOrigin >= 512) {
                int closestDistance = 500;
                double minDiff = abs(500 - (int)distanceFromOrigin);

                for (auto& pair : distanceProbabilities) {
                    double diff = abs(pair.first - (int)distanceFromOrigin);
                    if (diff < minDiff) {
                        minDiff = diff;
                        closestDistance = pair.first;
                    }
                }

                StrongholdCell cell;
                cell.centerX = centerX;
                cell.centerZ = centerZ;
                cell.xMin = xMin;
                cell.xMax = xMax;
                cell.zMin = zMin;
                cell.zMax = zMax;
                cell.prob = distanceProbabilities[closestDistance];
                cell.distance = distanceFromOrigin;
                cell.distanceRange = closestDistance;

                strongholdCells.push_back(cell);
            }
        }
    }
}

// Helper function to find the closest chunk center (2,2) point within a cell
ChunkCandidate StrongholdCalculator::findBestChunkInCell(const StrongholdCell& cell,
    double rayX, double rayZ,
    double directionX, double directionZ,
    double standardDeviation) {
    vector<ChunkCandidate> chunkCandidates;

    // Generate all possible chunk centers within the cell
    int cellXMin = (int)floor(cell.xMin / 16);
    int cellXMax = (int)floor(cell.xMax / 16);
    int cellZMin = (int)floor(cell.zMin / 16);
    int cellZMax = (int)floor(cell.zMax / 16);

    for (int chunkX = cellXMin; chunkX <= cellXMax; chunkX++) {
        for (int chunkZ = cellZMin; chunkZ <= cellZMax; chunkZ++) {
            // Check if chunk is actually within the cell bounds
            double chunkWorldXMin = chunkX * 16.0;
            double chunkWorldXMax = chunkWorldXMin + 16.0;
            double chunkWorldZMin = chunkZ * 16.0;
            double chunkWorldZMax = chunkWorldZMin + 16.0;

            // Check overlap with cell
            if (chunkWorldXMax > cell.xMin && chunkWorldXMin < cell.xMax &&
                chunkWorldZMax > cell.zMin && chunkWorldZMin < cell.zMax) {

                // The stronghold generates at the 2,2 point of the chunk
                double strongholdX = chunkX * 16.0 + 2.0;
                double strongholdZ = chunkZ * 16.0 + 2.0;

                // Calculate distance from ray to this stronghold point
                double distanceToRay = calculateDistanceFromPointToRay(strongholdX, strongholdZ,
                    rayX, rayZ,
                    directionX, directionZ);

                // Calculate probability based on distance and standard deviation
                double probability = calculateGaussianProbability(distanceToRay, standardDeviation);

                ChunkCandidate candidate;
                candidate.chunkX = chunkX;
                candidate.chunkZ = chunkZ;
                candidate.strongholdX = (int)strongholdX;
                candidate.strongholdZ = (int)strongholdZ;
                candidate.distanceFromRay = distanceToRay;
                candidate.probability = probability;

                chunkCandidates.push_back(candidate);
            }
        }
    }

    // Find the chunk with highest probability (closest to ray)
    if (!chunkCandidates.empty()) {
        sort(chunkCandidates.begin(), chunkCandidates.end(),
            [](const ChunkCandidate& a, const ChunkCandidate& b) {
                return a.probability > b.probability;
            });
        return chunkCandidates[0];
    }

    // Fallback if no chunks found
    ChunkCandidate fallback;
    fallback.chunkX = (int)floor(cell.centerX / 16);
    fallback.chunkZ = (int)floor(cell.centerZ / 16);
    fallback.strongholdX = fallback.chunkX * 16 + 2;
    fallback.strongholdZ = fallback.chunkZ * 16 + 2;
    fallback.distanceFromRay = 0.0;
    fallback.probability = 0.1;
    return fallback;
}

double StrongholdCalculator::calculateDistanceFromPointToRay(double pointX, double pointZ,
    double rayX, double rayZ,
    double directionX, double directionZ) {
    // Calculate the perpendicular distance from a point to a ray
    // Using the formula: ||(P - R) - ((P - R) · D) * D|| where P=point, R=ray origin, D=direction

    double vectorToPointX = pointX - rayX;
    double vectorToPointZ = pointZ - rayZ;

    double dotProduct = vectorToPointX * directionX + vectorToPointZ * directionZ;

    double projectionX = rayX + dotProduct * directionX;
    double projectionZ = rayZ + dotProduct * directionZ;

    double distanceX = pointX - projectionX;
    double distanceZ = pointZ - projectionZ;

    return sqrt(distanceX * distanceX + distanceZ * distanceZ);
}

double StrongholdCalculator::calculateGaussianProbability(double distance, double standardDeviation) {
    // Calculate probability using Gaussian distribution
    // Higher probability for points closer to the ray
    double exponent = -(distance * distance) / (2.0 * standardDeviation * standardDeviation);
    return exp(exponent);
}

void StrongholdCalculator::calculateStrongholdFromAngle(double playerX, double playerZ, double angle) {
    appState.strongholdCandidates.clear();

    // Apply eye offset: +0.8 to both x and z
    double eyeStartX = playerX + 0.8;
    double eyeStartZ = playerZ + 0.8;

    double angleRad = angle * M_PI / 180.0;
    double dx = sin(angleRad);
    double dz = -cos(angleRad);

    vector<StrongholdCandidate> candidates;

    // Standard deviation for single eye measurement (adjust based on measurement accuracy)
    double singleEyeStdDev = 8.0; // blocks

    // Project the line and find intersections with stronghold cells
    for (const auto& cell : strongholdCells) {
        // Check if the line intersects this cell
        double toCenterX = cell.centerX - eyeStartX;
        double toCenterZ = cell.centerZ - eyeStartZ;
        double t = (toCenterX * dx + toCenterZ * dz);

        if (t > 0) { // Only forward projections
            double projectionX = eyeStartX + t * dx;
            double projectionZ = eyeStartZ + t * dz;

            // Check if projection is within cell bounds
            if (projectionX >= cell.xMin && projectionX <= cell.xMax &&
                projectionZ >= cell.zMin && projectionZ <= cell.zMax) {

                // Find the best chunk within this cell
                ChunkCandidate bestChunk = findBestChunkInCell(cell, eyeStartX, eyeStartZ, dx, dz, singleEyeStdDev);

                StrongholdCandidate candidate;
                candidate.projectionX = bestChunk.strongholdX;
                candidate.projectionZ = bestChunk.strongholdZ;
                candidate.netherX = (int)round(bestChunk.strongholdX / 8.0);
                candidate.netherZ = (int)round(bestChunk.strongholdZ / 8.0);
                candidate.cellCenterX = cell.centerX;
                candidate.cellCenterZ = cell.centerZ;
                candidate.chunkX = bestChunk.chunkX;
                candidate.chunkZ = bestChunk.chunkZ;
                candidate.distance = sqrt(pow(bestChunk.strongholdX - playerX, 2) + pow(bestChunk.strongholdZ - playerZ, 2));
                candidate.distanceFromOrigin = (int)round(cell.distance);
                candidate.distanceRange = cell.distanceRange;
                candidate.rayDistance = bestChunk.distanceFromRay;

                // Combine cell probability with chunk precision probability
                double cellProbability = calculateProbability(candidate);
                candidate.probability = cellProbability * bestChunk.probability;
                candidate.isTriangulated = false;

                candidates.push_back(candidate);
            }
        }
    }

    if (!candidates.empty()) {
        // Sort by probability (highest first), then by distance if probabilities are similar
        sort(candidates.begin(), candidates.end(), [](const StrongholdCandidate& a, const StrongholdCandidate& b) {
            if (abs(a.probability - b.probability) < 0.001) {
                return a.distance < b.distance;
            }
            return a.probability > b.probability;
            });

        // Take up to 5 most likely candidates (increased for better chunk precision)
        for (size_t i = 0; i < min((size_t)5, candidates.size()); i++) {
            appState.strongholdCandidates.push_back(candidates[i]);
        }

        appState.status = "Stronghold locations calculated with chunk precision";
        appState.showResults = true;
        appState.mode = MODE_RESULTS;
    }
    else {
        // Fallback: project to a reasonable distance and give approximate coordinates
        double fallbackDistance = 1500; // Default assumption
        double fallbackX = eyeStartX + dx * fallbackDistance;
        double fallbackZ = eyeStartZ + dz * fallbackDistance;

        StrongholdCandidate fallback;
        fallback.projectionX = (int)round(fallbackX);
        fallback.projectionZ = (int)round(fallbackZ);
        fallback.netherX = (int)round(fallbackX / 8.0);
        fallback.netherZ = (int)round(fallbackZ / 8.0);
        fallback.distance = fallbackDistance;
        fallback.distanceFromOrigin = (int)round(sqrt(fallbackX * fallbackX + fallbackZ * fallbackZ));
        fallback.probability = 0.5; // Unknown probability
        fallback.isTriangulated = false;

        appState.strongholdCandidates.push_back(fallback);
        appState.status = "Approximate location (no exact cell match)";
        appState.showResults = true;
        appState.mode = MODE_RESULTS;
    }
}

void StrongholdCalculator::calculateStrongholdFromTriangulation(const ThrowData& throw1, const ThrowData& throw2) {
    appState.strongholdCandidates.clear();

    // Apply eye offset: +0.8 to both x and z for both throws
    double x1 = throw1.position.x + 0.8;
    double z1 = throw1.position.z + 0.8;
    double angle1Rad = throw1.angle * M_PI / 180.0;
    double dx1 = sin(angle1Rad);
    double dz1 = -cos(angle1Rad);

    double x2 = throw2.position.x + 0.8;
    double z2 = throw2.position.z + 0.8;
    double angle2Rad = throw2.angle * M_PI / 180.0;
    double dx2 = sin(angle2Rad);
    double dz2 = -cos(angle2Rad);

    double intersectionX, intersectionZ;
    if (linesIntersect(x1, z1, dx1, dz1, x2, z2, dx2, dz2, intersectionX, intersectionZ)) {
        // Found intersection point - now find the best chunk near this point

        // Standard deviation for triangulated measurement (more precise)
        double triangulatedStdDev = 4.0; // blocks

        // Find which cell contains this intersection
        StrongholdCell* containingCell = nullptr;
        for (auto& cell : strongholdCells) {
            if (intersectionX >= cell.xMin && intersectionX <= cell.xMax &&
                intersectionZ >= cell.zMin && intersectionZ <= cell.zMax) {
                containingCell = &cell;
                break;
            }
        }

        if (containingCell != nullptr) {
            // Use the intersection point as our "ray" to find best chunk
            ChunkCandidate bestChunk = findBestChunkInCell(*containingCell,
                intersectionX, intersectionZ,
                0.0, 0.0, // No specific direction for triangulation
                triangulatedStdDev);

            StrongholdCandidate triangulated;
            triangulated.projectionX = bestChunk.strongholdX;
            triangulated.projectionZ = bestChunk.strongholdZ;
            triangulated.netherX = (int)round(bestChunk.strongholdX / 8.0);
            triangulated.netherZ = (int)round(bestChunk.strongholdZ / 8.0);
            triangulated.chunkX = bestChunk.chunkX;
            triangulated.chunkZ = bestChunk.chunkZ;
            triangulated.distance = sqrt(pow(bestChunk.strongholdX - throw1.position.x, 2) +
                pow(bestChunk.strongholdZ - throw1.position.z, 2));
            triangulated.distanceFromOrigin = (int)round(sqrt(bestChunk.strongholdX * bestChunk.strongholdX +
                bestChunk.strongholdZ * bestChunk.strongholdZ));
            triangulated.probability = 0.95 * bestChunk.probability; // High confidence from triangulation
            triangulated.isTriangulated = true;
            triangulated.rayDistance = bestChunk.distanceFromRay;

            appState.strongholdCandidates.push_back(triangulated);
            appState.status = "Stronghold location found via triangulation with chunk precision";
        }
        else {
            // Intersection is outside known cells - use raw intersection
            StrongholdCandidate triangulated;
            triangulated.projectionX = (int)round(intersectionX);
            triangulated.projectionZ = (int)round(intersectionZ);
            triangulated.netherX = (int)round(intersectionX / 8.0);
            triangulated.netherZ = (int)round(intersectionZ / 8.0);
            triangulated.distance = sqrt(pow(intersectionX - throw1.position.x, 2) +
                pow(intersectionZ - throw1.position.z, 2));
            triangulated.distanceFromOrigin = (int)round(sqrt(intersectionX * intersectionX +
                intersectionZ * intersectionZ));
            triangulated.probability = 0.90; // High confidence from triangulation
            triangulated.isTriangulated = true;

            appState.strongholdCandidates.push_back(triangulated);
            appState.status = "Stronghold location found via triangulation (outside known cells)";
        }

        appState.showResults = true;
        appState.mode = MODE_RESULTS;
    }
    else {
        // Lines are parallel or don't intersect well - fall back to single angle calculation
        calculateStrongholdFromAngle(throw1.position.x, throw1.position.z, throw1.angle);
        appState.status = "Triangulation failed - using first throw only with chunk precision";
    }
}

double StrongholdCalculator::calculateProbability(const StrongholdCandidate& candidate) {
    // Get base probability from distance distribution
    double baseProbability = getDistanceProbability(candidate.distanceFromOrigin);

    // Normalize by the total number of strongholds at this distance range
    int strongholdCountAtDistance = 0;
    for (const auto& cell : strongholdCells) {
        if (cell.distanceRange == candidate.distanceRange) {
            strongholdCountAtDistance++;
        }
    }

    if (strongholdCountAtDistance > 0) {
        return baseProbability / strongholdCountAtDistance;
    }

    return baseProbability;
}

double StrongholdCalculator::getDistanceProbability(int distanceFromOrigin) {
    // Find the closest distance range
    int closestDistance = 500;
    double minDiff = abs(500 - distanceFromOrigin);

    for (const auto& pair : distanceProbabilities) {
        double diff = abs(pair.first - distanceFromOrigin);
        if (diff < minDiff) {
            minDiff = diff;
            closestDistance = pair.first;
        }
    }

    auto it = distanceProbabilities.find(closestDistance);
    if (it != distanceProbabilities.end()) {
        return it->second;
    }

    return 0.01; // Default low probability
}

bool StrongholdCalculator::linesIntersect(double x1, double z1, double dx1, double dz1,
    double x2, double z2, double dx2, double dz2,
    double& intersectionX, double& intersectionZ) {
    // Calculate intersection of two lines using parametric form
    // Line 1: (x1, z1) + t1 * (dx1, dz1)
    // Line 2: (x2, z2) + t2 * (dx2, dz2)

    double denominator = dx1 * dz2 - dz1 * dx2;

    // Check if lines are parallel (or nearly parallel)
    if (abs(denominator) < 1e-10) {
        return false;
    }

    double t1 = ((x2 - x1) * dz2 - (z2 - z1) * dx2) / denominator;

    // Calculate intersection point
    intersectionX = x1 + t1 * dx1;
    intersectionZ = z1 + t1 * dz1;

    // Verify both t values are positive (intersection is in front of both throws)
    double t2 = ((x1 - x2) * dz1 - (z1 - z2) * dx1) / (-denominator);

    return t1 > 0 && t2 > 0;
}
