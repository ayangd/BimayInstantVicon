/*
BimayInstantVicon - Instantly open ongoing video conference
Copyright (C) 2021 Michael Dlone

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Credential.hpp"

namespace BimayInstantVicon {
	const char* Credential::encryptionPassphrase = "bimay";
	const std::string Credential::magic = "BimayCred";

	Credential::Credential(std::string& txtCred, CredentialFileType type) {
		parseRaw(txtCred, type);
	}

	Credential::Credential() {
		type = CredentialFileType::Plain; // Not really important, just avoiding warnings
	}

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
		std::string magicCheck(magic.size(), ' ');

		// Check for magic
		credFileStream.read(&magicCheck[0], magic.size());
		bool hasMagic = magic.compare(magicCheck) == 0;

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

	CredentialParseException::CredentialParseException(const std::string& reason) : Exception(reason) {}
}