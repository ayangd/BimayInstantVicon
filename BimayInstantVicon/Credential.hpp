#ifndef _BIMAY_INSTANT_VICON_CREDENTIAL_HPP
#define _BIMAY_INSTANT_VICON_CREDENTIAL_HPP

#include <string>
#include <sstream>
#include <base64.h>
#include <filters.h>

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
	public:
		std::string username;
		std::string nim;
		std::string password;
		CredentialFileType type;
		Credential(std::string txtCred, CredentialFileType type);
		Credential();
		void save(std::ostream& credFileStream);

		static Credential* parse(std::istream& credFileStream);
	};

}

#endif
