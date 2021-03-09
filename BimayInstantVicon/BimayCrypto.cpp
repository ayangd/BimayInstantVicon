#include "BimayCrypto.hpp"

#include <crypto++/hmac.h>
#include <crypto++/sha.h>
#include <crypto++/filters.h>
#include <crypto++/pwdbased.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>
#include <crypto++/base64.h>

namespace BimayInstantVicon {
	std::string BimayCrypto::labPasswordEncrypt(std::string& username, std::string& password) {
		uint8_t passwordsalt[CryptoPP::HMAC<CryptoPP::SHA1>::DIGESTSIZE];
		CryptoPP::HMAC<CryptoPP::SHA1> hmac((uint8_t*)username.c_str(), username.length());
		CryptoPP::StringSource ss(username, true,
			new CryptoPP::HashFilter(hmac,
				new CryptoPP::ArraySink(
					passwordsalt, CryptoPP::HMAC<CryptoPP::SHA1>::DIGESTSIZE
				)
			)
		);

		uint8_t rfcbytes[48];
		CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA1> pbkdf2;
		pbkdf2.DeriveKey(rfcbytes, sizeof(rfcbytes), 0, (uint8_t*)username.c_str(), username.length(), (uint8_t*)passwordsalt, sizeof(passwordsalt), 10);

		CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption cfbEncryptor(rfcbytes, 32, rfcbytes + 32);

		std::string base64output;
		uint8_t* unicodePassword = new uint8_t[password.size() * 2];
		int i = 0;
		for (auto it = password.begin(); it != password.end(); it++) {
			unicodePassword[i++] = *it;
			unicodePassword[i++] = (uint8_t)0;
		}
		CryptoPP::ArraySource ss2(unicodePassword, password.size() * 2, true,
			new CryptoPP::StreamTransformationFilter(cfbEncryptor,
				new CryptoPP::Base64Encoder(
					new CryptoPP::StringSink(
						base64output
					)
				)
			)
		);
		base64output.pop_back();
		return base64output;
	}
}