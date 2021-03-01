#include "Credential.hpp"

namespace BimayInstantVicon {
	const char* Credential::encryptionPassphrase = "bimay";
	const char* Credential::magic = "BimayCred";

	Credential::Credential(std::string txtCred, CredentialFileType type) {
		std::stringstream stream(txtCred);
		this->type = type;
		if (type == CredentialFileType::v1) {
			std::getline(stream, this->username);
			std::getline(stream, this->password);
		}
		else if (type == CredentialFileType::v2) {
			std::getline(stream, this->username);
			std::getline(stream, this->nim);
			std::getline(stream, this->password);
		}
	}

	Credential::Credential() {}

	// Just a simple XOR encryption, with Base64 encoding output.
	/* Encryption for Credentials */
	std::string Credential::encrypt(std::string& in) {
		uint8_t* buf = new uint8_t[in.size()];
		int i = 0;
		for (auto it = in.begin(); it != in.end(); it++) {
			buf[i] = (uint8_t)(*it ^ encryptionPassphrase[i % (sizeof(encryptionPassphrase) - 1)]);
			i++;
		}
		std::string out;
		CryptoPP::ArraySource vs(buf, in.size(), true,
			new CryptoPP::Base64Encoder(
				new CryptoPP::StringSink(out)
			)
		);
		delete[] buf;
		return out;
	}

	/* Decryption for Credentials */
	std::string Credential::decrypt(std::string& in) {
		uint8_t buf[1024];
		CryptoPP::ArraySink as(buf, 1024);
		CryptoPP::StringSource ss(in, true,
			new CryptoPP::Base64Decoder(
				new CryptoPP::Redirector(
					as
				)
			)
		);
		int totalPutLength = as.TotalPutLength();
		for (int i = 0; i < totalPutLength; i++) {
			buf[i] ^= encryptionPassphrase[i % (sizeof(encryptionPassphrase) - 1)];
		}
		return std::string((char*)buf, totalPutLength);
	}

	void Credential::save(std::ostream& credFileStream) {
		credFileStream << magic;
		credFileStream << "v" << 2 << ":"; // Credential version 2

		// Construct data
		std::stringstream stream;
		stream << this->username << std::endl << this->nim << std::endl << this->password << std::endl;
		std::string credStr(stream.str());

		// Write encrypted to file stream
		credFileStream << encrypt(credStr);
	}

	Credential* Credential::parse(std::istream& credFileStream) {
		credFileStream.seekg(0, credFileStream.end);
		int size = (int)credFileStream.tellg();
		credFileStream.seekg(0, credFileStream.beg);
		char* buf = new char[size + 1]; // +1 for null at the back
		credFileStream.read(buf, size);
		buf[size] = 0;                  // Null terminate the string
		int i = 0;

		// Check for magic
		bool hasMagic = true;
		while (buf[i] != 0 && magic[i] != 0) {
			if (buf[i] != magic[i]) {
				hasMagic = false;
				break;
			}
			i++;
		}

		if (!hasMagic) {
			// File doesn't have magic. Read in plain text.
			std::string credPlain(buf);
			Credential* cred = new Credential(credPlain, CredentialFileType::Plain);
			delete[] buf;
			return cred;
		}

		if (buf[i++] != 'v') { // Without v, it's illegal
			delete[] buf;
			return NULL;
		}
		char number[16];
		int numPos = 0;
		while (buf[i] != ':') {
			number[numPos++] = buf[i++];
		}
		number[numPos] = 0;    // Null terminate
		i++;                   // Skip colon

		if (atoi(number) == 1) {      // Version 1
			std::string strbuf(buf + i);
			delete[] buf;
			return new Credential(decrypt(strbuf), CredentialFileType::v1);
		}
		else if (atoi(number) == 2) { // Version 2
			std::string strbuf(buf + i);
			delete[] buf;
			return new Credential(decrypt(strbuf), CredentialFileType::v2);
		}
		else {
			// Illegal credential version for this program version.
			delete[] buf;
			return NULL;
		}
	}
}