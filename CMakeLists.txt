cmake_minimum_required(VERSION 3.16)

project(WeatherAnalyzer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(include)
include_directories(${PQXX_INCLUDE_DIRS})

add_executable(weather_analyzer
        src/main.cpp
        src/Database.cpp
        src/OpenMeteoClient.cpp
        src/CityRepository.cpp
        src/StationRepository.cpp
        src/ObservationRepository.cpp
        src/ImportLogRepository.cpp
        src/ImportService.cpp
        src/AnalyticsService.cpp
        src/AnomalyService.cpp
        src/ForecastService.cpp
        src/ReportService.cpp

)

target_link_directories(weather_analyzer PRIVATE ${PQXX_LIBRARY_DIRS})

target_link_libraries(weather_analyzer
        ${PQXX_LIBRARIES}
        CURL::libcurl
        nlohmann_json::nlohmann_json
)