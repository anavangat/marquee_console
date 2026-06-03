#include <iostream>
#include <windows.h>
#include <conio.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <cstdlib>

const int MAX_HISTORY_SIZE = 5;

struct Marquee {
    double posX = 1, posY = 5;
    int speedX = 15, speedY = 5;
    int sizeX = 31, sizeY = 6;
    std::vector<std::string> messageLines = {
        " _____   __      __   _____",
        "|  __ \\  \\ \\    / /  |  __ \\",
        "| |  | |  \\ \\  / /   | |  | |",
        "| |  | |   \\ \\/ /    | |  | |",
        "| |__| |    \\  /     | |__| |",
        "|_____/      \\/      |_____/ "
    };
};

struct ConsoleSize { int cols; int rows; };
ConsoleSize getConsoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    return {cols, rows};
}

/**
 * Clears the console screen using `cls` system command.
 */
void clearScreen() {
    system("cls");
}

std::string toLower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return s;
}

void setCursor(int x, int y) {
    COORD pos = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

/**
 * Prints the marquee header
 */
void printHeader() {
    std::cout << "*********************************" << std::endl;
    std::cout << "*  Displaying marquee console!  *" << std::endl;
    std::cout << "*********************************" << std::endl;
}

/**
 * Updates the position of the marquee and draws it.
 */
void updateMarquee(Marquee *marquee, std::chrono::duration<double> dt, int screenWidth, int screenHeight) {
    // update
    marquee->posX += marquee->speedX * dt.count();
    marquee->posY += marquee->speedY * dt.count();

    if (marquee->posX >= (screenWidth - marquee->sizeX))
    {
        marquee->posX = screenWidth - marquee->sizeX;
        marquee->speedX = -marquee->speedX;
    }
    else if (marquee->posX <= 1)
    {
        marquee->posX = 1;
        marquee->speedX = -marquee->speedX;
    }

    if (marquee->posY >= (screenHeight - marquee->sizeY))
    {
        marquee->posY = screenHeight - marquee->sizeY;
        marquee->speedY = -marquee->speedY;
    }
    else if (marquee->posY <= 3)
    {
        marquee->posY = 3;
        marquee->speedY = -marquee->speedY;
    }

    // draw
    int drawX = static_cast<int>(std::round(marquee->posX));
    int drawY = static_cast<int>(std::round(marquee->posY));
    for (size_t row = 0; row < marquee->messageLines.size(); row++) {
        setCursor(drawX, drawY + static_cast<int>(row));
        std::cout << marquee->messageLines[row];
    }
}

void printInputMessage(const std::string& inputBuffer, int screenHeight) {
    setCursor(0, screenHeight - 9);
    std::cout << "Enter a command for MARQUEE_CONSOLE: " << inputBuffer;
}

bool keyPolling(std::string& inputBuffer, std::vector<std::string>& history) {
    if (kbhit()) {
        char ch = getch();
        if (ch == 13) { // enter
            if (toLower(inputBuffer) == "exit") return false;

            if (history.size() >= MAX_HISTORY_SIZE) history.erase(history.end());
            history.insert(history.begin(), inputBuffer);
            inputBuffer.clear();
        }
        else if (ch == '\b') { // backspace
            if (!inputBuffer.empty()) inputBuffer.pop_back();
        }
        else {
            inputBuffer.push_back(ch);
        }
    }
    return true;
}

void printHistory(const std::vector<std::string>& history, int screenHeight) {
    setCursor(0, screenHeight - 8);
    for (std::string cmd : history) {
        std::cout << "  > " << cmd << std::endl;
    }
}

int main() {
    Marquee m = {};

    auto size = getConsoleSize();
    int screenWidth = size.cols;
    int screenHeight = size.rows;

    std::string inputBuffer = "";
    std::vector<std::string> history{};

    bool running = true;

    auto last = std::chrono::steady_clock::now();
    const std::chrono::milliseconds targetFrame(10); // 100 Hz display => 10ms

    while (running) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = now - last;
        last = now;

        clearScreen();
        printHeader();
        updateMarquee(&m, dt, screenWidth, screenHeight - 10);
        printInputMessage(inputBuffer, screenHeight);
        running = keyPolling(inputBuffer, history);
        printHistory(history, screenHeight);
        setCursor(37 + inputBuffer.size(), screenHeight - 9);

        std::this_thread::sleep_for(targetFrame);
    }

    return 0;
}