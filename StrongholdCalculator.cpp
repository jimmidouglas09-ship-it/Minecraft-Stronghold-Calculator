#include "StrongholdCalculator.h"

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

void StrongholdCalculator::calculateStrongholdFromAngle(double playerX, double playerZ, double angle) {
    appState.strongholdCandidates.clear();

    double eyeStartX = playerX + 0.5;
    double eyeStartZ = playerZ + 0.5;

    double angleRad = angle * M_PI / 180.0;
    double dx = sin(angleRad);
    double dz = -cos(angleRad);

    vector<StrongholdCandidate> candidates;

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

                StrongholdCandidate candidate;
                candidate.projectionX = (int)round(projectionX);
                candidate.projectionZ = (int)round(projectionZ);
                candidate.netherX = (int)round(projectionX / 8.0);
                candidate.netherZ = (int)round(projectionZ / 8.0);
                candidate.cellCenterX = cell.centerX;
                candidate.cellCenterZ = cell.centerZ;
                candidate.distance = sqrt(pow(projectionX - playerX, 2) + pow(projectionZ - playerZ, 2));
                candidate.distanceFromOrigin = (int)round(cell.distance);
                candidate.distanceRange = cell.distanceRange;
                candidate.probability = calculateProbability(candidate);
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

        // Take up to 3 most likely candidates
        for (size_t i = 0; i < min((size_t)3, candidates.size()); i++) {
            appState.strongholdCandidates.push_back(candidates[i]);
        }

        appState.status = "Stronghold locations calculated from angle projection";
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

    double x1 = throw1.position.x + 0.5;
    double z1 = throw1.position.z + 0.5;
    double angle1Rad = throw1.angle * M_PI / 180.0;
    double dx1 = sin(angle1Rad);
    double dz1 = -cos(angle1Rad);

    double x2 = throw2.position.x + 0.5;
    double z2 = throw2.position.z + 0.5;
    double angle2Rad = throw2.angle * M_PI / 180.0;
    double dx2 = sin(angle2Rad);
    double dz2 = -cos(angle2Rad);

    double intersectionX, intersectionZ;
    if (linesIntersect(x1, z1, dx1, dz1, x2, z2, dx2, dz2, intersectionX, intersectionZ)) {
        // Found intersection point - this is likely the stronghold
        StrongholdCandidate triangulated;
        triangulated.projectionX = (int)round(intersectionX);
        triangulated.projectionZ = (int)round(intersectionZ);
        triangulated.netherX = (int)round(intersectionX / 8.0);
        triangulated.netherZ = (int)round(intersectionZ / 8.0);
        triangulated.distance = sqrt(pow(intersectionX - throw1.position.x, 2) + pow(intersectionZ - throw1.position.z, 2));
        triangulated.distanceFromOrigin = (int)round(sqrt(intersectionX * intersectionX + intersectionZ * intersectionZ));
        triangulated.probability = 0.95; // High confidence from triangulation
        triangulated.isTriangulated = true;

        appState.strongholdCandidates.push_back(triangulated);
        appState.status = "Stronghold location found via triangulation";
        appState.showResults = true;
        appState.mode = MODE_RESULTS;
    }
    else {
        // Lines are parallel or don't intersect well - fall back to single angle calculation
        calculateStrongholdFromAngle(throw1.position.x, throw1.position.z, throw1.angle);
        appState.status = "Triangulation failed - using first throw only";
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