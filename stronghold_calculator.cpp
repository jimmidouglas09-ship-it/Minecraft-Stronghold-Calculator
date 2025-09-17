// Enhanced stronghold calculator with uncertainty handling and Bedrock eye position fix
#define NOMINMAX
#include "stronghold_calculator.h"
#include <map>
#include <algorithm>
#include <cmath>
#include <vector>

// Distance probabilities from the HTML version
std::map<int, double> distanceProbabilities = {
    {500, 0.0262}, {600, 0.0639}, {800, 0.1705}, {900, 0.1582}, {1000, 0.1427},
    {1100, 0.1204}, {1200, 0.0919}, {1300, 0.1133}, {1400, 0.1139}, {1500, 0.1228},
    {1700, 0.0586}, {1800, 0.0535}, {1900, 0.0610}, {2100, 0.0590}, {2200, 0.0431},
    {2300, 0.0375}, {2400, 0.0292}, {2500, 0.0493}, {2600, 0.0382}, {2700, 0.0347},
    {2800, 0.0258}, {3000, 0.0171}, {3100, 0.0169}, {3200, 0.0189}
};

// Standard deviation for angle measurements (in degrees)
const double ANGLE_STD_DEV = 2.0; // Adjustable based on measurement precision
// Standard deviation for F4 distance measurements (in blocks)
const double F4_DISTANCE_STD_DEV = 25.0; // Adjustable based on F4 precision

std::vector<StrongholdCell> strongholdCells;
std::vector<StrongholdCandidate> strongholdCandidates;

// Helper function to calculate Gaussian probability
double gaussianProbability(double x, double mean, double stdDev) {
    if (stdDev <= 0) return 0.0;
    double exponent = -0.5 * std::pow((x - mean) / stdDev, 2);
    return std::exp(exponent) / (stdDev * std::sqrt(2.0 * M_PI));
}

// Generate multiple angle samples for uncertainty
std::vector<double> generateAngleSamples(double centerAngle, int numSamples = 5) {
    std::vector<double> angles;
    for (int i = 0; i < numSamples; i++) {
        double offset = (i - numSamples / 2) * (ANGLE_STD_DEV / 2.0);
        angles.push_back(centerAngle + offset);
    }
    return angles;
}

// Generate distance samples for F4 uncertainty
std::vector<double> generateDistanceSamples(double centerDistance, int numSamples = 5) {
    std::vector<double> distances;
    for (int i = 0; i < numSamples; i++) {
        double offset = (i - numSamples / 2) * (F4_DISTANCE_STD_DEV / 2.0);
        distances.push_back(std::max(0.0, centerDistance + offset));
    }
    return distances;
}

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

    // BEDROCK FIX: Eye of ender starts flying from (playerX + 0.5, playerZ + 0.5)
    double eyeStartX = playerX + 0.5;
    double eyeStartZ = playerZ + 0.5;

    // Use F4 distance if F4 was pressed first
    bool useTargetDistance = false;
    if (appState.f4PressedFirst && appState.calculatedDistance > 0) {
        useTargetDistance = true;
        targetDistance = appState.calculatedDistance;
    }

    // Map to accumulate probabilities for each cell
    std::map<const StrongholdCell*, double> cellProbabilities;
    std::map<const StrongholdCell*, std::vector<std::pair<double, double>>> cellProjections;

    // Generate angle samples to account for uncertainty
    std::vector<double> angleSamples = generateAngleSamples(eyeAngle);

    // Generate distance samples if using F4
    std::vector<double> distanceSamples;
    if (useTargetDistance) {
        distanceSamples = generateDistanceSamples(targetDistance);
    }
    else {
        distanceSamples.push_back(0); // Placeholder for non-F4 case
    }

    // Process each combination of angle and distance samples
    for (double angleTest : angleSamples) {
        double angleRad = angleTest * M_PI / 180.0;
        double dx = std::sin(angleRad);
        double dz = -std::cos(angleRad);

        // Weight for this angle sample
        double angleWeight = gaussianProbability(angleTest, eyeAngle, ANGLE_STD_DEV);

        if (useTargetDistance) {
            // F4 case: test multiple distance samples
            for (double distanceTest : distanceSamples) {
                double distanceWeight = gaussianProbability(distanceTest, targetDistance, F4_DISTANCE_STD_DEV);
                double combinedWeight = angleWeight * distanceWeight;

                // Calculate the point at this distance along this ray FROM THE EYE START POSITION
                double exactX = eyeStartX + distanceTest * dx;
                double exactZ = eyeStartZ + distanceTest * dz;

                // Check all cells for intersection or proximity
                for (auto& cell : strongholdCells) {
                    // Find the closest point on this cell to the exact point
                    double clampedX = std::max(cell.xMin, std::min(cell.xMax, exactX));
                    double clampedZ = std::max(cell.zMin, std::min(cell.zMax, exactZ));

                    double distanceToCell = std::sqrt(
                        std::pow(clampedX - exactX, 2) + std::pow(clampedZ - exactZ, 2)
                    );

                    // Allow up to 50 blocks deviation from F4 distance
                    if (distanceToCell <= 50.0) {
                        const StrongholdCell* cellPtr = &cell;
                        if (cellProbabilities.count(cellPtr) > 0) {
                            cellProbabilities[cellPtr] += combinedWeight * cell.prob;
                        }
                        else {
                            cellProbabilities[cellPtr] = combinedWeight * cell.prob;
                        }
                        cellProjections[cellPtr].push_back({ clampedX, clampedZ });
                    }
                }

                // Also consider exact F4 points that don't hit any cell
                bool hitAnyCell = false;
                for (const auto& cell : strongholdCells) {
                    double clampedX = std::max(cell.xMin, std::min(cell.xMax, exactX));
                    double clampedZ = std::max(cell.zMin, std::min(cell.zMax, exactZ));
                    double distanceToCell = std::sqrt(
                        std::pow(clampedX - exactX, 2) + std::pow(clampedZ - exactZ, 2)
                    );
                    if (distanceToCell <= 50.0) {
                        hitAnyCell = true;
                        break;
                    }
                }

                // If no cell was hit, add as standalone candidate
                if (!hitAnyCell) {
                    // Create a virtual "cell" for non-cell locations
                    static std::vector<StrongholdCell> virtualCells;
                    StrongholdCell virtualCell;
                    virtualCell.centerX = exactX;
                    virtualCell.centerZ = exactZ;
                    virtualCell.xMin = exactX - 1;
                    virtualCell.xMax = exactX + 1;
                    virtualCell.zMin = exactZ - 1;
                    virtualCell.zMax = exactZ + 1;
                    virtualCell.prob = 0.05; // Lower probability for non-cell locations
                    virtualCell.distance = std::sqrt(exactX * exactX + exactZ * exactZ);
                    virtualCell.distanceRange = (int)std::round(virtualCell.distance / 100) * 100;

                    virtualCells.push_back(virtualCell);
                    const StrongholdCell* virtualPtr = &virtualCells.back();
                    cellProbabilities[virtualPtr] = combinedWeight * virtualCell.prob;
                    cellProjections[virtualPtr].push_back({ exactX, exactZ });
                }
            }
        }
        else {
            // Non-F4 case: ray-casting logic with angle uncertainty, but from eye start position
            for (const auto& cell : strongholdCells) {
                double toCenterX = cell.centerX - eyeStartX;
                double toCenterZ = cell.centerZ - eyeStartZ;
                double t = (toCenterX * dx + toCenterZ * dz);

                if (t > 0) {
                    double projectionX = eyeStartX + t * dx;
                    double projectionZ = eyeStartZ + t * dz;

                    if (projectionX >= cell.xMin && projectionX <= cell.xMax &&
                        projectionZ >= cell.zMin && projectionZ <= cell.zMax) {

                        double clampedX = std::max(cell.xMin, std::min(cell.xMax, projectionX));
                        double clampedZ = std::max(cell.zMin, std::min(cell.zMax, projectionZ));

                        const StrongholdCell* cellPtr = &cell;
                        if (cellProbabilities.count(cellPtr) > 0) {
                            cellProbabilities[cellPtr] += angleWeight * cell.prob;
                        }
                        else {
                            cellProbabilities[cellPtr] = angleWeight * cell.prob;
                        }
                        cellProjections[cellPtr].push_back({ clampedX, clampedZ });
                    }
                }
            }
        }
    }

    // Convert accumulated probabilities to candidates
    for (const auto& pair : cellProbabilities) {
        const StrongholdCell* cell = pair.first;
        double accumulatedProb = pair.second;

        if (accumulatedProb > 0 && cellProjections.count(cell) > 0 && !cellProjections.at(cell).empty()) {
            // Average the projection points
            double avgX = 0, avgZ = 0;
            const auto& projections = cellProjections.at(cell);
            for (const auto& projection : projections) {
                avgX += projection.first;
                avgZ += projection.second;
            }
            avgX /= projections.size();
            avgZ /= projections.size();

            // Calculate distance from PLAYER position to projection (for display purposes)
            double distanceToProjection = std::sqrt(
                std::pow(avgX - playerX, 2) + std::pow(avgZ - playerZ, 2)
            );

            StrongholdCandidate candidate;
            candidate.projectionX = (int)std::round(avgX);
            candidate.projectionZ = (int)std::round(avgZ);
            candidate.netherX = (int)std::round(avgX / 8.0);
            candidate.netherZ = (int)std::round(avgZ / 8.0);
            candidate.cellCenterX = cell->centerX;
            candidate.cellCenterZ = cell->centerZ;
            candidate.rawProb = accumulatedProb;
            candidate.distance = (int)std::round(distanceToProjection);
            candidate.distanceFromOrigin = (int)std::round(cell->distance);
            candidate.distanceRange = cell->distanceRange;

            if (cell->xMin == cell->centerX - 1 && cell->xMax == cell->centerX + 1) {
                // Virtual cell (exact F4 point)
                candidate.bounds = L"Exact F4 distance point";
            }
            else {
                std::wstringstream ss;
                ss << L"(" << (int)cell->xMin << L", " << (int)cell->zMin
                    << L") to (" << (int)cell->xMax << L", " << (int)cell->zMax << L")";
                candidate.bounds = ss.str();
            }

            strongholdCandidates.push_back(candidate);
        }
    }

    // Calculate conditional probabilities
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
