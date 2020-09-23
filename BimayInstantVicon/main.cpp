#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>
#include <chrono>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <ShlObj.h>
#include <Fileapi.h>
#include <shellapi.h>

using json = nlohmann::json;

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

/* Find function wraps */
/*
std::string findMatch(std::string str, std::regex regex) {
	std::smatch m;
	std::regex_search(str, m, regex);
	return m[1];
}

std::vector<std::string> findMatches(std::string str, std::regex regex) {
	std::string curstr = str;
	std::vector<std::string> vec;
	std::smatch m;
	while (std::regex_search(curstr, m, regex)) {
		for (size_t i = 1; i < m.size(); i++) {
			vec.push_back(m[i]);
		}
		curstr = m.suffix().str();
	}
	return vec;
}
*/

/* BinusMaya find tools */
/*
std::string findLoaderPhpLink() {
	return findMatch(buffer, std::regex("<script.*?src=\"(\\.\\./login/loader\\.php\\?serial=.+?)\">.*?</script>"));
}

std::vector<std::string> findLoginInputs() {
	return findMatches(buffer, std::regex("<input.*?name=\"(.+?)\".*?>"));
}

std::vector<std::string> findHiddenInputs() {
	return findMatches(buffer, std::regex("<input.*?name=\"(.+?)\".*?value=\"(.+?)\".*?>"));
}

std::vector<std::string> findMatchingClasses(std::string& classSchedule, std::string& classMapping) {
	std::string curstr = classSchedule;
	std::vector<std::string> vec;
	std::smatch m;
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	//struct std::tm* parts = std::localtime_s(&now);
	struct std::tm parts = {};
	localtime_s(&parts, &now);
	std::regex scheduleRegex("{.*?\"CRSE_CODE\"\\s*?:\\s*?\"(.+?)\".*?\"START_DT\"\\s*?:\\s*?\"(\\d+)-(\\d+)-(\\d+).*?\".*?\"MEETING_TIME_START\"\\s*?:\\s*?\"(\\d+):(\\d+)\".*?\"MEETING_TIME_END\"\\s*?:\\s*?\"(\\d+):(\\d+)\".*?\"CLASS_SECTION\"\\s*?:\\s*?\"(.+?)\".*?}");
	while (std::regex_match(curstr, m, scheduleRegex)) {
		if (std::stoi(m[2]) == parts.tm_year + 1900 && std::stoi(m[3]) == parts.tm_mon + 1 && std::stoi(m[4]) == parts.tm_mday) {
			if (std::stoi(m[5]) * 60 + std::stoi(m[6]) > parts.tm_hour * 60 + parts.tm_min - 30 && std::stoi(m[7]) * 60 + std::stoi(m[8]) < parts.tm_hour * 60 + parts.tm_min + 40) {
				std::regex mapRegex("{.*?\"STRM\"\\s*?:\\s*?\"(.+?)\".*?\"CLASS_NBR\"\\s*?:\\s*?\"(.+?)\".*?\"CRSE_ID\"\\s*?:\\s*?\"(.+?)\".*?\"CRSE_CODE\"\\s*?:\\s*?\"(.+?)\".*?}");
				std::smatch mm;
				std::string curstrm = classMapping;
				while (std::regex_match(curstrm, mm, mapRegex)) {
					if (mm[4].compare(m[9]) == 0) {
						vec.push_back(mm[4]);
						vec.push_back(mm[3]);
						vec.push_back(mm[1]);
						vec.push_back(mm[2]);
					}
					curstrm = mm.suffix().str();
				}
			}
		}
		curstr = m.suffix().str();
	}
	return vec;
}
*/

/* cURL encode wrapper for C++ std::string */
std::string urlEncode(std::string str) {
	char* cstr = curl_easy_escape(curl, str.c_str(), 0);
	std::string s(cstr);
	curl_free(cstr);
	return s;
}

/* get roaming folder */
std::wstring getRoamingAppdataFolder() {
	wchar_t* dir = NULL;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &dir);
	std::wstring wstr(dir);
	CoTaskMemFree(static_cast<void*>(dir));
	return wstr;
}

/* get time from seconds, starting from Dec 31, 1899 */
std::tm getTimeFromEpoch(int64_t secondsEpoch) {
	time_t date = time_t(secondsEpoch);
	tm t;
	localtime_s(&t, &date);
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

/* Wait for any key press */
void waitKeypress() {
	HANDLE hstdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hstdin, &mode);
	SetConsoleMode(hstdin, mode & ~ENABLE_LINE_INPUT);
	std::cin.get();
	SetConsoleMode(hstdin, mode);
}

/* Prompt password, without character echo */
std::string promptPassword() {
	std::string password;
	HANDLE hstdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hstdin, &mode);
	SetConsoleMode(hstdin, mode & ~ENABLE_ECHO_INPUT);
	std::getline(std::cin, password);
	SetConsoleMode(hstdin, mode);
	std::cout << std::endl;
	return password;
}

int main() {
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_COOKIELIST, "");

	// Make credential datas
	// Credentials are saved in "%APPDATA%\BimayInstantVicon\cred.txt"
	std::wstring appdata = getRoamingAppdataFolder() + LR"(\)";
	std::wstring dataFolder = appdata + LR"(BimayInstantVicon\)";
	std::wstring credFilename = dataFolder + LR"(cred.txt)";
	std::string username, password;

	// Create directory if non-existent
	CreateDirectory(dataFolder.c_str(), NULL);

	// Check if credential exists
	WIN32_FIND_DATA findData;
	if (FindFirstFile(credFilename.c_str(), &findData) == INVALID_HANDLE_VALUE) {
		// Credential not found
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
		cred << username << std::endl << password << std::endl;
		cred.close();
	}
	else {
		// Credential found
		std::ifstream cred(credFilename);
		if (!cred) {
			std::cout << "Can't load credential." << std::endl;
			return -1;
		}
		std::getline(cred, username);
		std::getline(cred, password);
		cred.close();
	}
	
	std::string credData = "Username=" + urlEncode(username) + "&Password=" + urlEncode(password);

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
			std::cout << "Failed." << std::endl << "Invalid credential." << std::endl;
			DeleteFile(credFilename.c_str());
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

	// Iterate every list item from response
	for (json j : scheduleResponse) {
		// Get current time and date
		time_t now = time(NULL);
		tm t;
		localtime_s(&t, &now);

		tm jd = getJSONDate(j["StartDate"].get<std::string>());
		tm jt = getJSONTime(j["StartTime"].get<std::string>());
		std::string meetingUrl = j["MeetingUrl"].get<std::string>();

		// Only match date, time range, and has meeting url
		if (dateMatches(t, jd) && timeInRange(jt, t, -50 * 60, 90 * 60) && meetingUrl.compare("-") != 0) {
			std::cout << "Opening \"" << meetingUrl << "\"...";
			ShellExecuteA(0, 0, meetingUrl.c_str(), 0, 0, SW_SHOW);
			std::cout << "Done." << std::endl;
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

	/* Retrieve from https://binusmaya.binus.ac.id/, but the site is not stable for retrieval.
	curl_easy_setopt(curl, CURLOPT_URL, "https://binusmaya.binus.ac.id/login/");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

	std::cout << "Getting login page...";
	curlCode = curl_easy_perform(curl);
	if (curlCode != CURLE_OK) {
		std::cout << "Failed." << std::endl << curl_easy_strerror(curlCode) << std::endl;
		return -1;
	}
	else {
		// Check response code
		long responseCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		if (responseCode != 200) {
			std::cout << "Failed." << std::endl << "Response code is " << responseCode << "." << std::endl;
			return -1;
		}

		std::cout << "Done." << std::endl;
	}

	std::string loaderPhpLink = findLoaderPhpLink();
	std::vector<std::string> fieldNames = findLoginInputs();
	std::string usernameName = fieldNames[0];
	std::string passwordName = fieldNames[1];
	std::string submitName = fieldNames[2];

	std::ofstream loginPage("login.html");
	loginPage << buffer;
	loginPage.close();

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, ("https://binusmaya.binus.ac.id/login/" + loaderPhpLink).c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

	std::cout << "Getting loader.php page...";
	buffer.clear();
	curlCode = curl_easy_perform(curl);
	if (curlCode != CURLE_OK) {
		std::cout << "Failed." << std::endl << curl_easy_strerror(curlCode) << std::endl;
		return -1;
	}
	else {
		// Check response code
		long responseCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		if (responseCode != 200) {
			std::cout << "Failed." << std::endl << "Response code is " << responseCode << "." << std::endl;
			return -1;
		}

		std::cout << "Done." << std::endl;
	}

	std::ofstream loaderScript("loader.js");
	loaderScript << buffer;
	loaderScript.close();

	std::vector<std::string> hiddenFields = findHiddenInputs();
	if (hiddenFields.size() == 0) {
		std::cout << "hiddenFields is empty." << std::endl;
	}
	std::string hid1name = hiddenFields[0];
	std::string hid1val = hiddenFields[1];
	std::string hid2name = hiddenFields[2];
	std::string hid2val = hiddenFields[3];

	std::ifstream cred("cred.txt");
	if (!cred.is_open()) {
		std::cout << "Can't read credentials." << std::endl;
		return -1;
	}
	std::string username;
	std::string password;
	std::getline(cred, username);
	std::getline(cred, password);
	cred.close();

	buffer.clear();
	buffer =
		urlEncode(usernameName) + "=" + urlEncode(username) + "&" +
		urlEncode(passwordName) + "=" + urlEncode(password) + "&" +
		urlEncode(submitName  ) + "=" + urlEncode("Login" ) + "&" +
		urlEncode(hid1name    ) + "=" + urlEncode(hid1val ) + "&" +
		urlEncode(hid2name    ) + "=" + urlEncode(hid2val );
	//buffer =
	//	usernameName + "=" + username + "&" +
	//	passwordName + "=" + password + "&" +
	//	submitName   + "=" + "Login"  + "&" +
	//	hid1name     + "=" + hid1val  + "&" +
	//	hid2name     + "=" + hid2val;

	std::ofstream form("form.txt");
	form << buffer;
	form.close();

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, "https://binusmaya.binus.ac.id/login/sys_login.php");
	curl_easy_setopt(curl, CURLOPT_REFERER, "https://binusmaya.binus.ac.id/login/");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlVoidWriteCallback);

	std::cout << "Logging in...";
	curlCode = curl_easy_perform(curl);
	if (curlCode != CURLE_OK) {
		std::cout << "Failed." << std::endl << curl_easy_strerror(curlCode) << std::endl;
		return -1;
	}
	else {
		// Check response code
		long responseCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		if (responseCode != 302) {
			std::cout << "Failed." << std::endl << "Response code is " << responseCode << "." << std::endl;
			return -1;
		}

		// Check redirect
		char* redirectUrl = NULL;
		curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirectUrl);
		std::string url(redirectUrl);
		if (url.compare("https://binusmaya.binus.ac.id/block_user.php") == 0) {
			std::cout << "We're in boi!!" << std::endl;
		}
		else {
			std::cout << "Failed." << std::endl << "Wrong username or password" << std::endl;
			std::cout << "Redirected to " << url << std::endl;
			return -1;
		}
	}

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_URL, "https://binusmaya.binus.ac.id/services/ci/index.php/student/class_schedule/classScheduleGetStudentClassSchedule");
	curl_easy_setopt(curl, CURLOPT_REFERER, "https://binusmaya.binus.ac.id/newStudent");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

	std::cout << "Getting class schedules...";
	buffer.clear();
	curlCode = curl_easy_perform(curl);
	if (curlCode != CURLE_OK) {
		std::cout << "Failed." << std::endl << curl_easy_strerror(curlCode) << std::endl;
		return -1;
	}
	else {
		// Check response code
		long responseCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		if (responseCode != 302) {
			std::cout << "Failed." << std::endl << "Response code is " << responseCode << "." << std::endl;
			return -1;
		}

		std::cout << "Done." << std::endl;
	}
	std::string classSchedules = buffer;

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_URL, "https://binusmaya.binus.ac.id/services/ci/index.php/student/init/getStudentCourseMenuCourses");
	curl_easy_setopt(curl, CURLOPT_REFERER, "https://binusmaya.binus.ac.id/newStudent");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

	std::cout << "Getting class mapping...";
	buffer.clear();
	curlCode = curl_easy_perform(curl);
	if (curlCode != CURLE_OK) {
		std::cout << "Failed." << std::endl << curl_easy_strerror(curlCode) << std::endl;
		return -1;
	}
	else {
		// Check response code
		long responseCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		if (responseCode != 302) {
			std::cout << "Failed." << std::endl << "Response code is " << responseCode << "." << std::endl;
			return -1;
		}

		std::cout << "Done." << std::endl;
	}
	std::string classMapping = buffer;

	std::vector<std::string> result = findMatchingClasses(classSchedules, classMapping);
	std::cout << "Result: " << result.size() << std::endl;
	*/

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}