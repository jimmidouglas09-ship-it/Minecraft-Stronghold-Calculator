#define NOMINMAX
#include "stronghold_calculator.h"

// Distance probabilities from the HTML version
std::map<int, double> distanceProbabilities = {
    {500, 0.0262}, {600, 0.0639}, {800, 0.1705}, {900, 0.1582}, {1000, 0.1427},
    {1100, 0.1204}, {1200, 0.0919}, {1300, 0.1133}, {1400, 0.1139}, {1500, 0.1228},
    {1700, 0.0586}, {1800, 0.0535}, {1900, 0.0610}, {2100, 0.0590}, {2200, 0.0431},
    {2300, 0.0375}, {2400, 0.0292}, {2500, 0.0493}, {2600, 0.0382}, {2700, 0.0347},
    {2800, 0.0258}, {3000, 0.0171}, {3100, 0.0169}, {3200, 0.0189}
};

std::vector<StrongholdCell> strongholdCells;
std::vector<StrongholdCandidate> strongholdCandidates;

void generateStrongholdCells() {
    strongholdCells.clear();
    int cellSize = 272;
    int gap = 160;
    int totalStep = cellSize + gap; // 432 blocks between cell starts

    for (int xIndex = -15; xIndex <= 15; xIndex++) {
        for (int zIndex = -15; zIndex <= 15; zIndex++) {
            double xMin, xMax, zMin, zMax;

            // Calculate X bounds
            if (xIndex >= 0) {
                xMin = xIndex * totalStep;
                xMax = xMin + cellSize;
            }
            else {
                xMax = xIndex * totalStep - gap;
                xMin = xMax - cellSize;
            }

            // Calculate Z bounds
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

            // Check if this cell is at least 512 blocks from origin
            double distanceFromOrigin = std::sqrt(centerX * centerX + centerZ * centerZ);
            if (distanceFromOrigin >= 512) {
                // Find the closest distance range in our data
                int closestDistance = 500;
                double minDiff = std::abs(500 - (int)distanceFromOrigin);

                for (auto& pair : distanceProbabilities) {
                    double diff = std::abs(pair.first - (int)distanceFromOrigin);
                    if (diff < minDiff) {
                        minDiff = diff;
                        closestDistance = pair.first;
                    }
                }

                double prob = distanceProbabilities[closestDistance];

                StrongholdCell cell;
                cell.centerX = centerX;
                cell.centerZ = centerZ;
                cell.xMin = xMin;
                cell.xMax = xMax;
                cell.zMin = zMin;
                cell.zMax = zMax;
                cell.prob = prob;
                cell.distance = distanceFromOrigin;
                cell.distanceRange = closestDistance;

                strongholdCells.push_back(cell);
            }
        }
    }
}

void calculateStrongholdLocationWithDistance(double playerX, double playerZ, double eyeAngle, double targetDistance) {
    strongholdCandidates.clear();
    appState.distanceValidationFailed = false;
    appState.validationErrorMessage = L"";

    double angleRad = eyeAngle * M_PI / 180.0;
    double dx = std::sin(angleRad);
    double dz = -std::cos(angleRad);

    // Use F4 distance if F4 was pressed first
    bool useTargetDistance = false;
    if (appState.f4PressedFirst && appState.calculatedDistance > 0) {
        useTargetDistance = true;
        targetDistance = appState.calculatedDistance;
    }

    // If we have a target distance from F4, find the exact location or closest cell
    if (useTargetDistance) {
        // Calculate the exact point at the target distance along the ray
        double exactX = playerX + targetDistance * dx;
        double exactZ = playerZ + targetDistance * dz;

        // Find the closest stronghold cell to this exact point
        double closestDistance = DBL_MAX;
        StrongholdCell* closestCell = nullptr;
        double bestClampedX = exactX;
        double bestClampedZ = exactZ;

        for (auto& cell : strongholdCells) {
            // Find the closest point on this cell to the exact point
            double clampedX = std::max(cell.xMin, std::min(cell.xMax, exactX));
            double clampedZ = std::max(cell.zMin, std::min(cell.zMax, exactZ));

            double distanceToCell = std::sqrt(
                std::pow(clampedX - exactX, 2) + std::pow(clampedZ - exactZ, 2)
            );

            if (distanceToCell < closestDistance) {
                closestDistance = distanceToCell;
                closestCell = &cell;
                bestClampedX = clampedX;
                bestClampedZ = clampedZ;
            }
        }

        StrongholdCandidate candidate;

        // If closest cell is within 50 blocks, snap to cell boundary
        if (closestCell && closestDistance <= 50.0) {
            candidate.projectionX = (int)std::round(bestClampedX);
            candidate.projectionZ = (int)std::round(bestClampedZ);
            candidate.cellCenterX = closestCell->centerX;
            candidate.cellCenterZ = closestCell->centerZ;
            candidate.rawProb = closestCell->prob;
            candidate.distanceFromOrigin = (int)std::round(closestCell->distance);
            candidate.distanceRange = closestCell->distanceRange;

            std::wstringstream ss;
            ss << L"(" << (int)closestCell->xMin << L", " << (int)closestCell->zMin
                << L") to (" << (int)closestCell->xMax << L", " << (int)closestCell->zMax << L")";
            candidate.bounds = ss.str();
        }
        else {
            // More than 50 blocks away, use exact F4 distance point
            candidate.projectionX = (int)std::round(exactX);
            candidate.projectionZ = (int)std::round(exactZ);
            candidate.cellCenterX = exactX;
            candidate.cellCenterZ = exactZ;
            candidate.rawProb = 0.1; // Default probability for non-cell locations
            candidate.distanceFromOrigin = (int)std::round(targetDistance);
            candidate.distanceRange = (int)std::round(targetDistance / 100) * 100; // Round to nearest 100
            candidate.bounds = L"Exact F4 distance point";
        }

        candidate.netherX = (int)std::round(candidate.projectionX / 8.0);
        candidate.netherZ = (int)std::round(candidate.projectionZ / 8.0);
        candidate.conditionalProb = 1.0; // 100% since F4 gives exact distance
        candidate.distance = (int)std::round(targetDistance);

        strongholdCandidates.push_back(candidate);
    }
    else {
        // Original logic for when no F4 distance is available
        for (const auto& cell : strongholdCells) {
            double toCenterX = cell.centerX - playerX;
            double toCenterZ = cell.centerZ - playerZ;
            double t = (toCenterX * dx + toCenterZ * dz);

            if (t > 0) {
                double projectionX = playerX + t * dx;
                double projectionZ = playerZ + t * dz;

                if (projectionX >= cell.xMin && projectionX <= cell.xMax &&
                    projectionZ >= cell.zMin && projectionZ <= cell.zMax) {

                    double distanceToProjection = std::sqrt(
                        std::pow(projectionX - playerX, 2) +
                        std::pow(projectionZ - playerZ, 2)
                    );

                    double clampedX = std::max(cell.xMin, std::min(cell.xMax, projectionX));
                    double clampedZ = std::max(cell.zMin, std::min(cell.zMax, projectionZ));

                    StrongholdCandidate candidate;
                    candidate.projectionX = (int)std::round(clampedX);
                    candidate.projectionZ = (int)std::round(clampedZ);
                    candidate.netherX = (int)std::round(clampedX / 8.0);
                    candidate.netherZ = (int)std::round(clampedZ / 8.0);
                    candidate.cellCenterX = cell.centerX;
                    candidate.cellCenterZ = cell.centerZ;
                    candidate.rawProb = cell.prob;
                    candidate.distance = (int)std::round(distanceToProjection);
                    candidate.distanceFromOrigin = (int)std::round(cell.distance);
                    candidate.distanceRange = cell.distanceRange;

                    std::wstringstream ss;
                    ss << L"(" << (int)cell.xMin << L", " << (int)cell.zMin
                        << L") to (" << (int)cell.xMax << L", " << (int)cell.zMax << L")";
                    candidate.bounds = ss.str();

                    strongholdCandidates.push_back(candidate);
                }
            }
        }

        // Calculate conditional probabilities for non-F4 case
        double totalRawProb = 0.0;
        for (const auto& candidate : strongholdCandidates) {
            totalRawProb += candidate.rawProb;
        }

        if (totalRawProb > 0) {
            for (auto& candidate : strongholdCandidates) {
                candidate.conditionalProb = candidate.rawProb / totalRawProb;
            }
        }

        // Sort by conditional probability (highest first)
        std::sort(strongholdCandidates.begin(), strongholdCandidates.end(),
            [](const StrongholdCandidate& a, const StrongholdCandidate& b) {
                return a.conditionalProb > b.conditionalProb;
            });
    }
}