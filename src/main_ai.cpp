// Minimal Windows-only C++ immediate-mode marquee console
#include <windows.h>
#include <algorithm>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>

using namespace std::chrono_literals;

HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

struct ConsoleSize { int cols; int rows; };

ConsoleSize getConsoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    return {cols, rows};
}

void clearScreen(int cols, int rows) {
    DWORD written;
    COORD home = {0, 0};
    FillConsoleOutputCharacterA(hOut, ' ', cols * rows, home, &written);
    FillConsoleOutputAttribute(hOut, 0x07, cols * rows, home, &written);
    SetConsoleCursorPosition(hOut, home);
}

void setCursor(int x, int y) {
    COORD pos = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(hOut, pos);
}

void hideCursor(bool hide=true) {
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(hOut, &info);
    info.bVisible = !hide;
    SetConsoleCursorInfo(hOut, &info);
}

// Poll keyboard input using Peek/ReadConsoleInput. Non-blocking.
// Returns true if an input event produced a character (or control like backspace/enter).
bool pollKey(char &outch, bool &isEnter, bool &isBackspace, bool &isEscape) {
    DWORD events = 0;
    if (!GetNumberOfConsoleInputEvents(hIn, &events)) return false;
    if (events == 0) return false;

    INPUT_RECORD rec;
    DWORD read = 0;
    if (!ReadConsoleInputA(hIn, &rec, 1, &read) || read == 0) return false;

    if (rec.EventType == KEY_EVENT) {
        KEY_EVENT_RECORD &k = rec.Event.KeyEvent;
        if (!k.bKeyDown) return false; // only process key-down
        isEnter = false; isBackspace = false; isEscape = false;
        if (k.wVirtualKeyCode == VK_RETURN) { isEnter = true; return true; }
        if (k.wVirtualKeyCode == VK_BACK) { isBackspace = true; return true; }
        if (k.wVirtualKeyCode == VK_ESCAPE) { isEscape = true; return true; }
        char ch = k.uChar.AsciiChar;
        if (ch != 0) { outch = ch; return true; }
    }
    return false;
}

int main() {
    // Configure console input mode: turn off line input and echo so we get key events.
    DWORD mode = 0;
    GetConsoleMode(hIn, &mode);
    SetConsoleMode(hIn, (mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)) | ENABLE_WINDOW_INPUT);

    auto size = getConsoleSize();
    int cols = size.cols;
    int rows = size.rows;

    hideCursor(true);

    // Layout: header rows 0-1, marquee around rows/3, input prompt at last line, history above it.
    const int headerLines = 2;
    int marqueeRow = std::max(2, rows / 3);
    int inputRow = rows - 1;
    int historyTop = headerLines;
    int historyBottom = inputRow - 1;

    std::string marqueeText = "Hello world in marquee!";
    double x = 0.0;
    double vx = 20.0; // characters per second

    std::string inputBuffer;
    std::vector<std::string> history; // newest-first

    auto last = std::chrono::steady_clock::now();
    const std::chrono::milliseconds targetFrame(16); // ~60 Hz

    bool running = true;
    while (running) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = now - last;
        last = now;

        // Poll input events once per frame (can be tuned independently later)
        char ch = 0; bool isEnter=false, isBack=false, isEsc=false;
        while (pollKey(ch, isEnter, isBack, isEsc)) {
            if (isEsc) { running = false; break; }
            if (isEnter) {
                if (!inputBuffer.empty()) {
                    history.insert(history.begin(), inputBuffer);
                    inputBuffer.clear();
                }
            } else if (isBack) {
                if (!inputBuffer.empty()) inputBuffer.pop_back();
            } else if (ch >= 32) {
                inputBuffer.push_back(ch);
            }
        }

        // Update marquee
        double maxX = std::max(1, cols - (int)marqueeText.size() - 1);
        x += vx * dt.count();
        if (x < 1) { x = 1; vx = -vx; }
        if (x > maxX) { x = maxX; vx = -vx; }

        // Redraw whole screen
        clearScreen(cols, rows);

        // Header
        setCursor(0, 0);
        std::cout << std::string(cols, '*') << "\n";
        setCursor(0, 1);
        std::cout << "* Displaying a marquee console!" << std::string(std::max(0, cols - 26), ' ') << "*";

        // Marquee
        setCursor((int)x, marqueeRow);
        std::cout << marqueeText;

        // Input prompt
        setCursor(0, inputRow);
        std::cout << "Enter a command for MARQUEE_CONSOLE: " << inputBuffer;

        // History (newest-first) rendered above the input line
        int row = inputRow - 1;
        for (size_t i = 0; i < history.size() && row >= historyTop; ++i, --row) {
            setCursor(0, row);
            std::string s = history[i];
            if ((int)s.size() > cols) s = s.substr(0, cols);
            std::cout << s << std::string(std::max(0, cols - (int)s.size()), ' ');
        }

        // flush output
        std::cout.flush();

        // Sleep to cap frame rate
        std::this_thread::sleep_for(targetFrame);
    }

    // Cleanup: restore cursor
    hideCursor(false);
    SetConsoleMode(hIn, mode);
    clearScreen(cols, rows);
    setCursor(0,0);
    return 0;
}
