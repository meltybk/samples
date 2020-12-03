// Author: melty <11942219+meltybk@users.noreply.github.com>

#include "analyzer.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {

  std::string compile_file;
  if (argc > 1) {
    compile_file = argv[1];
  }

  if (compile_file.empty()) {
    std::cout << "Should to specify compile file.";
    return -1;
  }

  const char *base_class = argv[2];

  std::vector<const char *> include_paths;
  const char *output_path = nullptr;

  for (int i = 0; i < argc; ++i) {
    if (strncmp(argv[i], "-I", 2) == 0) {
      include_paths.emplace_back(argv[i]);
    }

    if (strncmp(argv[i], "-o", 2) == 0) {
      output_path = argv[i + 1];
    }
  }
  if (!output_path) {
    std::cout << "Should to specify output path by -o option.";
    return -1;
  }

  Analyzer analyzer;
  analyzer.Analyze(compile_file.c_str(), include_paths, base_class);
  analyzer.GenerateJSON(output_path);

  return 0;
}
