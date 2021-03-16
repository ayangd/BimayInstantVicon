#include "Credential.hpp"

namespace BimayInstantVicon {
	const char* Credential::encryptionPassphrase = "bimay";
	const std::string Credential::magic = "BimayCred";

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
		char buf[1024];

		// Check for magic
		credFileStream.read(buf, magic.size());
		bool hasMagic = magic.compare(0, magic.size(), buf) == 0;

		if (!hasMagic) {
			// File doesn't have magic. Read in plain text.
			credFileStream.seekg(0, std::ios_base::beg);
			std::string credPlain(std::istreambuf_iterator<char>(credFileStream), {});
			parseRaw(credPlain, CredentialFileType::Plain);
			return;
		}

		char vChar;
		credFileStream >> vChar;
		if (vChar != 'v') { // Without v, it's illegal
			throw CredentialParseException("Illegal format: Expected v after magic.");
		}
		int version;
		credFileStream >> version;

		char colon;
		credFileStream >> colon;
		if (colon != ':') {
			throw CredentialParseException("Illegal format: Expected a colon after number version.");
		}

		static constexpr CredentialFileType credVersionArray[] = {
			CredentialFileType::Plain /* Unused */,
			CredentialFileType::v1,
			CredentialFileType::v2
		};
		if (version >= 1 && version <= 2) {
			std::string base64str(std::istreambuf_iterator<char>(credFileStream), {});
			std::string decryptString = decrypt(base64str);
			parseRaw(decryptString, credVersionArray[version]);
		}
		else {
			// Illegal credential version for this program version.
			throw CredentialParseException("Unsupported version.");
		}
	}

	CredentialParseException::CredentialParseException() : Exception() {}

	CredentialParseException::CredentialParseException(std::string reason) : Exception(reason) {}
}