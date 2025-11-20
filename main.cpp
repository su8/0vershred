/*
Copyright 11/20/2025 https://github.com/su8/0vershred
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <fstream>
#include <stdexcept>

void fillFile(const std::string &str, std::size_t size);

int main(int argc, char *argv[]) {
  for (unsigned int x = 1; x <= static_cast<unsigned int>(argc) - 1; x++) { fillFile(argv[x], std::filesystem::file_size(argv[x])); }
  return EXIT_SUCCESS;
}

void fillFile(const std::string &str, std::size_t size) {
  try {
    std::ofstream file(str, std::ios::binary | std::ios::trunc);
    if (!file) { std::cerr << "Error: Unable to open file." << std::endl; return; }
    static const std::size_t bufferSize = 4096; // 4 KB buffer
    std::vector<char> buffer(bufferSize, 0);
    while (size > 0) {
      std::size_t chunkSize = (size < bufferSize) ? size : bufferSize;
      file.write(buffer.data(), chunkSize);
      size -= chunkSize;
    }
    file.close();
  } catch (const std::exception &e) { std::cerr << "Error: " << e.what() << std::endl; return; }
}