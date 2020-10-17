#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <vector>
#include <chrono>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <base64.h>
#include <filesystem>
#include <codecvt>
#include <filters.h>
#include <hmac.h>
#include <sha.h>
#include <hex.h>
#include <pwdbased.h>
#include <modes.h>
#include <aes.h>

#include "main.hpp"

using json = nlohmann::json;

/* Encrypting password */
const char* encrPass = "bimay";

/* Credential file enum */
enum class CredentialFileType {
	Plain, v1
};
/* Credential Class */
class Credential {
public:
	std::string username;
	std::string password;
	CredentialFileType type;
	Credential(std::string txtCred, CredentialFileType type) {
		std::stringstream stream(txtCred);
		std::getline(stream, this->username);
		std::getline(stream, this->password);
		this->type = type;
	}
};

/* Credential file variables */
const char* credMagic = "BimayCred";

/* cURL global variables for global request access */
// cURL handler
CURL* curl;
// cURL error code
CURLcode curlCode;

/* cURL buffer and callback providers */
// cURL buffer. Clean when receiving new data.
std::string buffer;
// cURL write callback for curl_easy_setopt CURLOPT_WRITEFUNCTION
size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	size_t total = size * nmemb;
	buffer.append(ptr, total);
	return total;
}
// cURL read callback for curl_easy_setopt CURLOPT_READFUNCTION
size_t curlVoidWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	return size * nmemb;
}

/* cURL encode wrapper for C++ std::string */
std::string urlEncode(std::string str) {
	char* cstr = curl_easy_escape(curl, str.c_str(), 0);
	std::string s(cstr);
	curl_free(cstr);
	return s;
}

/* get time from seconds, starting from Dec 31, 1899 */
std::tm getTimeFromEpoch(int64_t secondsEpoch) {
	time_t date = time_t(secondsEpoch);
	tm t;
#ifdef _MSC_VER
	localtime_s(&t, &date);
#else
	tm* tp = localtime(&date);
	memcpy(tp, &t, sizeof(tm));
#endif
	return t;
}

/* Handle /Date(secondEpoch)/ string in JSON */
std::tm getJSONDate(std::string date) {
	std::smatch sm;
	std::regex_search(date, sm, std::regex("\\d+"));
	return getTimeFromEpoch(std::stoll(sm[0]) / 1000);
}

/* Handle HH:mm:ss string in JSON */
std::tm getJSONTime(std::string time) {
	std::smatch sm;
	std::regex_search(time, sm, std::regex("(\\d{2}):(\\d{2}):(\\d{2})"));
	struct tm t = { std::stoi(sm[3]), std::stoi(sm[2]), std::stoi(sm[1]) };
	return t;
}

/* Date matching helper */
bool dateMatches(std::tm tm1, std::tm tm2) {
	return tm1.tm_year == tm2.tm_year && tm1.tm_mon == tm2.tm_mon && tm1.tm_mday == tm2.tm_mday;
}

/* Time range mathing helper */
bool timeInRange(std::tm t, std::tm tRanged, int secondOffset, int secondRange) {
	int sec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
	int secRanged = tRanged.tm_hour * 3600 + tRanged.tm_min * 60 + tRanged.tm_sec;
	return sec > secRanged + secondOffset && sec < secRanged + secondOffset + secondRange;
}

// Just a simple XOR encryption, with Base64 encoding output.
/* Encryption for Credentials */
std::string encrypt(std::string& in) {
	std::vector<byte> buf(in.begin(), in.end());
	int i = 0;
	for (auto it = buf.begin(); it != buf.end(); it++) {
		*it ^= encrPass[i % (sizeof(encrPass) - 1)];
		i++;
	}
	std::string out;
	CryptoPP::VectorSource vs(buf, true,
		new CryptoPP::Base64Encoder(
			new CryptoPP::StringSink(out)
		)
	);
	return out;
}

/* Decryption for Credentials */
std::string decrypt(std::string& in) {
	std::vector<byte> buf;
	CryptoPP::StringSource ss(in, true,
		new CryptoPP::Base64Decoder(
			new CryptoPP::VectorSink(buf)
		)
	);
	int i = 0;
	for (auto it = buf.begin(); it != buf.end(); it++) {
		*it ^= encrPass[i % (sizeof(encrPass) - 1)];
		i++;
	}
	return std::string(buf.begin(), buf.end());
}

/* Parse the content of the credential file */
Credential* parseCredential(std::istream& credFileStream) {
	credFileStream.seekg(0, credFileStream.end);
	int size = (int) credFileStream.tellg();
	credFileStream.seekg(0, credFileStream.beg);
	char* buf = new char[size + 1]; // +1 for null at the back
	credFileStream.read(buf, size);
	buf[size] = 0;                  // Null terminate the string
	int i = 0;

	// Check for magic
	bool hasMagic = true;
	while (buf[i] != 0 && credMagic[i] != 0) {
		if (buf[i] != credMagic[i]) {
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

	if (atoi(number) == 1) { // Version 1
		std::string strbuf(buf + i);
		delete[] buf;
		return new Credential(decrypt(strbuf), CredentialFileType::v1);
	}
	else {
		// Illegal credential version for this program version.
		delete[] buf;
		return NULL;
	}
}

/* Save credential to file */
void saveCredential(std::ostream& credFileStream, Credential& credential) {
	credFileStream << credMagic;
	credFileStream << "v" << 1 << ":"; // Credential version 1

	// Construct data
	std::stringstream stream;
	stream << credential.username << std::endl << credential.password << std::endl;
	std::string credStr(stream.str());

	// Write encrypted to file stream
	credFileStream << encrypt(credStr);
}

int main() {
	// Testing
	byte passwordsalt[CryptoPP::HMAC<CryptoPP::SHA1>::DIGESTSIZE];
	std::string username, password;
	std::cin >> username;
	std::cin >> password;
	CryptoPP::HMAC<CryptoPP::SHA1> hmac((byte*)username.c_str(), username.length());
	CryptoPP::StringSource ss(username, true,
		new CryptoPP::HashFilter(hmac,
			new CryptoPP::ArraySink(
				passwordsalt, CryptoPP::HMAC<CryptoPP::SHA1>::DIGESTSIZE
			)
		)
	);
	
	byte rfcbytes[48];
	CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA1> pbkdf2;
	pbkdf2.DeriveKey(rfcbytes, sizeof(rfcbytes), 0, (byte*)username.c_str(), username.length(), (byte*)passwordsalt, sizeof(passwordsalt), 10);

	CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption cfbEncryptor(rfcbytes, 32, rfcbytes + 32);

	std::vector<byte> cipher;
	std::vector<byte> unicodePassword;
	for (auto it = password.begin(); it != password.end(); it++) {
		unicodePassword.push_back(*it);
		unicodePassword.push_back(0);
	}
	CryptoPP::VectorSource ss2(unicodePassword, true,
		new CryptoPP::StreamTransformationFilter(cfbEncryptor,
			new CryptoPP::VectorSink(cipher)
		)
	);

	std::cout << "Output size: " << cipher.size() << std::endl;
	for (auto it = cipher.begin(); it != cipher.end(); it++) {
		std::cout << (int)*it << " ";
	}
	std::cout << std::endl;

	return 0;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_COOKIELIST, "");
	
	// Make credential datas
	// Credentials are saved in "%APPDATA%\BimayInstantVicon\cred.txt"
	std::string appdata = getHomeDir() + "/";
	std::string dataFolder = appdata + "BimayInstantVicon/";
	std::string credFilename = dataFolder + "cred.txt";
	Credential* credential = NULL;

	// Create directory if non-existent
	std::filesystem::create_directory(dataFolder);

	// Check if credential exists
	if (!std::filesystem::exists(credFilename)) {
		// Credential not found
		std::string username, password;
		std::cout << "No credential found. Please manually make one." << std::endl;
		std::cout << "Username: ";
		std::getline(std::cin, username);
		std::cout << "Password: ";
		password = promptPassword();
		std::ofstream cred(credFilename);
		if (!cred) {
			std::cout << "Can't save credential." << std::endl;
			return -1;
		}
		std::stringstream credStream;
		credStream << username << std::endl << password << std::endl;
		credential = new Credential(credStream.str(), CredentialFileType::Plain);
		saveCredential(cred, *credential);
		cred.close();
	}
	else {
		// Credential found
		std::ifstream cred(credFilename);
		if (!cred) {
			std::cout << "Can't load credential." << std::endl;
			return -1;
		}
		credential = parseCredential(cred);
		if (credential == NULL) {
			std::cout << "Credential format is not recognizable. Deleting one." << std::endl;
			std::filesystem::remove(credFilename);
			return -1;
		}
		cred.close();
	}
	
	std::string credData = "Username=" + urlEncode(credential->username) + "&Password=" + urlEncode(credential->password);

	curl_easy_setopt(curl, CURLOPT_URL, "https://myclass.apps.binus.ac.id/Auth/Login");
	curl_easy_setopt(curl, CURLOPT_REFERER, "https://myclass.apps.binus.ac.id/Auth"); // Prevent unauthorized access
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, credData.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
	buffer.clear();

	json loginResponse;
	std::cout << "Logging in...";
	curlCode = curl_easy_perform(curl);
	if (curlCode != CURLE_OK) {
		std::cout << "Failed." << std::endl << curl_easy_strerror(curlCode) << std::endl;
		return -1;
	}
	else {
		long responseCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		if (responseCode != 200) {
			std::cout << "Failed." << std::endl << "Response code is " << responseCode << "." << std::endl;
			return -1;
		}

		loginResponse = json::parse(buffer);
		if (!loginResponse["Status"]) {
			std::cout << "Failed." << std::endl << "Invalid credential. Deleting one." << std::endl;
			std::filesystem::remove(credFilename);
			return -1;
		}
	}
	std::cout << "Done." << std::endl;

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_URL, "https://myclass.apps.binus.ac.id/Home/GetViconSchedule");
	curl_easy_setopt(curl, CURLOPT_REFERER, "https://myclass.apps.binus.ac.id/Home/Index"); // Prevent unauthorized access
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
	buffer.clear();

	std::cout << "Retrieving vicon schedule...";
	curlCode = curl_easy_perform(curl);
	if (curlCode != CURLE_OK) {
		std::cout << "Failed." << std::endl << curl_easy_strerror(curlCode) << std::endl;
		return -1;
	}
	else {
		long responseCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		if (responseCode != 200) {
			std::cout << "Failed." << std::endl << "Response code is " << responseCode << "." << std::endl;
			return -1;
		}
	}
	std::cout << "Done." << std::endl;
	json scheduleResponse = json::parse(buffer);
	
	bool found = false;

	// Get current time and date
	time_t now = time(NULL);
	tm t;
#ifdef _MSC_VER
	localtime_s(&t, &now);
#else
	tm* tp = localtime(&now);
	memcpy(tp, &t, sizeof(tm));
#endif

	// Iterate every list item from response
	for (json j : scheduleResponse) {
		tm jd = getJSONDate(j["StartDate"].get<std::string>());
		tm jt = getJSONTime(j["StartTime"].get<std::string>());
		std::string meetingUrl = j["MeetingUrl"].get<std::string>();

		// Only match date, time range, and has meeting url
		if (dateMatches(t, jd) && timeInRange(jt, t, -50 * 60, 90 * 60) && meetingUrl.compare("-") != 0) {
			std::cout << "Opening \"" << meetingUrl << "\"...";
			openurl(meetingUrl);
			std::cout << "Done." << std::endl;
			if (std::regex_match(j["SsrComponentDescription"].get<std::string>(), std::regex(".*Laboratory"))) {
				std::cout << "It's a lab class! Opening \"https://laboratory.binus.ac.id/lab\"...";
				openurl("https://laboratory.binus.ac.id/lab");
				std::cout << "Done." << std::endl;
			}
			std::cout << "Class info: " <<
				j["CourseCode"] << " " <<
				j["CourseTitleEn"] << " " <<
				j["ClassCode"] << " " <<
				"Week " << j["WeekSession"] << " " <<
				"Session " << j["CourseSessionNumber"] <<
				std::endl;
			found = true;
			break;
		}
	}

	if (!found) {
		std::cout << "No class within time range." << std::endl;
	}

	std::cout << "Press any key to exit.";
	waitKeypress();
	std::cout << std::endl;

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}