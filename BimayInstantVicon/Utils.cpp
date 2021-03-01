#include "Utils.hpp"

#include <ctime>
#include <regex>

namespace BimayInstantVicon {

    std::string Utils::wstrToStr(std::wstring wstr) {
        std::string str;
        for (auto it = wstr.begin(); it != wstr.end(); it++) {
            str += (char)*it;
        }
        return str;
    }

// Define cross-platform macros
#if defined(_WIN32)       // Windows
    void Utils::openurl(std::string url) {
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

    void Utils::openurl(std::string url) {
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
        curl_easy_setopt(curl, CURLOPT_COOKIELIST, "");
        curlCode = CURLE_OK;
    }

    void Curl::post(const char* url, const char* referer, const char* postFields, Callbacks writeCallback) {
        post(url, referer, postFields, writeCallback, false);
    }

    void Curl::post(const char* url, const char* referer, const char* postFields, Callbacks writeCallback, bool shouldSucceed) {
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (referer != NULL) {
            curl_easy_setopt(curl, CURLOPT_REFERER, referer);
        }
        if (postFields != NULL) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields);
        }
        switch (writeCallback) {
        case Callbacks::CALLBACK_ENABLE:
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
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
                throw CurlException(StringStreaming() << "cURL POST response code: " << respCode << StringStreaming::Stop());
            }
        }
    }

    void Curl::get(const char* url, const char* referer, Callbacks writeCallback, bool shouldSucceed) {
        get(url, referer, writeCallback, false);
    }

    void Curl::get(const char* url, const char* referer, Callbacks writeCallback, bool shouldSucceed) {
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (referer != NULL) {
            curl_easy_setopt(curl, CURLOPT_REFERER, referer);
        }
        switch (writeCallback) {
        case Callbacks::CALLBACK_ENABLE:
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
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
                throw CurlException(StringStreaming() << "cURL GET response code: " << respCode << StringStreaming::Stop());
            }
        }
    }

    std::string Curl::urlEncode(std::string& str) {
        char* cstr = curl_easy_escape(curl, str.c_str(), 0);
        std::string s(cstr);
        curl_free(cstr);
        return s;
    }

    long Curl::getResponseCode() {
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

    CURLcode Curl::getError() {
        return curlCode;
    }

    std::string Curl::getErrorMessage() {
        return std::string(curl_easy_strerror(curlCode));
    }

    Exception::Exception() {
        reason = "";
    }

    Exception::Exception(std::string reason) {
        this->reason = reason;
    }

    std::string Exception::getReason() {
        return reason;
    }

    CurlException::CurlException() : Exception() {}

    CurlException::CurlException(std::string reason) : Exception(reason) {}

    Time::Time() {
        time = {};
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
    }

    Time* Time::getTimeFromJSON(std::string time) {
        Time* t = new Time();
        std::smatch sm;
        std::regex_search(time, sm, std::regex("(\\d{2}):(\\d{2}):(\\d{2})"));
        t->time = { std::stoi(sm[3]), std::stoi(sm[2]), std::stoi(sm[1]) };
        return t;
    }

    Time* Time::getDateFromJSON(std::string date) {
        std::smatch sm;
        std::regex_search(date, sm, std::regex("\\d+"));
        return getTimeFromEpoch(std::stoll(sm[0]) / 1000);
    }

    Time* Time::getCurrentTime() {
        Time* t = new Time();
        time_t now = std::time(NULL);
#ifdef _MSC_VER
        localtime_s(&t->time, &now);
#else
        tm* tp = localtime(&date);
        memcpy(tp, &t->time, sizeof(std::tm));
#endif
    }

    int Time::getSecond() {
        return time.tm_sec;
    }

    int Time::getMinute() {
        return time.tm_min;
    }

    int Time::getHour() {
        return time.tm_hour;
    }

    int Time::getDayOfMonth() {
        return time.tm_mday;
    }

    int Time::getMonth() {
        return time.tm_mon;
    }

    int Time::getYear() {
        return time.tm_year;
    }

    StringStreaming& StringStreaming::operator<<(const std::string s) {
        ss << s;
        return *this;
    }

    StringStreaming& StringStreaming::operator<<(const char* c) {
        ss << c;
        return *this;
    }

    StringStreaming& StringStreaming::operator<<(const char c) {
        ss << c;
        return *this;
    }

    StringStreaming& StringStreaming::operator<<(const int i) {
        ss << i;
        return *this;
    }

    std::string StringStreaming::operator<<(Stop s) {
        return ss.str();
    }
}