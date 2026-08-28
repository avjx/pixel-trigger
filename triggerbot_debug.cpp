#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>

class Triggerbot {
private:
    struct Config {
        int gridSize = 3;
        int spacing = 2;
        float changeThreshold = 0.40f;
        int centerX = 960;
        int centerY = 540;
        int checkDelayMs = 5;
        bool isRunning = true;
        std::vector<COLORREF> previousPixels;
        std::vector<COLORREF> currentPixels;
        int totalPixels = 0;
    } config;

    std::chrono::steady_clock::time_point lastTriggerTime;
    bool bindWasPressed = false;

public:
    Triggerbot(){
        initializeGrid();
        loadSettings();
        lastTriggerTime = std::chrono::steady_clock::now();
    }

    void initializeGrid(){
        config.totalPixels = config.gridSize * config.gridSize;
        config.previousPixels.resize(config.totalPixels, RGB(0,0,0));
        config.currentPixels.resize(config.totalPixels, RGB(0,0,0));
    }

    void captureGrid(){
        HDC hdc = GetDC(NULL);
        if (!hdc) return;

        int halfSize = (config.gridSize / 2) * config.spacing;
        int index = 0;

        for (int y = -halfSize; y <= halfSize; y += config.spacing) {
            for (int x = -halfSize; x <= halfSize; x += config.spacing) {
                config.currentPixels[index] = GetPixel(hdc,
                    config.centerX + x,
                    config.centerY + y);
                index++;
            }
        }

        ReleaseDC(NULL, hdc);
    }

    float calculateChange(){
        int changedPixels = 0;
        int threshold = 20;

        for (int i = 0; i < config.totalPixels; i++) {
            COLORREF prev = config.previousPixels[i];
            COLORREF curr = config.currentPixels[i];

            int dr = abs(GetRValue(prev) - GetRValue(curr));
            int dg = abs(GetGValue(prev) - GetGValue(curr));
            int db = abs(GetBValue(prev) - GetBValue(curr));

            if (dr > threshold || dg > threshold || db > threshold) {
                changedPixels++;
            }
        }

        return (float)changedPixels / config.totalPixels;
    }

    bool isBindPressed(){
        return (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
    }

    void triggerClick(){
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

        lastTriggerTime = std::chrono::steady_clock::now();
    }

    bool canTrigger(){
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTriggerTime);
        return elapsed.count() >= 100;
    }

    void run(){
        std::cout << "grid: " << config.gridSize << "x" << config.gridSize << std::endl;
        std::cout << "threshold: " << (config.changeThreshold * 100) << "%" << std::endl;
        std::cout << "delay: " << config.checkDelayMs << "ms" << std::endl;

        bool bindPressed = false;

        while (config.isRunning) {
            bool currentBind = isBindPressed();

            if (currentBind && !bindPressed) {
                captureGrid();
                config.previousPixels = config.currentPixels;
            }

            if (currentBind) {
                captureGrid();
                float changePercent = calculateChange();

                if (changePercent >= config.changeThreshold && canTrigger()) {
                    triggerClick();
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }

                config.previousPixels = config.currentPixels;
            }

            bindPressed = currentBind;
            std::this_thread::sleep_for(std::chrono::milliseconds(config.checkDelayMs));
        }
    }

    void loadSettings() {
        config.centerX = GetSystemMetrics(SM_CXSCREEN)/2;
        config.centerY = GetSystemMetrics(SM_CYSCREEN)/2;
        config.gridSize = 3;
        config.spacing = 2;
        config.changeThreshold = 0.40f;
        config.checkDelayMs = 1;
    }
};

int main() {
    SetConsoleOutputCP(CP_UTF8);
    Triggerbot bot;
    bot.run();
    system("pause");
    return 0;
}
