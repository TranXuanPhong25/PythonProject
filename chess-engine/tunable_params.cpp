#include "tunable_params.hpp"
#include <fstream>
#include <iostream>
#include <string>

namespace TunableParams
{
    // Reverse Futility Pruning parameters
    int RFP_MARGIN = 75;
    int RFP_DEPTH = 5;
    int RFP_IMPROVING_BONUS = 62;

    // Late Move Reduction parameters
    int LMR_BASE = 75;
    int LMR_DIVISION = 225;

    // Null Move Pruning parameters
    int NMP_BASE = 3;
    int NMP_DIVISION = 3;
    int NMP_MARGIN = 180;

    // Late Move Pruning / Movecount based pruning
    int LMP_DEPTH_THRESHOLD = 7;
    
    // Futility pruning
    int FUTILITY_MARGIN = 150;
    int FUTILITY_DEPTH = 6;
    int FUTILITY_IMPROVING = 24;

    // Quiescence search
    int QS_FUTILITY_MARGIN = 177;

    // SEE Pruning thresholds
    int SEE_QUIET_MARGIN_BASE = -70;
    int SEE_NOISY_MARGIN_BASE = -15;

    // Aspiration window
    int ASPIRATION_DELTA = 12;

    // History pruning
    int HISTORY_PRUNING_THRESHOLD = 4000;

    void init_default_params()
    {
        // Reset all parameters to their default values
        RFP_MARGIN = 75;
        RFP_DEPTH = 5;
        RFP_IMPROVING_BONUS = 62;
        LMR_BASE = 75;
        LMR_DIVISION = 225;
        NMP_BASE = 3;
        NMP_DIVISION = 3;
        NMP_MARGIN = 180;
        LMP_DEPTH_THRESHOLD = 7;
        FUTILITY_MARGIN = 150;
        FUTILITY_DEPTH = 6;
        FUTILITY_IMPROVING = 24;
        QS_FUTILITY_MARGIN = 177;
        SEE_QUIET_MARGIN_BASE = -70;
        SEE_NOISY_MARGIN_BASE = -15;
        ASPIRATION_DELTA = 12;
        HISTORY_PRUNING_THRESHOLD = 4000;
    }

    bool save_params(const std::string& filename)
    {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        // Make sure to include ALL parameters, starting with RFP parameters
        file << "RFP_MARGIN " << RFP_MARGIN << std::endl;
        file << "RFP_DEPTH " << RFP_DEPTH << std::endl;
        file << "RFP_IMPROVING_BONUS " << RFP_IMPROVING_BONUS << std::endl;
        
        // Then include the remaining parameters
        file << "LMR_BASE " << LMR_BASE << std::endl;
        file << "LMR_DIVISION " << LMR_DIVISION << std::endl;
        file << "NMP_BASE " << NMP_BASE << std::endl;
        file << "NMP_DIVISION " << NMP_DIVISION << std::endl;
        file << "NMP_MARGIN " << NMP_MARGIN << std::endl;
        file << "LMP_DEPTH_THRESHOLD " << LMP_DEPTH_THRESHOLD << std::endl;
        file << "FUTILITY_MARGIN " << FUTILITY_MARGIN << std::endl;
        file << "FUTILITY_DEPTH " << FUTILITY_DEPTH << std::endl;
        file << "FUTILITY_IMPROVING " << FUTILITY_IMPROVING << std::endl;
        file << "QS_FUTILITY_MARGIN " << QS_FUTILITY_MARGIN << std::endl;
        file << "SEE_QUIET_MARGIN_BASE " << SEE_QUIET_MARGIN_BASE << std::endl;
        file << "SEE_NOISY_MARGIN_BASE " << SEE_NOISY_MARGIN_BASE << std::endl;
        file << "ASPIRATION_DELTA " << ASPIRATION_DELTA << std::endl;
        file << "HISTORY_PRUNING_THRESHOLD " << HISTORY_PRUNING_THRESHOLD << std::endl;
        
        file.close();
        return true;
    }

    bool load_params(const std::string& filename)
    {
        std::ifstream in(filename);
        if (!in.is_open()) {
            std::cerr << "Error: Could not open parameter file " << filename << std::endl;
            return false;
        }

        std::string param;
        int value;

        while (in >> param >> value) {
            if (param == "RFP_MARGIN") RFP_MARGIN = value;
            else if (param == "RFP_DEPTH") RFP_DEPTH = value;
            else if (param == "RFP_IMPROVING_BONUS") RFP_IMPROVING_BONUS = value;
            else if (param == "LMR_BASE") LMR_BASE = value;
            else if (param == "LMR_DIVISION") LMR_DIVISION = value;
            else if (param == "NMP_BASE") NMP_BASE = value;
            else if (param == "NMP_DIVISION") NMP_DIVISION = value;
            else if (param == "NMP_MARGIN") NMP_MARGIN = value;
            else if (param == "LMP_DEPTH_THRESHOLD") LMP_DEPTH_THRESHOLD = value;
            else if (param == "FUTILITY_MARGIN") FUTILITY_MARGIN = value;
            else if (param == "FUTILITY_DEPTH") FUTILITY_DEPTH = value;
            else if (param == "FUTILITY_IMPROVING") FUTILITY_IMPROVING = value;
            else if (param == "QS_FUTILITY_MARGIN") QS_FUTILITY_MARGIN = value;
            else if (param == "SEE_QUIET_MARGIN_BASE") SEE_QUIET_MARGIN_BASE = value;
            else if (param == "SEE_NOISY_MARGIN_BASE") SEE_NOISY_MARGIN_BASE = value;
            else if (param == "ASPIRATION_DELTA") ASPIRATION_DELTA = value;
            else if (param == "HISTORY_PRUNING_THRESHOLD") HISTORY_PRUNING_THRESHOLD = value;
            else {
                std::cerr << "Unknown parameter: " << param << std::endl;
            }
        }

        in.close();
        return true;
    }
}