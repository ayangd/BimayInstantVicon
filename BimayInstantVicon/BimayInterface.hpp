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

#ifndef _BIMAY_INSTANT_VICON_BIMAYINTERFACE_HPP
#define _BIMAY_INSTANT_VICON_BIMAYINTERFACE_HPP

#include <string>
#include <vector>
#include <ctime>
#include "Utils.hpp"

namespace BimayInstantVicon {

	/// <summary>
	/// A class to communicate with MyClass web application.
	/// </summary>
	class MyClassInterface
	{
	private:
		bool loggedIn;
		Curl* curl;
	public:
		MyClassInterface(Curl* curl);
		void login(const std::string& username, const std::string& password);
		bool isLoggedIn() const;

		class Schedule; // proto

		std::vector<Schedule*>* getSchedules();

		/// <summary>
		/// A class to hold individual schedule.
		/// </summary>
		class Schedule
		{
		private:
			Time* date;
			Time* startTime;
			Time* endTime;
			std::string classCode;
			std::string room;
			std::string campus;
			std::string deliveryMode;
			std::string courseCode;
			std::string courseTitle;
			int week;
			int session;
			std::string meetingId;
			std::string meetingPassword;
			std::string url;
		public:
			Schedule(
				Time* date,
				Time* startTime,
				Time* endTime,
				std::string classCode,
				std::string room,
				std::string campus,
				std::string deliveryMode,
				std::string courseCode,
				std::string courseTitle,
				int week,
				int session,
				std::string meetingId,
				std::string meetingPassword,
				std::string url);
			~Schedule();
			Time* getDate() const;
			Time* getStartTime() const;
			Time* getEndTime() const;
			std::string getClassCode() const;
			std::string getRoom() const;
			std::string getCampus() const;
			std::string getDeliveryMode() const;
			std::string getCourseCode() const;
			std::string getCourseTitle() const;
			int getWeek() const;
			int getSession() const;
			std::string getMeetingId() const;
			std::string getMeetingPassword() const;
			std::string getUrl() const;
		};
	};

	class LoginException : public Exception {
	public:
		LoginException();
		LoginException(const std::string& reason);
	};

}

#endif
