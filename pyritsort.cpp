#include <iostream>      // std::cerr
#include <fstream>       // std::ofstream
#include <string>        // std::string
#include <unordered_map> // std::unordered_map

#include <unistd.h>      // open, write
#include <cstring>       // std::strcspn
#include <cstdint>       // uint8_t
#include <fcntl.h>       // O_RDONLY, etc
#include <sys/types.h>   // off_t
#include <sys/mman.h>    // mmap
#include <sys/stat.h>    // fstat

#define get_record_len(record_ptr)    (record_ptr->record_len)
#define get_psk_len(record_ptr)       (record_ptr->record_len - 32 - 1)
#define get_pmk_len()                 (32)
#define get_psk_address(record_ptr)   (record_ptr->data)
#define get_pmk_address(record_ptr)   (record_ptr->data + get_psk_len(record_ptr))
#define get_address_of_next_record(record_ptr) ( (struct record_t*)( ((uint8_t*)record_ptr) + record_ptr->record_len ) ) // struct record_t* get_address_of_next_record(struct record_t* record_ptr)
#define get_address_of_next_word(current_word_ptr, current_word_len) (current_word_ptr + current_word_len + 1) // uint8_t* get_address_of_next_word(uint8_t* current_word, size_t current_word_len)

struct record_t {
	uint8_t record_len;
	uint8_t data[63+32]; // 63+32 is to keep enough space so that the longest possible psk to fit.
	// to access psk and pmk use get_psk_address, get_pmk_address, get_psk_len, and get_pmk_len
	// note that record_ptr += sizeof(struct record_t) to increment is incorrect,
	// since the struct holds extra space which is not necessarily used
	// record_ptr += get_record_len(record_ptr) is correct
};

size_t get_total_count_of_records(struct record_t* first_record, off_t unsortedcwpafile_len);
size_t get_total_count_of_words(uint8_t* first_word, off_t wordfile_len);

int main(int argc, char* argv[])
{
	// check argument count
	if (argc != 4) {
		std::cerr << "error! usage: "
		          << argv[0]
		          << " <wordlist.txt>"
		          << " <unsorted.cwpa>"
		          << " <sorted.cwpa>"
		          << std::endl;
		return 1;
	}

	// open output file
	std::ofstream sortedcwpafile(argv[3], std::ofstream::binary | std::ios::trunc);

	// open, get size of, and mmap input files
	int         wordfile_fd = open(argv[1], O_RDONLY);
	int unsortedcwpafile_fd = open(argv[2], O_RDONLY);
	struct stat unsortedcwpafile_st, wordfile_st;
	fstat(unsortedcwpafile_fd, &unsortedcwpafile_st);
	fstat(        wordfile_fd,         &wordfile_st);
	off_t unsortedcwpafile_len = unsortedcwpafile_st.st_size;
	off_t         wordfile_len =         wordfile_st.st_size;
	void* unsortedcwpafile_original_mmap_addr = mmap(0, unsortedcwpafile_len, PROT_READ, MAP_SHARED, unsortedcwpafile_fd, 0);
	void*         wordfile_original_mmap_addr = mmap(0,         wordfile_len, PROT_READ, MAP_SHARED, wordfile_fd, 0);
	if (unsortedcwpafile_original_mmap_addr == MAP_FAILED || wordfile_original_mmap_addr == MAP_FAILED) {
		std::cerr << "error! failed to mmap files"
		          << std::endl;
		return 1;
	}
	uint8_t* unsortedcwpafile_data_start = (uint8_t*)unsortedcwpafile_original_mmap_addr + 40; // skip cowpatty header + ssid
	uint8_t*         wordfile_data_start = (uint8_t*)        wordfile_original_mmap_addr;

	struct record_t* current_record = (struct record_t*)unsortedcwpafile_data_start;
	uint8_t* current_word = (uint8_t*)wordfile_data_start;
	size_t current_word_len;

	size_t total_count_of_records = get_total_count_of_records(current_record, unsortedcwpafile_len);
	size_t total_count_of_words   = get_total_count_of_words(  current_word,           wordfile_len);



	std::unordered_map<std::string, struct record_t*> hm;

	// add records to hashmap
	for (size_t i = 0; i < total_count_of_records; i++, current_record = get_address_of_next_record(current_record)) {
		hm.insert({
				std::string((char*)get_psk_address(current_record), get_psk_len(current_record)),
				current_record
		});
	}

	sortedcwpafile.write((const char*)unsortedcwpafile_original_mmap_addr, 40);
	// locate and write records in the same order as the wordlist
	for (size_t i = 0; i < total_count_of_words; i++, current_word = get_address_of_next_word(current_word, current_word_len)) {
		current_word_len = std::strcspn((char*)current_word, "\n");
		if (current_word_len < 8 || current_word_len > 63) // skip invalid wifi passwords
			continue;

		try {
			current_record = hm.at( std::string((char*)current_word, current_word_len) );
		} catch (std::out_of_range) {
			std::cerr << "error! word \""
			          << std::string((char*)current_word, current_word_len)
			          << "\" was not found in "
			          << argv[2]
			          << std::endl
			          << "are you sure "
			          << argv[2]
			          << " was generated using "
			          << argv[1]
			          << "?"
			          << std::endl;
			return 1;
		}
		sortedcwpafile.write((const char*)current_record, current_word_len + 32 + 1);
	}



	// cleanup
	if ((munmap(unsortedcwpafile_original_mmap_addr, unsortedcwpafile_len) == -1 ) || (munmap(wordfile_original_mmap_addr, wordfile_len) == -1)) {
		std::cerr << "error! failed to munmap files! continuing..."
		          << std::endl;
	}
	sortedcwpafile.flush();
	sortedcwpafile.close();
	close(unsortedcwpafile_fd);
	close(wordfile_fd);

	std::cerr << "success! "
	          << argv[2]
	          << " was sorted using "
	          << argv[1]
	          << " to create "
	          << argv[3]
	          << std::endl;
	return 0;
}

size_t get_total_count_of_records(struct record_t* first_record, off_t unsortedcwpafile_len) {
	size_t count = 0;
	for (struct record_t* tmp = first_record; (uint8_t*)tmp != (uint8_t*)first_record + unsortedcwpafile_len - 40; tmp = get_address_of_next_record(tmp))
		count++;
	return count;
}
size_t get_total_count_of_words(uint8_t* first_word, off_t wordfile_len) {
	size_t count = 0;
	for (uint8_t* tmp = first_word; tmp != first_word + wordfile_len; tmp++)
		if (*tmp == '\n')
			count++;
	return count;
}
