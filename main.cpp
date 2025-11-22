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
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <mutex>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/statvfs.h>
#include <errno.h>
#include <cstring>
#endif /* _WIN23 */

static void shredFile(const std::string &str, std::size_t size);
static inline unsigned long int blockSize(void);

namespace fs = std::filesystem;
std::mutex mtx;

int main(int argc, char *argv[]) {
  unsigned int x = (argc == 2) ? 1U : 3U;
  unsigned int z = static_cast<unsigned int>(argc) - 1;
  unsigned int y = 0U;
  unsigned int w = (argc == 2) ? 0U : std::strtoul(argv[2], static_cast<char **>(nullptr), 10);
  std::vector<std::thread> threads;
  for (; x <= z; x++) { if (!fs::exists(argv[x])) { std::cerr << argv[x] << " doesn't exists. Nothing to be done." << std::endl; return EXIT_FAILURE; }
    if (argc > 2 && argv[1][0] == '-' && argv[1][1] == 'i') { for (y = 0U; y < w; y++) { threads.emplace_back(shredFile, argv[x], fs::file_size(argv[x])); } for (auto &th : threads) { if (th.joinable()) { th.join(); } } }
    else { shredFile(argv[x], fs::file_size(argv[x])); } }
  return EXIT_SUCCESS;
}

static void shredFile(const std::string &str, std::size_t size) {
  try {
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream file(str, std::ios::binary | std::ios::trunc);
    if (!file) { std::cerr << "Error: Unable to open " << str << " file for writing. Exiting." << std::endl; return; }
    static const std::size_t bufferSize = blockSize();
    static const std::vector<char> buffer(bufferSize, 0);
    while (size > 0) {
      std::size_t chunkSize = (size < bufferSize) ? size : bufferSize;
      file.write(buffer.data(), chunkSize);
      size -= chunkSize;
    }
    file.flush();
    file.close();
  } catch (const std::exception &e) { std::lock_guard<std::mutex> lock(mtx); std::cerr << "Error: " << e.what() << std::endl; return; }
}

static inline unsigned long int blockSize(void) {
#if defined(_WIN32)
  DWORD sectorsPerCluster;
  DWORD bytesPerSector;
  DWORD numberOfFreeClusters;
  DWORD totalNumberOfClusters;
  std::wstring wpath = L"C:\\";
  if (!GetDiskFreeSpaceW(wpath.c_str(), &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters)) { throw std::runtime_error("GetDiskFreeSpaceW failed with error code: " + std::to_string(GetLastError())); }
  return static_cast<unsigned long int>(sectorsPerCluster * bytesPerSector);
#else
  struct statvfs fsinfo;
  if (statvfs("/", &fsinfo) != 0) { throw std::runtime_error("statvfs failed: " + std::string(std::strerror(errno))); }
  return static_cast<unsigned long int>(fsinfo.f_bsize);
#endif /* _WIN32 */
}