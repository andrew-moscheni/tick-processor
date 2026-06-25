#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

//define globals locally here (MAKE SEPARATE FILE - INCLUDE IN GITIGNORE!!!)
const char* API_KEY = "YOUR_API_KEY";
const char* API_SECRET_KEY = "YOUR_API_SECRET";
const char* WS_URL = "wss://stream.data.alpaca.markets/v2/iex";

using json = nlohmann::json;

struct BarData {
    std::string symbol;
    double open = 0;
    double close = 0;
    double high = 0;
    double low = 0;
    double volume = 0;
    double trade_count = 0;
    double vwap = 0;
    std::string timestamp;
    bool dirty = false;
};

// two mutexes are required for synchronization
std::mutex data_mtx;
std::map<std::string, BarData> market_data;
std::vector<std::string> symbols;

std::mutex cv_mtx;
std::condition_variable cv;
std::atomic<bool> SETUP_COMPLETE{false};
std::atomic<bool> RUNNING{true};

namespace ansi {
    const char* RESET = "\033[0m";
    const char* BOLD = "\033[1m";
    const char* RED = "\033[31m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";
    const char* CYAN  = "\033[36m";
    const char* WHITE = "\033[37m";
    const char* BRIGHT_WHITE = "\033[97m";

    void clear_screen() { std::cout << "\033[2J\033[H";}
    void cursor_home()  { std::cout << "\033[H";}
    void hide_cursor()  { std::cout << "\033[?25l";}
    void show_cursor()  { std::cout << "\033[?25h";}
    void move_up(int n) { std::cout << "\033[" << n << "A";}
    void clear_line()   { std::cout << "\033[2K";}
}

constexpr int W_SYM = 8;
constexpr int W_PRICE = 12;
constexpr int W_VOL = 14;
constexpr int W_TC = 10;
constexpr int W_VWAP = 12;
constexpr int W_TS = 22;

std::string separator() {
    auto dash = [](int n){ return std::string(n, '-'); };
    return "+" + dash(W_SYM+2)   + "+" + dash(W_PRICE+2) + "+" + dash(W_PRICE+2) +
           "+" + dash(W_PRICE+2) + "+" + dash(W_PRICE+2) + "+" + dash(W_VOL+2)   +
           "+" + dash(W_TC+2)    + "+" + dash(W_VWAP+2)  + "+" + dash(W_TS+2)    + "+\n";
}

std::string header() {
    std::ostringstream o;
    auto col = [](const std::string& s, int w){
        return " " + s + std::string(std::max(0, w - (int)s.size()), ' ') + " ";
    };
    o << "|" << col("SYMBOL",  W_SYM)
      << "|" << col("OPEN",    W_PRICE)
      << "|" << col("CLOSE",   W_PRICE)
      << "|" << col("HIGH",    W_PRICE)
      << "|" << col("LOW",     W_PRICE)
      << "|" << col("VOLUME",  W_VOL)
      << "|" << col("TRADES",  W_TC)
      << "|" << col("VWAP",    W_VWAP)
      << "|" << col("UPDATED", W_TS)
      << "|\n";
    return o.str();
}

std::string format_row(const BarData& b) {
    std::ostringstream o;
    o << std::fixed;

    auto scol = [](const std::string& s, int w){
        std::string t = s.substr(0, w);
        return " " + t + std::string(std::max(0, w - (int)t.size()), ' ') + " ";
    };
    auto fcol = [](double v, int w, int prec = 2){
        std::ostringstream tmp;
        tmp << std::fixed << std::setprecision(prec) << v;
        std::string s = tmp.str();
        return " " + std::string(std::max(0, w - (int)s.size()), ' ') + s + " ";
    };

    std::string close_str;
    {
        std::ostringstream tmp;
        tmp << std::fixed << std::setprecision(2) << b.close;
        close_str = tmp.str();
    }
    const char* close_color = (b.close >= b.open) ? ansi::GREEN : ansi::RED;

    o << "|" << ansi::BOLD << ansi::CYAN << scol(b.symbol, W_SYM) << ansi::RESET
      << "|" << fcol(b.open,        W_PRICE)
      << "|" << close_color << " "
              << std::string(std::max(0, W_PRICE - (int)close_str.size()), ' ')
              << close_str << " " << ansi::RESET
      << "|" << fcol(b.high,        W_PRICE)
      << "|" << fcol(b.low,         W_PRICE)
      << "|" << fcol(b.volume,      W_VOL, 0)
      << "|" << fcol(b.trade_count, W_TC,  0)
      << "|" << fcol(b.vwap,        W_VWAP)
      << "|" << scol(b.timestamp,   W_TS)
      << "|\n";
    return o.str();
}

int table_height(int num_symbols) {
    return 3 + num_symbols * 2 + 1;
}

struct WsState {
    std::string buffer;
};

void handle_message(const std::string& raw) {
    try {
        auto msgs = json::parse(raw);
        if (!msgs.is_array()) return;

        for (auto& msg : msgs) {
            std::string type = msg.value("T", "");

            if (type == "b" || type == "d") {
                std::string sym = msg.value("S", "");
                if (sym.empty()) continue;

                std::lock_guard<std::mutex> lock(data_mtx);
                auto& bar = market_data[sym];
                bar.symbol = sym;
                bar.open = msg.value("o",  0.0);
                bar.close = msg.value("c",  0.0);
                bar.high = msg.value("h",  0.0);
                bar.low = msg.value("l",  0.0);
                bar.volume = msg.value("v",  0.0);
                bar.trade_count = msg.value("n",  0.0);
                bar.vwap = msg.value("vw", 0.0);
                bar.timestamp = msg.value("t",  "");
                if (bar.timestamp.size() > 19) bar.timestamp = bar.timestamp.substr(0, 19);
                bar.dirty = true;
            }

            if (type == "t") {
                std::string sym = msg.value("S", "");
                if (sym.empty()) continue;

                std::lock_guard<std::mutex> lock(data_mtx);
                auto it = market_data.find(sym);
                if (it != market_data.end()) {
                    double price = msg.value("p", 0.0);
                    it->second.close = price;
                    if (price > it->second.high) it->second.high = price;
                    if (price < it->second.low || it->second.low == 0) it->second.low = price;
                    it->second.volume += msg.value("s", 0.0);
                    it->second.trade_count += 1;
                    it->second.timestamp = msg.value("t", it->second.timestamp);
                    if (it->second.timestamp.size() > 19)
                        it->second.timestamp = it->second.timestamp.substr(0, 19);
                    it->second.dirty = true;
                }
            }

            if (type == "success") {
                std::string msg_str = msg.value("msg", "");
                if (msg_str == "authenticated") {
                    std::lock_guard<std::mutex> lock(cv_mtx);
                }
            }
        }
    } catch (...) {}
}

int processor_func() {
    {
        std::lock_guard<std::mutex> lock(data_mtx);
        for (auto& sym : symbols) {
            market_data[sym] = BarData{sym};
        }
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to init curl\n";
        return 1;
    }

    json sub_msg = {
        {"action", "subscribe"},
        {"bars",   symbols},
        {"trades", symbols}
    };

    json auth_msg = {
        {"action", "auth"},
        {"key",    API_KEY},
        {"secret", API_SECRET_KEY}
    };

    curl_easy_setopt(curl, CURLOPT_URL, WS_URL);
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "WebSocket connect failed: " << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
        return 1;
    }

    auto ws_send = [&](const std::string& payload) {
        size_t sent = 0;
        curl_ws_send(curl, payload.c_str(), payload.size(), &sent, 0, CURLWS_TEXT);
    };

    auto ws_recv = [&](int timeout_ms = 3000) -> std::string {
        std::string accumulated;
        char buf[65536];
        const struct curl_ws_frame* meta = nullptr;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

        while (std::chrono::steady_clock::now() < deadline) {
            size_t nread = 0;
            CURLcode rc = curl_ws_recv(curl, buf, sizeof(buf), &nread, &meta);
            if (rc == CURLE_OK && nread > 0) {
                accumulated.append(buf, nread);
                if (meta && !(meta->flags & CURLWS_CONT)) break;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        return accumulated;
    };

    ws_recv(2000);
    ws_send(auth_msg.dump());
    ws_recv(2000);
    ws_send(sub_msg.dump());
    ws_recv(2000);

    {
        std::lock_guard<std::mutex> lock(cv_mtx);
        SETUP_COMPLETE = true;
    }
    cv.notify_one();

    while (RUNNING) {
        std::string frame = ws_recv(500);
        if (!frame.empty()) handle_message(frame);
    }

    curl_easy_cleanup(curl);
    return 0;
}

int client_func() {
    std::unique_lock<std::mutex> lock(cv_mtx);
    cv.wait(lock, [] { return SETUP_COMPLETE.load(); });

    ansi::hide_cursor();
    ansi::clear_screen();

    bool first_frame = true;
    int  last_height = 0;

    while (RUNNING) {
        auto frame_start = std::chrono::steady_clock::now();

        std::ostringstream frame;

        frame << ansi::BOLD << ansi::BRIGHT_WHITE
              << " ╔══ Alpaca Live Market Data ══╗\n" << ansi::RESET;

        frame << separator();
        frame << ansi::BOLD << header() << ansi::RESET;
        frame << separator();

        {
            std::lock_guard<std::mutex> lock(data_mtx);
            for (auto& sym : symbols) {
                auto it = market_data.find(sym);
                if (it != market_data.end()) {
                    frame << format_row(it->second);
                    it->second.dirty = false;
                } else {
                    BarData empty; empty.symbol = sym;
                    frame << format_row(empty);
                }
                frame << separator();
            }
        }

        auto now  = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char tbuf[32];
        std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", std::localtime(&time));
        frame << ansi::YELLOW << " Last render: " << tbuf
              << "  |  Press Ctrl+C to quit" << ansi::RESET << "\n";

        if (first_frame) {
            std::cout << frame.str();
            first_frame = false;
        } else {
            std::cout << "\033[" << last_height << "A\033[J";
            std::cout << frame.str();
        }
        std::cout.flush();

        const std::string& s = frame.str();
        last_height = (int)std::count(s.begin(), s.end(), '\n');

        auto elapsed   = std::chrono::steady_clock::now() - frame_start;
        auto sleep_for = std::chrono::milliseconds(500) - elapsed;
        if (sleep_for > std::chrono::milliseconds(0))
            std::this_thread::sleep_for(sleep_for);
    }

    ansi::show_cursor();
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " AAPL MSFT TSLA ...\n";
        return 1;
    }

    for (int i = 1; i < argc; ++i)
        symbols.push_back(std::string(argv[i]));

    curl_global_init(CURL_GLOBAL_ALL);

    std::signal(SIGINT, [](int) {
        RUNNING = false;
        ansi::show_cursor();
        std::cout << "\n\nShutting down...\n";
    });

    std::thread processor(processor_func);
    std::thread client(client_func);

    processor.join();
    client.join();

    // close gracefully on disconnect
    curl_global_cleanup();
    return 0;
}