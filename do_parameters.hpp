//needs a type, name, default value, and a hint
DO_PARAMETER (float, elapsed_time, 0.0f, "");
DO_PARAMETER (float, speed, 5.0f, "");
DO_PARAMETER (float, frequency, 0.4f, "");
DO_PARAMETER (float, tremor_amount, 0.4f, "");
DO_PARAMETER (float, dA, 0.12f, "float 0.0001 1.0");
DO_PARAMETER (float, cangiante_variable, 0.05f, "float 0.0 1.0");
DO_PARAMETER (float, dilution_variable, 0.95f, "float 0.0 1.0");
DO_PARAMETER (float, density_amount, 1.0f, "float 0.0 5.0");
DO_PARAMETER (float, depth_threshold, 0.0f, "float 0.0 0.001");
DO_PARAMETER (int, blur_amount, 3, "int 0 10");
DO_PARAMETER (bool, bleed, true, "");
DO_PARAMETER (bool, distortion, true, "");
DO_PARAMETER (int, show, 7, "int 0 7");
DO_PARAMETER (std::string, filename, "test0", "");
