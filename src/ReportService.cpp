#include "ReportService.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <pqxx/pqxx>
#include <sstream>
#include <string>
#include <vector>

namespace {
    struct ChartPoint {
        std::string label;
        double value;
    };

    struct TrendInfo {
        bool hasTrend = false;
        double slope = 0.0;
        std::string description;
    };

    std::string escapeHtml(const std::string& text) {
        std::string result;

        for (char ch : text) {
            if (ch == '&') {
                result += "&amp;";
            } else if (ch == '<') {
                result += "&lt;";
            } else if (ch == '>') {
                result += "&gt;";
            } else if (ch == '"') {
                result += "&quot;";
            } else {
                result += ch;
            }
        }

        return result;
    }

    std::string formatDouble(double value) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(2) << value;
        return out.str();
    }

    std::string getMonthName(int month) {
        if (month == 1) return "Январь";
        if (month == 2) return "Февраль";
        if (month == 3) return "Март";
        if (month == 4) return "Апрель";
        if (month == 5) return "Май";
        if (month == 6) return "Июнь";
        if (month == 7) return "Июль";
        if (month == 8) return "Август";
        if (month == 9) return "Сентябрь";
        if (month == 10) return "Октябрь";
        if (month == 11) return "Ноябрь";
        if (month == 12) return "Декабрь";

        return "Неизвестно";
    }

    std::string makeFileNameSafe(std::string text) {
        for (char& ch : text) {
            if (ch == ' ' || ch == ':' || ch == '/' || ch == '\\') {
                ch = '_';
            }
        }

        return text;
    }

    std::string quoteCommandArgument(const std::string& text) {
        std::string result = "\"";

        for (char ch : text) {
            if (ch == '"') {
                result += "\\\"";
            } else {
                result += ch;
            }
        }

        result += "\"";
        return result;
    }

    std::string makeFileUrl(const std::string& fileName) {
        std::filesystem::path absolutePath = std::filesystem::absolute(fileName);
        return "file://" + absolutePath.string();
    }

    std::string getTrendText(double slope) {
        if (std::abs(slope) < 0.03) {
            return "Температурный тренд за выбранный период можно считать стабильным.";
        }

        if (slope > 0.0) {
            return "За выбранный период наблюдается положительный температурный тренд: средняя температура постепенно увеличивается.";
        }

        return "За выбранный период наблюдается отрицательный температурный тренд: средняя температура постепенно уменьшается.";
    }

    TrendInfo calculateTrend(const std::vector<ChartPoint>& points) {
        TrendInfo info;

        if (points.size() < 2) {
            info.hasTrend = false;
            info.description = "Недостаточно данных для расчёта температурного тренда.";
            return info;
        }

        double sumX = 0.0;
        double sumY = 0.0;
        double sumXY = 0.0;
        double sumXX = 0.0;

        for (size_t i = 0; i < points.size(); i++) {
            double x = static_cast<double>(i);
            double y = points[i].value;

            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumXX += x * x;
        }

        double n = static_cast<double>(points.size());
        double denominator = n * sumXX - sumX * sumX;

        if (denominator == 0.0) {
            info.hasTrend = false;
            info.description = "Не удалось рассчитать температурный тренд.";
            return info;
        }

        info.slope = (n * sumXY - sumX * sumY) / denominator;
        info.hasTrend = true;
        info.description = getTrendText(info.slope);

        return info;
    }

    std::string makeLineChart(
        const std::vector<ChartPoint>& data,
        const std::string& title,
        const std::string& unit
    ) {
        if (data.empty()) {
            return "<div class='empty-block'>Нет данных для построения графика.</div>";
        }

        double minValue = data[0].value;
        double maxValue = data[0].value;

        for (const ChartPoint& point : data) {
            minValue = std::min(minValue, point.value);
            maxValue = std::max(maxValue, point.value);
        }

        if (std::abs(maxValue - minValue) < 0.0001) {
            maxValue += 1.0;
            minValue -= 1.0;
        }

        const int width = 900;
        const int height = 310;
        const int left = 70;
        const int right = 30;
        const int top = 42;
        const int bottom = 62;

        int chartWidth = width - left - right;
        int chartHeight = height - top - bottom;

        std::ostringstream svg;

        svg << "<div class='chart-card'>";
        svg << "<h3>" << escapeHtml(title) << "</h3>";
        svg << "<svg width='" << width << "' height='" << height << "' viewBox='0 0 "
            << width << " " << height << "'>";

        svg << "<rect x='0' y='0' width='" << width << "' height='" << height
            << "' rx='18' fill='#f8fcff'/>";

        for (int i = 0; i <= 4; i++) {
            double ratio = static_cast<double>(i) / 4.0;
            int y = top + static_cast<int>(ratio * chartHeight);
            double value = maxValue - ratio * (maxValue - minValue);

            svg << "<line x1='" << left << "' y1='" << y
                << "' x2='" << width - right << "' y2='" << y
                << "' stroke='#d8ecf8' stroke-width='1'/>";

            svg << "<text x='15' y='" << y + 5
                << "' font-size='12' fill='#57758a'>"
                << formatDouble(value)
                << "</text>";
        }

        svg << "<polyline fill='none' stroke='#2196d3' stroke-width='3' points='";

        for (size_t i = 0; i < data.size(); i++) {
            double xRatio = 0.0;

            if (data.size() > 1) {
                xRatio = static_cast<double>(i) / static_cast<double>(data.size() - 1);
            }

            double yRatio = (data[i].value - minValue) / (maxValue - minValue);

            int x = left + static_cast<int>(xRatio * chartWidth);
            int y = top + chartHeight - static_cast<int>(yRatio * chartHeight);

            svg << x << "," << y << " ";
        }

        svg << "'/>";

        for (size_t i = 0; i < data.size(); i++) {
            double xRatio = 0.0;

            if (data.size() > 1) {
                xRatio = static_cast<double>(i) / static_cast<double>(data.size() - 1);
            }

            double yRatio = (data[i].value - minValue) / (maxValue - minValue);

            int x = left + static_cast<int>(xRatio * chartWidth);
            int y = top + chartHeight - static_cast<int>(yRatio * chartHeight);

            svg << "<circle cx='" << x << "' cy='" << y
                << "' r='4' fill='#0b76b7'/>";

            if (i == 0 || i == data.size() - 1 || data.size() <= 12) {
                svg << "<text x='" << x - 25 << "' y='" << height - 25
                    << "' font-size='11' fill='#57758a'>"
                    << escapeHtml(data[i].label)
                    << "</text>";
            }
        }

        svg << "<text x='" << width - 120 << "' y='24' font-size='13' fill='#57758a'>"
            << escapeHtml(unit)
            << "</text>";

        svg << "</svg>";
        svg << "</div>";

        return svg.str();
    }

    std::string makeBarChart(
    const std::vector<ChartPoint>& data,
    const std::string& title,
    const std::string& unit
) {
    if (data.empty()) {
        return "<div class='empty-block'>Нет данных для построения диаграммы.</div>";
    }

    double minValue = data[0].value;
    double maxValue = data[0].value;

    for (const ChartPoint& point : data) {
        minValue = std::min(minValue, point.value);
        maxValue = std::max(maxValue, point.value);
    }
    minValue = std::min(minValue, 0.0);
    maxValue = std::max(maxValue, 0.0);

    if (std::abs(maxValue - minValue) < 0.0001) {
        maxValue += 1.0;
        minValue -= 1.0;
    }

    const int width = 900;
    const int height = 330;
    const int left = 70;
    const int right = 30;
    const int top = 42;
    const int bottom = 80;

    int chartWidth = width - left - right;
    int chartHeight = height - top - bottom;

    auto getY = [&](double value) {
        double ratio = (value - minValue) / (maxValue - minValue);
        return top + chartHeight - static_cast<int>(ratio * chartHeight);
    };

    int zeroY = getY(0.0);

    int barGap = 18;
    int barWidth = std::max(
        18,
        (chartWidth - static_cast<int>(data.size()) * barGap) /
        static_cast<int>(data.size())
    );

    std::ostringstream svg;

    svg << "<div class='chart-card'>";
    svg << "<h3>" << escapeHtml(title) << "</h3>";
    svg << "<svg width='" << width << "' height='" << height << "' viewBox='0 0 "
        << width << " " << height << "'>";

    svg << "<rect x='0' y='0' width='" << width << "' height='" << height
        << "' rx='18' fill='#f8fcff'/>";

    for (int i = 0; i <= 4; i++) {
        double ratio = static_cast<double>(i) / 4.0;
        double value = maxValue - ratio * (maxValue - minValue);
        int y = getY(value);

        svg << "<line x1='" << left << "' y1='" << y
            << "' x2='" << width - right << "' y2='" << y
            << "' stroke='#d8ecf8' stroke-width='1'/>";

        svg << "<text x='15' y='" << y + 5
            << "' font-size='12' fill='#57758a'>"
            << formatDouble(value)
            << "</text>";
    }
    svg << "<line x1='" << left << "' y1='" << zeroY
        << "' x2='" << width - right << "' y2='" << zeroY
        << "' stroke='#6aa9c8' stroke-width='2'/>";

    for (size_t i = 0; i < data.size(); i++) {
        int x = left + static_cast<int>(i) * (barWidth + barGap);

        int valueY = getY(data[i].value);
        int y = std::min(valueY, zeroY);
        int barHeight = std::abs(valueY - zeroY);

        svg << "<rect x='" << x << "' y='" << y
            << "' width='" << barWidth << "' height='" << barHeight
            << "' rx='7' fill='#48b9e8'/>";

        svg << "<text x='" << x << "' y='" << height - 40
            << "' font-size='11' fill='#57758a'>"
            << escapeHtml(data[i].label)
            << "</text>";

        svg << "<text x='" << x << "' y='" << y - 6
            << "' font-size='11' fill='#31566d'>"
            << formatDouble(data[i].value)
            << "</text>";
    }

    svg << "<text x='" << width - 120 << "' y='24' font-size='13' fill='#57758a'>"
        << escapeHtml(unit)
        << "</text>";

    svg << "</svg>";
    svg << "</div>";

    return svg.str();
}

    std::string buildConclusion(
        bool hasStats,
        double avgTemp,
        double totalPrecipitation,
        int rainyDaysCount,
        int anomalyCount,
        const TrendInfo& trendInfo
    ) {
        if (!hasStats) {
            return "Для выбранного периода недостаточно данных, чтобы сформировать итоговый аналитический вывод.";
        }

        std::ostringstream text;

        text << "За выбранный период средняя температура составила "
             << formatDouble(avgTemp)
             << " °C. Суммарное количество осадков составило "
             << formatDouble(totalPrecipitation)
             << " мм, количество дней с осадками — "
             << rainyDaysCount
             << ". ";

        if (anomalyCount == 0) {
            text << "Температурные аномалии за выбранный период не обнаружены. ";
        } else {
            text << "Система обнаружила "
                 << anomalyCount
                 << " аномальных температурных дней. ";
        }

        if (trendInfo.hasTrend) {
            text << trendInfo.description << " ";
        }

        text << "В разделе прогноза представлены ожидаемые значения температуры "
                "и вероятности осадков, рассчитанные методом скользящего среднего.";

        return text.str();
    }
}

ReportService::ReportService(Database& db) : db(db) {}

std::string ReportService::createHtmlReportFile(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate
) {
    pqxx::work tx(db.getConnection());

    pqxx::result stats = tx.exec_params(
        "SELECT "
        "AVG(o.temperature) AS avg_temp, "
        "MIN(o.temperature) AS min_temp, "
        "MAX(o.temperature) AS max_temp, "
        "AVG(o.humidity) AS avg_humidity, "
        "SUM(o.precipitation) AS total_precipitation "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3",
        cityName,
        startDate,
        endDate
    );

    pqxx::result dailyTemp = tx.exec_params(
        "SELECT "
        "o.observation_time::date AS day_date, "
        "AVG(o.temperature) AS avg_temp "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "AND o.temperature IS NOT NULL "
        "GROUP BY o.observation_time::date "
        "ORDER BY o.observation_time::date",
        cityName,
        startDate,
        endDate
    );

    pqxx::result monthlyTemp = tx.exec_params(
        "SELECT "
        "EXTRACT(YEAR FROM o.observation_time)::int AS year_num, "
        "EXTRACT(MONTH FROM o.observation_time)::int AS month_num, "
        "AVG(o.temperature) AS avg_temp "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "AND o.temperature IS NOT NULL "
        "GROUP BY year_num, month_num "
        "ORDER BY year_num, month_num",
        cityName,
        startDate,
        endDate
    );

    pqxx::result monthlyPrecipitation = tx.exec_params(
        "SELECT "
        "EXTRACT(YEAR FROM o.observation_time)::int AS year_num, "
        "EXTRACT(MONTH FROM o.observation_time)::int AS month_num, "
        "SUM(o.precipitation) AS total_precipitation "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "GROUP BY year_num, month_num "
        "ORDER BY year_num, month_num",
        cityName,
        startDate,
        endDate
    );

    pqxx::result seasonalTemp = tx.exec_params(
        "SELECT "
        "CASE "
        "    WHEN EXTRACT(MONTH FROM o.observation_time) IN (12, 1, 2) THEN 'Зима' "
        "    WHEN EXTRACT(MONTH FROM o.observation_time) IN (3, 4, 5) THEN 'Весна' "
        "    WHEN EXTRACT(MONTH FROM o.observation_time) IN (6, 7, 8) THEN 'Лето' "
        "    ELSE 'Осень' "
        "END AS season_name, "
        "CASE "
        "    WHEN EXTRACT(MONTH FROM o.observation_time) IN (12, 1, 2) THEN 1 "
        "    WHEN EXTRACT(MONTH FROM o.observation_time) IN (3, 4, 5) THEN 2 "
        "    WHEN EXTRACT(MONTH FROM o.observation_time) IN (6, 7, 8) THEN 3 "
        "    ELSE 4 "
        "END AS season_order, "
        "AVG(o.temperature) AS avg_temp "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "AND o.temperature IS NOT NULL "
        "GROUP BY season_name, season_order "
        "ORDER BY season_order",
        cityName,
        startDate,
        endDate
    );

    pqxx::result rainyDays = tx.exec_params(
        "SELECT COUNT(DISTINCT o.observation_time::date) AS days_count "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "AND o.precipitation > 0",
        cityName,
        startDate,
        endDate
    );

    pqxx::result anomalies = tx.exec_params(
        "SELECT "
        "o.observation_time::date AS observation_date, "
        "a.anomaly_type, "
        "a.actual_value, "
        "a.expected_value, "
        "a.deviation "
        "FROM anomalies a "
        "JOIN weather_observations o ON o.id = a.observation_id "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "ORDER BY o.observation_time::date",
        cityName,
        startDate,
        endDate
    );

    pqxx::result tempForecast = tx.exec_params(
        "SELECT forecast_date, predicted_value "
        "FROM forecast_results f "
        "JOIN cities c ON c.id = f.city_id "
        "WHERE c.name = $1 "
        "AND f.parameter_name = 'temperature' "
        "AND f.forecast_date > $2::date "
        "AND f.forecast_date <= $2::date + INTERVAL '7 days' "
        "ORDER BY forecast_date",
        cityName,
        endDate
    );

    pqxx::result precipitationForecast = tx.exec_params(
        "SELECT forecast_date, predicted_value "
        "FROM forecast_results f "
        "JOIN cities c ON c.id = f.city_id "
        "WHERE c.name = $1 "
        "AND f.parameter_name = 'precipitation_probability' "
        "AND f.forecast_date > $2::date "
        "AND f.forecast_date <= $2::date + INTERVAL '7 days' "
        "ORDER BY forecast_date",
        cityName,
        endDate
    );

    pqxx::result cityTemperatureComparison = tx.exec_params(
        "SELECT "
        "c.name AS city_name, "
        "AVG(o.temperature) AS avg_temp "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE o.observation_time::date BETWEEN $1 AND $2 "
        "AND o.temperature IS NOT NULL "
        "GROUP BY c.name "
        "ORDER BY avg_temp DESC",
        startDate,
        endDate
    );

    pqxx::result cityPrecipitationComparison = tx.exec_params(
        "SELECT "
        "c.name AS city_name, "
        "SUM(o.precipitation) AS total_precipitation "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE o.observation_time::date BETWEEN $1 AND $2 "
        "GROUP BY c.name "
        "ORDER BY total_precipitation DESC",
        startDate,
        endDate
    );

    tx.commit();

    std::vector<ChartPoint> dailyTempPoints;
    std::vector<ChartPoint> monthlyTempPoints;
    std::vector<ChartPoint> monthlyPrecipitationPoints;
    std::vector<ChartPoint> seasonalTempPoints;
    std::vector<ChartPoint> tempForecastPoints;
    std::vector<ChartPoint> precipitationForecastPoints;
    std::vector<ChartPoint> cityTemperaturePoints;
    std::vector<ChartPoint> cityPrecipitationPoints;

    for (const auto& row : dailyTemp) {
        dailyTempPoints.push_back({
            row["day_date"].as<std::string>(),
            row["avg_temp"].as<double>()
        });
    }

    for (const auto& row : monthlyTemp) {
        int month = row["month_num"].as<int>();
        int year = row["year_num"].as<int>();

        monthlyTempPoints.push_back({
            getMonthName(month) + " " + std::to_string(year),
            row["avg_temp"].as<double>()
        });
    }

    for (const auto& row : monthlyPrecipitation) {
        int month = row["month_num"].as<int>();
        int year = row["year_num"].as<int>();

        double value = 0.0;

        if (!row["total_precipitation"].is_null()) {
            value = row["total_precipitation"].as<double>();
        }

        monthlyPrecipitationPoints.push_back({
            getMonthName(month) + " " + std::to_string(year),
            value
        });
    }

    for (const auto& row : seasonalTemp) {
        seasonalTempPoints.push_back({
            row["season_name"].as<std::string>(),
            row["avg_temp"].as<double>()
        });
    }

    for (const auto& row : tempForecast) {
        tempForecastPoints.push_back({
            row["forecast_date"].as<std::string>(),
            row["predicted_value"].as<double>()
        });
    }

    for (const auto& row : precipitationForecast) {
        precipitationForecastPoints.push_back({
            row["forecast_date"].as<std::string>(),
            row["predicted_value"].as<double>()
        });
    }

    for (const auto& row : cityTemperatureComparison) {
        if (!row["avg_temp"].is_null()) {
            cityTemperaturePoints.push_back({
                row["city_name"].as<std::string>(),
                row["avg_temp"].as<double>()
            });
        }
    }

    for (const auto& row : cityPrecipitationComparison) {
        double value = 0.0;

        if (!row["total_precipitation"].is_null()) {
            value = row["total_precipitation"].as<double>();
        }

        cityPrecipitationPoints.push_back({
            row["city_name"].as<std::string>(),
            value
        });
    }

    double avgTemp = 0.0;
    double minTemp = 0.0;
    double maxTemp = 0.0;
    double avgHumidity = 0.0;
    double totalPrecipitation = 0.0;
    int rainyDaysCount = 0;

    bool hasStats = !stats.empty() && !stats[0]["avg_temp"].is_null();

    if (hasStats) {
        avgTemp = stats[0]["avg_temp"].as<double>();
        minTemp = stats[0]["min_temp"].as<double>();
        maxTemp = stats[0]["max_temp"].as<double>();

        if (!stats[0]["avg_humidity"].is_null()) {
            avgHumidity = stats[0]["avg_humidity"].as<double>();
        }

        if (!stats[0]["total_precipitation"].is_null()) {
            totalPrecipitation = stats[0]["total_precipitation"].as<double>();
        }
    }

    if (!rainyDays.empty() && !rainyDays[0]["days_count"].is_null()) {
        rainyDaysCount = rainyDays[0]["days_count"].as<int>();
    }

    int anomalyCount = static_cast<int>(anomalies.size());
    TrendInfo trendInfo = calculateTrend(dailyTempPoints);

    std::filesystem::create_directories("reports");

    std::string fileName =
        "reports/weather_report_" +
        makeFileNameSafe(cityName) + "_" +
        makeFileNameSafe(startDate) + "_" +
        makeFileNameSafe(endDate) +
        ".html";

    std::ofstream report(fileName);

    if (!report.is_open()) {
        return "";
    }

    report << "<!DOCTYPE html>\n";
    report << "<html lang='ru'>\n";
    report << "<head>\n";
    report << "<meta charset='UTF-8'>\n";
    report << "<title>Аналитический отчёт по погоде</title>\n";

    report << "<style>\n";
    report << "@page { size: A4; margin: 12mm; }\n";
    report << "body { margin: 0; font-family: Arial, sans-serif; background: #eef8fd; color: #17384d; }\n";
    report << ".page { width: 980px; margin: 0 auto; background: white; border-radius: 24px; overflow: hidden; box-shadow: 0 18px 45px rgba(30, 117, 160, 0.18); }\n";
    report << ".hero { background: linear-gradient(135deg, #5cc6f2, #1678b8); color: white; padding: 34px 42px; }\n";
    report << ".hero h1 { margin: 0 0 12px 0; font-size: 34px; letter-spacing: 0.4px; }\n";
    report << ".hero p { margin: 5px 0; font-size: 16px; opacity: 0.95; }\n";
    report << ".section { padding: 26px 42px; border-bottom: 1px solid #d9edf7; page-break-inside: avoid; }\n";
    report << ".section h2 { margin: 0 0 18px 0; font-size: 23px; color: #176f9f; }\n";
    report << ".section h3 { color: #176f9f; }\n";
    report << ".cards { display: grid; grid-template-columns: repeat(4, 1fr); gap: 14px; }\n";
    report << ".card { background: #f3fbff; border: 1px solid #cdebf8; border-radius: 18px; padding: 16px; }\n";
    report << ".card .label { font-size: 12px; color: #5c7d90; margin-bottom: 8px; }\n";
    report << ".card .value { font-size: 22px; font-weight: bold; color: #0c6fa7; }\n";
    report << "table { width: 100%; border-collapse: collapse; margin-top: 14px; font-size: 14px; }\n";
    report << "th { background: #dff4fd; color: #145d84; text-align: left; padding: 11px; }\n";
    report << "td { padding: 10px 11px; border-bottom: 1px solid #e3f1f7; }\n";
    report << "tr:nth-child(even) td { background: #f7fcff; }\n";
    report << ".chart-card { margin-top: 16px; padding: 18px; background: #f8fcff; border: 1px solid #d5edf8; border-radius: 22px; page-break-inside: avoid; }\n";
    report << ".chart-card h3 { margin: 0 0 10px 0; color: #176f9f; font-size: 18px; }\n";
    report << ".empty-block { padding: 18px; background: #f7fcff; border: 1px dashed #9fd5ef; border-radius: 16px; color: #5d7d91; }\n";
    report << ".note { background: #ecf9ff; border-left: 5px solid #4cb9e8; padding: 15px 17px; border-radius: 14px; color: #31566d; line-height: 1.5; }\n";
    report << ".footer { padding: 22px 42px; color: #66879a; font-size: 13px; background: #f5fbfe; }\n";
    report << "</style>\n";

    report << "</head>\n";
    report << "<body>\n";
    report << "<div class='page'>\n";

    report << "<div class='hero'>\n";
    report << "<h1>Аналитический отчёт по погоде</h1>\n";
    report << "<p>Город: <b>" << escapeHtml(cityName) << "</b></p>\n";
    report << "<p>Период наблюдений: " << escapeHtml(startDate) << " — " << escapeHtml(endDate) << "</p>\n";
    report << "<p>Источник данных: Open-Meteo Historical Weather API</p>\n";
    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>1. Общая сводка</h2>\n";

    if (!hasStats) {
        report << "<div class='empty-block'>За выбранный период метеорологические наблюдения не найдены.</div>\n";
    } else {
        report << "<div class='cards'>\n";

        report << "<div class='card'><div class='label'>Средняя температура</div><div class='value'>"
               << formatDouble(avgTemp) << " °C</div></div>\n";

        report << "<div class='card'><div class='label'>Минимальная температура</div><div class='value'>"
               << formatDouble(minTemp) << " °C</div></div>\n";

        report << "<div class='card'><div class='label'>Максимальная температура</div><div class='value'>"
               << formatDouble(maxTemp) << " °C</div></div>\n";

        report << "<div class='card'><div class='label'>Дней с осадками</div><div class='value'>"
               << rainyDaysCount << "</div></div>\n";

        report << "</div>\n";

        report << "<p class='note'>Отчёт построен на основе почасовых метеорологических наблюдений, "
                  "загруженных из открытого источника и сохранённых в базе данных PostgreSQL. "
                  "Температурные показатели рассчитаны по средним, минимальным и максимальным значениям. "
                  "День считается днём с осадками, если хотя бы в одном наблюдении за этот день количество осадков было больше нуля.</p>\n";
    }

    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>2. Изменение температуры</h2>\n";
    report << makeLineChart(dailyTempPoints, "Средняя дневная температура", "°C");
    report << makeLineChart(monthlyTempPoints, "Средняя месячная температура", "°C");
    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>3. Сезонный анализ температуры</h2>\n";
    report << makeBarChart(seasonalTempPoints, "Средняя температура по сезонам", "°C");

    if (!seasonalTempPoints.empty()) {
        report << "<table>\n";
        report << "<tr><th>Сезон</th><th>Средняя температура, °C</th></tr>\n";

        for (const ChartPoint& point : seasonalTempPoints) {
            report << "<tr>";
            report << "<td>" << escapeHtml(point.label) << "</td>";
            report << "<td>" << formatDouble(point.value) << "</td>";
            report << "</tr>\n";
        }

        report << "</table>\n";
    }

    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>4. Долгосрочный температурный тренд</h2>\n";

    if (trendInfo.hasTrend) {
        report << "<div class='cards'>\n";
        report << "<div class='card'><div class='label'>Коэффициент тренда</div><div class='value'>"
               << formatDouble(trendInfo.slope)
               << " °C/день</div></div>\n";
        report << "</div>\n";
        report << "<p class='note'>" << escapeHtml(trendInfo.description) << "</p>\n";
    } else {
        report << "<div class='empty-block'>" << escapeHtml(trendInfo.description) << "</div>\n";
    }

    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>5. Осадки</h2>\n";
    report << "<div class='cards'>\n";
    report << "<div class='card'><div class='label'>Суммарные осадки</div><div class='value'>"
           << formatDouble(totalPrecipitation) << " мм</div></div>\n";
    report << "<div class='card'><div class='label'>Средняя влажность</div><div class='value'>"
           << formatDouble(avgHumidity) << "%</div></div>\n";
    report << "</div>\n";
    report << makeBarChart(monthlyPrecipitationPoints, "Осадки по месяцам", "мм");
    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>6. Сравнение городов</h2>\n";
    report << makeBarChart(cityTemperaturePoints, "Средняя температура по городам", "°C");
    report << makeBarChart(cityPrecipitationPoints, "Суммарные осадки по городам", "мм");
    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>7. Таблица аналитических показателей</h2>\n";
    report << "<table>\n";
    report << "<tr><th>Месяц</th><th>Средняя температура, °C</th><th>Осадки, мм</th></tr>\n";

    for (size_t i = 0; i < monthlyTempPoints.size(); i++) {
        std::string monthLabel = monthlyTempPoints[i].label;
        double tempValue = monthlyTempPoints[i].value;
        std::string precipitationText = "-";

        for (const ChartPoint& point : monthlyPrecipitationPoints) {
            if (point.label == monthLabel) {
                precipitationText = formatDouble(point.value);
                break;
            }
        }

        report << "<tr>";
        report << "<td>" << escapeHtml(monthLabel) << "</td>";
        report << "<td>" << formatDouble(tempValue) << "</td>";
        report << "<td>" << precipitationText << "</td>";
        report << "</tr>\n";
    }

    report << "</table>\n";
    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>8. Найденные аномальные дни</h2>\n";

    if (anomalies.empty()) {
        report << "<div class='empty-block'>За выбранный период аномальные дни не найдены.</div>\n";
    } else {
        report << "<table>\n";
        report << "<tr><th>Дата</th><th>Тип аномалии</th><th>Фактическое значение</th><th>Ожидаемое значение</th><th>Отклонение</th></tr>\n";

        for (const auto& row : anomalies) {
            std::string type = row["anomaly_type"].as<std::string>();

            if (type == "HIGH_TEMPERATURE_ANOMALY") {
                type = "Температура выше ожидаемой";
            } else if (type == "LOW_TEMPERATURE_ANOMALY") {
                type = "Температура ниже ожидаемой";
            }

            report << "<tr>";
            report << "<td>" << row["observation_date"].as<std::string>() << "</td>";
            report << "<td>" << escapeHtml(type) << "</td>";
            report << "<td>" << formatDouble(row["actual_value"].as<double>()) << " °C</td>";
            report << "<td>" << formatDouble(row["expected_value"].as<double>()) << " °C</td>";
            report << "<td>" << formatDouble(row["deviation"].as<double>()) << " °C</td>";
            report << "</tr>\n";
        }

        report << "</table>\n";
    }

    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>9. Результаты прогноза</h2>\n";
    report << "<p class='note'>Прогноз строится на 7 дней после окончания выбранного периода наблюдений.</p>\n";
    report << makeLineChart(tempForecastPoints, "Прогноз температуры", "°C");
    report << makeBarChart(precipitationForecastPoints, "Прогноз вероятности осадков", "%");

    report << "<h3>Таблица прогноза</h3>\n";
    report << "<table>\n";
    report << "<tr><th>Дата</th><th>Прогноз температуры, °C</th><th>Вероятность осадков, %</th></tr>\n";

    size_t maxRows = std::max(tempForecastPoints.size(), precipitationForecastPoints.size());

    for (size_t i = 0; i < maxRows; i++) {
        std::string date = "-";
        std::string tempText = "-";
        std::string rainText = "-";

        if (i < tempForecastPoints.size()) {
            date = tempForecastPoints[i].label;
            tempText = formatDouble(tempForecastPoints[i].value);
        }

        if (i < precipitationForecastPoints.size()) {
            if (date == "-") {
                date = precipitationForecastPoints[i].label;
            }

            rainText = formatDouble(precipitationForecastPoints[i].value);
        }

        report << "<tr>";
        report << "<td>" << escapeHtml(date) << "</td>";
        report << "<td>" << tempText << "</td>";
        report << "<td>" << rainText << "</td>";
        report << "</tr>\n";
    }

    report << "</table>\n";
    report << "</div>\n";

    report << "<div class='section'>\n";
    report << "<h2>10. Итоговый аналитический вывод</h2>\n";
    report << "<p class='note'>"
           << escapeHtml(buildConclusion(
               hasStats,
               avgTemp,
               totalPrecipitation,
               rainyDaysCount,
               anomalyCount,
               trendInfo
           ))
           << "</p>\n";
    report << "</div>\n";

    report << "<div class='footer'>\n";
    report << "Отчёт сформирован системой Weather Analyzer на основе данных Open-Meteo, сохранённых в PostgreSQL.\n";
    report << "</div>\n";

    report << "</div>\n";
    report << "</body>\n";
    report << "</html>\n";

    report.close();

    return fileName;
}

bool ReportService::convertHtmlToPdf(
    const std::string& htmlFileName,
    const std::string& pdfFileName
) {
    std::string chromePath = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";

    if (!std::filesystem::exists(chromePath)) {
        std::cout << "\nGoogle Chrome was not found." << std::endl;
        std::cout << "Install it using:" << std::endl;
        std::cout << "brew install --cask google-chrome" << std::endl;
        return false;
    }

    std::string htmlUrl = makeFileUrl(htmlFileName);

    std::string command =
        quoteCommandArgument(chromePath) +
        " --headless=new"
        " --disable-gpu"
        " --no-pdf-header-footer"
        " --print-to-pdf=" + quoteCommandArgument(std::filesystem::absolute(pdfFileName).string()) +
        " " + quoteCommandArgument(htmlUrl);

    int result = std::system(command.c_str());

    if (result != 0) {
        std::cout << "\nPDF conversion failed." << std::endl;
        std::cout << "Command was:" << std::endl;
        std::cout << command << std::endl;
        return false;
    }

    return std::filesystem::exists(pdfFileName);
}

void ReportService::createPdfReport(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate
) {
    std::string htmlFileName = createHtmlReportFile(
        cityName,
        startDate,
        endDate
    );

    if (htmlFileName.empty()) {
        std::cout << "\nCannot create HTML report." << std::endl;
        return;
    }

    std::string pdfFileName = htmlFileName;

    if (pdfFileName.ends_with(".html")) {
        pdfFileName = pdfFileName.substr(0, pdfFileName.size() - 5) + ".pdf";
    } else {
        pdfFileName += ".pdf";
    }

    bool converted = convertHtmlToPdf(
        htmlFileName,
        pdfFileName
    );

    if (!converted) {
        std::cout << "\nHTML report was created, but PDF was not generated." << std::endl;
        std::cout << "HTML file: " << htmlFileName << std::endl;
        return;
    }

    std::cout << "\nPDF report created successfully." << std::endl;
    std::cout << "PDF file: " << pdfFileName << std::endl;
    std::cout << "Intermediate HTML file: " << htmlFileName << std::endl;
}