/*
	Copyright 2018 php42

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <exception>
#include "../common/typedefs.h"

namespace libtpdp
{

constexpr unsigned int ARCHIVE_MAGIC = 0x5844;
constexpr unsigned int ARCHIVE_HEADER_SIZE = 28;
constexpr unsigned int ARCHIVE_FILENAME_HEADER_SIZE = 4;
constexpr unsigned int ARCHIVE_FILE_HEADER_SIZE = 44;
constexpr unsigned int ARCHIVE_DIR_HEADER_SIZE = 16;
constexpr unsigned int ARCHIVE_NO_COMPRESSION = 0xffffffff;

struct ArcError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

#pragma pack(push, 1)
struct ArchiveHeader
{
	uint16_t magic;					/* 0x5844 "DX" */
	uint16_t version;
	uint32_t size;					/* size of the file tables (from filename_table_offset till the end of the file) */
	uint32_t data_offset;
	uint32_t filename_table_offset;
	uint32_t file_table_offset;		/* relative to filename_table_offset */
	uint32_t dir_table_offset;		/* relative to filename_table_offset */
	uint32_t codepage;              /* codepage used for encoding of filenames? */

	ArchiveHeader() = default;
	ArchiveHeader(const void *data) {read(data);}

    void read(const void *data) { memcpy(this, data, sizeof(*this)); }
    void write(void *data) const { memcpy(data, this, sizeof(*this)); }
};

struct ArchiveFilenameHeader
{
	uint16_t length;				/* length of the string divided by 4 (padded with zeros if the actual string is shorter).
									 * there are actually two strings of this length, one in all caps followed by one with the real
									 * capitalization. the strings are always NULL terminated */
	uint16_t checksum;              /* all the bytes of the UPPERCASE filename added together */

	ArchiveFilenameHeader() = default;
	ArchiveFilenameHeader(const void *data) {read(data);}

    void read(const void *data) { memcpy(this, data, sizeof(*this)); }
    void write(void *data) const { memcpy(data, this, sizeof(*this)); }
};

struct ArchiveFileHeader
{
	uint32_t filename_offset;		/* relative to filename_table_offset */
	uint32_t attributes;            /* file attributes as from winapi GetFileAttributes() */
	uint64_t time_created;          /* file timestamps as if by winapi GetFileTime() */
	uint64_t time_accessed;
	uint64_t time_written;
	uint32_t data_offset;			/* relative to data_offset in the archive header */
	uint32_t data_size;
	uint32_t compressed_size;		/* 0xffffffff for no compression */

	ArchiveFileHeader() = default;
	ArchiveFileHeader(const void *data) {read(data);}

    void read(const void *data) { memcpy(this, data, sizeof(*this)); }
    void write(void *data) const { memcpy(data, this, sizeof(*this)); }
};

struct ArchiveDirHeader
{
	uint32_t dir_offset;			/* offset of the directory's own file header relative to file_table_offset in the archive header */
	uint32_t parent_dir_offset;		/* relative to dir_table_offset */
	uint32_t num_files;				/* number of files in the directory */
	uint32_t file_header_offset;	/* offset of the directory's contents file headers relative to file_table_offset */

	ArchiveDirHeader() = default;
	ArchiveDirHeader(const void *data) {read(data);}

    void read(const void *data) { memcpy(this, data, sizeof(*this)); }
    void write(void *data) const { memcpy(data, this, sizeof(*this)); }
};
#pragma pack(pop)

static_assert(sizeof(ArchiveHeader) == ARCHIVE_HEADER_SIZE);
static_assert(sizeof(ArchiveFilenameHeader) == ARCHIVE_FILENAME_HEADER_SIZE);
static_assert(sizeof(ArchiveFileHeader) == ARCHIVE_FILE_HEADER_SIZE);
static_assert(sizeof(ArchiveDirHeader) == ARCHIVE_DIR_HEADER_SIZE);

class ArcFile;

/* TODO: implement const_iterators and fix const qualification of iterator methods */

/* this is the main facility for manipulating the game data archives.
 * it uses an STL-like interface for iterating the files and directories contained within.
 * note that all functions that return an iterator may return invalid iterators upon error.
 * you can check for validity with e.g. it < arc.end()
 * also note that the repack and insert methods invalidate all iterators.
 * use the return value of these functions to update your iterators. */
class Archive
{
public:
    class iterator;
    class directory_iterator;

    static const std::size_t npos;

private:
	ArchiveHeader header_;

	std::size_t data_used_, data_max_;
    FileBuf data_; // file buffer

    std::size_t file_table_offset_;
    std::size_t dir_table_offset_;

	bool is_ynk_;

	Archive(const Archive&) = delete;
	Archive& operator=(const Archive&) = delete;

    std::size_t get_header_offset(const std::string& filepath) const;
    std::size_t get_dir_header_offset(std::size_t file_header_offset) const;

    iterator make_iterator(std::size_t offset) const;
    directory_iterator make_dir_iterator(std::size_t offset) const;

	void parse();
    void encrypt();
    void decrypt() { encrypt(); } // encryption is symmetrical, this is an alias of encrypt()

    std::size_t offset_from_file_index(std::size_t index) const { return file_table_offset_ + (index * ARCHIVE_FILE_HEADER_SIZE); }
    std::size_t offset_from_dir_index(std::size_t index) const { return dir_table_offset_ + (index * ARCHIVE_DIR_HEADER_SIZE); }

	std::size_t decompress(const void *src, void *dest) const;

    /* capitalized and all bytes added together.
     * this is a WEAK HASH, there WILL be collisions.
     * so don't depend on this to uniquely identify a file.
     * used for the 'checksum' field of ArchiveFilenameHeader */
    unsigned int hash_filename(const std::string& filename) const;
    unsigned int hash_filename(const std::wstring& filename) const;

    void reallocate(std::size_t sz);

public:
	Archive() : header_(), data_used_(0), data_max_(0), is_ynk_(false) {};
    Archive(const void *data, std::size_t len, bool is_ynk);
    ~Archive() { close(); }

    /* allow move semantics */
    Archive(Archive&&) = default;
    Archive& operator=(Archive&&) = default;

    explicit operator bool() const { return (data_ != nullptr); }

    const char *data() const { return data_.get(); }
    char *data() { return data_.get(); }
    std::size_t size() const { return data_used_; }

    /* throws ArcError on failure */
	void open(const std::string& filename);
	void open(const std::wstring& filename);

    bool save(const std::string& filename);
    bool save(const std::wstring& filename);

	/* returns 0 on error, nonzero otherwise. if dest is NULL, returns decompressed size of the requested file */
	std::size_t get_file(const std::string& filepath, void *dest) const;
    std::size_t get_file(const iterator& file, void *dest) const;

    /* returns empty ArcFile on error */
    ArcFile get_file(const std::string& filepath) const;
    ArcFile get_file(const iterator& file) const;

    /* this will replace existing files only
     * returns iterator to replaced file, all other iterators are invalidated
     * other existing ArcFiles are not invalidated so long as you do not add any new files to the archive */
    iterator repack_file(const std::string& filepath, const void *src, size_t len);
    iterator repack_file(const iterator& file, const void *src, size_t len);
    iterator repack_file(const ArcFile& file);

    /* filename of the given object. root folder returns empty string
     * if normalized == true, returns filename in all caps */
    std::string get_filename(const iterator& file, bool normalized = false) const;
    std::string get_filename(const directory_iterator& file, bool normalized = false) const;

    /* full filepath of the given object. root folder returns empty string */
    std::string get_path(const iterator& file) const;
    std::string get_path(const directory_iterator& file) const;

    /* iterators to the first and last objects in the Archive, used for crawling the entire filesystem
     * note that iterators may point to both files and folders */
    iterator begin() const;
    iterator end() const; // returns iterator to one past the end, do not dereference

    /* iterators to the files contained within the given folder */
    iterator begin(const directory_iterator& folder) const;
    iterator end(const directory_iterator& folder) const; // returns iterator to one past the end, do not dereference

    /* iterators to the directory tree, used for crawling all folders in the archive
     * directory_iterator always points to a folder */
    directory_iterator dir_begin() const;
    directory_iterator dir_end() const; // returns iterator to one past the end, do not dereference

    /* get an iterator to the specified file
     * returns an iterator to end() or dir_end() if no match is found */
    iterator find(const std::string& filepath) const;
    directory_iterator find_dir(const std::string& path) const;

    /* returns a directory_iterator to the containing folder of the given object */
    directory_iterator get_dir(const iterator& it) const;

    /* takes an iterator to a folders ArciveFileHeader entry and converts it to directory_iterator */
    directory_iterator dir_from_file(const iterator& it) const;

    /* insert element before 'it'
     * invalidates all iterators */
    iterator insert(const iterator& it, const ArchiveFileHeader& header);
    directory_iterator insert(const directory_iterator& it, const ArchiveDirHeader& header);
    std::size_t insert_filename_header(const std::string& filename); // filename MUST be shift-jis encoded

    /* insert a new file into an EXISTING folder */
    iterator insert(const void *file, std::size_t size, const std::string& filepath);

    /* returns true if object pointed by it represents a folder */
    bool is_dir(iterator it) const;

	bool is_ynk() const { return is_ynk_; }

    void close() { data_.reset(); data_used_ = 0; data_max_ = 0; is_ynk_ = false; }
};

/* container for files extracted from an Archive */
class ArcFile
{
private:
    std::unique_ptr<char[]> buf_;
    std::size_t len_;
    std::size_t index_;

public:
    ArcFile() : buf_(), len_(0), index_(Archive::npos) {};
    ArcFile(char *buf, std::size_t len, std::size_t index) : buf_(buf), len_(len), index_(index) {};
    ArcFile(std::unique_ptr<char[]>&& buf, std::size_t len, std::size_t index) : buf_(std::move(buf)), len_(len), index_(index) {};
    ArcFile(const ArcFile&) = delete;
    ArcFile(ArcFile&& other) : buf_(std::move(other.buf_)), len_(other.len_), index_(other.index_) { other.reset(); };

    ArcFile& operator =(const ArcFile&) = delete;
    ArcFile& operator =(ArcFile&& other) { buf_ = std::move(other.buf_); len_ = other.len_; index_ = other.index_; other.reset(); return *this; }

    char *data() const { return buf_.get(); }
    std::size_t size() const { return len_; }
    std::size_t index() const { return index_; }

    void reset() { buf_.reset(); len_ = 0; index_ = Archive::npos; }
    void reset(char *buf, std::size_t sz, std::size_t index) { buf_.reset(buf); len_ = sz; index_ = index; }

    explicit operator bool() const { return ((bool)buf_ && len_ && (index_ != Archive::npos)); }
};

/* points to a filesystem object inside an Archive
 * may point to either a file or a folder */
class Archive::iterator
{
private:
    char *data_;
    std::size_t index_;
    std::size_t offset_;

public:
    iterator() : data_(nullptr), index_(Archive::npos), offset_(Archive::npos) {};
    iterator(char *data, std::size_t index, std::size_t offset) : data_(data), index_(index), offset_(offset) {};
    iterator(const iterator&) = default;
    iterator& operator=(const iterator&) = default;

    char *data() const { return data_; }
    std::size_t index() const { return index_; }
    std::size_t offset() const { return offset_; }

    ArchiveFileHeader& operator*() const { return *(ArchiveFileHeader*)data_; }
    ArchiveFileHeader *operator->() const { return (ArchiveFileHeader*)data_; }

    iterator& operator++()
    {
        data_ += ARCHIVE_FILE_HEADER_SIZE;
        offset_ += ARCHIVE_FILE_HEADER_SIZE;
        ++index_;
        return *this;
    }

    iterator operator++(int)
    {
        iterator ret(*this);
        operator++();
        return ret;
    }

    iterator& operator--()
    {
        data_ -= ARCHIVE_FILE_HEADER_SIZE;
        offset_ -= ARCHIVE_FILE_HEADER_SIZE;
        --index_;
        return *this;
    }

    iterator operator--(int)
    {
        iterator ret(*this);
        operator--();
        return ret;
    }
};

/* points to a folder inside an Archive */
class Archive::directory_iterator
{
private:
    char *data_;
    std::size_t index_;
    std::size_t offset_;

public:
    directory_iterator() : data_(nullptr), index_(Archive::npos), offset_(Archive::npos) {};
    directory_iterator(char *data, std::size_t index, std::size_t offset) : data_(data), index_(index), offset_(offset) {};
    directory_iterator(const directory_iterator&) = default;
    directory_iterator& operator=(const directory_iterator&) = default;

    char *data() const { return data_; }
    std::size_t index() const { return index_; }
    std::size_t offset() const { return offset_; }

    ArchiveDirHeader& operator*() const { return *(ArchiveDirHeader*)data_; }
    ArchiveDirHeader *operator->() const { return (ArchiveDirHeader*)data_; }

    directory_iterator& operator++()
    {
        data_ += ARCHIVE_DIR_HEADER_SIZE;
        offset_ += ARCHIVE_DIR_HEADER_SIZE;
        ++index_;
        return *this;
    }

    directory_iterator operator++(int)
    {
        directory_iterator ret(*this);
        operator++();
        return ret;
    }

    directory_iterator& operator--()
    {
        data_ -= ARCHIVE_DIR_HEADER_SIZE;
        offset_ -= ARCHIVE_DIR_HEADER_SIZE;
        --index_;
        return *this;
    }

    directory_iterator operator--(int)
    {
        directory_iterator ret(*this);
        operator--();
        return ret;
    }
};

static inline bool operator==(const Archive::iterator& lhs, const Archive::iterator& rhs) { return lhs.index() == rhs.index(); }
static inline bool operator!=(const Archive::iterator& lhs, const Archive::iterator& rhs) { return !(lhs == rhs); }
static inline bool operator<(const Archive::iterator& lhs, const Archive::iterator& rhs) { return lhs.index() < rhs.index(); }
static inline bool operator>(const Archive::iterator& lhs, const Archive::iterator& rhs) { return rhs < lhs; }
static inline bool operator<=(const Archive::iterator& lhs, const Archive::iterator& rhs) { return !(lhs > rhs); }
static inline bool operator>=(const Archive::iterator& lhs, const Archive::iterator& rhs) { return !(lhs < rhs); }

static inline bool operator==(const Archive::directory_iterator& lhs, const Archive::directory_iterator& rhs) { return lhs.index() == rhs.index(); }
static inline bool operator!=(const Archive::directory_iterator& lhs, const Archive::directory_iterator& rhs) { return !(lhs == rhs); }
static inline bool operator<(const Archive::directory_iterator& lhs, const Archive::directory_iterator& rhs) { return lhs.index() < rhs.index(); }
static inline bool operator>(const Archive::directory_iterator& lhs, const Archive::directory_iterator& rhs) { return rhs < lhs; }
static inline bool operator<=(const Archive::directory_iterator& lhs, const Archive::directory_iterator& rhs) { return !(lhs > rhs); }
static inline bool operator>=(const Archive::directory_iterator& lhs, const Archive::directory_iterator& rhs) { return !(lhs < rhs); }

}
