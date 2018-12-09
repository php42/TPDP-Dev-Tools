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

#include <algorithm>
#include "savefile.h"
#include "puppet.h"
#include "../common/endian.h"
#include "../common/filesystem.h"
#include <cstring>
#include <map>
#include "../common/textconvert.h"

static const uint8_t save_magic[] = {0x8C, 0xB6, 0x91, 0x7A, 0x90, 0x6C, 0x8C, 0x60, 0x89, 0x89, 0x95, 0x91, 0x00};
static const uint8_t save_magic_ynk[] = {0x8C, 0xB6, 0x91, 0x7A, 0x90, 0x6C, 0x8C, 0x60, 0x89, 0x89, 0x95, 0x91, 0x41, 0x50, 0x00};

static const unsigned int pocket_offsets[] = {0x00, 0x80, 0x100, 0x200, 0x300, 0x400, 0x480};
static const unsigned int pocket_sizes[] = {0x40, 0x40, 0x80, 0x80, 0x80, 0x40, 0x20};

namespace libtpdp
{

/* TODO: replace raw pointers to heap memory with unique_ptr */

SaveFile::SaveFile(SaveFile&& old)
{
    close();

	cryptobuf_ = old.cryptobuf_;
	savebuf_ = old.savebuf_;
	cryptobuf_len_ = old.cryptobuf_len_;
	savebuf_len_ = old.savebuf_len_;
	puppet_offset_ = old.puppet_offset_;
	item_offset_ = old.item_offset_;
	item_num_offset_ = old.item_num_offset_;
	seed_ = old.seed_;
	num_boxes_ = old.num_boxes_;
	filename_ = old.filename_;
	wfilename_ = old.wfilename_;
	is_expansion_ = old.is_expansion_;

	old.cryptobuf_ = NULL;
	old.savebuf_ = NULL;
	old.close();
}

SaveFile& SaveFile::operator=(SaveFile&& old)
{
    close();

	cryptobuf_ = old.cryptobuf_;
	savebuf_ = old.savebuf_;
	cryptobuf_len_ = old.cryptobuf_len_;
	savebuf_len_ = old.savebuf_len_;
	puppet_offset_ = old.puppet_offset_;
	item_offset_ = old.item_offset_;
	item_num_offset_ = old.item_num_offset_;
	seed_ = old.seed_;
	num_boxes_ = old.num_boxes_;
	filename_ = old.filename_;
	wfilename_ = old.wfilename_;
	is_expansion_ = old.is_expansion_;

	old.cryptobuf_ = NULL;
	old.savebuf_ = NULL;
	old.close();
	return *this;
}

bool SaveFile::open(const std::string& filename, const void *rand_data, std::size_t rand_len)
{
	close();

	std::size_t len = 0;
	auto buf = read_file(filename, len);
	if(buf == nullptr)
        return false;

	filename_ = filename;
	cryptobuf_ = (const char*)rand_data;
    cryptobuf_len_ = rand_len;

	return load_savefile(buf.get(), len);
}

bool SaveFile::open(const std::wstring& filename, const void *rand_data, std::size_t rand_len)
{
	close();

	std::size_t len = 0;
	auto buf = read_file(filename, len);
	if(buf == nullptr)
        return false;

	wfilename_ = filename;
	cryptobuf_ = (const char*)rand_data;
    cryptobuf_len_ = rand_len;

	return load_savefile(buf.get(), len);
}

bool SaveFile::save()
{
    if(empty())
        return false;

    std::size_t len;
	char *outfile = generate_save(len);

	bool ret = false;
	if(!filename_.empty())
	{
	    ret = write_file(filename_, outfile, len);
	}
	else if(!wfilename_.empty())
	{
	    ret = write_file(wfilename_, outfile, len);
	}

	delete[] outfile;
    return ret;
}

bool SaveFile::save(const std::string& filename)
{
    if(empty())
        return false;

    std::size_t len;
	char *outfile = generate_save(len);

	bool ret = write_file(filename, outfile, len);

	delete[] outfile;
    return ret;
}

bool SaveFile::save(const std::wstring& filename)
{
    if(empty())
        return false;

    std::size_t len;
	char *outfile = generate_save(len);

	bool ret = write_file(filename, outfile, len);

	delete[] outfile;
    return ret;
}

bool SaveFile::load_savefile(char *buf, std::size_t len)
{
	if(cryptobuf_ == NULL)
        return false;

	if(memcmp(buf, save_magic, sizeof(save_magic)) == 0)
		is_expansion_ = false;
	else if(memcmp(buf, save_magic_ynk, sizeof(save_magic_ynk)) == 0)
		is_expansion_ = true;
	else
	{
		close();
		return false;
	}

	char *packed_buf = buf + SAVEFILE_DATA_OFFSET;
	uint32_t packed_len = read_le32(buf + SAVEFILE_LENGTH_OFFSET);
	seed_ = read_le32(buf + SAVEFILE_SEED_OFFSET);

	if((SAVEFILE_DATA_OFFSET + packed_len) > len)
    {
        close();
        return false;
    }

	descramble(packed_buf, cryptobuf_, seed_, packed_len);

	decrypt(packed_buf, seed_, packed_len);

	savebuf_len_ = decompress(packed_buf, NULL);

	if((!is_expansion_) && (savebuf_len_ != 0x44993))
	{
		close();
		return false;
	}
	else if(is_expansion_ && (savebuf_len_ != 0x5beeb))
	{
		close();
		return false;
	}

	savebuf_ = new char[savebuf_len_];

	if(decompress(packed_buf, savebuf_) != savebuf_len_)
	{
		close();
		return false;
	}

	num_boxes_ = SAVEFILE_NUM_BOXES;
	puppet_offset_ = SAVEFILE_PUPPET_OFFSET;
	item_offset_ = SAVEFILE_ITEM_OFFSET;
	item_num_offset_ = SAVEFILE_ITEM_NUM_OFFSET;
	if(is_expansion_)
	{
		num_boxes_ = SAVEFILE_NUM_BOXES_YNK;
		puppet_offset_ = SAVEFILE_PUPPET_OFFSET_YNK;
		item_offset_ = SAVEFILE_ITEM_OFFSET_YNK;
		item_num_offset_ = SAVEFILE_ITEM_NUM_OFFSET_YNK;
	}

	decrypt_all_puppets();

	return true;
}

char *SaveFile::generate_save(std::size_t& size)
{
    /* allocate an oversized buffer in case compression expands the data */
    char *filebuf = new char[savebuf_len_ * 2];
    memcpy(filebuf, cryptobuf_, cryptobuf_len_);
    memcpy(filebuf, is_expansion_ ? save_magic_ynk : save_magic, is_expansion_ ? sizeof(save_magic_ynk) : sizeof(save_magic));

	encrypt_all_puppets();
	seed_ = update_savefile_hash(savebuf_, savebuf_len_);
	write_le32(filebuf + SAVEFILE_SEED_OFFSET, seed_);
	std::size_t len = compress(savebuf_, filebuf + SAVEFILE_DATA_OFFSET, savebuf_len_);
	write_le32(filebuf + SAVEFILE_LENGTH_OFFSET, (uint32_t)len);
	encrypt(filebuf + SAVEFILE_DATA_OFFSET, seed_, len);
	scramble(filebuf + SAVEFILE_DATA_OFFSET, cryptobuf_, seed_, (uint32_t)len);
	decrypt_all_puppets();

    size = std::max((len + SAVEFILE_DATA_OFFSET), cryptobuf_len_);
    return filebuf;
}

std::size_t SaveFile::get_puppet_offset(unsigned int index)
{
	if(!(index < (30 * num_boxes_) + 6))
		return -1;

	if(index < 6)
		return index * PUPPET_SIZE_PARTY;
	else
		return (6 * PUPPET_SIZE_PARTY) + ((index - 6) * PUPPET_SIZE_BOX);
}

void SaveFile::decrypt_all_puppets()
{
	char *puppet = savebuf_ + puppet_offset_;

	for(unsigned int i = 0; i < (30 * num_boxes_) + 6; ++i)
	{
		if((read_le32(puppet) != 0) || (i < 6))
			decrypt_puppet(puppet, cryptobuf_, PUPPET_SIZE);

		if(i < 6)
			puppet += PUPPET_SIZE_PARTY;
		else
			puppet += PUPPET_SIZE_BOX;
	}
}

void SaveFile::encrypt_all_puppets()
{
	char *puppet = savebuf_ + puppet_offset_;

	for(unsigned int i = 0; i < (30 * num_boxes_) + 6; ++i)
	{
		if((read_le32(puppet) != 0) || (i < 6))
		{
			update_puppet_hash(puppet);
			encrypt_puppet(puppet, cryptobuf_, PUPPET_SIZE);
		}

		if(i < 6)
			puppet += PUPPET_SIZE_PARTY;
		else
			puppet += PUPPET_SIZE_BOX;
	}
}

void SaveFile::close()
{
	if(savebuf_ != NULL)
		delete[] savebuf_;

	cryptobuf_ = NULL;
	savebuf_ = NULL;

	cryptobuf_len_ = 0;
	savebuf_len_ = 0;
    num_boxes_ = 0;

	filename_.clear();
	wfilename_.clear();
}

bool SaveFile::get_puppet(Puppet& puppet, unsigned int index)
{
    if(empty())
        return false;

	std::size_t offset = get_puppet_offset(index);
	if(offset == (std::size_t)-1)
		return false;

	offset += puppet_offset_;

	uint32_t hash = read_le32(savebuf_ + offset + PUPPET_SIZE);
	if(hash == 0)
		return false;

	puppet.read(savebuf_ + offset, index < 6);

	return true;
}

void SaveFile::delete_puppet(unsigned int index)
{
    if(empty())
        return;

	std::size_t offset = get_puppet_offset(index);
	if(offset == (std::size_t)-1)
		return;
	offset += puppet_offset_;

	int size;
	if(index < 6)
		size = PUPPET_SIZE_PARTY;
	else
		size = PUPPET_SIZE_BOX;

	memset(savebuf_ + offset, 0, size);

	if(index < 6)
		Puppet().write(savebuf_ + offset, true);
}

void SaveFile::save_puppet(const Puppet& puppet, unsigned int index)
{
    if(empty())
        return;

	std::size_t offset = get_puppet_offset(index);
	if(offset == (std::size_t)-1)
		return;
	offset += puppet_offset_;

	puppet.write(savebuf_ + offset, index < 6);
	update_puppet_hash(savebuf_ + offset);
}

int SaveFile::get_item_id(unsigned int pocket, unsigned int index)
{
    if(empty())
        return -1;

	if(!(pocket < 7))
		return -1;

	if(!((index * 2) < pocket_sizes[pocket]))
		return -1;

	std::size_t offset = item_offset_ + pocket_offsets[pocket];

	return read_le16(&savebuf_[offset + (index * 2)]);
}

int SaveFile::get_item_quantity(unsigned int item_id)
{
    if(empty())
        return -1;

	if(!(item_id < 1024))
		return -1;

	uint8_t *buf = (uint8_t*)savebuf_ + item_num_offset_;

	return buf[item_id];
}

bool SaveFile::set_item_id(unsigned int pocket, unsigned int index, unsigned int item_id)
{
    if(empty())
        return false;

	if(!(pocket < 7))
		return false;

	if(!((index * 2) < pocket_sizes[pocket]))
		return false;

	if(!(item_id < 1024))
		return false;

	std::size_t offset = item_offset_;
	for(unsigned int i = 0; i <= pocket; ++i)
		offset += pocket_offsets[i];

	write_le16(&savebuf_[offset + (index * 2)], item_id);

	return true;
}

bool SaveFile::set_item_quantity(unsigned int item_id, unsigned int quantity)
{
    if(empty())
        return false;

	if(!(item_id < 1024))
		return false;

	uint8_t *buf = (uint8_t*)savebuf_ + item_num_offset_;

	buf[item_id] = uint8_t(quantity);

	return true;
}

std::wstring SaveFile::get_box_name(std::size_t index)
{
    if(empty() || (index >= num_boxes_))
        return L"None";

    std::size_t offset = is_expansion_ ? SAVEFILE_BOX_NAME_OFFSET_YNK : SAVEFILE_BOX_NAME_OFFSET;
    offset += index * 32;

    return sjis_to_utf(&savebuf_[offset]);
}

void SaveFile::set_box_name(const std::wstring& name, std::size_t index)
{
    if(empty() || (index >= num_boxes_))
        return;

    std::size_t offset = is_expansion_ ? SAVEFILE_BOX_NAME_OFFSET_YNK : SAVEFILE_BOX_NAME_OFFSET;
    offset += index * 32;

    std::string str = utf_to_sjis(name);
    snprintf(&savebuf_[offset], 32, "%s", str.c_str());
}

std::wstring SaveFile::get_player_name()
{
    if(empty())
        return L"None";

	return sjis_to_utf(&savebuf_[0xb9]);
}

void SaveFile::set_player_name(const std::wstring& name)
{
    if(empty())
        return;

	std::string str = utf_to_sjis(name);
	snprintf(&savebuf_[0xb9], 32, "%s", str.c_str());
}

uint32_t SaveFile::get_player_id()
{
    if(empty())
        return 0;

	return read_le32(&savebuf_[0xd9]);
}

void SaveFile::set_player_id(uint32_t id)
{
    if(empty())
        return;

	write_le32(&savebuf_[0xd9], id);
}

uint32_t SaveFile::get_player_secret_id()
{
    if(empty())
        return 0;

	return read_le32(&savebuf_[0xdd]);
}

void SaveFile::set_player_secret_id(uint32_t id)
{
    if(empty())
        return;

	write_le32(&savebuf_[0xdd], id);
}

uint32_t SaveFile::update_savefile_hash(void *data, std::size_t len)
{
	uint8_t *buf = (uint8_t*)data;
	uint32_t seed = read_le32(&buf[0x24]);

	uint32_t hash = seed;
	for(unsigned int i = 0xb8; i < len; ++i)
		hash = (buf[i] + (hash * 2)) % 0x7FFFFFFF;

	write_le32(&buf[0x10], hash);
	snprintf((char*)&buf[0x28], 32, "%u", hash);

	return hash;
}

void SaveFile::update_puppet_hash(void *data)
{
	uint32_t hash = 0;
	uint8_t *buf = (uint8_t*)data;

	for(unsigned int i = 0; i < PUPPET_SIZE; i += 3)
		hash += buf[i];

	write_le32(buf + PUPPET_SIZE, hash);
}

void SaveFile::descramble(void *src, const void *rand_data, uint32_t seed, uint32_t len)
{
	uint8_t *buf = (uint8_t*)src;
	const uint8_t *randbuf = (const uint8_t*)rand_data;
	uint32_t crypto1, crypto2;
	uint32_t count = len;
	--count;
	crypto1 = count * 6;
	crypto2 = count * 5;

	while((count & 0x80000000) == 0)
	{
		uint32_t index1 = read_le32(&randbuf[((seed + count + 0x14ddb) & 0x3fff) * 4]);
		index1 += crypto2;
		index1 = index1 % len;

		uint32_t index2 = read_le32(&randbuf[((seed + count + 0x14ddc) & 0x3fff) * 4]);
		index2 += crypto1;
		index2 = index2 % len;

		uint8_t temp = buf[index1];
		buf[index1] = buf[index2];
		buf[index2] = temp;

		--count;
		crypto1 -= 6;
		crypto2 -= 5;
	}
}

void SaveFile::scramble(void *src, const void *rand_data, uint32_t seed, uint32_t len)
{
	uint8_t *buf = (uint8_t*)src;
	const uint8_t *randbuf = (const uint8_t*)rand_data;
	uint32_t crypto1, crypto2;
	uint32_t count = 0;
	crypto1 = 0;
	crypto2 = 0;

	while(count < len)
	{
		uint32_t index1 = read_le32(&randbuf[((seed + count + 0x14ddb) & 0x3fff) * 4]);
		index1 += crypto2;
		index1 = index1 % len;

		uint32_t index2 = read_le32(&randbuf[((seed + count + 0x14ddc) & 0x3fff) * 4]);
		index2 += crypto1;
		index2 = index2 % len;

		uint8_t temp = buf[index1];
		buf[index1] = buf[index2];
		buf[index2] = temp;

		++count;
		crypto1 += 6;
		crypto2 += 5;
	}
}

void SaveFile::decrypt(void *src, uint32_t seed, std::size_t len)
{
	uint8_t *buf = (uint8_t*)src;
    for(std::size_t i = 0; i < len; i += 2)
    {
        if(i + 1 >= len)
            break;
        buf[i] = ~buf[i];
        if((i & 3) == 0)
            buf[i + 1] = ~buf[i + 1];
        buf[i + 1] -= uint8_t(seed);
    }
}

void SaveFile::encrypt(void *src, uint32_t seed, std::size_t len)
{
	uint8_t *buf = (uint8_t*)src;
    for(std::size_t i = 0; i < len; i += 2)
    {
        if(i + 1 >= len)
            break;
        buf[i] = ~buf[i];
        buf[i + 1] += uint8_t(seed);
        if((i & 3) == 0)
            buf[i + 1] = ~buf[i + 1];
    }
}

/* 'key' is used as a control code for the compression stream, when 'key' is encountered,
* the next byte indicates the offset in bytes from the end of the output stream to copy from.
* the byte after that indicates the total length to copy to the output stream.
*
* if 'key' is immediately followed by another key, this behaviour is escaped and 'key' is
* written to the output stream as a single byte. to avoid being escaped, offsets of 'key'
* and above are incremented by 1.
*
* any values other than 'key' are written to the output stream as a single byte. */
std::size_t SaveFile::decompress(const void *src, void *dest)
{
	uint32_t output_size, input_size, bytes_written = 0;
	uint8_t *outptr, key, offset, len;
	const uint8_t *inptr;
	const uint8_t *endin;

	inptr = (const uint8_t*)src;
	outptr = (uint8_t*)dest;

	output_size = read_le32(&inptr[0]);
	input_size = read_le32(&inptr[4]) - 12;

	if(dest == NULL)
		return output_size;

	key = inptr[8];

	inptr += 12;
	endin = inptr + input_size;

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

		offset = inptr[1];
		if(offset > key)
			--offset;

		len = inptr[2];

		inptr += 3;

		if(bytes_written + len > output_size)
			return bytes_written;
		memcpy(outptr, outptr - offset, len);
		outptr += len;
		bytes_written += len;
	}

	return bytes_written;
}

/* TODO: don't even bother with compression
 * just scrap this algorithm and output an uncompressed stream
 * (this is valid so long as the escape sequence is used to escape occurrences of 'key') */

/* This algorithm uses brute-force to find the best compression using the method described above.
 * it is therefore rather slow.
 *
 * as an additional restriction, the length cannot be larger than the offset. the algorithm supports
 * this (sequence would be repeated), but the game does not. also, neither the length nor the offset
 * can be less than 4. */
std::size_t SaveFile::compress(const void *src, void *dest, std::size_t src_len)
{
	unsigned int frequency_table[256];
	std::size_t bytes_written = 0;
	uint8_t *destp, *outptr, key = 0, len = 0, offset = 0;
	const uint8_t *inptr;
	const uint8_t *srcp;
	const uint8_t *endin;

	inptr = (const uint8_t*)src;
	outptr = (uint8_t*)dest;
	srcp = inptr;
	destp = outptr;
	endin = inptr + src_len;

	for(unsigned int i = 0; i < src_len; ++i)
	{
		++frequency_table[srcp[i]];
	}

	for(unsigned int i = 0; i < 256; ++i)
	{
		if(frequency_table[i] < frequency_table[key])
			key = i;
	}

	write_le32(&outptr[0], (uint32_t)src_len);
	write_le32(&outptr[8], key);
	outptr += 12;
	bytes_written += 12;

	while(inptr < endin)
	{
		if(inptr[0] == key)
		{
			outptr[0] = key;
			outptr[1] = key;

			++inptr;
			outptr += 2;
			bytes_written += 2;
			continue;
		}

		offset = 0;
		len = 0;
		for(int i = 4; i < 0xff; ++i)
		{
			if((inptr - i) < srcp)
				continue;

			for(int j = 4; j <= i; ++j)
			{
				if(inptr + j > endin)
					continue;

				int k = j;
				const uint8_t *pos = inptr;
				while(k >= i)
				{
					if(memcmp(pos - i, pos, i) != 0)
					{
						k = -1;
						break;
					}

					pos += i;
					k -= i;
				}

				if(k > 0)
				{
					if(memcmp(pos - i, pos, k) != 0)
						k = -1;
				}

				if(k < 0)
					continue;

				if(j > len)
				{
					offset = i;
					len = j;
				}
			}
		}

		if(len < 4)
			len = 0;

		if(offset >= key)
			++offset;

		if(len > 0)
		{
			outptr[0] = key;
			outptr[1] = offset;
			outptr[2] = len;

			outptr += 3;
			inptr += len;
			bytes_written += 3;
		}
		else
		{
			outptr[0] = inptr[0];

			++outptr;
			++bytes_written;
			++inptr;
		}
	}

	write_le32(&destp[4], (uint32_t)bytes_written);

	return bytes_written;
}

void SaveFile::decrypt_puppet(void *src, const void *rand_data, std::size_t len)
{
	uint8_t *buf = (uint8_t*)src;
	const uint8_t *randbuf = (const uint8_t*)rand_data;

	for(unsigned int i = 0; i < (len / 3); ++i)
	{
		int index = (i * 3) % len;
		uint32_t crypto = read_le32(&randbuf[(i * 4) & 0x3fff]);

		/* need the higher half of this multiplication, right shifted 1 */
		uint32_t temp = (uint64_t(uint64_t(0xAAAAAAABu) * uint64_t(crypto)) >> 33);
		temp *= 3;

		if(crypto - temp == 0)
			buf[index] = ~buf[index];
		buf[index] -= uint8_t(crypto);
	}
}

void SaveFile::encrypt_puppet(void *src, const void *rand_data, std::size_t len)
{
	uint8_t *buf = (uint8_t*)src;
	const uint8_t *randbuf = (const uint8_t*)rand_data;

	for(unsigned int i = 0; i < (len / 3); ++i)
	{
		int index = (i * 3) % len;
		uint32_t crypto = read_le32(&randbuf[(i * 4) & 0x3fff]);

		uint32_t temp = (uint64_t(uint64_t(0xAAAAAAABu) * uint64_t(crypto)) >> 33);
		temp *= 3;

		buf[index] += uint8_t(crypto);
		if(crypto - temp == 0)
			buf[index] = ~buf[index];
	}
}

}
