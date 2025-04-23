#pragma once
#include <string>
namespace TunableParams
{
    // Reverse Futility Pruning parameters
    extern int RFP_MARGIN;
    extern int RFP_DEPTH;
    extern int RFP_IMPROVING_BONUS;

    // Late Move Reduction parameters
    extern int LMR_BASE;
    extern int LMR_DIVISION;

    // Null Move Pruning parameters
    extern int NMP_BASE;
    extern int NMP_DIVISION;
    extern int NMP_MARGIN;

    // Late Move Pruning / Movecount based pruning
    extern int LMP_DEPTH_THRESHOLD;
    
    // Futility pruning
    extern int FUTILITY_MARGIN;
    extern int FUTILITY_DEPTH;
    extern int FUTILITY_IMPROVING;

    // Quiescence search
    extern int QS_FUTILITY_MARGIN;

    // SEE Pruning thresholds
    extern int SEE_QUIET_MARGIN_BASE;
    extern int SEE_NOISY_MARGIN_BASE;

    // Aspiration window
    extern int ASPIRATION_DELTA;

    // History pruning
    extern int HISTORY_PRUNING_THRESHOLD;

    // Initialize with default values
    void init_default_params();

    // Save current parameters to file
    bool save_params(const std::string& filename);
    
    // Load parameters from file
    bool load_params(const std::string& filename);
}