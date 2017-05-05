// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_VFS_H_
#define BERRYDB_INCLUDE_VFS_H_

#include <string>

#include "berrydb/platform.h"

namespace berrydb {

class BlockAccessFile;
enum class Status : int;

/** Pure virtual interface for platform services.
 *
 * The name "Vfs" was chosen because most of the services revolve around file
 * access,
 */
class Vfs {
 public:
  /** Opens a file designed for reads/writes at (large) block granularities.
   *
   * This method is used for the store data files.
   *
   * @param file_path   the file to be opened or created
   * @param block_shift log2(block size); the block size can be computed as
   *                    1 << block_shift
   * @param file        if the call succeeds, populated with a BlockAccessFile*
   *                    that can be used to access the file
   * @return            attempting to open a non-existing file may result in
   *                    kIoError or kNotFound; all other errors will result in
   *                    kIoError
   */
  virtual Status OpenForBlockAccess(
      const std::string& file_path, size_t block_shift,
      bool create_if_missing, bool error_if_exists,
      BlockAccessFile** result) = 0;


  /** Deletes a file from the filesystem.
   *
   * This method is used to remove log files.
   *
   * @param  file_path the file to be deleted
   * @return           most likely kSuccess or kIoError; attempting to delete a
   *                   non-existing file may result in kNotFound, but that is
   *                   not a requirement
   */
  virtual Status DeleteFile(const std::string& file_path) = 0;
};

/** Interface for accessing files via block-based I/O.
 *
 * This interface is used for accessing store files. The block size is the store
 * page size. */
class BlockAccessFile {
 public:
  /** Reads blocks from the file.
   *
   * @param  offset must be a multiple of the block size used to open the file
   * @param  bytes  must be a multiple of the block size used to open the file
   * @param  buffer receives the bytes from the file
   * @return most likely kSuccess or kIoError
   */
  virtual Status Read(size_t offset, size_t byte_count, uint8_t* buffer) = 0;

  /** Writes blocks to the file.
   *
   * @param  buffer the bytes to be written to the file
   * @param  offset must be a multiple of the block size used to open the file
   * @param  bytes  must be a multiple of the block size used to open the file
   * @return most likely kSuccess or kIoError
   */
  virtual Status Write(uint8_t* buffer, size_t offset, size_t byte_count) = 0;

  /** Closes the file and releases its underlying resources.
   *
   * This call deallocates the memory used for the BlockAccessFile, so it cannot
   * be used.
   */
  virtual Status Close() = 0;

 protected:
  /** Instances must be created using Vfs::OpenForBlockAccess(). */
  BlockAccessFile();
  /** Instances must be destroyed using Close(). */
  virtual ~BlockAccessFile() = 0;
};

/**
 * The VFS associated with resource pools by default.
 *
 * If the vfs/ directory is included, BerryDB will provide a default VFS
 * implementation. Embedders that wish to replace the default should not include
 * vfs/ in their BerryDB build, and should implement berrydb::DefaultVfs().
 */
Vfs* DefaultVfs();

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_VFS_H_
