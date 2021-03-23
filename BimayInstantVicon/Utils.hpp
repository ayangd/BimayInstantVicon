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

#ifndef _BIMAY_INSTANT_VICON_UTILS_HPP
#define _BIMAY_INSTANT_VICON_UTILS_HPP

#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <ctime>
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
		static std::string wstrToStr(const std::wstring& wstr);
		static std::string getInstantLink(const std::string& link);
		static void openurl(const std::string& url);

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
		static bool globalCleanedUp;
		static void checkCleanedUp();

		static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
		static size_t voidWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	public:
		enum class Callbacks
		{
			CALLBACK_ENABLE, CALLBACK_VOID
		};
		Curl();
		~Curl();
		void post(const char* url, const char* referer, const char* postFields, Callbacks callbackOption, bool shouldSucceed);
		void post(const char* url, const char* referer, const char* postFields, Callbacks callbackOption);
		void get(const char* url, const char* referer, Callbacks callbackOption, bool shouldSucceed);
		void get(const char* url, const char* referer, Callbacks callbackOption);

		long getResponseCode() const;
		std::string& getBuffer();

		std::string urlEncode(const std::string& str);
		CURLcode getError() const;
		std::string getErrorMessage() const;
		static void globalCleanup();
	};

	class Exception {
	private:
		std::string reason;
	public:
		Exception();
		Exception(const std::string& reason);
		std::string getReason();
	};

	class CurlException : public Exception {
	public:
		CurlException();
		CurlException(const std::string& reason);
	};

	class Time
	{
	private:
		std::tm time;
	public:
		Time();
		static Time* getTimeFromEpoch(int64_t seconds);
		static Time* getTimeFromJSON(const std::string& time);
		static Time* getDateFromJSON(const std::string& date);
		int getSecond() const;
		int getMinute() const;
		int getHour() const;
		int getDayOfMonth() const;
		int getMonth() const;
		int getYear() const;
		bool isInRangeOf(const Time& reference, uint64_t referenceSecondMinus, uint64_t referenceSecondPlus) const;
		bool dateMatches(const Time& reference) const;
	};
}

#endif
