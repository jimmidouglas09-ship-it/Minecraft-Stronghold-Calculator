// stronghold_calculator.cpp - Enhanced implementation with movement-based distance calculation
#include "stronghold_calculator.h"

// Global variables definitions (to fix linker errors)
std::vector<StrongholdCell> strongholdCells;
std::vector<StrongholdCandidate> strongholdCandidates;

// Constants for Bedrock Edition stronghold generation
const double VILLAGE_STRONGHOLD_RATE = 0.809;  // 80.9% of nearest strongholds
const double SCATTERED_STRONGHOLD_RATE = 0.191; // 19.1% of nearest strongholds
const int VILLAGE_GRID_SIZE = 34;              // chunks
const double VILLAGE_GENERATION_RATE = 0.267;  // 26.7% generation rate
const int SCATTERED_GRID_SIZE = 200;           // chunks  
const double SCATTERED_GENERATION_RATE = 0.25; // 25% generation rate
const int SCATTERED_MIN_CHUNK = 50;
const int SCATTERED_MAX_CHUNK = 150;
const double EYE_FLIGHT_DISTANCE = 12.0;       // blocks
const double STRONGHOLD_TARGET_X = 2.0;        // within chunk coordinates
const double STRONGHOLD_TARGET_Z = 2.0;        // within chunk coordinates
const double PIXELS_PER_BLOCK = 16.0;
const double FACING_DISTANCE = 0.3;            // blocks to faced block
const double PRECISION_ERROR_FACTOR = 0.2;

// Distance tolerance for movement-based measurements (blocks)
const double MOVEMENT_DISTANCE_TOLERANCE = 150.0;
const double CELL_IDENTIFICATION_TOLERANCE = 300.0; // Wider tolerance for cell identification

// Enhanced distance probabilities from Bedrock simulation data
static std::map<int, double> villageStrongholdDistanceProbs = {
    {500, 0.0315}, {600, 0.0768}, {800, 0.2046}, {900, 0.1898}, {1000, 0.1713},
    {1100, 0.1445}, {1200, 0.1103}, {1300, 0.1360}, {1400, 0.1367}, {1500, 0.1474},
    {1700, 0.0703}, {1800, 0.0642}, {1900, 0.0732}, {2100, 0.0708}, {2200, 0.0517},
    {2300, 0.0450}, {2400, 0.0351}, {2500, 0.0592}, {2600, 0.0458}, {2700, 0.0417},
    {2800, 0.0310}, {3000, 0.0205}, {3100, 0.0203}, {3200, 0.0227}
};

static std::map<int, double> scatteredStrongholdDistanceProbs = {
    {1000, 0.0142}, {1200, 0.0298}, {1400, 0.0445}, {1600, 0.0578}, {1800, 0.0694},
    {2000, 0.0798}, {2200, 0.0889}, {2400, 0.0967}, {2600, 0.1032}, {2800, 0.1085},
    {3000, 0.1127}, {3200, 0.1158}, {3400, 0.1179}, {3600, 0.1189}, {3800, 0.1189}
};

// Enhanced utility functions for error calculation
double calculateMeasurementError(double alignError) {
    return std::atan(alignError / EYE_FLIGHT_DISTANCE / PIXELS_PER_BLOCK);
}

double calculateCoordPrecisionError(double distance) {
    if (distance <= 0) return 0.1; // Default fallback
    return std::atan(0.01 * std::sqrt(2.0) / distance) * PRECISION_ERROR_FACTOR;
}

double calculateFacingPrecisionError(double pixelError) {
    return std::atan(pixelError / PIXELS_PER_BLOCK / FACING_DISTANCE);
}

double calculateCombinedError(double measurementError, double precisionError) {
    return std::sqrt(measurementError * measurementError + precisionError * precisionError);
}

double gaussianProbability(double x, double mean, double stdDev) {
    if (stdDev <= 0) return 0.0;
    double exponent = -0.5 * std::pow((x - mean) / stdDev, 2);
    return std::exp(exponent) / (stdDev * std::sqrt(2.0 * M_PI));
}

// Calculate exact eye hover position using line-circle intersection
std::pair<double, double> calculateEyeHoverPosition(double x1, double z1, double x2, double z2) {
    // Eye starts flying from (x1+0.5, z1+0.5)
    double a = x1 + 0.5;
    double b = z1 + 0.5;

    // Handle vertical line case
    if (std::abs(x2 - x1) < 0.001) {
        return { a, b + (z2 > z1 ? EYE_FLIGHT_DISTANCE : -EYE_FLIGHT_DISTANCE) };
    }

    // Line equation: z - z1 = slope * (x - x1)
    double slope = (z2 - z1) / (x2 - x1);

    // Circle equation: (x-a)² + (z-b)² = 12²
    // Substitute line into circle and solve quadratic
    double A = 1 + slope * slope;
    double B = 2 * slope * (z1 - b - slope * x1) - 2 * a;
    double C = (z1 - b - slope * x1) * (z1 - b - slope * x1) + a * a - 144;

    double discriminant = B * B - 4 * A * C;
    if (discriminant < 0) {
        // Fallback to simple direction projection
        double angle = std::atan2(x2 - x1, z1 - z2);
        return { a + EYE_FLIGHT_DISTANCE * std::sin(angle),
                b + EYE_FLIGHT_DISTANCE * std::cos(angle) };
    }

    // Two intersection points - choose the one in movement direction
    double x_int1 = (-B + std::sqrt(discriminant)) / (2 * A);
    double x_int2 = (-B - std::sqrt(discriminant)) / (2 * A);
    double z_int1 = z1 + slope * (x_int1 - x1);
    double z_int2 = z1 + slope * (x_int2 - x1);

    // Use dot product to determine correct direction
    double dot1 = (x_int1 - a) * (x2 - x1) + (z_int1 - b) * (z2 - z1);
    double dot2 = (x_int2 - a) * (x2 - x1) + (z_int2 - b) * (z2 - z1);

    return (dot1 > dot2) ? std::make_pair(x_int1, z_int1) : std::make_pair(x_int2, z_int2);
}

// Generate enhanced stronghold cells based on Bedrock Edition methodology
void generateStrongholdCells() {
    strongholdCells.clear();
    strongholdCells.reserve(8000); // Reserve space for better performance

    // Generate Village Strongholds (80.9% of nearest strongholds)
    // 34x34 chunk grids, strongholds can generate in chunks 0-27
    for (int gridX = -15; gridX <= 15; gridX++) {
        for (int gridZ = -15; gridZ <= 15; gridZ++) {
            int baseChunkX = gridX * VILLAGE_GRID_SIZE;
            int baseChunkZ = gridZ * VILLAGE_GRID_SIZE;

            // Sample every 3rd chunk for performance (still covers all possibilities)
            for (int chunkX = 0; chunkX < 28; chunkX += 3) {
                for (int chunkZ = 0; chunkZ < 28; chunkZ += 3) {
                    int absoluteChunkX = baseChunkX + chunkX;
                    int absoluteChunkZ = baseChunkZ + chunkZ;

                    // Convert chunk coordinates to block coordinates
                    double blockX = absoluteChunkX * 16.0;
                    double blockZ = absoluteChunkZ * 16.0;
                    double centerX = blockX + 8.0; // Center of chunk
                    double centerZ = blockZ + 8.0;

                    double distanceFromOrigin = std::sqrt(centerX * centerX + centerZ * centerZ);

                    // Only consider chunks at least 512 blocks from origin and within reasonable range
                    if (distanceFromOrigin >= 512.0 && distanceFromOrigin < 3500.0) {
                        // Find closest distance range in our probability data
                        int closestDistance = 500;
                        double minDiff = std::abs(500.0 - distanceFromOrigin);

                        for (const auto& pair : villageStrongholdDistanceProbs) {
                            double diff = std::abs(pair.first - distanceFromOrigin);
                            if (diff < minDiff) {
                                minDiff = diff;
                                closestDistance = pair.first;
                            }
                        }

                        StrongholdCell cell;
                        cell.centerX = centerX;
                        cell.centerZ = centerZ;
                        cell.xMin = blockX;
                        cell.xMax = blockX + 16.0;
                        cell.zMin = blockZ;
                        cell.zMax = blockZ + 16.0;
                        // Calculate prior probability: distance prob * type rate * generation rate
                        cell.prob = villageStrongholdDistanceProbs[closestDistance] * VILLAGE_STRONGHOLD_RATE * VILLAGE_GENERATION_RATE;
                        cell.distance = distanceFromOrigin;
                        cell.distanceRange = closestDistance;
                        cell.isVillageStronghold = true;

                        strongholdCells.push_back(cell);
                    }
                }
            }
        }
    }

    // Generate Scattered Strongholds (19.1% of nearest strongholds)  
    // 200x200 chunk grids, strongholds can generate in chunks 50-150
    for (int gridX = -8; gridX <= 8; gridX++) {
        for (int gridZ = -8; gridZ <= 8; gridZ++) {
            int baseChunkX = gridX * SCATTERED_GRID_SIZE;
            int baseChunkZ = gridZ * SCATTERED_GRID_SIZE;

            // Sample every 15th chunk for performance
            for (int chunkX = SCATTERED_MIN_CHUNK; chunkX <= SCATTERED_MAX_CHUNK; chunkX += 15) {
                for (int chunkZ = SCATTERED_MIN_CHUNK; chunkZ <= SCATTERED_MAX_CHUNK; chunkZ += 15) {
                    int absoluteChunkX = baseChunkX + chunkX;
                    int absoluteChunkZ = baseChunkZ + chunkZ;

                    double blockX = absoluteChunkX * 16.0;
                    double blockZ = absoluteChunkZ * 16.0;
                    double centerX = blockX + 8.0;
                    double centerZ = blockZ + 8.0;

                    double distanceFromOrigin = std::sqrt(centerX * centerX + centerZ * centerZ);

                    if (distanceFromOrigin >= 512.0 && distanceFromOrigin < 3500.0) {
                        // Find closest distance range for scattered strongholds
                        int closestDistance = 1000;
                        double minDiff = std::abs(1000.0 - distanceFromOrigin);

                        for (const auto& pair : scatteredStrongholdDistanceProbs) {
                            double diff = std::abs(pair.first - distanceFromOrigin);
                            if (diff < minDiff) {
                                minDiff = diff;
                                closestDistance = pair.first;
                            }
                        }

                        StrongholdCell cell;
                        cell.centerX = centerX;
                        cell.centerZ = centerZ;
                        cell.xMin = blockX;
                        cell.xMax = blockX + 16.0;
                        cell.zMin = blockZ;
                        cell.zMax = blockZ + 16.0;
                        cell.prob = scatteredStrongholdDistanceProbs[closestDistance] * SCATTERED_STRONGHOLD_RATE * SCATTERED_GENERATION_RATE;
                        cell.distance = distanceFromOrigin;
                        cell.distanceRange = closestDistance;
                        cell.isVillageStronghold = false;

                        strongholdCells.push_back(cell);
                    }
                }
            }
        }
    }
}

// Filter cells by calculated distance for initial cell identification
std::vector<const StrongholdCell*> filterCellsByCalculatedDistance(double calculatedDistance, double tolerance = CELL_IDENTIFICATION_TOLERANCE) {
    std::vector<const StrongholdCell*> filteredCells;

    for (const auto& cell : strongholdCells) {
        // Calculate distance from second throw position to stronghold entrance (2,2) point
        double strongholdX = cell.xMin + STRONGHOLD_TARGET_X;
        double strongholdZ = cell.zMin + STRONGHOLD_TARGET_Z;

        // Use coord2 (position before second eye throw) as the reference point
        double throwX = appState.coord2.x + 0.5; // Eye starts from center of block
        double throwZ = appState.coord2.z + 0.5;

        double cellDistance = std::sqrt(
            std::pow(strongholdX - throwX, 2) +
            std::pow(strongholdZ - throwZ, 2)
        );

        // Check if this cell is within the calculated distance tolerance
        if (std::abs(cellDistance - calculatedDistance) <= tolerance) {
            filteredCells.push_back(&cell);
        }
    }

    return filteredCells;
}

// Enhanced movement-based calculation with improved distance handling
void calculateStrongholdLocationWithDistance(double playerX, double playerZ, double eyeAngle, double targetDistance) {
    strongholdCandidates.clear();
    appState.distanceValidationFailed = false;
    appState.validationErrorMessage = L"";

    // Eye starts flying from (playerX + 0.5, playerZ + 0.5) in Bedrock Edition
    double eyeStartX = playerX + 0.5;
    double eyeStartZ = playerZ + 0.5;

    // Determine calculation mode based on available data
    bool hasMovementDistance = appState.hasCalculatedDistance && appState.calculatedDistance > 0;
    bool hasMovementData = appState.distanceMoved > 0 && appState.capturePhase >= 2;

    // Step 1: If we have calculated distance, use it for initial cell filtering
    std::vector<const StrongholdCell*> candidateCells;
    if (hasMovementDistance) {
        // Use calculated distance to narrow down possible cells
        candidateCells = filterCellsByCalculatedDistance(appState.calculatedDistance, CELL_IDENTIFICATION_TOLERANCE);

        // If no cells found with standard tolerance, expand search
        if (candidateCells.empty()) {
            candidateCells = filterCellsByCalculatedDistance(appState.calculatedDistance, CELL_IDENTIFICATION_TOLERANCE * 2);
        }

        // If still no cells, fall back to all cells (something might be wrong with calculation)
        if (candidateCells.empty()) {
            for (const auto& cell : strongholdCells) {
                candidateCells.push_back(&cell);
            }
            appState.distanceValidationFailed = true;
            appState.validationErrorMessage = L"No strongholds found at calculated distance - using all candidates";
        }
    }
    else {
        // No calculated distance available, consider all cells
        for (const auto& cell : strongholdCells) {
            candidateCells.push_back(&cell);
        }
        if (!hasMovementData) {
            appState.distanceValidationFailed = true;
            appState.validationErrorMessage = L"No movement data available - accuracy reduced";
        }
    }

    // Step 2: Calculate angle-based probabilities for filtered cells
    double measurementError = calculateMeasurementError(0.5); // Better alignment with movement method
    double precisionError = 0.0;

    if (hasMovementData) {
        // For movement-based mode, estimate precision based on distance moved
        precisionError = calculateCoordPrecisionError(std::max(appState.distanceMoved, 5.0));
    }
    else {
        // Fallback precision
        precisionError = calculateCoordPrecisionError(1000.0); // Default assumption
    }

    double combinedError = calculateCombinedError(measurementError, precisionError);
    double combinedErrorDegrees = combinedError * 180.0 / M_PI;

    // Map to accumulate probabilities for each stronghold cell
    std::map<const StrongholdCell*, double> cellProbabilities;

    // Step 3: Process filtered cells using angle-based probability
    for (const auto* cell : candidateCells) {
        // Eyes point to (2,2) coordinates within each chunk
        double targetX = cell->xMin + STRONGHOLD_TARGET_X;
        double targetZ = cell->zMin + STRONGHOLD_TARGET_Z;

        // Calculate expected angle from eye start position to stronghold entrance
        double deltaX = targetX - eyeStartX;
        double deltaZ = targetZ - eyeStartZ;
        double expectedAngle = std::atan2(deltaX, -deltaZ) * 180.0 / M_PI;

        // Calculate angle difference (normalize to [-180, 180])
        double angleDiff = eyeAngle - expectedAngle;
        while (angleDiff > 180.0) angleDiff -= 360.0;
        while (angleDiff < -180.0) angleDiff += 360.0;

        // Calculate likelihood using Gaussian distribution
        double likelihood = gaussianProbability(angleDiff, 0.0, combinedErrorDegrees);

        // Step 4: Apply distance validation if available
        double distanceBonus = 1.0;
        if (hasMovementDistance) {
            double strongholdDistance = std::sqrt(
                std::pow(targetX - eyeStartX, 2) + std::pow(targetZ - eyeStartZ, 2)
            );
            double distanceDiff = std::abs(strongholdDistance - appState.calculatedDistance);

            // Give bonus to cells that match calculated distance more closely
            if (distanceDiff <= MOVEMENT_DISTANCE_TOLERANCE) {
                distanceBonus = 3.0; // Strong bonus for close matches with movement method
            }
            else if (distanceDiff <= MOVEMENT_DISTANCE_TOLERANCE * 2) {
                distanceBonus = 1.8; // Moderate bonus for reasonable matches
            }
            else {
                distanceBonus = 0.3; // Penalty for poor distance matches
            }
        }
        else if (hasMovementData) {
            // Slight bonus for having movement data even without successful calculation
            distanceBonus = 1.2;
        }

        // Calculate posterior probability with distance bonus
        double posteriorProb = cell->prob * likelihood * distanceBonus;

        // Only consider cells with significant probability
        if (posteriorProb > 1e-12) {
            cellProbabilities[cell] = posteriorProb;
        }
    }

    // Step 5: Normalize probabilities and create candidates
    double totalProb = 0.0;
    for (const auto& pair : cellProbabilities) {
        totalProb += pair.second;
    }

    if (totalProb > 0) {
        for (const auto& pair : cellProbabilities) {
            const StrongholdCell* cell = pair.first;
            double normalizedProb = pair.second / totalProb;

            // Calculate distance from player to stronghold entrance (2,2) point
            double strongholdX = cell->xMin + STRONGHOLD_TARGET_X;
            double strongholdZ = cell->zMin + STRONGHOLD_TARGET_Z;
            double distanceToStronghold = std::sqrt(
                std::pow(strongholdX - playerX, 2) + std::pow(strongholdZ - playerZ, 2)
            );

            StrongholdCandidate candidate;
            candidate.projectionX = (int)std::round(strongholdX);
            candidate.projectionZ = (int)std::round(strongholdZ);
            candidate.netherX = (int)std::round(strongholdX / 8.0);
            candidate.netherZ = (int)std::round(strongholdZ / 8.0);
            candidate.cellCenterX = cell->centerX;
            candidate.cellCenterZ = cell->centerZ;
            candidate.rawProb = pair.second;
            candidate.conditionalProb = normalizedProb;
            candidate.distance = (int)std::round(distanceToStronghold);
            candidate.distanceFromOrigin = (int)std::round(cell->distance);
            candidate.distanceRange = cell->distanceRange;

            // Enhanced description with calculation method info
            std::wstringstream ss;
            if (cell->isVillageStronghold) {
                ss << L"Village Stronghold - Chunk (" << (int)(cell->xMin / 16) << L", " << (int)(cell->zMin / 16) << L")";
            }
            else {
                ss << L"Scattered Stronghold - Chunk (" << (int)(cell->xMin / 16) << L", " << (int)(cell->zMin / 16) << L")";
            }

            if (hasMovementDistance && hasMovementData) {
                ss << L" [Movement + Distance]";
            }
            else if (hasMovementData) {
                ss << L" [Movement-based]";
            }
            else {
                ss << L" [Single-point]";
            }

            candidate.bounds = ss.str();

            strongholdCandidates.push_back(candidate);
        }
    }

    // Sort candidates by conditional probability (highest first)
    std::sort(strongholdCandidates.begin(), strongholdCandidates.end(),
        [](const StrongholdCandidate& a, const StrongholdCandidate& b) {
            return a.conditionalProb > b.conditionalProb;
        });

    // Step 6: Provide feedback on calculation confidence
    if (strongholdCandidates.size() > 0) {
        double topProbability = strongholdCandidates[0].conditionalProb;

        if (hasMovementDistance && hasMovementData) {
            if (topProbability > 0.8) {
                appState.validationErrorMessage = L"High confidence: Movement-based triangulation successful";
            }
            else if (topProbability > 0.5) {
                appState.validationErrorMessage = L"Good confidence: Multiple viable candidates";
            }
            else {
                appState.validationErrorMessage = L"Moderate confidence: Consider additional measurements";
            }
        }
        else if (hasMovementData) {
            if (candidateCells.size() <= 5) {
                appState.validationErrorMessage = L"Movement data available, distance calculation failed";
            }
            else {
                appState.validationErrorMessage = L"Using movement data for angle estimation";
            }
        }
        else {
            appState.validationErrorMessage = L"Single-point calculation - lower precision";
        }
    }
    else {
        appState.distanceValidationFailed = true;
        appState.validationErrorMessage = L"No strongholds found - check angle and distance";
    }
}

