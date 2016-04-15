/// @file
/// PSFFile class implementation.

#include <stdint.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

#include "byteio.hpp"
#include "psf_file.hpp"

namespace fs = std::experimental::filesystem;

namespace {

/// The data of file signature.
constexpr auto kPSFSignature = "PSF";

/// The length of file signature.
constexpr auto kPSFSignatureSize = 3;

/// The data of the PSF tag marker.
constexpr auto kPSFTagMarker = "[TAG]";

/// The length of the PSF tag marker
constexpr auto kPSFTagMarkerSize = 5;

} // namespace

/// Constructs a new PSFFile.
PSFFile::PSFFile() :
    version_(0) {
}

/// Open Portable Sound Format from a file.
PSFFile::PSFFile(const std::string & filename) {
  // get input file size
  std::uintmax_t psf_size = fs::file_size(filename);

  // open input file
  std::ifstream in;
  in.exceptions(std::ios::badbit);
  in.open(filename, std::ios::binary);

  // check signature
  std::string signature(kPSFSignatureSize, 0);
  in.read(&signature[0], kPSFSignatureSize);
  if (in.gcount() != kPSFSignatureSize) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to read the PSF signature.";
    throw std::runtime_error(message_buffer.str());
  }
  if (signature != kPSFSignature) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Invalid PSF signature. ";
    throw std::runtime_error(message_buffer.str());
  }

  // read the version byte
  uint8_t version;
  if (!ReadStreamAsInt8(in, version)) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to read the version byte.";
    throw std::runtime_error(message_buffer.str());
  }

  // read the size of reserved area
  uint32_t reserved_size;
  if (!ReadStreamAsInt32L(in, reserved_size)) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to read the size of reserved area.";
    throw std::runtime_error(message_buffer.str());
  }

  // read the size of compressed program
  uint32_t compressed_exe_size;
  if (!ReadStreamAsInt32L(in, compressed_exe_size)) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to read the size of compressed program.";
    throw std::runtime_error(message_buffer.str());
  }

  // crc32 of compressed program
  uint32_t compressed_exe_crc32;
  if (!ReadStreamAsInt32L(in, compressed_exe_crc32)) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to read the CRC32 of compressed program.";
    throw std::runtime_error(message_buffer.str());
  }

  // check the size consistency
  size_t psf_mandatory_size = 0x10 + reserved_size + compressed_exe_size;
  if (psf_mandatory_size > psf_size) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "File is too short than expected.";
    throw std::runtime_error(message_buffer.str());
  }

  // set the version byte
  set_version(version);

  // read the reserved area
  reserved().resize(reserved_size);
  in.read(&reserved()[0], reserved_size);
  if (in.gcount() != reserved_size) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to read the reserved area.";
    throw std::runtime_error(message_buffer.str());
  }

  // read the compressed program
  compressed_exe().resize(compressed_exe_size);
  in.read(&compressed_exe()[0], compressed_exe_size);
  if (in.gcount() != compressed_exe_size) {
    std::ostringstream message_buffer;
    message_buffer << filename << ": " << "Unable to read the compressed program.";
    throw std::runtime_error(message_buffer.str());
  }

  // set the CRC32 of the compressed program
  set_compressed_exe_crc32(compressed_exe_crc32);

  // check the tag marker (optional area)
  if (psf_mandatory_size + kPSFTagMarkerSize <= psf_size) {
    std::string tag_marker(kPSFTagMarkerSize, 0);
    in.read(&tag_marker[0], kPSFTagMarkerSize);
    if (in.gcount() == kPSFTagMarkerSize && tag_marker == kPSFTagMarker) {
      // read entire of the tag area
      size_t tag_size = (size_t)psf_size - (0x10 + reserved_size + compressed_exe_size + kPSFTagMarkerSize);
      std::string tag_string(tag_size, 0);
      in.read(&tag_string[0], tag_size);
      if (in.gcount() != tag_size) {
        std::ostringstream message_buffer;
        message_buffer << filename << ": " << "Unable to read the tag area.";
        throw std::runtime_error(message_buffer.str());
      }

      // Parse tag section. Details are available here:
      // http://wiki.neillcorlett.com/PSFTagFormat
      std::unordered_map<std::string, std::string> tags;
      size_t tag_start_offset = 0;
      while (tag_start_offset < tag_string.size()) {
        // Search the end position of the current line.
        size_t tag_end_offset = tag_string.find("\n", tag_start_offset);
        if (tag_end_offset == std::string::npos) {
          // Tag section must end with a newline.
          // Read the all remaining bytes if a newline lacks though.
          tag_end_offset = tag_string.size();
        }

        // Search the variable=value separator.
        std::string tag_line = tag_string.substr(tag_start_offset, tag_end_offset - tag_start_offset);
        size_t tag_separator_offset = tag_line.find("=", 0);
        if (tag_separator_offset == std::string::npos) {
          // Blank lines, or lines not of the form "variable=value", are ignored.
          tag_start_offset = tag_end_offset + 1;
          continue;
        }
        tag_separator_offset += tag_start_offset;

        // Determine the start/end position of variable.
        size_t name_start_offset = tag_start_offset;
        size_t name_end_offset = tag_separator_offset;
        size_t value_start_offset = tag_separator_offset + 1;
        size_t value_end_offset = tag_end_offset;

        // Whitespace at the beginning/end of the line and before/after the = are ignored.
        // All characters 0x01-0x20 are considered whitespace.
        // (There must be no null (0x00) characters.)
        // Trim them.
        while (name_end_offset > name_start_offset && static_cast<unsigned char>(tag_string[name_end_offset - 1]) <= 0x20) {
          name_end_offset--;
        }
        while (value_end_offset > value_start_offset && static_cast<unsigned char>(tag_string[value_end_offset - 1]) <= 0x20) {
          value_end_offset--;
        }
        while (name_start_offset < name_end_offset && static_cast<unsigned char>(tag_string[name_start_offset]) <= 0x20) {
          name_start_offset++;
        }
        while (value_start_offset < value_end_offset && static_cast<unsigned char>(tag_string[value_start_offset]) <= 0x20) {
          value_start_offset++;
        }

        // Read variable=value as string.
        std::string key = tag_string.substr(name_start_offset, name_end_offset - name_start_offset);
        std::string value = tag_string.substr(value_start_offset, value_end_offset - value_start_offset);

        // Multiple-line variables must appear as consecutive lines using the same variable name.
        // For instance:
        //   comment=This is a
        //   comment=multiple-line
        //   comment=comment.
        // Therefore, check if the variable had already appeared.
        std::unordered_map<std::string, std::string>::iterator it = tags.lower_bound(key);
        if (it != tags.end() && it->first == key) {
          it->second += "\n";
          it->second += value;
        }
        else {
          tags.insert(it, std::make_pair(key, value));
        }

        // parse next line
        tag_start_offset = tag_end_offset + 1;
      }

      set_tags(std::move(tags));
    }
  }
}

/// Returns the version byte.
uint8_t PSFFile::version() const {
  return version_;
}

/// Sets the version byte.
void PSFFile::set_version(uint8_t version) {
  version_ = version;
}

/// Returns the reserved area.
std::string & PSFFile::reserved() {
  return reserved_;
}

/// Returns the reserved area.
const std::string & PSFFile::reserved() const {
  return reserved_;
}

/// Sets the reserved area.
void PSFFile::set_reserved(std::string reserved) {
  reserved_ = std::move(reserved);
}

/// Returns the compressed program.
std::string & PSFFile::compressed_exe() {
  return compressed_exe_;
}

/// Returns the compressed program.
const std::string & PSFFile::compressed_exe() const {
  return compressed_exe_;
}

/// Sets the compressed program.
void PSFFile::set_compressed_exe(std::string compressed_exe) {
  compressed_exe_ = std::move(compressed_exe);
}

/// Returns the CRC32 of the compressed program.
uint32_t PSFFile::compressed_exe_crc32() const {
  return compressed_exe_crc32_;
}

/// Sets the CRC32 of the compressed program.
void PSFFile::set_compressed_exe_crc32(uint32_t compressed_exe_crc32) {
  compressed_exe_crc32_ = compressed_exe_crc32;
}

/// Returns the key-value list of tags.
std::unordered_map<std::string, std::string> & PSFFile::tags() {
  return tags_;
}

/// Returns the key-value list of tags.
const std::unordered_map<std::string, std::string> & PSFFile::tags() const {
  return tags_;
}

/// Sets the key-value list of tags.
void PSFFile::set_tags(std::unordered_map<std::string, std::string> tags) {
  tags_ = std::move(tags);
}

void PSFFile::write(const std::string & filename) const {
  // open output file
  std::ofstream out;
  out.exceptions(std::ios::badbit | std::ios::failbit);
  out.open(filename, std::ios::binary);

  // write the signature
  out << kPSFSignature;

  // write the version byte
  InsertInt8(out, version());

  // write the size of reserved area
  InsertInt32L(out, static_cast<uint32_t>(reserved().size()));

  // write the size of the compressed program
  InsertInt32L(out, static_cast<uint32_t>(compressed_exe().size()));

  // write the CRC32 of the compressed program
  InsertInt32L(out, compressed_exe_crc32());

  // write the reserved area
  out.write(reserved().data(), reserved().size());

  // write the compressed program
  out.write(compressed_exe().data(), compressed_exe().size());

  // write tags if available
  if (!tags().empty())
  {
    // write the tag marker
    out << kPSFTagMarker;

    // write each tags
    for (const auto & pair : tags()) {
      const std::string & key = pair.first;
      const std::string & value = pair.second;

      // write key-value pair for each lines
      std::istringstream value_reader(value);
      std::string line;
      while (std::getline(value_reader, line)) {
        out << key << "=" << value << "\n";
      }
    }
  }
}
