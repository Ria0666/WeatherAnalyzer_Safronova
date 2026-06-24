#include "AnalyticsService.h"
#include "AnomalyService.h"
#include "CityRepository.h"
#include "Database.h"
#include "ForecastService.h"
#include "ImportLogRepository.h"
#include "ImportService.h"
#include "ObservationRepository.h"
#include "OpenMeteoClient.h"
#include "ReportService.h"
#include "StationRepository.h"

#include <cctype>
#include <iostream>
#include <limits>
#include <string>

bool isDateCorrect(const std::string& date) {
    if (date.size() != 10) {
        return false;
    }

    if (date[4] != '-' || date[7] != '-') {
        return false;
    }

    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            continue;
        }

        if (!isdigit(date[i])) {
            return false;
        }
    }

    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));

    if (month < 1 || month > 12) {
        return false;
    }

    if (day < 1 || day > 31) {
        return false;
    }

    return true;
}

void clearInputLine() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

bool checkDates(const std::string& startDate, const std::string& endDate) {
    if (!isDateCorrect(startDate) || !isDateCorrect(endDate)) {
        std::cout << "Wrong date format. Use YYYY-MM-DD, for example 2024-01-01." << std::endl;
        return false;
    }

    if (startDate > endDate) {
        std::cout << "Start date cannot be later than end date." << std::endl;
        return false;
    }

    return true;
}

bool checkWindowSize(int windowDays) {
    if (windowDays <= 0) {
        std::cout << "Window size must be positive." << std::endl;
        return false;
    }

    if (windowDays > 30) {
        std::cout << "Window size is too large. Use value from 1 to 30." << std::endl;
        return false;
    }

    return true;
}

int main() {
    try {
        std::string connectionString =
            "host=localhost "
            "port=5432 "
            "dbname=weather_db "
            "user=postgres "
            "password=postgres";

        Database db(connectionString);

        OpenMeteoClient client;

        CityRepository cityRepo(db);
        StationRepository stationRepo(db);
        ObservationRepository obsRepo(db);
        ImportLogRepository logRepo(db);

        ImportService importService(
            client,
            cityRepo,
            stationRepo,
            obsRepo,
            logRepo
        );

        AnalyticsService analytics(db);
        AnomalyService anomalyService(db);
        ForecastService forecastService(db);
        ReportService reportService(db);

        int choice = -1;

        while (choice != 0) {
            std::cout << "\nWeather Analyzer" << std::endl;
            std::cout << "1. Load data from Open-Meteo" << std::endl;
            std::cout << "2. Temperature statistics" << std::endl;
            std::cout << "3. Monthly temperature" << std::endl;
            std::cout << "4. Monthly precipitation" << std::endl;
            std::cout << "5. Rainy days count" << std::endl;
            std::cout << "6. Compare cities by average temperature" << std::endl;
            std::cout << "7. Find temperature anomalies by regression" << std::endl;
            std::cout << "8. Show anomalous days" << std::endl;
            std::cout << "9. Build temperature forecast" << std::endl;
            std::cout << "10. Show temperature forecast" << std::endl;
            std::cout << "11. Build precipitation probability forecast" << std::endl;
            std::cout << "12. Show precipitation probability forecast" << std::endl;
            std::cout << "13. Create PDF report" << std::endl;
            std::cout << "0. Exit" << std::endl;
            std::cout << "Choose: ";

            std::cin >> choice;

            if (std::cin.fail()) {
                std::cin.clear();
                clearInputLine();
                std::cout << "Wrong menu option." << std::endl;
                continue;
            }

            if (choice == 1) {
                std::string startDate;
                std::string endDate;

                std::cout << "Start date, for example 2024-01-01: ";
                std::cin >> startDate;

                std::cout << "End date, for example 2024-01-07: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                int sourceId = 1;

                ImportResult result = importService.importWeather(
                    sourceId,
                    startDate,
                    endDate
                );

                std::cout << "\nImport finished" << std::endl;
                std::cout << "Total rows: " << result.totalRows << std::endl;
                std::cout << "Loaded rows: " << result.loadedRows << std::endl;
                std::cout << "Skipped rows: " << result.skippedRows << std::endl;
            }

            else if (choice == 2) {
                std::string cityName;
                std::string startDate;
                std::string endDate;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Start date: ";
                std::cin >> startDate;

                std::cout << "End date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                analytics.showTemperatureStats(
                    cityName,
                    startDate,
                    endDate
                );
            }

            else if (choice == 3) {
                std::string cityName;
                int year;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Year: ";
                std::cin >> year;

                if (std::cin.fail()) {
                    std::cin.clear();
                    clearInputLine();
                    std::cout << "Wrong year." << std::endl;
                    continue;
                }

                analytics.showMonthlyTemperature(cityName, year);
            }

            else if (choice == 4) {
                std::string cityName;
                int year;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Year: ";
                std::cin >> year;

                if (std::cin.fail()) {
                    std::cin.clear();
                    clearInputLine();
                    std::cout << "Wrong year." << std::endl;
                    continue;
                }

                analytics.showMonthlyPrecipitation(cityName, year);
            }

            else if (choice == 5) {
                std::string cityName;
                std::string startDate;
                std::string endDate;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Start date: ";
                std::cin >> startDate;

                std::cout << "End date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                double limit = 0.0;

                analytics.showRainyDays(
                    cityName,
                    startDate,
                    endDate,
                    limit
                );
            }

            else if (choice == 6) {
                std::string startDate;
                std::string endDate;

                std::cout << "Start date: ";
                std::cin >> startDate;

                std::cout << "End date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                analytics.compareCitiesTemperature(
                    startDate,
                    endDate
                );
            }

            else if (choice == 7) {
                std::string cityName;
                std::string startDate;
                std::string endDate;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Start date: ";
                std::cin >> startDate;

                std::cout << "End date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                anomalyService.findTemperatureAnomaliesByRegression(
                    cityName,
                    startDate,
                    endDate
                );
            }

            else if (choice == 8) {
                std::string cityName;
                std::string startDate;
                std::string endDate;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Start date: ";
                std::cin >> startDate;

                std::cout << "End date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                anomalyService.showAnomalies(
                    cityName,
                    startDate,
                    endDate
                );
            }

            else if (choice == 9) {
                std::string cityName;
                std::string startDate;
                std::string endDate;
                int windowDays;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Forecast start date: ";
                std::cin >> startDate;

                std::cout << "Forecast end date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                std::cout << "Window size in days, for example 7: ";
                std::cin >> windowDays;

                if (std::cin.fail()) {
                    std::cin.clear();
                    clearInputLine();
                    std::cout << "Wrong window size." << std::endl;
                    continue;
                }

                if (!checkWindowSize(windowDays)) {
                    continue;
                }

                forecastService.buildTemperatureForecast(
                    cityName,
                    startDate,
                    endDate,
                    windowDays
                );
            }

            else if (choice == 10) {
                std::string cityName;
                std::string startDate;
                std::string endDate;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Start date: ";
                std::cin >> startDate;

                std::cout << "End date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                forecastService.showTemperatureForecast(
                    cityName,
                    startDate,
                    endDate
                );
            }

            else if (choice == 11) {
                std::string cityName;
                std::string startDate;
                std::string endDate;
                int windowDays;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Forecast start date: ";
                std::cin >> startDate;

                std::cout << "Forecast end date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                std::cout << "Window size in days, for example 7: ";
                std::cin >> windowDays;

                if (std::cin.fail()) {
                    std::cin.clear();
                    clearInputLine();
                    std::cout << "Wrong window size." << std::endl;
                    continue;
                }

                if (!checkWindowSize(windowDays)) {
                    continue;
                }

                forecastService.buildPrecipitationProbabilityForecast(
                    cityName,
                    startDate,
                    endDate,
                    windowDays
                );
            }

            else if (choice == 12) {
                std::string cityName;
                std::string startDate;
                std::string endDate;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Start date: ";
                std::cin >> startDate;

                std::cout << "End date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                forecastService.showPrecipitationProbabilityForecast(
                    cityName,
                    startDate,
                    endDate
                );
            }

            else if (choice == 13) {
                std::string cityName;
                std::string startDate;
                std::string endDate;

                std::cout << "City name: ";
                clearInputLine();
                std::getline(std::cin, cityName);

                std::cout << "Start date: ";
                std::cin >> startDate;

                std::cout << "End date: ";
                std::cin >> endDate;

                if (!checkDates(startDate, endDate)) {
                    continue;
                }

                reportService.createPdfReport(
                    cityName,
                    startDate,
                    endDate
                );
            }

            else if (choice == 0) {
                std::cout << "Exit." << std::endl;
            }

            else {
                std::cout << "Wrong menu option." << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Program error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}