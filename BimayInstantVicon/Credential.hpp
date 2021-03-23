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

#ifndef _BIMAY_INSTANT_VICON_CREDENTIAL_HPP
#define _BIMAY_INSTANT_VICON_CREDENTIAL_HPP

#include <string>
#include <sstream>
#include <memory>
#include <base64.h>

#include "Utils.hpp"

namespace BimayInstantVicon {
	enum class CredentialFileType {
		Plain, v1, v2
	};

	class Credential
	{
	private:
		static std::string encrypt(std::string& in);
		static std::string decrypt(std::string& in);
		static const char* encryptionPassphrase;
		static const std::string magic;
		void parseRaw(std::string& in, CredentialFileType type);
	public:
		std::string username;
		std::string nim;
		std::string password;
		CredentialFileType type;
		Credential(std::string& txtCred, CredentialFileType type);
		Credential(std::istream& credFileStream);
		Credential();
		void save(std::ostream& credFileStream);
	};

	class CredentialParseException : public Exception {
	public:
		CredentialParseException();
		CredentialParseException(const std::string& reason);
	};

}

#endif
