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
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <system_error>
#include <random>
#include <chrono>

#if defined(_WIN32)
#include <io.h>
#define SYNC_FILE(f) _commit(_fileno(f))
#else
#include <unistd.h>
#define SYNC_FILE(f) fsync(fileno(f))
#endif /* _WIN32 */

namespace fs = std::filesystem;

static bool shredFile(const fs::path &path);
static std::string obfuscateFilename(void);
static std::mt19937_64 &rng(void);
static void usage(const char *prog);

int main(int argc, char *argv[]) {
  if (argc < 2) { usage(argv[0]); return EXIT_FAILURE; }

  char mode = 's';
  unsigned long rounds = 1UL;
  int first = 1;

  if (argv[1][0] == '-' && (argv[1][1] == 'i' || argv[1][1] == 'r') && argv[1][2] == '\0') {
    if (argc < 4) { usage(argv[0]); return EXIT_FAILURE; }
    char *endPtr = nullptr;
    errno = 0;
    rounds = std::strtoul(argv[2], &endPtr, 10);
    if (errno != 0 || endPtr == argv[2] || *endPtr != '\0' || rounds == 0UL) {
      std::cerr << "Error: '" << argv[2] << "' is not a valid positive number." << std::endl;
      return EXIT_FAILURE;
    }
    mode = argv[1][1];
    first = 3;
  }

  int status = EXIT_SUCCESS;
  auto start = std::chrono::steady_clock::now();

  for (int x = first; x < argc; x++) {
    const fs::path path(argv[x]);
    std::error_code ec;
    if (!fs::is_regular_file(path, ec)) {
      std::cerr << argv[x] << " doesn't exist or is not a regular file. Skipping." << std::endl;
      status = EXIT_FAILURE;
      continue;
    }

    if (mode == 'i') {
      for (unsigned long y = 0UL; y < rounds; y++) {
        if (!shredFile(path)) { status = EXIT_FAILURE; break; }
      }
    } else if (mode == 'r') {
      fs::path current = path;
      for (unsigned long q = 0UL; q < rounds; q++) {
        fs::path next = current.parent_path() / obfuscateFilename();
        fs::rename(current, next, ec);
        if (ec) { std::cerr << "Error: unable to rename " << current << ": " << ec.message() << std::endl; status = EXIT_FAILURE; break; }
        current = next;
      }
      if (shredFile(current)) {
        fs::remove(current, ec);
        if (ec) { std::cerr << "Error: unable to remove " << current << ": " << ec.message() << std::endl; status = EXIT_FAILURE; }
      } else { status = EXIT_FAILURE; }
    } else {
      if (!shredFile(path)) { status = EXIT_FAILURE; }
    }
  }

  std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
  std::cout << "Shredding took " << elapsed.count() << " seconds." << std::endl;
  return status;
}

static void usage(const char *prog) {
  std::cerr << "Usage: " << prog << " file...\n"
            << "       " << prog << " -i N file...   overwrite each file N times\n"
            << "       " << prog << " -r N file...   rename N times, overwrite once, then delete" << std::endl;
}

static std::mt19937_64 &rng(void) {
  static std::mt19937_64 gen(std::random_device{}());
  return gen;
}

static bool shredFile(const fs::path &path) {
  std::error_code ec;
  std::uintmax_t remaining = fs::file_size(path, ec);
  if (ec) { std::cerr << "Error: unable to get size of " << path << ": " << ec.message() << std::endl; return false; }

  /* Open in place, WITHOUT truncating, so the original data blocks get overwritten */
  FILE *file = std::fopen(path.string().c_str(), "r+b");
  if (file == nullptr) { std::cerr << "Error: Unable to open " << path << " for writing: " << std::strerror(errno) << std::endl; return false; }

  static const std::uintmax_t maxChunk = 8U * 1024U * 1024U;
  const std::size_t bufBytes = static_cast<std::size_t>(remaining < maxChunk ? remaining : maxChunk);
  std::vector<std::uint64_t> words((bufBytes + sizeof(std::uint64_t) - 1U) / sizeof(std::uint64_t));
  bool ok = true;

  while (remaining > 0U) {
    const std::size_t chunk = static_cast<std::size_t>(remaining < bufBytes ? remaining : bufBytes);
    const std::size_t nWords = (chunk + sizeof(std::uint64_t) - 1U) / sizeof(std::uint64_t);
    for (std::size_t i = 0U; i < nWords; i++) { words[i] = rng()(); }  /* fresh random data for every chunk */
    if (std::fwrite(words.data(), 1U, chunk, file) != chunk) {
      std::cerr << "Error: write failed on " << path << ": " << std::strerror(errno) << std::endl;
      ok = false;
      break;
    }
    remaining -= chunk;
  }

  if (ok && (std::fflush(file) != 0 || SYNC_FILE(file) != 0)) {
    std::cerr << "Error: unable to flush " << path << " to disk: " << std::strerror(errno) << std::endl;
    ok = false;
  }
  if (std::fclose(file) != 0) { ok = false; }
  return ok;
}

static std::string obfuscateFilename(void) {
  static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  std::uniform_int_distribution<std::size_t> distribution(0U, chars.size() - 1U);
  std::string randomString;
  randomString.reserve(50U);
  for (unsigned int x = 0U; x < 50U; x++) { randomString += chars[distribution(rng())]; }
  return randomString;
}