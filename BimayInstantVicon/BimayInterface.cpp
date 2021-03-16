#include "BimayInterface.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace BimayInstantVicon {
	
	MyClassInterface::MyClassInterface(Curl* curl)
	{
		this->curl = curl;
		loggedIn = false;
	}

	void MyClassInterface::login(const std::string& username, const std::string& password) {
		std::string credData = "Username=" + curl->urlEncode(username) + "&Password=" + curl->urlEncode(password);
		curl->post("https://myclass.apps.binus.ac.id/Auth/Login", "https://myclass.apps.binus.ac.id/Auth", credData.c_str(), Curl::Callbacks::CALLBACK_ENABLE, true);
		json loginResponse = json::parse(curl->getBuffer());
		if (loginResponse["Status"]) {
			loggedIn = true;
		}
	}

	bool MyClassInterface::isLoggedIn() const {
		return loggedIn;
	}

	std::vector<MyClassInterface::Schedule*>* MyClassInterface::getSchedules() {
		curl->get("https://myclass.apps.binus.ac.id/Home/GetViconSchedule", "https://myclass.apps.binus.ac.id/Home/Index", Curl::Callbacks::CALLBACK_ENABLE);
		json schedules = json::parse(curl->getBuffer());
		std::vector<MyClassInterface::Schedule*>* scheduleVector = new std::vector<MyClassInterface::Schedule*>();
		for (json schedule : schedules) {
			scheduleVector->push_back(
				new MyClassInterface::Schedule(
					Time::getDateFromJSON(schedule["StartDate"].get<std::string>()),
					Time::getTimeFromJSON(schedule["StartTime"].get<std::string>()),
					Time::getTimeFromJSON(schedule["EndTime"].get<std::string>()),
					schedule["ClassCode"].get<std::string>(),
					schedule["Room"].get<std::string>(),
					schedule["Campus"].get<std::string>(),
					schedule["DeliveryMode"].get<std::string>(),
					schedule["CourseCode"].get<std::string>(),
					schedule["CourseTitleEn"].get<std::string>(),
					schedule["WeekSession"].get<int>(),
					schedule["CourseSessionNumber"].get<int>(),
					schedule["MeetingId"].get<std::string>(),
					schedule["MeetingPassword"].get<std::string>(),
					schedule["MeetingUrl"].get<std::string>()
				)
			);
		}
		return scheduleVector;
	}

	MyClassInterface::Schedule::Schedule(
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
		std::string url)
	{
		this->date = date;
		this->startTime = startTime;
		this->endTime = endTime;
		this->classCode = classCode;
		this->room = room;
		this->campus = campus;
		this->deliveryMode = deliveryMode;
		this->courseCode = courseCode;
		this->courseTitle = courseTitle;
		this->week = week;
		this->session = session;
		this->meetingId = meetingId;
		this->meetingPassword = meetingPassword;
		this->url = url;
	}

	MyClassInterface::Schedule::~Schedule() {
		delete date;
		delete startTime;
		delete endTime;
	}

	Time* MyClassInterface::Schedule::getDate() const {
		return date;
	}

	Time* MyClassInterface::Schedule::getStartTime() const {
		return startTime;
	}

	Time* MyClassInterface::Schedule::getEndTime() const {
		return endTime;
	}

	std::string MyClassInterface::Schedule::getClassCode() const {
		return classCode;
	}

	std::string MyClassInterface::Schedule::getRoom() const {
		return room;
	}

	std::string MyClassInterface::Schedule::getCampus() const {
		return campus;
	}

	std::string MyClassInterface::Schedule::getDeliveryMode() const {
		return deliveryMode;
	}

	std::string MyClassInterface::Schedule::getCourseCode() const {
		return courseCode;
	}

	std::string MyClassInterface::Schedule::getCourseTitle() const {
		return courseTitle;
	}

	int MyClassInterface::Schedule::getWeek() const {
		return week;
	}

	int MyClassInterface::Schedule::getSession() const {
		return session;
	}

	std::string MyClassInterface::Schedule::getMeetingId() const {
		return meetingId;
	}

	std::string MyClassInterface::Schedule::getMeetingPassword() const {
		return meetingPassword;
	}

	std::string MyClassInterface::Schedule::getUrl() const {
		return url;
	}

	LoginException::LoginException() : Exception() {}

	LoginException::LoginException(const std::string& reason) : Exception(reason) {}

}