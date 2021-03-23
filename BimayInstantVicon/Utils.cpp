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

#include "Utils.hpp"

#include <ctime>
#include <regex>

namespace BimayInstantVicon {

    std::string Utils::wstrToStr(const std::wstring& wstr) {
        std::string str;
        for (auto it = wstr.begin(); it != wstr.end(); it++) {
            str += (char)*it;
        }
        return str;
    }

    std::string Utils::getInstantLink(const std::string& link) {
        std::smatch sm;
        std::regex_search(link, sm, std::regex("https://binus.zoom.us/j/(\\d+)\\?pwd=(.+)"));
        std::stringstream outputStream;
        outputStream << "zoommtg://zoom.us/join?action=join&confno=" << sm[1] << "&pwd=" << sm[2];
        return outputStream.str();
    }

// Define cross-platform macros
#if defined(_WIN32)       // Windows
    void Utils::openurl(const std::string& url) {
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    std::string Utils::getHomeDir() {
        wchar_t* dir = NULL;
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &dir);
        std::wstring wstr(dir);
        CoTaskMemFree(static_cast<void*>(dir));
        return Utils::wstrToStr(wstr);
    }
    void Utils::waitKeypress() {
        HANDLE hstdin = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hstdin, &mode);
        SetConsoleMode(hstdin, mode & ~ENABLE_LINE_INPUT);
        std::cin.get();
        SetConsoleMode(hstdin, mode);
    }
    std::string Utils::promptPassword() {
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

#elif defined(__unix__)   // Both Linux and MacOS
    std::string Utils::getHomeDir() {
        return std::string(getenv("HOME"));
    }
    void Utils::waitKeypress() {
        struct termios savedState, newState;

        tcgetattr(STDIN_FILENO, &savedState);

        newState = savedState;

        /* disable canonical input and disable echo.  set minimal input to 1. */
        newState.c_lflag &= ~(ECHO | ICANON);
        newState.c_cc[VMIN] = 1;

        tcsetattr(STDIN_FILENO, TCSANOW, &newState);

        getchar();      /* block (withot spinning) until we get a keypress */

        /* restore the saved state */
        tcsetattr(STDIN_FILENO, TCSANOW, &savedState);
    }
    std::string Utils::promptPassword() {
        struct termios savedState, newState;
        tcgetattr(STDIN_FILENO, &savedState);
        newState = savedState;

        newState.c_lflag &= ~(ECHO | ICANON);
        newState.c_cc[VMIN] = 1;

        std::string password;
        tcsetattr(STDIN_FILENO, TCSANOW, &newState);
        std::getline(std::cin, password);
        tcsetattr(STDIN_FILENO, TCSANOW, &savedState);

        std::cout << std::endl;
        return password;
    }

    void Utils::openurl(const std::string& url) {
        std::string command = _urlopen + url;
        system(command.c_str());
    }

#endif
    Curl::Curl() {
        static bool initialized = false;
        if (!initialized) {
            initialized = true;
            curl_global_init(CURL_GLOBAL_ALL);
        }
        curl = curl_easy_init();
        if (curl == NULL) {
            throw CurlException("Curl easy init failed.");
        }
        curlCode = curl_easy_setopt(curl, CURLOPT_COOKIELIST, "");
        if (curlCode != CURLE_OK) {
            throw CurlException("Curl cookie init failed.");
        }
    }

    Curl::~Curl() {
        curl_easy_cleanup(curl);
    }

    void Curl::post(const char* url, const char* referer, const char* postFields, Callbacks callbackOption) {
        post(url, referer, postFields, callbackOption, false);
    }

    void Curl::post(const char* url, const char* referer, const char* postFields, Callbacks callbackOption, bool shouldSucceed) {
        checkCleanedUp();
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (referer != NULL) {
            curl_easy_setopt(curl, CURLOPT_REFERER, referer);
        }
        if (postFields != NULL) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields);
        }
        else {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }
        switch (callbackOption) {
        case Callbacks::CALLBACK_ENABLE:
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
            buffer.clear();
            break;
        case Callbacks::CALLBACK_VOID:
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, voidWriteCallback);
        }
        curlCode = curl_easy_perform(curl);
        if (curlCode != CURLE_OK) {
            throw CurlException("cURL error: " + getErrorMessage());
        }
        if (shouldSucceed) {
            long respCode = getResponseCode();
            if (respCode != 200) {
                std::stringstream ss;
                ss << "cURL POST response code: " << respCode;
                throw CurlException(ss.str());
            }
        }
    }

    void Curl::get(const char* url, const char* referer, Callbacks callbackOption) {
        get(url, referer, callbackOption, false);
    }

    void Curl::get(const char* url, const char* referer, Callbacks callbackOption, bool shouldSucceed) {
        checkCleanedUp(); 
        curl_easy_reset(curl);
        //curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (referer != NULL) {
            curl_easy_setopt(curl, CURLOPT_REFERER, referer);
        }
        switch (callbackOption) {
        case Callbacks::CALLBACK_ENABLE:
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
            buffer.clear();
            break;
        case Callbacks::CALLBACK_VOID:
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, voidWriteCallback);
        }
        curlCode = curl_easy_perform(curl);
        if (curlCode != CURLE_OK) {
            throw CurlException("cURL error: " + getErrorMessage());
        }
        if (shouldSucceed) {
            long respCode = getResponseCode();
            if (respCode != 200) {
                std::stringstream ss;
                ss << "cURL GET response code: " << respCode;
                throw CurlException(ss.str());
            }
        }
    }

    std::string Curl::urlEncode(const std::string& str) {
        checkCleanedUp(); 
        char* cstr = curl_easy_escape(curl, str.c_str(), 0);
        std::string s(cstr);
        curl_free(cstr);
        return s;
    }

    long Curl::getResponseCode() const {
        checkCleanedUp(); 
        long respCode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respCode);
        return respCode;
    }

    std::string& Curl::getBuffer() {
        return buffer;
    }

    size_t Curl::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        Curl* curlInstance = (Curl*)userdata;
        size_t total = size * nmemb;
        curlInstance->buffer.append(ptr, total);
        return total;
    }

    size_t Curl::voidWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        return size * nmemb;
    }

    CURLcode Curl::getError() const {
        return curlCode;
    }

    std::string Curl::getErrorMessage() const {
        return std::string(curl_easy_strerror(curlCode));
    }

    bool Curl::globalCleanedUp = false;

    void Curl::checkCleanedUp() {
        if (globalCleanedUp) {
            throw CurlException("cURL has been cleaned up.");
        }
    }

    void Curl::globalCleanup() {
        curl_global_cleanup();
        globalCleanedUp = true;
    }

    Exception::Exception() {
        reason = "";
    }

    Exception::Exception(const std::string& reason) {
        this->reason = reason;
    }

    std::string Exception::getReason() {
        return reason;
    }

    CurlException::CurlException() : Exception() {}

    CurlException::CurlException(const std::string& reason) : Exception(reason) {}

    Time::Time() {
        time_t now = std::time(NULL);
#ifdef _MSC_VER
        localtime_s(&time, &now);
#else
        tm* tp = localtime(&now);
        memcpy(tp, &time, sizeof(std::tm));
#endif
    }

    Time* Time::getTimeFromEpoch(int64_t seconds) {
        Time* t = new Time();
        time_t date = time_t(seconds);
#ifdef _MSC_VER
        localtime_s(&t->time, &date);
#else
        tm* tp = localtime(&date);
        memcpy(tp, &t->time, sizeof(std::tm));
#endif
        return t;
    }

    Time* Time::getTimeFromJSON(const std::string& time) {
        Time* t = new Time();
        std::smatch sm;
        std::regex_search(time, sm, std::regex("(\\d{2}):(\\d{2}):(\\d{2})"));
        t->time = { std::stoi(sm[3]), std::stoi(sm[2]), std::stoi(sm[1]) };
        return t;
    }

    Time* Time::getDateFromJSON(const std::string& date) {
        std::smatch sm;
        std::regex_search(date, sm, std::regex("\\d+"));
        return getTimeFromEpoch(std::stoll(sm[0]) / 1000);
    }

    int Time::getSecond() const {
        return time.tm_sec;
    }

    int Time::getMinute() const {
        return time.tm_min;
    }

    int Time::getHour() const {
        return time.tm_hour;
    }

    int Time::getDayOfMonth() const {
        return time.tm_mday;
    }

    int Time::getMonth() const {
        return time.tm_mon;
    }

    int Time::getYear() const {
        return time.tm_year;
    }

    bool Time::isInRangeOf(const Time& reference, uint64_t referenceSecondMinus, uint64_t referenceSecondPlus) const {
        uint64_t secondAccumulation = (uint64_t) getHour() * 3600 + getMinute() * 60 + getSecond();
        uint64_t referenceSecondAccumulation = (uint64_t)reference.getHour() * 3600 + reference.getMinute() * 60 + reference.getSecond();
        return secondAccumulation > referenceSecondAccumulation - referenceSecondMinus &&
            secondAccumulation < referenceSecondAccumulation + referenceSecondPlus;
    }

    bool Time::dateMatches(const Time& reference) const {
        return getDayOfMonth() == reference.getDayOfMonth() &&
            getMonth() == reference.getMonth() &&
            getYear() == reference.getYear();
    }
}