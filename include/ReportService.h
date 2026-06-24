#pragma once

#include "Database.h"

#include <string>

class ReportService {
private:
    Database& db;

    std::string createHtmlReportFile(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate
    );

    bool convertHtmlToPdf(
        const std::string& htmlFileName,
        const std::string& pdfFileName
    );

public:
    explicit ReportService(Database& db);

    void createPdfReport(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate
    );
};