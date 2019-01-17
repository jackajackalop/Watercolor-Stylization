#pragma once

#include <string>

//data_path returns a path relative to the executable's location.
// use data_path to reference data files.
//   std::ifstream meshes_file(data_path("data/meshes.blob"), std::ios::binary);
//   load_png(data_path("data/texture.png"), ... );
std::string data_path(std::string const &suffix);

//user_path returns an OS-specific location for writing/reading user data.
// use data_path for save games and config files.
// std::ofstream config(user_path("game.save"));
std::string user_path(std::string const &suffix);
