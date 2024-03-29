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

static inline std::size_t run_length(const void *block1, const void *block2, std::size_t sz)
{
    auto b1 = (const uint8_t*)block1;
    auto b2 = (const uint8_t*)block2;

    for(auto i = 0u; i < sz; ++i)
    {
        if(b1[i] != b2[i])
            return i;
    }

    return sz;
}

namespace libtpdp
{

/* FIXME: lots of very old, bad code */

SaveFile::SaveFile(SaveFile&& old)
{
    close();

    cryptobuf_ = old.cryptobuf_;
    savebuf_ = std::move(old.savebuf_);
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
    old.savebuf_.reset();
    old.close();
}

SaveFile& SaveFile::operator=(SaveFile&& old)
{
    close();

    cryptobuf_ = old.cryptobuf_;
    savebuf_ = std::move(old.savebuf_);
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
    old.savebuf_.reset();
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
    auto outfile = generate_save(len);

    bool ret = false;
    if(!filename_.empty())
    {
        ret = write_file(filename_, outfile.get(), len);
    }
    else if(!wfilename_.empty())
    {
        ret = write_file(wfilename_, outfile.get(), len);
    }

    return ret;
}

bool SaveFile::save(const std::string& filename)
{
    if(empty())
        return false;

    std::size_t len;
    auto outfile = generate_save(len);

    bool ret = write_file(filename, outfile.get(), len);

    return ret;
}

bool SaveFile::save(const std::wstring& filename)
{
    if(empty())
        return false;

    std::size_t len;
    auto outfile = generate_save(len);

    bool ret = write_file(filename, outfile.get(), len);

    return ret;
}

bool SaveFile::dump_raw_data(const std::wstring& filename, bool decrypt)
{
    if(empty())
        return false;

    if(!decrypt)
        encrypt_all_puppets();
    bool ret = write_file(filename, savebuf_.get(), savebuf_len_);
    if(!decrypt)
        decrypt_all_puppets();

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

    savebuf_ = std::make_unique<char[]>(savebuf_len_);

    if(decompress(packed_buf, savebuf_.get()) != savebuf_len_)
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

std::unique_ptr<char[]> SaveFile::generate_save(std::size_t& size)
{
    /* allocate an oversized buffer in case compression expands the data */
    auto filebuf = std::make_unique<char[]>(savebuf_len_ * 2);
    memcpy(filebuf.get(), cryptobuf_, cryptobuf_len_);
    memcpy(filebuf.get(), is_expansion_ ? save_magic_ynk : save_magic, is_expansion_ ? sizeof(save_magic_ynk) : sizeof(save_magic));

    encrypt_all_puppets();
    seed_ = update_savefile_hash(savebuf_.get(), savebuf_len_);
    write_le32(&filebuf[SAVEFILE_SEED_OFFSET], seed_);
    std::size_t len = compress(savebuf_.get(), &filebuf[SAVEFILE_DATA_OFFSET], savebuf_len_);
    write_le32(&filebuf[SAVEFILE_LENGTH_OFFSET], (uint32_t)len);
    encrypt(&filebuf[SAVEFILE_DATA_OFFSET], seed_, len);
    scramble(&filebuf[SAVEFILE_DATA_OFFSET], cryptobuf_, seed_, (uint32_t)len);
    decrypt_all_puppets();

    size = std::max((len + SAVEFILE_DATA_OFFSET), cryptobuf_len_);
    return filebuf;
}

std::size_t SaveFile::get_puppet_offset(unsigned int index)
{
    if(!(index < (30 * num_boxes_) + 6))
        return (unsigned int)-1;

    if(index < 6)
        return index * PUPPET_SIZE_PARTY;
    else
        return (6 * PUPPET_SIZE_PARTY) + ((index - 6) * PUPPET_SIZE_BOX);
}

void SaveFile::decrypt_all_puppets()
{
    char *puppet = &savebuf_[puppet_offset_];

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
    char *puppet = &savebuf_[puppet_offset_];

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
    savebuf_.reset();

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

    uint32_t hash = read_le32(&savebuf_[offset + PUPPET_SIZE]);
    if(hash == 0)
        return false;

    puppet.read(&savebuf_[offset], index < 6);

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

    memset(&savebuf_[offset], 0, size);

    if(index < 6)
        Puppet().write(&savebuf_[offset], true);
}

void SaveFile::save_puppet(const Puppet& puppet, unsigned int index)
{
    if(empty())
        return;

    std::size_t offset = get_puppet_offset(index);
    if(offset == (std::size_t)-1)
        return;
    offset += puppet_offset_;

    puppet.write(&savebuf_[offset], index < 6);
    update_puppet_hash(&savebuf_[offset]);
}

int SaveFile::get_item_id(unsigned int pocket, unsigned int index)
{
    if(empty())
        return -1;

    if(!(pocket < 7))
        return -1;

    if(index >= pocket_sizes[pocket])
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

    uint8_t *buf = (uint8_t*)&savebuf_[item_num_offset_];

    return buf[item_id];
}

bool SaveFile::set_item_id(unsigned int pocket, unsigned int index, unsigned int item_id)
{
    if(empty())
        return false;

    if(!(pocket < 7))
        return false;

    if(index >= pocket_sizes[pocket])
        return false;

    if(!(item_id < 1024))
        return false;

    std::size_t offset = item_offset_ + pocket_offsets[pocket];

    write_le16(&savebuf_[offset + (index * 2)], (uint16_t)item_id);

    return true;
}

bool SaveFile::set_item_quantity(unsigned int item_id, unsigned int quantity)
{
    if(empty())
        return false;

    if(!(item_id < 1024))
        return false;

    uint8_t *buf = (uint8_t*)&savebuf_[item_num_offset_];

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

uint32_t SaveFile::get_money()
{
    if(empty())
        return 0;

    return read_le32(&savebuf_[SAVEFILE_PLAYER_OFFSET + 0x29]);
}

void SaveFile::set_money(uint32_t val)
{
    if(empty())
        return;

    write_le32(&savebuf_[SAVEFILE_PLAYER_OFFSET + 0x29], val);
}

uint32_t SaveFile::get_playtime()
{
    if(empty())
        return 0;

    return read_le32(&savebuf_[SAVEFILE_PLAYER_OFFSET + 0x2D]);
}

void SaveFile::set_playtime(uint32_t val)
{
    if(empty())
        return;

    write_le32(&savebuf_[SAVEFILE_PLAYER_OFFSET + 0x2D], val);
}

uint8_t SaveFile::get_player_gender()
{
    if(empty())
        return 0;

    return savebuf_[SAVEFILE_PLAYER_OFFSET];
}

void SaveFile::set_player_gender(uint8_t val)
{
    if(empty())
        return;

    savebuf_[SAVEFILE_PLAYER_OFFSET] = val;
}

uint16_t SaveFile::get_fav_puppet()
{
    if(empty())
        return 0;

    return read_le16(&savebuf_[0x4F5]);
}

void SaveFile::set_fav_puppet(uint16_t val)
{
    if(empty())
        return;

    write_le16(&savebuf_[0x4F5], val);
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
    for(std::size_t i = 0; (i + 1) < len; i += 2)
    {
        buf[i] = ~buf[i];
        if((i & 3) == 0)
            buf[i + 1] = ~buf[i + 1];
        buf[i + 1] -= uint8_t(seed);
    }
}

void SaveFile::encrypt(void *src, uint32_t seed, std::size_t len)
{
    uint8_t *buf = (uint8_t*)src;
    for(std::size_t i = 0; (i + 1) < len; i += 2)
    {
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
    uint8_t key, offset, len;
    const uint8_t *endin;

    const uint8_t *inptr = (const uint8_t*)src;
    uint8_t *outptr = (uint8_t*)dest;

    output_size = read_le32(&inptr[0]);
    input_size = read_le32(&inptr[4]) - 12;

    if(dest == NULL)
        return output_size;

    key = inptr[8];

    inptr += 12;
    endin = inptr + input_size; // FIXME: don't do this

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

/* Simple run-length encoding using the scheme described above. */
std::size_t SaveFile::compress(const void *src, void *dest, std::size_t src_len)
{
    unsigned int frequency_table[256] = {};
    std::size_t bytes_written = 0;
    uint8_t key = 0;

    uint8_t *outptr = (uint8_t*)dest;
    uint8_t *destp = outptr;

    const uint8_t *inptr = (const uint8_t*)src;
    const uint8_t *endin = inptr + src_len;
    const uint8_t *srcp = inptr;

    for(unsigned int i = 0; i < src_len; ++i)
    {
        ++frequency_table[srcp[i]];
    }

    for(unsigned int i = 0; i < 256; ++i)
    {
        if(frequency_table[i] < frequency_table[key])
            key = (uint8_t)i;
    }

    write_le32(&outptr[0], (uint32_t)src_len);
    write_le32(&outptr[8], key);
    outptr += 12;
    bytes_written += 12;

    while(inptr < endin)
    {
        uint8_t offset = 0, len = 0;
        auto maxlen = std::min((std::size_t)(endin - inptr), (std::size_t)0xfe);
        auto maxoff = std::min((std::size_t)(inptr - srcp), (std::size_t)0xfe);
        maxlen = std::min(maxoff, maxlen);
        for(auto i = maxoff; i > 3; --i)
        {
            if(len >= i)
                break;

            auto sz = std::min(i, maxlen);
            if(sz < 4)
                continue;

            auto rl = run_length(inptr - i, inptr, sz);
            if(rl > len)
            {
                len = (uint8_t)rl;
                offset = (uint8_t)i;
            }
        }

        if(offset >= key)
            ++offset;

        if(len > 3)
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
            if(inptr[0] == key)
            {
                outptr[0] = key;
                outptr[1] = key;

                outptr += 2;
                bytes_written += 2;
                ++inptr;
            }
            else
            {
                outptr[0] = inptr[0];

                ++outptr;
                ++bytes_written;
                ++inptr;
            }
        }
    }

    write_le32(&destp[4], (uint32_t)bytes_written);

    return bytes_written;
}

}
