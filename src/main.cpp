#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>


// Функция экранирования HTML-символов
std::string html_escape(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

// Наибольшая общая подпоследовательность (для посимвольного diff)
std::vector<std::pair<int,int>> lcs_backtrack(const std::string &a, const std::string &b) {
    int n = (int)a.size(), m = (int)b.size();
    std::vector<std::vector<int>> dp(n+1, std::vector<int>(m+1, 0));
    for (int i = n-1; i >= 0; --i)
        for (int j = m-1; j >= 0; --j)
            dp[i][j] = (a[i] == b[j]) ? dp[i+1][j+1] + 1 : std::max(dp[i+1][j], dp[i][j+1]);

    std::vector<std::pair<int,int>> res;
    int i = 0, j = 0;
    while (i < n && j < m) {
        if (a[i] == b[j]) { res.emplace_back(i, j); ++i; ++j; }
        else if (dp[i+1][j] >= dp[i][j+1]) ++i;
        else ++j;
    }
    return res;
}

// Посимвольный diff: рендер старой строки (подсвечивает только удаления)
std::string render_old_with_deletions(const std::string &oldLine, const std::string &newLine) {
    auto matches = lcs_backtrack(oldLine, newLine);
    std::string out;
    int ia = 0;
    size_t idx = 0;
    while (idx < matches.size()) {
        int ma = matches[idx].first;
        // удалённые символы в старой строке до совпадения
        if (ia < ma) {
            out += "<span class='del'>" + html_escape(oldLine.substr(ia, ma - ia)) + "</span>";
        }
        // общий символ
        out += html_escape(std::string(1, oldLine[ma]));
        ia = ma + 1;
        ++idx;
    }
    // хвост удалённых символов в старой строке (если остались)
    if (ia < (int)oldLine.size()) {
        out += "<span class='del'>" + html_escape(oldLine.substr(ia)) + "</span>";
    }
    return out;
}

// Посимвольный diff: рендер новой строки (подсвечивает только вставки)
std::string render_new_with_insertions(const std::string &oldLine, const std::string &newLine) {
    auto matches = lcs_backtrack(oldLine, newLine);
    std::string out;
    int ib = 0;
    size_t idx = 0;
    while (idx < matches.size()) {
        int mb = matches[idx].second;
        // вставленные символы в новой строке до совпадения
        if (ib < mb) {
            out += "<span class='ins'>" + html_escape(newLine.substr(ib, mb - ib)) + "</span>";
        }
        // общий символ (из новой строки)
        out += html_escape(std::string(1, newLine[mb]));
        ib = mb + 1;
        ++idx;
    }
    // хвост вставленных символов в новой строке (если остались)
    if (ib < (int)newLine.size()) {
        out += "<span class='ins'>" + html_escape(newLine.substr(ib)) + "</span>";
    }
    return out;
}

// Основная функция diff → HTML
int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " old.txt new.txt output.html\n";
        return 1;
    }

    std::ifstream f1(argv[1]), f2(argv[2]);
    if (!f1.is_open()) { std::cerr << "Cannot open file: " << argv[1] << std::endl; return 1; }
    if (!f2.is_open()) { std::cerr << "Cannot open file: " << argv[2] << std::endl; return 1; }

    std::vector<std::string> oldLines, newLines;
    std::string line;
    while (std::getline(f1, line)) oldLines.push_back(line);
    while (std::getline(f2, line)) newLines.push_back(line);

    std::ofstream out(argv[3]);
    out << "<html><head><meta charset='UTF-8'><style>\n"
        << "body { font-family: monospace; }\n"
        << "table { width: 100%; border-collapse: collapse; }\n"
        << "td { vertical-align: top; padding: 2px 8px; }\n"
        << "th { background: #f0f0f0; padding: 4px; }\n"
        << ".del { background:#ffecec; text-decoration:line-through; color:#a33; }\n"
        << ".ins { background:#eaffea; color:#070; }\n"
        << ".removed { background:#ffeeee; }\n"
        << ".added { background:#eeffee; }\n"
        << "</style></head><body>\n";

    // Заголовок с названиями файлов
    out << "<h2>Diff between: "
        << html_escape(argv[1]) << " (old) and "
        << html_escape(argv[2]) << " (new)</h2>\n";

    out << "<table border='1'>\n"
        << "<tr><th>" << html_escape(argv[1]) << " (old)</th>"
        << "<th>" << html_escape(argv[2]) << " (new)</th></tr>\n";

    size_t i = 0, j = 0;
    while (i < oldLines.size() || j < newLines.size()) {
        std::string left, right;

        if (i < oldLines.size() && j < newLines.size()) {
            if (oldLines[i] == newLines[j]) {
                left = html_escape(oldLines[i]);
                right = html_escape(newLines[j]);
                out << "<tr><td>" << left << "</td><td>" << right << "</td></tr>\n";
                ++i; ++j;
            } else {
                // теперь: слева — старое с удалениями, справа — новое с вставками
                left = render_old_with_deletions(oldLines[i], newLines[j]);
                right = render_new_with_insertions(oldLines[i], newLines[j]);
                out << "<tr><td class='removed'>" << left << "</td>"
                    << "<td class='added'>" << right << "</td></tr>\n";
                ++i; ++j;
            }
        } else if (i < oldLines.size()) {
            // остаток только в старом файле
            left = html_escape(oldLines[i]);
            out << "<tr><td class='removed'>" << left << "</td><td></td></tr>\n";
            ++i;
        } else if (j < newLines.size()) {
            // остаток только в новом файле
            right = html_escape(newLines[j]);
            out << "<tr><td></td><td class='added'>" << right << "</td></tr>\n";
            ++j;
        }
    }

    out << "</table></body></html>";
    out.close();

    std::cout << "Diff saved to " << argv[3] << std::endl;
    return 0;
}