#ifndef _BIMAY_INSTANT_VICON_BIMAYINTERFACE_HPP
#define _BIMAY_INSTANT_VICON_BIMAYINTERFACE_HPP

#include <string>
#include <vector>
#include "Utils.hpp"

namespace BimayInstantVicon {

	class MyClassInterface
	{
	private:
		bool loggedIn;
		Curl& curl;
	public:
		MyClassInterface(Curl& curl);
		void login(std::string username, std::string password);
		bool isLoggedIn();

		class Schedule;

		std::vector<Schedule> getSchedules();

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
			std::string week;
			std::string session;
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
				std::string week,
				std::string session,
				std::string meetingId,
				std::string meetingPassword,
				std::string url);
			~Schedule();
			Time* getDate();
			Time* getStartTime();
			Time* getEndTime();
			std::string getClassCode();
			std::string getRoom();
			std::string getCampus();
			std::string getDeliveryMode();
			std::string getCourseCode();
			std::string getCourseTitle();
			std::string getWeek();
			std::string getSession();
			std::string getMeetingId();
			std::string getMeetingPassword();
			std::string getUrl();
		};
	};

	class LoginException : public Exception {
	public:
		LoginException();
		LoginException(std::string reason);
	};

}

#endif
