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

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>

#include "BimayInterface.hpp"
#include "Credential.hpp"
#include "Utils.hpp"

using namespace BimayInstantVicon;

/// <summary>
/// Saves the credential data
/// </summary>
/// <param name="filename">The file name to save the data</param>
/// <param name="credential">The data that are being saved</param>
void saveCredential(const std::string& filename, Credential* credential) {
	try {
		std::ofstream cred;
		cred.exceptions(std::ofstream::badbit);
		cred.open(filename);
		credential->save(cred);
		cred.close();
	}
	catch (std::ofstream::failure& f) {
		std::cout << "Credential I/O failure: " << f.what() << std::endl;
	}
}

Credential* loadCredential(const std::string& path) {
	if (!std::filesystem::exists(path)) {
		std::cout << "Local credential not found. Creating one." << std::endl;
		return new Credential();
	}

	try {
		std::ifstream cred;
		cred.exceptions(std::ifstream::failbit);
		cred.open(path);
		return new Credential(cred);
		cred.close();
	}
	catch (CredentialParseException& e) {
		std::cout << "Credential parsing failed: " << e.getReason() << std::endl;
		return new Credential();
	}
	catch (std::runtime_error& e) {
		std::cout << "Credential parsing failed: " << e.what() << std::endl;
		return new Credential();
	}
	catch (std::ifstream::failure& f) {
		std::cout << "Credential I/O failure: " << f.what() << std::endl;
		return new Credential();
	}
}

void promptCredential(Credential* credential) {
	std::cout << "Username: ";
	std::getline(std::cin, credential->username);
	std::cout << "Password (invisible): ";
	credential->password = Utils::promptPassword();
}

/// <summary>
/// Main entry of the program
/// </summary>
/// <returns>0 if success. Other than that, an error occured.</returns>
int main() {
	Curl curl;
	
	// Initialize path variables
	// Credentials are saved in "%APPDATA%\BimayInstantVicon\cred.txt"
	std::string appdata = Utils::getHomeDir() + "/";
	std::string dataFolder = appdata + "BimayInstantVicon/";
	std::string credFilename = dataFolder + "cred.txt";

	// Create directory if non-existent
	std::filesystem::create_directory(dataFolder);

	Credential* credential = loadCredential(credFilename);

	if (credential->username.empty()) {
		promptCredential(credential);
		saveCredential(credFilename, credential);
	}
	
	MyClassInterface myClassInterface(&curl);

	// Login loop
	constexpr static int maxRetry = 5; // constant at compile time
	int retry = maxRetry;
	while (true) {
		std::cout << "Logging in...";
		try {
			myClassInterface.login(credential->username, credential->password);
		}
		catch (CurlException e) {
			std::cout << "Failed. " << e.getReason() << std::endl;
			if (retry > 0) {
				std::cout << "Retrying (" << maxRetry - retry + 1 << "/" << maxRetry << ")";
				retry--;
				continue;
			}
			// Max retry reached
			std::cout << "Please try again later." << std::endl;
			return -1;
		}
		if (myClassInterface.isLoggedIn()) {
			break;
		}
		retry = maxRetry; // Reset retry when login failed
		std::cout << "Failed. Try again with different username and password." << std::endl;
		promptCredential(credential);
		saveCredential(credFilename, credential);
	}
	std::cout << "Done." << std::endl;

	std::cout << "Retrieving vicon schedule...";
	std::vector<MyClassInterface::Schedule*>* schedules;

	retry = maxRetry;
	while (true) {
		try {
			schedules = myClassInterface.getSchedules();
		}
		catch (CurlException e) {
			std::cout << "Failed. " << e.getReason() << std::endl;
			if (retry > 0) {
				std::cout << "Retrying (" << maxRetry - retry + 1 << "/" << maxRetry << ")";
				retry--;
				continue;
			}
			// Max retry reached
			std::cout << "Please try again later." << std::endl;
			return -1;
		}
		std::cout << "Done." << std::endl;
		break;
	}
	
	Time currentTime;
	auto foundSchedule = std::find_if(
		schedules->begin(),
		schedules->end(),
		[currentTime](const MyClassInterface::Schedule* schedule) {
			Time* scheduleDate = schedule->getDate();
			Time* scheduleStartTime = schedule->getStartTime();
			return currentTime.dateMatches(*scheduleDate) &&
				currentTime.isInRangeOf(*scheduleStartTime, 50 * 60, 40 * 60) &&
				schedule->getUrl().compare("-") != 0;
		}
	);

	if (foundSchedule == schedules->end()) {
		std::cout << "No class within time range." << std::endl;
	}
	else {
		MyClassInterface::Schedule& schedule = **foundSchedule;
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
	}

	std::cout << "Press any key to exit.";
	Utils::waitKeypress();
	std::cout << std::endl;

	Curl::globalCleanup();
	return 0;
}