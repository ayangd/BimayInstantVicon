#ifndef _BIMAY_INSTANT_VICON_UTILS_HPP
#define _BIMAY_INSTANT_VICON_UTILS_HPP

#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <curl/curl.h>

#if defined(_WIN32)
#include <Windows.h>
#include <ShlObj.h>
#elif defined(__unix__)
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#endif

#if defined (__linux__)
#define _urlopen "xdg-open "
#elif defined (__APPLE__)
#define _urlopen "open "
#endif

namespace BimayInstantVicon {
	class Utils
	{
	public:
		static std::string wstrToStr(std::wstring wstr);
		static void openurl(std::string url);

		static std::string getHomeDir();
		static void waitKeypress();
		static std::string promptPassword();
	};

	class Curl
	{
	private:
		CURL* curl;
		CURLcode curlCode;
		std::string buffer;

		static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
		static size_t voidWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	public:
		enum class Callbacks
		{
			CALLBACK_ENABLE, CALLBACK_VOID
		};
		Curl();
		void post(const char* url, const char* referer, const char* postFields, Callbacks writeCallback, bool shouldSucceed);
		void post(const char* url, const char* referer, const char* postFields, Callbacks writeCallback);
		void get(const char* url, const char* referer, Callbacks writeCallback, bool shouldSucceed);
		void get(const char* url, const char* referer, Callbacks writeCallback);

		long getResponseCode();
		std::string& getBuffer();

		std::string urlEncode(std::string& str);
		CURLcode getError();
		std::string getErrorMessage();
	};

	class Exception {
	private:
		std::string reason;
	public:
		Exception();
		Exception(std::string reason);
		std::string getReason();
	};

	class CurlException : public Exception {
	public:
		CurlException();
		CurlException(std::string reason);
	};

	class Time
	{
	private:
		std::tm time;
		Time();
	public:
		static Time* getTimeFromEpoch(int64_t seconds);
		static Time* getTimeFromJSON(std::string time);
		static Time* getDateFromJSON(std::string date);
		static Time* getCurrentTime();
		int getSecond();
		int getMinute();
		int getHour();
		int getDayOfMonth();
		int getMonth();
		int getYear();
	};

	class StringStreaming {
	private:
		std::stringstream ss;
	public:
		class Stop {};
		StringStreaming& operator<<(const std::string s);
		StringStreaming& operator<<(const char* c);
		StringStreaming& operator<<(const char c);
		StringStreaming& operator<<(const int i);
		std::string operator<<(Stop s);
	};
}

#endif
