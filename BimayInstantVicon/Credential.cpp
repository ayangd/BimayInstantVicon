#include "Credential.hpp"

namespace BimayInstantVicon {
	const char* Credential::encryptionPassphrase = "bimay";
	const char* Credential::magic = "BimayCred";

	Credential::Credential(std::string& txtCred, CredentialFileType type) {
		parseRaw(txtCred, type);
	}

	Credential::Credential() {}

	// Just a simple XOR encryption, with Base64 encoding output.
	/* Encryption for Credentials */
	std::string Credential::encrypt(std::string& in) {
		std::string encrypted;
		int i = 0;
		for (auto c = in.begin(); c != in.end(); c++, i++) {
			encrypted.push_back(*c ^ encryptionPassphrase[i % (sizeof(encryptionPassphrase) - 1)]);
		}
		return base64_encode_mime(encrypted);
	}

	/* Decryption for Credentials */
	std::string Credential::decrypt(std::string& in) {
		std::string decoded = base64_decode(in, true);
		std::string unencrypted;
		int i = 0;
		for (auto c = decoded.begin(); c != decoded.end(); c++, i++) {
			unencrypted.push_back(*c ^ encryptionPassphrase[i % (sizeof(encryptionPassphrase) - 1)]);
		}
		return unencrypted;
	}

	void Credential::parseRaw(std::string& in, CredentialFileType type) {
		std::stringstream stream(in);
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

	Credential::Credential(std::istream& credFileStream) {
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
			delete[] buf;
			parseRaw(credPlain, CredentialFileType::Plain);
			return;
		}

		if (buf[i++] != 'v') { // Without v, it's illegal
			delete[] buf;
			throw CredentialParseException("Unknown Credential version.");
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
			std::string decryptString = decrypt(strbuf);
			parseRaw(decryptString, CredentialFileType::v1);
			return;
		}
		else if (atoi(number) == 2) { // Version 2
			std::string strbuf(buf + i);
			delete[] buf;
			std::string decryptString = decrypt(strbuf);
			parseRaw(decryptString, CredentialFileType::v2);
		}
		else {
			// Illegal credential version for this program version.
			delete[] buf;
			throw CredentialParseException("Unsupported Credential version.");
		}
	}

	CredentialParseException::CredentialParseException() : Exception() {}

	CredentialParseException::CredentialParseException(std::string reason) : Exception(reason) {}
}