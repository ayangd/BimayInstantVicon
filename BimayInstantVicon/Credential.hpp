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
		static const char* magic;
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
		CredentialParseException(std::string reason);
	};

}

#endif
