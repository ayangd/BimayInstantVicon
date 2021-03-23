#include <string>
#include <filesystem>
#include <fstream>
#include <vector>

#include "BimayInterface.hpp"
#include "Credential.hpp"
#include "Utils.hpp"

using namespace BimayInstantVicon;

void checkCredentialChange(bool* changed, const std::string& filename, Credential* credential) {
	if (*changed) {
		try {
			std::ofstream cred;
			cred.exceptions(std::ofstream::badbit);
			cred.open(filename);
			credential->save(cred);
			*changed = false;
			cred.close();
		}
		catch (std::ofstream::failure& f) {
			std::cout << "Credential I/O failure: " << f.what() << std::endl;
		}
	}
}

int main() {
	Curl curl;
	
	// Make credential datas
	// Credentials are saved in "%APPDATA%\BimayInstantVicon\cred.txt"
	std::string appdata = Utils::getHomeDir() + "/";
	std::string dataFolder = appdata + "BimayInstantVicon/";
	std::string credFilename = dataFolder + "cred.txt";

	// Create directory if non-existent
	std::filesystem::create_directory(dataFolder);

	Credential* credential;

	// Check if credential exists
	if (!std::filesystem::exists(credFilename)) {
		// Credential not found
		std::cout << "Local credential not found. Creating one." << std::endl;
		credential = new Credential();
	}
	else {
		// Credential found
		try {
			std::ifstream cred;
			cred.exceptions(std::ifstream::failbit);
			cred.open(credFilename);
			credential = new Credential(cred);
			cred.close();
		}
		catch (CredentialParseException& e) {
			std::cout << "Credential parsing failed: " << e.getReason() << std::endl;
			credential = new Credential();
		}
		catch (std::runtime_error& e) {
			std::cout << "Credential parsing failed: " << e.what() << std::endl;
			credential = new Credential();
		}
		catch (std::ifstream::failure& f) {
			std::cout << "Credential I/O failure: " << f.what() << std::endl;
			credential = new Credential();
		}
	}

	bool credentialChanged = false;

	// Fill credential if empty
	if (credential->username.empty()) {
		std::cout << "Username: ";
		std::getline(std::cin, credential->username);
		credentialChanged = true;
	}
	if (credential->password.empty()) {
		std::cout << "Password (invisible): ";
		credential->password = Utils::promptPassword();
		credentialChanged = true;
	}
	checkCredentialChange(&credentialChanged, credFilename, credential);
	
	MyClassInterface myClassInterface(&curl);

	// Login loop
	while (true) {
		std::cout << "Logging in...";
		try {
			myClassInterface.login(credential->username, credential->password);
		}
		catch (CurlException e) {
			std::cout << "Failed. " << e.getReason() << std::endl;
			std::cout << "Please try again later." << std::endl;
			return -1;
		}
		if (myClassInterface.isLoggedIn()) {
			break;
		}
		std::cout << "Failed. Try again with different username and password." << std::endl;
		std::cout << "Username: ";
		std::getline(std::cin, credential->username);
		std::cout << "Password (invisible): ";
		credential->password = Utils::promptPassword();
	}
	checkCredentialChange(&credentialChanged, credFilename, credential);
	std::cout << "Done." << std::endl;

	std::cout << "Retrieving vicon schedule...";
	std::vector<MyClassInterface::Schedule*>* schedules;
	try {
		schedules = myClassInterface.getSchedules();
	}
	catch (CurlException e) {
		std::cout << "Failed. " << e.getReason() << std::endl;
		std::wcout << "Please try again later." << std::endl;
		return -1;
	}
	std::cout << "Done." << std::endl;
	
	bool found = false;
	Time currentTime;

	// Iterate every schedule and find schedule candidate
	for (auto it = schedules->begin(); it != schedules->end(); it++) {
		MyClassInterface::Schedule& schedule = **it;
		Time* scheduleDate = schedule.getDate();
		Time* scheduleStartTime = schedule.getStartTime();
		if (currentTime.dateMatches(*scheduleDate) &&
			currentTime.isInRangeOf(*scheduleStartTime, 50 * 60, 40 * 60) &&
			schedule.getUrl().compare("-") != 0) {
			std::string instantLink = Utils::getInstantLink(schedule.getUrl());
			std::cout << "Opening \"" << instantLink << "\"...";
			Utils::openurl(instantLink);
			std::cout << "Done." << std::endl;
			
			// Print out class details
			std::cout << "Course Code: " << schedule.getCourseCode() << std::endl;
			std::cout << "Course Name: " << schedule.getCourseTitle() << std::endl;
			std::cout << "Class Code: " << schedule.getClassCode() << std::endl;
			std::cout << "Week: " << schedule.getWeek() << std::endl;
			std::cout << "Session: " << schedule.getSession() << std::endl;
			found = true;
			break;
		}
	}

	if (!found) {
		std::cout << "No class within time range." << std::endl;
	}

	std::cout << "Press any key to exit.";
	Utils::waitKeypress();
	std::cout << std::endl;

	//Curl::globalCleanup();
	return 0;
}