#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
	// check argument count
	if (argc != 3) {
		std::cerr << "error! usage: "
		          << argv[0]
		          << " <inputcowpatty.pmk>"
		          << " <outputwindowpane.pmk>"
		          << std::endl;
		return 1;
	}

	// change filenames from shorthand of
	// stdin and stdout to their full form
	std::string  input_filename(argv[1]);
	std::string output_filename(argv[2]);
	if (input_filename == "-")
		input_filename = "/dev/stdin";
	if (output_filename == "-")
		output_filename = "/dev/stdout";

	// open input file now, we need to
	// verify that it is valid and sane
	// before we open (and erase) the
	// output file
	std::ifstream cwpa_file(input_filename, std::ios::binary);

	// verify the file is open and not empty
	if (!cwpa_file.is_open()) {
		std::cerr << "error! could not open supplied cowpatty hashfile"
                  << std::endl;
		return 1;
	}
	std::cout << cwpa_file.gcount();
//	std::

	char buf[32];

	// read the signature and ssid length
	// verify the signature
	cwpa_file.read(buf, 8);
	if (buf[0] !=  'A' || // Atty
		buf[1] !=  'P' || // P
		buf[2] !=  'W' || // W
		buf[3] !=  'C' || // Co
		buf[4] != 0x00 ||
		buf[5] != 0x00 ||
		buf[6] != 0x00 ) {
		std::cerr << "error! supplied cowpatty hashfile "
                  << "does not match cowpatty signature"
                  << std::endl;
		return 1;
	}

	char ssid_len = buf[7];
	if ((ssid_len <= 0) || (ssid_len >= 64)) {
		std::cerr << "error! supplied cowpatty hashfile "
		          << "has an invalid wpa ssid length: "
		          << int(ssid_len)
		          << ")"
		          << std::endl;
		return 1;
	}
	// now starting actual program logic,
	// other tests have passed.
	// opening output file in order to write soon
	std::ofstream wndp_file(output_filename, std::ios::binary | std::ios::trunc);

	// replace the signature
	// write it
	buf[0] = 'P'; // PAne
	buf[1] = 'D'; // doW
	buf[2] = 'N'; // n
	buf[3] = 'W'; // Wi
	wndp_file.write(buf, 4);

	// write the ssid length as only one
	// byte. we don't need to use 4 bytes
	// like cowpatty does because every
	// valid length of an ssid (1..63) can
	// be stored in just a byte (0..255)
	// instead of an int
	wndp_file.write(&ssid_len, 1);

	// read the ssid
	// write the ssid
	cwpa_file.read(buf, 32);
	wndp_file.write(buf, ssid_len);

	// main loop
	// here we read from the cowpatty file,
	// cut out the password and length
	// information, and write just the pmk
	// to the new windowpane file
	int pass_len;
	while (true) {
		// read the first byte of the
		// block, which is the block's
		// length. it is the sum of its own
		// length (constant, 1), the
		// password's length (variable), and
		// the pmk's length (constant, 32)
		cwpa_file.read(buf, 1);

		// calculate the password's length
		// and skip that many bytes (because
		// we are intentionally ignoring
		// those in our format to save space
		// also warn if the password length
		// doesn't make any sense
		pass_len = buf[0] - 1 - 32;
	//	std::cout << pass_len<<std::endl;
		if (pass_len <= 0 || pass_len >= 64) {
			std::cerr << "warning! password length stored in file "
			          << "is not within range of a wpa password ("
			          << int(pass_len)
			          << ")"
			          << std::endl
			          << "continuing anyway..."
			          << std::endl;
		}
		cwpa_file.ignore(pass_len);

		// these two questions need to be answered
		// we validate the data is valid before we
		// write it:
		// 1) did we just try to read data beyond
		//    the end of the file and because of
		//    that the data is invalid and we now
		//    need to discard what we just read?
		// 2) is this the last block? should we
		//    break out of this while (true) loop?

		// now we can read the pmk.
		// however before writing we need to
		// answer question #1.
		// we can do that by checking if the
		// eofbit has been set by any of the
		// read operations up until this point
		cwpa_file.read(buf, 32);
		if (!cwpa_file.eof()) {
			wndp_file.write(buf, 32);
			wndp_file.flush();
		} else {
			break;
		}

		// now this block is over.
		// however before continuing we need to
		// answer question #2.
		// we can do this by .peek()-ing. if
		// the peek is supposed to read beyond
		// the end of the the file, the eofbit
		// is set and then we can test for that
		cwpa_file.peek();
		if (cwpa_file.eof())
			break;
	}

	cwpa_file.close();
	wndp_file.close();
	return 0;
}
