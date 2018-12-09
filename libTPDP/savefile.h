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
#include "puppet.h"
#include <string>

namespace libtpdp
{

constexpr unsigned int SAVEFILE_LENGTH_OFFSET = 0x99A;
constexpr unsigned int SAVEFILE_SEED_OFFSET = 0x99E;
constexpr unsigned int SAVEFILE_DATA_OFFSET = 0x9A2;
constexpr unsigned int SAVEFILE_PUPPET_OFFSET = 0x22200;
constexpr unsigned int SAVEFILE_PUPPET_OFFSET_YNK = (SAVEFILE_PUPPET_OFFSET + 0x80);
constexpr unsigned int SAVEFILE_NUM_BOXES = 30;
constexpr unsigned int SAVEFILE_NUM_BOXES_YNK = 51;
constexpr unsigned int SAVEFILE_ITEM_OFFSET = 0x43951;
constexpr unsigned int SAVEFILE_ITEM_NUM_OFFSET = 0x4354F;
constexpr unsigned int SAVEFILE_ITEM_OFFSET_YNK = 0x5AB1F;
constexpr unsigned int SAVEFILE_ITEM_NUM_OFFSET_YNK = 0x5A71D;
constexpr unsigned int SAVEFILE_BOX_NAME_OFFSET = 0x4318F;
constexpr unsigned int SAVEFILE_BOX_NAME_OFFSET_YNK = 0x5A0BD;

class SaveFile
{
private:
	char *savebuf_;
	const char *cryptobuf_;
	std::size_t cryptobuf_len_, savebuf_len_;
	std::size_t puppet_offset_, item_offset_, item_num_offset_;
	uint32_t seed_, num_boxes_;
	std::string filename_;
	std::wstring wfilename_;
	bool is_expansion_;

	SaveFile(const SaveFile&) = delete;
	SaveFile& operator=(const SaveFile&) = delete;

	uint32_t update_savefile_hash(void *data, std::size_t len);
	void update_puppet_hash(void *data);

	void descramble(void *src, const void *rand_data, uint32_t seed, uint32_t len);
	void scramble(void *src, const void *rand_data, uint32_t seed, uint32_t len);

	void decrypt(void *src, uint32_t seed, std::size_t len);
	void encrypt(void *src, uint32_t seed, std::size_t len);

	std::size_t decompress(const void *src, void *dest);
	std::size_t compress(const void *src, void *dest, std::size_t src_len);

	void decrypt_puppet(void *src, const void *rand_data, std::size_t len);
	void encrypt_puppet(void *src, const void *rand_data, std::size_t len);

	void decrypt_all_puppets();
	void encrypt_all_puppets();

	bool load_savefile(char *buf, std::size_t len);
	char *generate_save(std::size_t& size);

	std::size_t get_puppet_offset(unsigned int index);

public:
	SaveFile() : savebuf_(NULL), cryptobuf_(NULL),
		cryptobuf_len_(0), savebuf_len_(0), num_boxes_(0), is_expansion_(false) {}
	~SaveFile() {close();}

	/* move constructor/assignment */
	SaveFile(SaveFile&& old);
	SaveFile& operator=(SaveFile&& old);

	/* get the raw, decrypted save data */
	inline const char *get_savebuf() const {return savebuf_;}
	inline std::size_t get_savebuf_len() const {return savebuf_len_;}

	/* rand_data is the random data source used for encryption,
	 * the file used for this is gn_dat1/common/efile.bin */
	bool open(const std::string& filename, const void *rand_data, std::size_t rand_len);
	bool open(const std::wstring& filename, const void *rand_data, std::size_t rand_len);

	bool save();								/* save to file (overwiting the original) */
	bool save(const std::string& filename);		/* save to a new file */
	bool save(const std::wstring& filename);

	void close();	/* release all associated resources */

	inline int get_max_puppets() const {return (num_boxes_ * 30) + 6;}
    inline int get_num_boxes() const { return num_boxes_; }
	bool get_puppet(Puppet& puppet, unsigned int index);		/* returns false if no puppet at that index */
	void delete_puppet(unsigned int index);						/* delete from memory */
	void save_puppet(const Puppet& puppet, unsigned int index);	/* save to memory */

	int get_item_id(unsigned int pocket, unsigned int index);	/* get id of the item stored in pocket. returns -1 if pocket/index is out-of-range. */
	int get_item_quantity(unsigned int item_id);				/* get the number of items held by id. returns -1 if id is out-of-range (>= 1024) */

	bool set_item_id(unsigned int pocket, unsigned int index, unsigned int item_id);	/* returns false on range error */
	bool set_item_quantity(unsigned int item_id, unsigned int quantity);

    std::wstring get_box_name(std::size_t index);
    void set_box_name(const std::wstring& name, std::size_t index);

	/* these do conversion to/from Shift-JIS internally */
	std::wstring get_player_name();
	void set_player_name(const std::wstring& name);

	uint32_t get_player_id();
	void set_player_id(uint32_t id);

	uint32_t get_player_secret_id();
	void set_player_secret_id(uint32_t id);

	inline bool is_expansion_file() const {return is_expansion_;}			/* returns true if this is a YnK save file */

	inline bool empty() const {return savebuf_ == NULL;}
};

}
