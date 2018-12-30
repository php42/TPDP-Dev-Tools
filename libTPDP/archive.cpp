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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "archive.h"
#include "../common/endian.h"
#include "../common/filesystem.h"
#include "../common/textconvert.h"
#include <cstdint>
#include <cctype>
#include <cwctype>
#include <cstring>
#include <cassert>
#include <intrin.h>

namespace libtpdp
{

static const uint8_t KEY[] = {0x9B, 0x16, 0xFE, 0x3A, 0xB9, 0xE0, 0xA3, 0x17, 0x9A, 0x23, 0x20, 0xAE};
static const uint8_t KEY_YNK[] = {0x9B, 0x16, 0xFE, 0x3A, 0x98, 0xC2, 0xA0, 0x73, 0x0B, 0x0B, 0xB5, 0x90};

/* venture forth into madness */

void Archive::parse()
{
    if(data_used_ < ARCHIVE_HEADER_SIZE)
        throw ArcError("Archive corrupt or unrecognized format.");

	if((read_le16(data_.get()) ^ read_le16(KEY)) != ARCHIVE_MAGIC)
        throw ArcError("Archive corrupt or unrecognized format.");

    if((read_le16(&data_[2]) ^ read_le16(&KEY[2])) > 5)
        throw ArcError("Unsupported archive version.\r\nUse version 4 (or 5) of the archive file format or yell at me to support newer versions.");

    if((read_le16(&data_[2]) ^ read_le16(&KEY[2])) < 4)
        throw ArcError("Unsupported archive version.\r\nUse version 4 (or 5) of the archive file format.");

	if((read_le32(&data_[8]) ^ read_le32(&KEY[8])) == 0x1c)
	{
		is_ynk_ = false;
	}
	else if((read_le32(&data_[8]) ^ read_le32(&KEY_YNK[8])) == 0x1c)
	{
		is_ynk_ = true;
	}
	else
        throw ArcError("Archive corrupt or unrecognized format.");

    decrypt();

	header_.read(data_.get());

    if(header_.data_offset > data_used_ ||
       header_.dir_table_offset > data_used_ ||
       header_.filename_table_offset > data_used_||
       header_.file_table_offset > data_used_ || 
       header_.size != (data_used_ - header_.filename_table_offset))
        throw ArcError("Archive corrupt or unrecognized format.");

    file_table_offset_ = header_.filename_table_offset + header_.file_table_offset;
    dir_table_offset_ = header_.filename_table_offset + header_.dir_table_offset;
}

void Archive::encrypt()
{
    const uint8_t *key = is_ynk_ ? KEY_YNK : KEY;
    auto data = data_.get();
    std::size_t pos = 0;

    /* SIMD optimzations for lulz */

#if !defined(ARC_NO_AVX) && !defined(ARC_NO_SSE)

    /* since the key is only 12 bytes, we need to repeat it 8 times to achieve
     * a repeatable sequence that can be broken into 32 byte blocks */
    alignas(32) unsigned char avx_key_buf[96];
    for(unsigned int i = 0; i < 8; ++i)
        memcpy(&avx_key_buf[i * 12], key, 12);

    __m256i avxkey1 = _mm256_load_si256((__m256i*)&avx_key_buf[0]);
    __m256i avxkey2 = _mm256_load_si256((__m256i*)&avx_key_buf[32]);
    __m256i avxkey3 = _mm256_load_si256((__m256i*)&avx_key_buf[64]);

    for(; (data_used_ - pos) >= 96; pos += 96)
    {
        auto block1 = _mm256_xor_si256(_mm256_load_si256((__m256i*)&data[pos]), avxkey1);
        auto block2 = _mm256_xor_si256(_mm256_load_si256((__m256i*)&data[pos + 32]), avxkey2);
        auto block3 = _mm256_xor_si256(_mm256_load_si256((__m256i*)&data[pos + 64]), avxkey3);
        _mm256_store_si256((__m256i*)&data[pos], block1);
        _mm256_store_si256((__m256i*)&data[pos + 32], block2);
        _mm256_store_si256((__m256i*)&data[pos + 64], block3);
    }
#elif !defined(ARC_NO_SSE)

    /* since the key is only 12 bytes, we need to repeat it 4 times to achieve
     * a repeatable sequence that can be broken into 16 byte blocks */
    alignas(16) unsigned char sse_key_buf[48];
    for(unsigned int i = 0; i < 4; ++i)
        memcpy(&sse_key_buf[i * 12], key, 12);

    __m128i ssekey1 = _mm_load_si128((__m128i*)&sse_key_buf[0]);
    __m128i ssekey2 = _mm_load_si128((__m128i*)&sse_key_buf[16]);
    __m128i ssekey3 = _mm_load_si128((__m128i*)&sse_key_buf[32]);

    for(; (data_used_ - pos) >= 48; pos += 48)
    {
        auto block1 = _mm_xor_si128(_mm_load_si128((__m128i*)&data[pos]), ssekey1);
        auto block2 = _mm_xor_si128(_mm_load_si128((__m128i*)&data[pos + 16]), ssekey2);
        auto block3 = _mm_xor_si128(_mm_load_si128((__m128i*)&data[pos + 32]), ssekey3);
        _mm_store_si128((__m128i*)&data[pos], block1);
        _mm_store_si128((__m128i*)&data[pos + 16], block2);
        _mm_store_si128((__m128i*)&data[pos + 32], block3);
    }
#else
    uint32_t k1 = *(uint32_t*)(key);
    uint32_t k2 = *(uint32_t*)(key + 4);
    uint32_t k3 = *(uint32_t*)(key + 8);

    /* optimization while we have at least 12 bytes of data */
    for(; (data_used_ - pos) >= 12; pos += 12)
    {
        *(uint32_t*)&data[pos] ^= k1;
        *(uint32_t*)&data[pos + 4] ^= k2;
        *(uint32_t*)&data[pos + 8] ^= k3;
    }
#endif

    /* finish off whatever is left */
    std::size_t j = 0;
    for(; pos < data_used_; ++pos)
    {
        data[pos] ^= key[j++];
        if(j >= sizeof(KEY))
            j = 0;
    }
}

void Archive::open(const std::string& filename)
{
	close();

    std::size_t sz;
	auto buf = aligned_read_file(filename, sz, 32);
    if(buf == nullptr)
        throw ArcError("File I/O read error.");

    data_ = std::move(buf);
    data_used_ = sz;
    data_max_ = sz;

    try
    {
        parse();
    }
    catch(const ArcError&)
    {
        close();
        throw;
    }
}

void Archive::open(const std::wstring& filename)
{
	close();

    std::size_t sz;
    auto buf = aligned_read_file(filename, sz, 32);
    if(buf == nullptr)
        throw ArcError("File I/O read error.");

    data_ = std::move(buf);
    data_used_ = sz;
    data_max_ = sz;

    try
    {
        parse();
    }
    catch(const ArcError&)
    {
        close();
        throw;
    }
}

bool Archive::save(const std::string& filename)
{
    if(!data_)
        return false;

    encrypt();

    bool ret = write_file(filename, data_.get(), data_used_);

    decrypt();

    return ret;
}

bool Archive::save(const std::wstring& filename)
{
    if(!data_)
        return false;

    encrypt();

    bool ret = write_file(filename, data_.get(), data_used_);

    decrypt();

    return ret;
}

std::size_t Archive::get_header_offset(const std::string& filepath) const
{
    std::wstring utf = sjis_to_utf(filepath); // avoid encoding problems
    for(auto& i : utf)
        i = std::towupper(i);

    std::string upper_path = utf_to_sjis(utf);

    std::size_t pos = upper_path.find_first_of("/\\");

    auto dir_it = dir_begin();
    std::size_t offset = dir_it->dir_offset + file_table_offset_;

    for(;;)
    {
        std::string dir = upper_path.substr(0, pos);
        upper_path.erase(0, (pos == std::string::npos) ? std::string::npos : (pos + 1));

        if(dir.empty())
        {
            if(pos != std::string::npos)
            {
                pos = upper_path.find_first_of("/\\");
                continue;
            }
            else
                break;
        }

        //auto checksum = hash_filename(dir);
        bool found = false;
        for(auto it = begin(dir_it); it < end(dir_it); ++it)
        {
            ArchiveFilenameHeader *filename_header = (ArchiveFilenameHeader*)&data_[it->filename_offset + header_.filename_table_offset];
            std::size_t name_offset = it->filename_offset + header_.filename_table_offset + ARCHIVE_FILENAME_HEADER_SIZE;
            std::string name;
            if(filename_header->length > 0)
                name = (const char*)&data_[name_offset /*+ (filename_header->length * 4)*/];
            if(name == dir)
            {
                //if(checksum != filename_header->checksum)
                //   throw ArcError("File: " + dir + "\r\nBad checksum.");
                found = true;
                offset = it.offset();
                dir_it = dir_from_file(it);
                break;
            }
        }
        if(!found)
            return -1;

        pos = upper_path.find_first_of("/\\");
    }

    return offset;
}

std::size_t Archive::get_dir_header_offset(std::size_t file_header_offset) const
{
    if(file_header_offset == -1)
        return -1;

    assert(file_header_offset >= file_table_offset_ && file_header_offset <= dir_table_offset_);
    if(file_header_offset < file_table_offset_ || file_header_offset > dir_table_offset_)
        return -1;

    /* alignment check */
    assert(((file_header_offset - file_table_offset_) % ARCHIVE_FILE_HEADER_SIZE) == 0);
    if(((file_header_offset - file_table_offset_) % ARCHIVE_FILE_HEADER_SIZE) != 0)
        return -1;

    for(auto offset = dir_table_offset_; offset < data_used_; offset += ARCHIVE_DIR_HEADER_SIZE)
    {
        if(((ArchiveDirHeader*)&data_[offset])->dir_offset == file_header_offset - file_table_offset_)
            return offset;
    }

    return -1;
}

Archive::iterator Archive::make_iterator(std::size_t offset) const
{
    assert((offset >= file_table_offset_ && offset <= dir_table_offset_) || offset == -1);
    if(offset < file_table_offset_ || offset > dir_table_offset_)
        return end();

    /* alignment check */
    assert(((offset - file_table_offset_) % ARCHIVE_FILE_HEADER_SIZE) == 0);
    if(((offset - file_table_offset_) % ARCHIVE_FILE_HEADER_SIZE) != 0)
        return end();

    return iterator(&data_[offset], (offset - file_table_offset_) / ARCHIVE_FILE_HEADER_SIZE, offset);
}

Archive::directory_iterator Archive::make_dir_iterator(std::size_t offset) const
{
    assert((offset >= dir_table_offset_ && offset <= data_used_) || offset == -1);
    if(offset < dir_table_offset_ || offset > data_used_)
        return dir_end();

    /* alignment check */
    assert(((offset - dir_table_offset_) % ARCHIVE_DIR_HEADER_SIZE) == 0);
    if(((offset - dir_table_offset_) % ARCHIVE_DIR_HEADER_SIZE) != 0)
        return dir_end();

    return directory_iterator(&data_[offset], (offset - dir_table_offset_) / ARCHIVE_DIR_HEADER_SIZE, offset);
}

std::size_t Archive::get_file(const iterator& it, void *dest) const
{
    assert((it.index() > 0) && (it < end()));
    if(it.index() == 0 || it >= end())
        return 0;

    ArchiveFileHeader file_header(it.data());

    if(dest == NULL)
        return file_header.data_size;

    if(file_header.data_size == 0)
        return 0;

    auto dataptr = &data_[file_header.data_offset + header_.data_offset];

    if(file_header.compressed_size == ARCHIVE_NO_COMPRESSION)
        memcpy(dest, dataptr, file_header.data_size);
    else
        return decompress(dataptr, dest);

    return file_header.data_size;
}

std::size_t Archive::get_file(const std::string& filepath, void *dest) const
{
    return get_file(find(filepath), dest);
}

ArcFile Archive::get_file(const std::string& filepath) const
{
    return get_file(find(filepath));
}

ArcFile Archive::get_file(const iterator& it) const
{
    assert((it.index() > 0) && (it < end()));
    if(it.index() == 0 || it >= end())
        return {};

    ArchiveFileHeader file_header(it.data());

    if(file_header.data_size == 0)
        return {};

    auto dataptr = &data_[file_header.data_offset + header_.data_offset];
    auto buf = std::make_unique<char[]>(file_header.data_size);

    if(file_header.compressed_size == ARCHIVE_NO_COMPRESSION)
        memcpy(buf.get(), dataptr, file_header.data_size);
    else
        if(decompress(dataptr, buf.get()) != file_header.data_size)
            return {};

    return ArcFile(std::move(buf), file_header.data_size, it.index());
}

Archive::iterator Archive::repack_file(const std::string & filepath, const void * src, size_t len)
{
    return repack_file(find(filepath), src, len);
}

Archive::iterator Archive::repack_file(const iterator& it, const void * src, size_t len)
{
    assert((it.index() > 0) && (it < end()));
    if(it.index() == 0 || it >= end())
        return end();

    size_t header_offset = file_table_offset_ + (it.index() * ARCHIVE_FILE_HEADER_SIZE);
    ArchiveFileHeader *header_ptr = (ArchiveFileHeader*)&data_[header_offset];

    ArchiveFileHeader file_header(*header_ptr);
    header_ptr->data_size = (uint32_t)len;
    header_ptr->compressed_size = ARCHIVE_NO_COMPRESSION;

    size_t orig_len = (file_header.compressed_size == ARCHIVE_NO_COMPRESSION) ? file_header.data_size : file_header.compressed_size;
    size_t diff = len - orig_len;
    size_t new_used = data_used_ + diff;

    if(new_used > data_max_) /* buffer is too small for the new file, reallocate */
        reallocate((std::size_t)(new_used * 1.15)); /* allocate 15% extra to avoid future reallocations */

    /* shift everything after the file we're replacing to account for the size difference */
    size_t i = file_header.data_offset + header_.data_offset + len;
    size_t j = file_header.data_offset + header_.data_offset + orig_len;
    memmove(&data_[i], &data_[j], data_used_ - j);                              /* adjust the gap */
    memcpy(&data_[file_header.data_offset + header_.data_offset], src, len);    /* copy new file into the gap */

    data_used_ = new_used;

    header_.filename_table_offset += (uint32_t)diff;
    memcpy(data_.get(), &header_, sizeof(header_));

    header_offset += diff;
    file_table_offset_ += diff;
    dir_table_offset_ += diff;

    /* adjust the offsets of all files after the repacked file */
    for(size_t i = file_table_offset_; i < dir_table_offset_; i += ARCHIVE_FILE_HEADER_SIZE)
    {
        uint32_t *off = (uint32_t*)&data_[i + 32];
        if(*off > file_header.data_offset)
            *off += (uint32_t)diff;
    }

    return make_iterator(header_offset);
}

Archive::iterator Archive::repack_file(const ArcFile& file)
{
    if(!file)
        return end();
    return repack_file(make_iterator(offset_from_file_index(file.index())), file.data(), file.size());
}

std::string Archive::get_filename(const iterator& it, bool normalized) const
{
    assert((it.index() > 0) && (it < end()));
    if(it.index() == 0 || it >= end())
        return {};

    std::size_t name_offset = it->filename_offset + header_.filename_table_offset;
    ArchiveFilenameHeader name_header(&data_[name_offset]);
    std::string name;
    if(name_header.length > 0)
        name = (const char*)&data_[name_offset + ARCHIVE_FILENAME_HEADER_SIZE + (normalized ? 0 : (name_header.length * 4))];
    return name;
}

std::string Archive::get_filename(const directory_iterator& it, bool normalized) const
{
    assert((it.index() > 0) && (it < dir_end()));
    if(it.index() == 0 || it >= dir_end())
        return {};

    std::size_t header_offset = it->dir_offset + file_table_offset_;

    return get_filename(make_iterator(header_offset), normalized);
}

std::string Archive::get_path(const iterator& it) const
{
    assert((it.index() > 0) && (it < end()));
    if(it.index() == 0 || it >= end())
        return {};

    auto dir_it = get_dir(it);

    if(dir_it >= dir_end())
        return {};

    std::string name = get_filename(it);
    while(dir_it->parent_dir_offset != 0xFFFFFFFF)
    {
        name = get_filename(dir_it) + "/" + name;
        dir_it = make_dir_iterator(dir_table_offset_ + dir_it->parent_dir_offset);
    }

    return name;
}

std::string Archive::get_path(const directory_iterator& it) const
{
    assert((it.index() > 0) && (it < dir_end()));
    if(it.index() == 0 || it >= dir_end())
        return {};

    std::size_t header_offset = it->dir_offset + header_.file_table_offset;

    return get_path(make_iterator(header_offset));
}

Archive::iterator Archive::begin() const
{
    return make_iterator(file_table_offset_);
}

Archive::iterator Archive::end() const
{
    return make_iterator(dir_table_offset_); // dir_table_offset begins immediately after the end of the file table
}

Archive::iterator Archive::begin(const directory_iterator& it) const
{
    if(it >= dir_end())
        return end();

    return make_iterator(file_table_offset_ + it->file_header_offset);
}

Archive::iterator Archive::end(const directory_iterator& it) const
{
    if(it >= dir_end())
        return end();

    auto offset = it->file_header_offset + (it->num_files * ARCHIVE_FILE_HEADER_SIZE);
    return make_iterator(file_table_offset_ + offset);
}

Archive::directory_iterator Archive::dir_begin() const
{
    return make_dir_iterator(dir_table_offset_);
}

Archive::directory_iterator Archive::dir_end() const
{
    return make_dir_iterator(data_used_);
}

Archive::iterator Archive::find(const std::string& filepath) const
{
    return make_iterator(get_header_offset(filepath));
}

Archive::directory_iterator Archive::find_dir(const std::string& filepath) const
{
    return make_dir_iterator(get_dir_header_offset(get_header_offset(filepath)));
}

Archive::directory_iterator Archive::get_dir(const iterator& it) const
{
    assert((it.index() > 0) && (it < end()));
    if(it.index() == 0 || it >= end())
        return dir_end();

    for(auto dir_it = dir_begin(); dir_it != dir_end(); ++dir_it)
    {
        if((begin(dir_it) <= it) && (end(dir_it) > it))
            return dir_it;
    }

    return dir_end();
}

Archive::directory_iterator Archive::dir_from_file(const iterator& it) const
{
    if(!is_dir(it))
        return dir_end();

    return make_dir_iterator(get_dir_header_offset(it.offset()));
}

Archive::iterator Archive::insert(const iterator& it, const ArchiveFileHeader& header)
{
    std::size_t new_len = data_used_ + sizeof(header);

    if(new_len > data_max_) // buffer is too small, reallocate
        reallocate((std::size_t)(new_len * 1.15));  // allocate 15% extra to avoid future reallocations

    // shift everything to make room for the new header
    memmove(&data_[it.offset() + sizeof(header)], &data_[it.offset()], data_used_ - it.offset());
    header.write(&data_[it.offset()]);

    data_used_ = new_len;

    header_.size += sizeof(header);
    header_.dir_table_offset += sizeof(header);
    dir_table_offset_ += sizeof(header);
    header_.write(data_.get());

    /* we now need to fix the offsets of all directories that reference
     * file headers at or after the one we inserted */
    for(auto dir_it = dir_begin(); dir_it < dir_end(); ++dir_it)
    {
        if(dir_it->dir_offset >= (it.offset() - file_table_offset_))
            dir_it->dir_offset += sizeof(ArchiveFileHeader);
        if(dir_it->file_header_offset >= (it.offset() - file_table_offset_))
            dir_it->file_header_offset += sizeof(ArchiveFileHeader);
    }

    return make_iterator(it.offset());
}

Archive::iterator Archive::insert(const void *file, std::size_t size, const std::string& filepath)
{
    std::size_t pos = filepath.find_last_of("/\\");
    std::string dir = filepath.substr(0, pos);
    std::string name;

    if(get_header_offset(filepath) != -1) // file already exists
        return end();

    if(pos == std::string::npos)
        name = filepath;
    else
        name = filepath.substr(pos + 1);

    auto filename_offset = insert_filename_header(name);

    // create a header for a zero-length file then repack it with our new file
    ArchiveFileHeader header;
    memset(&header, 0, sizeof(header));
    header.data_offset = header_.filename_table_offset - header_.data_offset;
    header.compressed_size = ARCHIVE_NO_COMPRESSION;
    header.filename_offset = (uint32_t)filename_offset - header_.filename_table_offset;

    auto dir_it = make_dir_iterator(get_dir_header_offset(get_header_offset(dir)));
    if(dir_it >= dir_end())
        return end();

    auto it = end(dir_it);
    dir_it->num_files += 1;

    it = insert(it, header);

    it = repack_file(it, file, size);

    return it;
}

Archive::directory_iterator Archive::insert(const directory_iterator& it, const ArchiveDirHeader& header)
{
    std::size_t new_len = data_used_ + sizeof(header);

    if(new_len > data_max_) // buffer is too small, reallocate
        reallocate((std::size_t)(new_len * 1.15));  // allocate 15% extra to avoid future reallocations

    // shift everything to make room for the new header
    memmove(&data_[it.offset() + sizeof(header)], &data_[it.offset()], data_used_ - it.offset());
    header.write(&data_[it.offset()]);

    data_used_ = new_len;

    header_.size += sizeof(header);
    header_.write(data_.get());
    return make_dir_iterator(it.offset());
}

std::size_t Archive::insert_filename_header(const std::string& filename)
{
    std::size_t name_len = (std::size_t)std::ceil((filename.size() + 1) / 4.0);
    std::size_t header_len = sizeof(ArchiveFilenameHeader) + ((name_len * 4) * 2);
    std::size_t new_len = data_used_ + header_len;
    ArchiveFilenameHeader header;
    memset(&header, 0, sizeof(header));
    header.length = (uint16_t)name_len;
    std::size_t ret = file_table_offset_;

    auto utf = sjis_to_utf(filename);
    for(auto& i : utf)
        i = std::towupper(i);
    auto upper_name = utf_to_sjis(utf);

    for(auto i : upper_name)
        header.checksum += (unsigned char)i;

    if(new_len > data_max_) // buffer is too small, reallocate
        reallocate((std::size_t)(new_len * 1.15));          // allocate 15% extra to avoid future reallocations

    // shift everything to make room for the new header
    memmove(&data_[file_table_offset_ + header_len], &data_[file_table_offset_], data_used_ - file_table_offset_);
    memset(&data_[file_table_offset_], 0, header_len);
    header.write(&data_[file_table_offset_]);

    memcpy(&data_[file_table_offset_ + sizeof(header)], upper_name.data(), upper_name.size());
    memcpy(&data_[file_table_offset_ + sizeof(header) + (name_len * 4)], filename.data(), filename.size());

    data_used_ = new_len;

    header_.size += (uint32_t)header_len;
    header_.dir_table_offset += (uint32_t)header_len;
    header_.file_table_offset += (uint32_t)header_len;
    file_table_offset_ += header_len;
    dir_table_offset_ += header_len;
    header_.write(data_.get());

    return ret;
}

bool Archive::is_dir(iterator it) const
{
    //return (get_dir_header_offset(it.offset()) != -1);
    return (it->attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::size_t Archive::decompress(const void *src, void *dest) const
{
	uint32_t output_size, input_size, offset, bytes_written = 0, len;
	uint8_t *outptr, key;
	const uint8_t *inptr;
	const uint8_t *endin;

	inptr = (const uint8_t*)src;
	outptr = (uint8_t*)dest;

	output_size = read_le32(&inptr[0]);
	input_size = read_le32(&inptr[4]);
    endin = inptr + input_size;

	if(dest == NULL)
		return output_size;

	key = inptr[8];
	inptr += 9;

	while(inptr < endin)
	{
		if(bytes_written >= output_size)
			return bytes_written;

		if(inptr[0] != key)
		{
			*(outptr++) = *(inptr++);
			++bytes_written;
			continue;
		}

		if(inptr[1] == key)	/* escape sequence */
		{
			*(outptr++) = key;
			inptr += 2;
			++bytes_written;
			continue;
		}

		unsigned int val = inptr[1];
		if(val > key)
			--val;

		inptr += 2;

		unsigned int offset_len = val & 3;
		len = val >> 3;

		if(val & 4)
		{
			len |= *inptr++ << 5;
		}

		len += 4;

		if(offset_len == 0)
		{
			offset = *inptr++;
		}
		else if(offset_len == 1)
		{
			offset = inptr[0];
			offset |= uint32_t(inptr[1]) << 8;
			inptr += 2;
		}
		else
		{
			offset = inptr[0];
			offset |= uint32_t(inptr[1]) << 8;
			offset |= uint32_t(inptr[2]) << 16;
			inptr += 3;
		}
		++offset;

		while(len > offset)
		{
			if((bytes_written + offset) > output_size)
				return bytes_written;
			memcpy(outptr, outptr - offset, offset);
			outptr += offset;
			len -= offset;
			bytes_written += offset;
			offset += offset;
		}

		if(len > 0)
		{
			if((bytes_written + len) > output_size)
				return bytes_written;
			memcpy(outptr, outptr - offset, len);
			outptr += len;
			bytes_written += len;
		}
	}

	return bytes_written;
}

unsigned int Archive::hash_filename(const std::string& filename) const
{
    auto utf = sjis_to_utf(filename); // avoid character encoding problems
    for(auto& i : utf)
        i = std::towupper(i);

    auto upper = utf_to_sjis(utf);

    unsigned int ret = 0;
    for(auto i : upper)
        ret += (unsigned char)i;
    return ret;
}

void Archive::reallocate(std::size_t sz)
{
    data_.reset((char*)_aligned_realloc(data_.release(), sz, 32));
    data_max_ = sz;
}

Archive::Archive(const void *data, std::size_t len, bool is_ynk) : data_used_(len), data_max_(len), is_ynk_(is_ynk)
{
    data_ = AlignedFileBuf((char*)_aligned_malloc(len, 32), _aligned_free);
    memcpy(data_.get(), data, len);
    header_.read(data_.get());

    file_table_offset_ = header_.filename_table_offset + header_.file_table_offset;
    dir_table_offset_ = header_.filename_table_offset + header_.dir_table_offset;
}

}
