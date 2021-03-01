#ifndef _BIMAY_INSTANT_VICON_BIMAYCRYPTO_HPP
#define _BIMAY_INSTANT_VICON_BIMAYCRYPTO_HPP

#include <string>

namespace BimayInstantVicon {

	class BimayCrypto
	{
		static std::string labPasswordEncrypt(std::string& username, std::string& password);
	};

}

#endif
