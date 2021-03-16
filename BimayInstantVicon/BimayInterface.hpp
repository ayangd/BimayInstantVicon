#ifndef _BIMAY_INSTANT_VICON_BIMAYINTERFACE_HPP
#define _BIMAY_INSTANT_VICON_BIMAYINTERFACE_HPP

#include <string>
#include <vector>
#include <ctime>
#include "Utils.hpp"

namespace BimayInstantVicon {

	class MyClassInterface
	{
	private:
		bool loggedIn;
		Curl* curl;
	public:
		MyClassInterface(Curl* curl);
		void login(const std::string& username, const std::string& password);
		bool isLoggedIn() const;

		class Schedule;

		std::vector<Schedule*>* getSchedules();

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
