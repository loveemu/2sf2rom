/// @file
/// 2SF2ROM: 2SF to NDS ROM Converter.

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#define PATH_MAX MAX_PATH
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include <zlib.h>

#include "byteio.hpp"
#include "psf_file.hpp"
#include "ZlibReader.h"
#include "cpath.h"

namespace {

/// The name of application.
constexpr auto kApplicationName = "2SF2ROM";

/// The version of application.
constexpr auto kApplicationVersion = "1.0";

/// The short description of application.
constexpr auto kApplicationDescription = "Program to turn a 2sf into a nds rom file.";

/// The website of application.
constexpr auto kApplicationWebsite = "https://github.com/loveemu/2sf2rom";

/// The version byte of 2SF file.
constexpr uint8_t k2SFVersionByte = 0x24;

/// The maximum ROM size of NDS.
constexpr size_t kNDSRomMaxSize = 128 * 1024 * 1024;

/// The maximum nest level of psflib.
constexpr int kPSFLibMaxNestLevel = 10;

} // namespace

/// Load ROM image from 2SF file.
/// @param filename the path to 2sf file.
/// @param rom the rom image to be loaded.
/// @param lib_nest_level the nest level of psflib.
/// @param first_load true for the first file.
void load_2sf(const std::string & filename, std::vector<char> & rom, int lib_nest_level = 0, bool first_load = true) {
  // check the psflib nest level
  if (lib_nest_level >= kPSFLibMaxNestLevel) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Nest level error on psflib loading.";
    throw std::out_of_range(message_buffer.str());
  }

  // save the current directory
  char pwd[PATH_MAX];
  getcwd(pwd, PATH_MAX);

  // get the absolute path
  char absolute_path[PATH_MAX];
  if (path_getabspath(filename.c_str(), absolute_path) == NULL) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to determine absolute path.";
    throw std::out_of_range(message_buffer.str());
  }

  // get the directory path
  char basedir[PATH_MAX];
  strcpy(basedir, absolute_path);
  path_dirname(basedir);

  // load the psf file
  PSFFile psf(filename);

  // check CRC32 of the compressed program
  uint32_t actual_crc32 = ::crc32(0L, reinterpret_cast<const Bytef *>(
    psf.compressed_exe().data()), static_cast<uInt>(psf.compressed_exe().size()));
  if (psf.compressed_exe_crc32() != actual_crc32) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "CRC32 error at the compressed program.";
    throw std::runtime_error(message_buffer.str());
  }

  // load psflibs
  int lib_index = 1;
  while (true) {
    // search for _libN tag
    std::ostringstream lib_tag_name_buffer;
    lib_tag_name_buffer << "_lib";
    if (lib_index > 1) {
      lib_tag_name_buffer << lib_index;
    }
    std::string lib_tag_name = lib_tag_name_buffer.str();

    // if no tag is present, end the lib loading
    if (psf.tags().count(lib_tag_name) == 0) {
      break;
    }

    // set the current directory to the parent psf directory
    chdir(basedir);

    // load the lib
    try {
      load_2sf(psf.tags()[lib_tag_name], rom, lib_nest_level + 1, first_load);
    }
    catch (std::exception) {
      chdir(pwd);
      throw;
    }
    chdir(pwd);
    first_load = false;

    // check the next lib
    lib_index++;
  }

  // read the exe header
  // - 4 bytes offset
  // - 4 bytes size
  ZlibReader compressed_exe(psf.compressed_exe().c_str(), psf.compressed_exe().size());
  uint32_t load_offset;
  uint32_t load_size;
  bool read_success = true;
  read_success &= compressed_exe.readInt(load_offset);
  read_success &= compressed_exe.readInt(load_size);
  if (!read_success) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to read the program header.";
    throw std::runtime_error(message_buffer.str());
  }

  // ensure the rom buffer size
  if (load_offset + load_size > kNDSRomMaxSize) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Load offset/size of 2SF is too large. ";
    throw std::out_of_range(message_buffer.str());
  }
  if (first_load) {
    rom.resize(load_offset + load_size, 0);
  }
  else {
    if (load_offset + load_size > rom.size()) {
      std::ostringstream message_buffer;
      message_buffer << filename << ": " << "Load offset/size of 2SF is out of bound.";
      throw std::out_of_range(message_buffer.str());
    }
  }

  // decompress the program area
  if (compressed_exe.read(&rom[load_offset], load_size) != load_size) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Failed to deflate data. Program data is corrupted.";
    throw std::out_of_range(message_buffer.str());
  }
}

/// Show usage of 2SF2ROM.
/// @param cmd the name of commmand.
void show_usage(std::string cmd) {
  std::cout << kApplicationName << " " << kApplicationVersion << std::endl;
  std::cout << "============================" << std::endl;
  std::cout << std::endl;

  std::cout << kApplicationDescription << std::endl;
  std::cout << "<" << kApplicationWebsite << ">" << std::endl;
  std::cout << std::endl;

  std::cout << "Usage" << std::endl;
  std::cout << "-----" << std::endl;
  std::cout << std::endl;

  std::cout << "`" << cmd << " [options] 2sf-file`" << std::endl;
  std::cout << std::endl;

  std::cout << "### Options" << std::endl;
  std::cout << std::endl;

  std::cout << "`--help`" << std::endl;
  std::cout << "  : Show this help." << std::endl;
  std::cout << std::endl;
  std::cout << "`-o filename`" << std::endl;
  std::cout << "  : Set the output filename." << std::endl;
  std::cout << std::endl;
}

/// Main of 2SF2ROM.
/// @param argc the number of command-line arguments.
/// @param argv the command-line arguments.
/// @return the exit status.
int main(int argc, char * argv[]) {
  try {
    std::string output_filename;

    // show usage if arg is empty
    if (argc <= 1) {
      show_usage(argv[0]);
      return 1;
    }

    // parse options
    int argi = 1;
    while (argi < argc && argv[argi][0] == '-') {
      std::string arg(argv[argi]);
      if (arg == "--help") {
        show_usage(argv[0]);
        return 1;
      }
      else if (arg == "-o") {
        if (argi + 1 >= argc) {
          std::ostringstream message_buffer;
          message_buffer << "Too few arguments for \"" << arg << "\"";
          throw std::invalid_argument(message_buffer.str());
        }

        output_filename = argv[argi + 1];
        argi++;
      }
      else {
        std::ostringstream message_buffer;
        message_buffer << "Unknown option \"" << arg << "\"";
        throw std::invalid_argument(message_buffer.str());
      }

      argi++;
    }

    if (argi == argc) {
      throw std::invalid_argument("No input files.");
    }

    if (argi + 1 > argc) {
      throw std::invalid_argument("Too many arguments.");
    }

    // determine filenames
    std::string filename(argv[argi]);
    if (output_filename.empty()) {
      const char * filename_c = filename.c_str();
      off_t ext = path_findext(filename_c) - filename_c;
      output_filename = filename.substr(0, ext) + ".data.bin";
    }

    // load rom image
    std::vector<char> rom;
    load_2sf(filename, rom);

    // write decompressed rom to file
    std::ofstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out.open(output_filename, std::ios::binary);
    out.write(rom.data(), rom.size());
  }
  catch (std::exception ex) {
    std::cout << "Error: " << ex.what() << std::endl;
    return 1;
  }
}
