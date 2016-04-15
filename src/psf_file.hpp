/// @file
/// PSFFile class header.

#ifndef PSF_FILE_HPP_
#define PSF_FILE_HPP_

#include <stdint.h>

#include <string>
#include <memory>
#include <unordered_map>

/// The PSFFile class represents a Portable Sound Format file.
///
/// @remarks This class does not provide any zlib-related functions.
class PSFFile {
public:
  /// Constructs a new PSFFile.
  PSFFile();

  /// Open Portable Sound Format from a file.
  /// @param filename path of the file.
  /// @return the new PSFFile object.
  ///
  /// @remarks This function does not check the validity of CRC32 fields.
  /// @remarks This function does not load any associated psflibs.
  explicit PSFFile(const std::string & filename);

  /// Constructs a new copy of specified PSFFile.
  /// @param origin a PSFFile object.
  PSFFile(const PSFFile & origin) = default;

  /// Copy-assigns a new value to the PSFFile, replacing its current contents.
  /// @param origin a PSFFile object.
  PSFFile & operator=(const PSFFile & origin) = default;

  /// Acquires the contents of specified PSFFile.
  /// @param origin a PSFFile object.
  PSFFile(PSFFile && origin) = default;

  /// Move-assigns a new value to the PSFFile, replacing its current contents.
  /// @param origin a PSFFile object.
  PSFFile & operator=(PSFFile && origin) = default;

  /// Destructs the PSFFile.
  virtual ~PSFFile() = default;

  /// Returns the version byte.
  /// @return the version byte.
  uint8_t version() const;

  /// Sets the version byte.
  /// @param version the version byte.
  void set_version(uint8_t version);

  /// Returns the reserved area.
  /// @return the reserved area.
  std::string & reserved();

  /// Returns the reserved area.
  /// @return the reserved area.
  const std::string & reserved() const;

  /// Sets the reserved area.
  /// @param reserved the reserved area.
  void set_reserved(std::string reserved);

  /// Returns the compressed program.
  /// @return the compressed program.
  std::string & compressed_exe();

  /// Returns the compressed program.
  /// @return the compressed program.
  const std::string & compressed_exe() const;

  /// Sets the compressed program.
  /// @param compressed_exe the compressed program.
  void set_compressed_exe(std::string compressed_exe);

  /// Returns the CRC32 of the compressed program.
  /// @return the CRC32 of the compressed program.
  uint32_t compressed_exe_crc32() const;

  /// Sets the CRC32 of the compressed program.
  /// @param compressed_exe_crc32 the CRC32 of the compressed program.
  void set_compressed_exe_crc32(uint32_t compressed_exe_crc32);

  /// Returns the key-value list of tags.
  /// @return the key-value list of tags.
  std::unordered_map<std::string, std::string> & tags();

  /// Returns the key-value list of tags.
  /// @return the key-value list of tags.
  const std::unordered_map<std::string, std::string> & tags() const;

  /// Sets the key-value list of tags.
  /// @param tags the key-value list of tags.
  void set_tags(std::unordered_map<std::string, std::string> tags);

  /// Write to PSF file.
  /// @param filename path of the file.
  ///
  /// @remarks This function does not check the validity of CRC32 fields.
  void write(const std::string & filename) const;

private:
  /// Version byte.
  ///
  /// The version byte is used to determine the type of PSF file.
  uint8_t version_;

  /// Reserved area.
  std::string reserved_;

  /// Compressed program.
  std::string compressed_exe_;

  /// CRC32 of the compressed program.
  uint32_t compressed_exe_crc32_;

  /// Key-Value list of tags.
  std::unordered_map<std::string, std::string> tags_;
};

#endif // !PSF_FILE_HPP_
