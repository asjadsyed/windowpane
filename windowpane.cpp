#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <cstring> // strlen()
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>


const constexpr static   size_t  pmk_len = 32;
const constexpr static   size_t  header_len = 4;
const constexpr static   size_t  ptk_buf_len = 80;//CryptoPP::HMAC<CryptoPP::SHA1>::DIGESTSIZE * 7;
const constexpr static   size_t  kck_len = 16;
const constexpr static   size_t  mic_len = 16;
const constexpr static  uint8_t* application_specific_string = (const uint8_t*)"Pairwise key expansion";
const constexpr static   size_t  nonce_len = 32;
const constexpr static   size_t  mac_len = 6;
const constexpr static  uint8_t  null_byte = 0;
const constexpr static uint32_t  tkip_keyver = 1;
const constexpr static uint32_t  ccmp_keyver = 2;


bool verify_header_magic(char* header) {
	return (header[0] == 'P' && header[1] == 'D' && header[2] == 'N' && header[3] == 'W');
}
bool verify_ssid_len(char ssid_len) {
	return ((ssid_len < 1) && (ssid_len > 64));
}
// hccap format: see https://hashcat.net/wiki/doku.php?id=hccap
struct hccap_t {
	 int8_t essid[36];
	uint8_t mac1[mac_len];
	uint8_t mac2[mac_len];
	uint8_t nonce1[nonce_len];
	uint8_t nonce2[nonce_len];
	uint8_t eapol[256];
	int32_t eapol_size;
	int32_t keyver;
	uint8_t keymic[16];
};

int main (int argc, char* argv[])
{
	if (argc != 4) {
		std::cerr << "error! usage: "
		          << argv[0]
		          << " <wordlist.txt>"
		          << " <hashes.wndp>"
		          << " <handshake.hccap>"
		          << std::endl;
		return 1;
	}

	std::ifstream word_file(argv[1]);
	std::ifstream wndp_file(argv[2], std::ios::binary);
	std::ifstream hccap_file(argv[3], std::ios::binary);

	if (! word_file) { std::cerr << "error! could not open file \"" << argv[1] << "\"" << std::endl; return 1; }
	if (! wndp_file) { std::cerr << "error! could not open file \"" << argv[2] << "\"" << std::endl; return 1; }
	if (!hccap_file) { std::cerr << "error! could not open file \"" << argv[3] << "\"" << std::endl; return 1; }

	struct hccap_t hccap_data;
	hccap_file.read((char*)&hccap_data, sizeof(hccap_data));

	char wndp_file_buf[pmk_len];
	wndp_file.read(wndp_file_buf, 5);
	if (!verify_header_magic(wndp_file_buf)) {
		std::cerr << "error! supplied windowpane hashfile "
		          << "does not match windowpane signature"
		          << std::endl;
		return 1;
	}
	char hccap_ssid_len = strlen((const char*)hccap_data.essid),
	      wndp_ssid_len = wndp_file_buf[4];
	if (verify_ssid_len(hccap_ssid_len))
		{ std::cerr << "error! supplied hccap file has an invalid ssid length (" << int(hccap_ssid_len) << ")" << std::endl; return 1; }
	if (verify_ssid_len(wndp_ssid_len))
		{ std::cerr << "error! supplied wndp file has an invalid ssid length (" << int(wndp_ssid_len) << ")" << std::endl; return 1; }
	if (hccap_ssid_len != wndp_ssid_len)
		{ std::cerr << "error! there is a discrepancy between ssid lengths (" << int(hccap_ssid_len) << " vs " << int(wndp_ssid_len) << ")" << std::endl; return 1; }

	wndp_file.read(wndp_file_buf, wndp_ssid_len);
	auto ssid = std::string(wndp_file_buf, wndp_ssid_len);
	if (std::strncmp((const char*)hccap_data.essid, wndp_file_buf, wndp_ssid_len) != 0)
		{ std::cerr << "error! there is a discrepancy between ssids (" << (const char*)hccap_data.essid << " vs " << ssid << ")" << std::endl; return 1; }
	// implement if (file size = header + 32*n) { error } one day


	CryptoPP::HMAC<CryptoPP::SHA1> hmac;
	uint8_t ptk[ptk_buf_len];
	uint8_t mic[CryptoPP::HMAC<CryptoPP::SHA1>::DIGESTSIZE];
	// compare macs and nonces first because we will be using that comparison
	// a lot within the loop
	bool   mac1_is_smaller = std::lexicographical_compare(hccap_data.mac1,   hccap_data.mac1 + 6,    hccap_data.mac2,   hccap_data.mac2 + 6);
	bool nonce1_is_smaller = std::lexicographical_compare(hccap_data.nonce1, hccap_data.nonce1 + 32, hccap_data.nonce2, hccap_data.nonce2 + 32);

	std::string next_guess;
	bool found_it = false, mics_match = false;
	// main loop!
	while (!word_file.eof()) {
		// read wordlist before reading hashfile so that if this word
		// doesn't match the length restrictions, we know it won't be
		// in the hashfile (cowpatty or pyrit or whoever would've
		// skipped it too)
		std::getline(word_file, next_guess, '\n');
		if ((next_guess.length() < 8) || (next_guess.length() > 63))
			continue;
		wndp_file.read(wndp_file_buf, pmk_len);

		// put in nonces and mac addresses into an HMAC with the PMK as the key
		// this will yield the PTK
		hmac.SetKey((uint8_t*)wndp_file_buf, pmk_len);
		for (byte j = 0; j < 4/*( (hccap_data.keyver == ccmp_keyver) ? 3 : 4)*/; j++) {
			// hmac.Restart(); // has no effect because .Final(...) was just called on it
			hmac.Update(application_specific_string, 22); // 22 == strlen("Pairwise key expansion")
			hmac.Update(&null_byte, 1);
			if (mac1_is_smaller) {
				hmac.Update(hccap_data.mac1, mac_len);
				hmac.Update(hccap_data.mac2, mac_len);
			} else {
				hmac.Update(hccap_data.mac2, mac_len);
				hmac.Update(hccap_data.mac1, mac_len);
			}
			if (nonce1_is_smaller) {
				hmac.Update(hccap_data.nonce1, nonce_len);
				hmac.Update(hccap_data.nonce2, nonce_len);
			} else {
				hmac.Update(hccap_data.nonce2, nonce_len);
				hmac.Update(hccap_data.nonce1, nonce_len);
			}
			hmac.Update(&j, 1);
			hmac.Final(ptk + (j * 20));//16 for md5 20 for sha1
		}

		// then from the PTK derive the mic, do this by putting in the eapolframe
		// into a HMAC with the KCK (bytes 0 through 16 of the PTK) as the key

		// hmac.Restart(); // has no effect because .Final(...) was just called on it
		hmac.SetKey(ptk + 0, kck_len);
		hmac.Update(hccap_data.eapol, hccap_data.eapol_size);
		hmac.Final(mic);

		// 	use c++14 later for std::equal
		mics_match = std::equal(hccap_data.keymic, hccap_data.keymic + mic_len, mic);
		if (mics_match) {
			found_it = true;
			break;
		}
		wndp_file.peek();
	}

	if (found_it) {
		std::cout << "success! passphrase is:" << std::endl << "[ " << next_guess << " ]" << std::endl;
	} else {
		std::cout << "error! pmk was not found in your hashfile/dictionary" << std::endl;
	}

	return 0;
}
