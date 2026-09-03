#pragma once

// Minimal, dependency-free JSON value type used as the human-inspectable
// wire format for save.hpp. This is intentionally not a general-purpose
// JSON library: it supports exactly the value shapes the save schema needs
// (null, bool, number, string, array, object) and fails clearly on
// malformed input rather than attempting partial recovery, matching
// docs/SAVE_FORMAT.md's "unknown or unsupported ... fail clearly" rule.

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace everward::simulation {

class JsonValue {
public:
    struct Null {};
    using Array = std::vector<JsonValue>;
    using Object = std::vector<std::pair<std::string, JsonValue>>;

    JsonValue() : data_(Null{}) {}
    JsonValue(bool value) : data_(value) {}
    JsonValue(double value) : data_(value) {}
    JsonValue(std::int64_t value) : data_(static_cast<double>(value)) {}
    JsonValue(std::string value) : data_(std::move(value)) {}
    JsonValue(const char* value) : data_(std::string(value)) {}
    JsonValue(Array value) : data_(std::move(value)) {}
    JsonValue(Object value) : data_(std::move(value)) {}

    [[nodiscard]] static JsonValue make_object() { return JsonValue(Object{}); }
    [[nodiscard]] static JsonValue make_array() { return JsonValue(Array{}); }

    [[nodiscard]] bool is_null() const noexcept { return std::holds_alternative<Null>(data_); }
    [[nodiscard]] bool is_object() const noexcept { return std::holds_alternative<Object>(data_); }
    [[nodiscard]] bool is_array() const noexcept { return std::holds_alternative<Array>(data_); }
    [[nodiscard]] bool is_string() const noexcept { return std::holds_alternative<std::string>(data_); }
    [[nodiscard]] bool is_number() const noexcept { return std::holds_alternative<double>(data_); }
    [[nodiscard]] bool is_bool() const noexcept { return std::holds_alternative<bool>(data_); }

    void set(std::string key, JsonValue value) {
        if (!is_object()) {
            throw std::logic_error("JsonValue::set called on a non-object value");
        }
        auto& object = std::get<Object>(data_);
        object.emplace_back(std::move(key), std::move(value));
    }

    void push_back(JsonValue value) {
        if (!is_array()) {
            throw std::logic_error("JsonValue::push_back called on a non-array value");
        }
        std::get<Array>(data_).push_back(std::move(value));
    }

    [[nodiscard]] const JsonValue* find(const std::string& key) const {
        if (!is_object()) {
            return nullptr;
        }
        for (const auto& [existing_key, existing_value] : std::get<Object>(data_)) {
            if (existing_key == key) {
                return &existing_value;
            }
        }
        return nullptr;
    }

    // Required field lookup: throws with a schema-identifying message rather
    // than returning a default so a missing/malformed save field fails
    // clearly instead of silently loading a fabricated zero/empty value.
    [[nodiscard]] const JsonValue& require(const std::string& key) const {
        const JsonValue* found = find(key);
        if (found == nullptr) {
            throw std::runtime_error("save data missing required field: " + key);
        }
        return *found;
    }

    [[nodiscard]] const Array& as_array() const {
        if (!is_array()) {
            throw std::runtime_error("save data field is not an array");
        }
        return std::get<Array>(data_);
    }

    [[nodiscard]] const std::string& as_string() const {
        if (!is_string()) {
            throw std::runtime_error("save data field is not a string");
        }
        return std::get<std::string>(data_);
    }

    [[nodiscard]] double as_double() const {
        if (!is_number()) {
            throw std::runtime_error("save data field is not a number");
        }
        return std::get<double>(data_);
    }

    [[nodiscard]] std::int64_t as_int64() const {
        const double value = as_double();
        const double rounded = std::round(value);
        if (std::abs(value - rounded) > 1e-6) {
            throw std::runtime_error("save data field is not an integral number");
        }
        return static_cast<std::int64_t>(rounded);
    }

    [[nodiscard]] bool as_bool() const {
        if (!is_bool()) {
            throw std::runtime_error("save data field is not a boolean");
        }
        return std::get<bool>(data_);
    }

    void write(std::ostream& out, int indent = 0) const {
        if (is_null()) {
            out << "null";
        } else if (is_bool()) {
            out << (std::get<bool>(data_) ? "true" : "false");
        } else if (is_number()) {
            write_number(out, std::get<double>(data_));
        } else if (is_string()) {
            write_string(out, std::get<std::string>(data_));
        } else if (is_array()) {
            write_array(out, std::get<Array>(data_), indent);
        } else {
            write_object(out, std::get<Object>(data_), indent);
        }
    }

    [[nodiscard]] std::string dump(int indent = 0) const {
        std::ostringstream out;
        write(out, indent);
        return out.str();
    }

    [[nodiscard]] static JsonValue parse(const std::string& text) {
        Parser parser(text);
        JsonValue value = parser.parse_value();
        parser.skip_whitespace();
        if (!parser.at_end()) {
            throw std::runtime_error("save data has trailing content after top-level value");
        }
        return value;
    }

private:
    std::variant<Null, bool, double, std::string, Array, Object> data_;

    static void indent_line(std::ostream& out, int indent) {
        out << '\n';
        for (int i = 0; i < indent; ++i) {
            out << "  ";
        }
    }

    static void write_string(std::ostream& out, const std::string& value) {
        out << '"';
        for (const unsigned char ch : value) {
            switch (ch) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (ch < 0x20) {
                        char buffer[8];
                        std::snprintf(buffer, sizeof(buffer), "\\u%04x", ch);
                        out << buffer;
                    } else {
                        out << static_cast<char>(ch);
                    }
            }
        }
        out << '"';
    }

    static void write_number(std::ostream& out, double value) {
        if (!std::isfinite(value)) {
            throw std::runtime_error("cannot serialize a non-finite number");
        }
        const double rounded = std::round(value);
        if (std::abs(value - rounded) < 1e-9 && std::abs(rounded) < 1e15) {
            out << static_cast<std::int64_t>(rounded);
            return;
        }
        std::ostringstream formatted;
        formatted.precision(17);
        formatted << value;
        out << formatted.str();
    }

    static void write_array(std::ostream& out, const Array& array, int indent) {
        if (array.empty()) {
            out << "[]";
            return;
        }
        out << '[';
        bool first = true;
        for (const auto& element : array) {
            if (!first) {
                out << ',';
            }
            first = false;
            indent_line(out, indent + 1);
            element.write(out, indent + 1);
        }
        indent_line(out, indent);
        out << ']';
    }

    static void write_object(std::ostream& out, const Object& object, int indent) {
        if (object.empty()) {
            out << "{}";
            return;
        }
        out << '{';
        bool first = true;
        for (const auto& [key, value] : object) {
            if (!first) {
                out << ',';
            }
            first = false;
            indent_line(out, indent + 1);
            write_string(out, key);
            out << ": ";
            value.write(out, indent + 1);
        }
        indent_line(out, indent);
        out << '}';
    }

    class Parser {
    public:
        explicit Parser(const std::string& text) : text_(text) {}

        void skip_whitespace() {
            while (pos_ < text_.size() &&
                   (text_[pos_] == ' ' || text_[pos_] == '\t' || text_[pos_] == '\n' || text_[pos_] == '\r')) {
                ++pos_;
            }
        }

        [[nodiscard]] bool at_end() const noexcept { return pos_ >= text_.size(); }

        JsonValue parse_value() {
            skip_whitespace();
            if (at_end()) {
                throw std::runtime_error("unexpected end of save data");
            }
            switch (text_[pos_]) {
                case '{': return parse_object();
                case '[': return parse_array();
                case '"': return JsonValue(parse_string());
                case 't': expect_literal("true"); return JsonValue(true);
                case 'f': expect_literal("false"); return JsonValue(false);
                case 'n': expect_literal("null"); return JsonValue();
                default: return parse_number();
            }
        }

    private:
        const std::string& text_;
        std::size_t pos_{0};

        [[nodiscard]] char peek() const {
            if (at_end()) {
                throw std::runtime_error("unexpected end of save data");
            }
            return text_[pos_];
        }

        char take() {
            const char ch = peek();
            ++pos_;
            return ch;
        }

        void expect(char expected) {
            if (take() != expected) {
                throw std::runtime_error(std::string("expected '") + expected + "' in save data");
            }
        }

        void expect_literal(const char* literal) {
            for (const char* p = literal; *p != '\0'; ++p) {
                expect(*p);
            }
        }

        JsonValue parse_object() {
            expect('{');
            JsonValue object = JsonValue::make_object();
            skip_whitespace();
            if (!at_end() && peek() == '}') {
                take();
                return object;
            }
            while (true) {
                skip_whitespace();
                std::string key = parse_string();
                skip_whitespace();
                expect(':');
                JsonValue value = parse_value();
                object.set(std::move(key), std::move(value));
                skip_whitespace();
                const char separator = take();
                if (separator == '}') {
                    break;
                }
                if (separator != ',') {
                    throw std::runtime_error("expected ',' or '}' in save data object");
                }
            }
            return object;
        }

        JsonValue parse_array() {
            expect('[');
            JsonValue array = JsonValue::make_array();
            skip_whitespace();
            if (!at_end() && peek() == ']') {
                take();
                return array;
            }
            while (true) {
                array.push_back(parse_value());
                skip_whitespace();
                const char separator = take();
                if (separator == ']') {
                    break;
                }
                if (separator != ',') {
                    throw std::runtime_error("expected ',' or ']' in save data array");
                }
            }
            return array;
        }

        std::string parse_string() {
            expect('"');
            std::string result;
            while (true) {
                const char ch = take();
                if (ch == '"') {
                    break;
                }
                if (ch != '\\') {
                    result.push_back(ch);
                    continue;
                }
                const char escaped = take();
                switch (escaped) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'u': {
                        const unsigned int codepoint = parse_hex4();
                        append_utf8(result, codepoint);
                        break;
                    }
                    default:
                        throw std::runtime_error("invalid escape sequence in save data string");
                }
            }
            return result;
        }

        unsigned int parse_hex4() {
            unsigned int value = 0;
            for (int i = 0; i < 4; ++i) {
                const char ch = take();
                value <<= 4;
                if (ch >= '0' && ch <= '9') {
                    value |= static_cast<unsigned int>(ch - '0');
                } else if (ch >= 'a' && ch <= 'f') {
                    value |= static_cast<unsigned int>(ch - 'a' + 10);
                } else if (ch >= 'A' && ch <= 'F') {
                    value |= static_cast<unsigned int>(ch - 'A' + 10);
                } else {
                    throw std::runtime_error("invalid \\u escape in save data string");
                }
            }
            return value;
        }

        static void append_utf8(std::string& out, unsigned int codepoint) {
            if (codepoint <= 0x7F) {
                out.push_back(static_cast<char>(codepoint));
            } else if (codepoint <= 0x7FF) {
                out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
        }

        JsonValue parse_number() {
            const std::size_t start = pos_;
            if (!at_end() && peek() == '-') {
                take();
            }
            if (at_end() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                throw std::runtime_error("invalid number in save data");
            }
            while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
                take();
            }
            if (!at_end() && peek() == '.') {
                take();
                if (at_end() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                    throw std::runtime_error("invalid number in save data");
                }
                while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
                    take();
                }
            }
            if (!at_end() && (peek() == 'e' || peek() == 'E')) {
                take();
                if (!at_end() && (peek() == '+' || peek() == '-')) {
                    take();
                }
                if (at_end() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                    throw std::runtime_error("invalid number in save data");
                }
                while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
                    take();
                }
            }
            const std::string token = text_.substr(start, pos_ - start);
            return JsonValue(std::stod(token));
        }
    };
};

} // namespace everward::simulation
