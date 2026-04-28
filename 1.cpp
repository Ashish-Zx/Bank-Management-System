#include <iostream>
#include <string>
#include <curl/curl.h>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <iomanip>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string extractValue(const std::string& json, const std::string& key) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return "";
    pos = json.find(":", pos) + 1;
    size_t start = json.find_first_not_of(" \t\n\r", pos);
    if (json[start] == '"') {
        start++;
        size_t end = json.find("\"", start);
        return json.substr(start, end - start);
    } else {
        size_t end = json.find_first_of(",}]", start);
        return json.substr(start, end - start);
    }
}

void displayWeather(const std::string& readBuffer) {
    std::string tempStr = extractValue(readBuffer, "temp");
    std::string description = extractValue(readBuffer, "description");
    std::string humidity = extractValue(readBuffer, "humidity");
    std::string windSpeed = extractValue(readBuffer, "speed");

    if (tempStr.empty() || description.empty()) {
        std::cerr << "Failed to extract weather data from the response." << std::endl;
    } else {
        try {
            double temperature = std::stod(tempStr) - 273.15;
            std::cout << "\033[2J\033[1;1H"; // Clear screen and move cursor to top-left
            std::cout << "Weather at coordinates (33.44, -94.04):" << std::endl;
            std::cout << std::string(40, '-') << std::endl;
            std::cout << std::left << std::setw(20) << "Temperature:" << std::fixed << std::setprecision(1) << temperature << "°C" << std::endl;
            std::cout << std::left << std::setw(20) << "Description:" << description << std::endl;
            std::cout << std::left << std::setw(20) << "Humidity:" << humidity << "%" << std::endl;
            std::cout << std::left << std::setw(20) << "Wind Speed:" << windSpeed << " m/s" << std::endl;
            std::cout << std::string(40, '-') << std::endl;
        } catch (const std::invalid_argument& e) {
            std::cerr << "Failed to convert temperature to double: " << e.what() << std::endl;
        }
    }
}

void countdownTimer(int seconds) {
    for (int i = seconds; i > 0; --i) {
        std::cout << "\033[2J\033[1;1H"; // Clear screen and move cursor to top-left
        std::cout << "Next update in " << i << " seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    double lal, lon;
    std::cout << "Enter latitude and longitude separated by a space (e.g., 33.44 -94.04): ";
    while (!(std::cin >> lal >> lon)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter numbers for latitude and longitude: ";
    }
    
    std::string apiKey = "55dc9ea1c438bb90f473a97988cbe5f5";
    std::string latitude = std::to_string(lal);
    std::string longitude = std::to_string(lon);

    curl = curl_easy_init();
    if (curl) {
        std::string url = "https://api.openweathermap.org/data/2.5/weather?lat=" + latitude + "&lon=" + longitude + "&appid=" + apiKey;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        while (true) {
            readBuffer.clear();
            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            } else {
                displayWeather(readBuffer);
            }

            countdownTimer(5);  // 5-second delay with countdown
        }

        curl_easy_cleanup(curl);
    }

    return 0;
}