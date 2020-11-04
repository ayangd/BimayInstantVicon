#ifndef _BIMAY_INSTANT_VICON_MAIN_HPP
#define _BIMAY_INSTANT_VICON_MAIN_HPP

#include <cstdlib>
#include <string>

std::string wstrToStr(std::wstring wstr) {
    std::string str;
    for (auto it = wstr.begin(); it != wstr.end(); it++) {
        str += (char) *it;
    }
    return str;
}

// Define cross-platform macros
#if defined(_WIN32)       // Windows
#include <Windows.h>
#include <ShlObj.h>

void openurl(std::string url) {
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

std::string getHomeDir() {
	wchar_t* dir = NULL;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &dir);
	std::wstring wstr(dir);
	CoTaskMemFree(static_cast<void*>(dir));
	return wstrToStr(wstr);
}
void waitKeypress() {
    HANDLE hstdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hstdin, &mode);
    SetConsoleMode(hstdin, mode & ~ENABLE_LINE_INPUT);
    std::cin.get();
    SetConsoleMode(hstdin, mode);
}
std::string promptPassword() {
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
#if defined (__linux__)   // Only Linux
#define _urlopen "xdg-open "

#elif defined (__APPLE__) // Only MacOS
#define _urlopen "open "

#endif                    // End only
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

std::string getHomeDir() {
	return std::string(getenv("HOME"));
}
void waitKeypress() {
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
std::string promptPassword() {
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

void openurl(std::string url) {
    std::string command = _urlopen + url;
    system(command.c_str());
}

#endif

#endif //_BIMAY_INSTANT_VICON_MAIN_HPP