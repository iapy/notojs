#include <notojs/parser/multipart.hpp>

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <utility>

namespace notojs::parser {
namespace {

using Parameters = std::unordered_map<std::string, std::string>;
using Headers = std::unordered_map<std::string, std::string_view>;

struct Delimiter
{
    std::size_t start;
    std::size_t next;
    bool closing;
};

Multipart failure(std::string error)
{
    return Multipart{{}, std::move(error)};
}

bool starts_with(std::string_view value, std::size_t offset, std::string_view prefix)
{
    return offset <= value.size() && prefix.size() <= value.size() - offset &&
        value.compare(offset, prefix.size(), prefix) == 0;
}

std::string_view trim_view(std::string_view value)
{
    while(!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.remove_prefix(1);
    while(!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.remove_suffix(1);
    return value;
}

std::string trim(std::string_view value)
{
    return std::string(trim_view(value));
}

std::string lower(std::string_view value)
{
    std::string result(value);
    std::transform(std::begin(result), std::end(result), std::begin(result), [](unsigned char ch){
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

bool equal(std::string_view left, std::string_view right)
{
    return left.size() == right.size() && std::equal(
        std::begin(left), std::end(left), std::begin(right),
        [](unsigned char a, unsigned char b){ return std::tolower(a) == std::tolower(b); }
    );
}

bool alphanumeric(unsigned char ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

bool token(char ch)
{
    unsigned char value = static_cast<unsigned char>(ch);
    return alphanumeric(value) || ch == '!' || ch == '#' || ch == '$' || ch == '%' ||
        ch == '&' || ch == '\'' || ch == '*' || ch == '+' || ch == '-' || ch == '.' ||
        ch == '^' || ch == '_' || ch == '`' || ch == '|' || ch == '~';
}

bool valid_boundary(std::string_view boundary)
{
    if(boundary.empty() || boundary.back() == ' ') return false;
    return std::all_of(std::begin(boundary), std::end(boundary), [](unsigned char ch){
        return alphanumeric(ch) || ch == '\'' || ch == '(' || ch == ')' || ch == '+' ||
            ch == '_' || ch == ',' || ch == '-' || ch == '.' || ch == '/' || ch == ':' ||
            ch == '=' || ch == '?' || ch == ' ';
    });
}

bool parse_parameters(
    std::string_view value,
    std::string &head,
    Parameters &parameters,
    std::string &error
)
{
    parameters.clear();
    std::size_t semicolon = value.find(';');
    head = trim(value.substr(0, semicolon));
    if(head.empty()) return (error = "missing value", false);
    if(semicolon == std::string_view::npos) return true;

    std::size_t cursor = semicolon;
    while(cursor < value.size())
    {
        if(value[cursor] != ';') return (error = "expected semicolon", false);
        ++cursor;
        while(cursor < value.size() && (value[cursor] == ' ' || value[cursor] == '\t')) ++cursor;
        if(cursor == value.size()) return true;

        std::size_t key_start = cursor;
        while(cursor < value.size() && token(value[cursor])) ++cursor;
        if(key_start == cursor) return (error = "invalid parameter name", false);
        std::string key = lower(value.substr(key_start, cursor - key_start));

        while(cursor < value.size() && (value[cursor] == ' ' || value[cursor] == '\t')) ++cursor;
        if(cursor == value.size() || value[cursor] != '=')
            return (error = "parameter is missing '='", false);
        ++cursor;
        while(cursor < value.size() && (value[cursor] == ' ' || value[cursor] == '\t')) ++cursor;

        std::string parameter;
        if(cursor < value.size() && value[cursor] == '"')
        {
            ++cursor;
            bool closed = false;
            while(cursor < value.size())
            {
                char ch = value[cursor++];
                if(ch == '"')
                {
                    closed = true;
                    break;
                }
                if(ch == '\\')
                {
                    if(cursor == value.size()) return (error = "incomplete quoted escape", false);
                    ch = value[cursor++];
                }
                unsigned char byte = static_cast<unsigned char>(ch);
                if((byte < 0x20 && ch != '\t') || byte == 0x7f)
                    return (error = "control character in quoted parameter", false);
                parameter.push_back(ch);
            }
            if(!closed) return (error = "unterminated quoted parameter", false);
            while(cursor < value.size() && (value[cursor] == ' ' || value[cursor] == '\t')) ++cursor;
            if(cursor < value.size() && value[cursor] != ';')
                return (error = "unexpected data after quoted parameter", false);
        }
        else
        {
            std::size_t parameter_start = cursor;
            while(cursor < value.size() && value[cursor] != ';') ++cursor;
            auto raw = value.substr(parameter_start, cursor - parameter_start);
            while(!raw.empty() && (raw.back() == ' ' || raw.back() == '\t')) raw.remove_suffix(1);
            if(raw.empty() || !std::all_of(std::begin(raw), std::end(raw), token))
                return (error = "invalid unquoted parameter", false);
            parameter.assign(std::begin(raw), std::end(raw));
        }

        if(parameters.find(key) != std::end(parameters))
            return (error = "duplicate parameter", false);
        parameters.emplace(std::move(key), std::move(parameter));
    }
    return true;
}

bool parse_headers(std::string_view value, Headers &headers, std::string &error)
{
    headers.clear();
    std::size_t cursor = 0;
    while(cursor <= value.size())
    {
        std::size_t end = value.find("\r\n", cursor);
        if(end == std::string_view::npos) end = value.size();
        auto line = value.substr(cursor, end - cursor);
        if(line.empty()) return (error = "empty header line", false);

        std::size_t colon = line.find(':');
        if(colon == std::string_view::npos) return (error = "header is missing ':'", false);
        auto raw_name = line.substr(0, colon);
        if(raw_name.empty() || !std::all_of(std::begin(raw_name), std::end(raw_name), token))
            return (error = "invalid header name", false);

        std::string name = lower(raw_name);
        if(headers.find(name) != std::end(headers)) return (error = "duplicate header", false);
        headers.emplace(std::move(name), trim_view(line.substr(colon + 1)));

        if(end == value.size()) break;
        cursor = end + 2;
    }
    return true;
}

std::optional<Delimiter> delimiter_at(
    std::string_view body,
    std::string_view delimiter,
    std::size_t start
)
{
    if(!starts_with(body, start, delimiter)) return std::nullopt;

    std::size_t cursor = start + delimiter.size();
    bool closing = starts_with(body, cursor, "--");
    if(closing) cursor += 2;
    while(cursor < body.size() && (body[cursor] == ' ' || body[cursor] == '\t')) ++cursor;

    if(cursor == body.size())
        return closing ? std::optional<Delimiter>{Delimiter{start, cursor, true}} : std::nullopt;
    if(!starts_with(body, cursor, "\r\n")) return std::nullopt;
    return Delimiter{start, cursor + 2, closing};
}

std::optional<Delimiter> find_delimiter(
    std::string_view body,
    std::string_view delimiter,
    std::size_t start,
    bool opening
)
{
    if(opening)
        if(auto found = delimiter_at(body, delimiter, 0); found) return found;

    std::string marker = "\r\n";
    marker.append(delimiter);
    std::size_t cursor = start;
    while((cursor = body.find(marker, cursor)) != std::string_view::npos)
    {
        std::size_t delimiter_start = cursor + 2;
        if(auto found = delimiter_at(body, delimiter, delimiter_start); found) return found;
        cursor = delimiter_start + delimiter.size();
    }
    return std::nullopt;
}

} // namespace

Multipart::operator bool() const noexcept
{
    return error.empty();
}

bool Multipart::is(std::string_view content_type)
{
    std::size_t semicolon = content_type.find(';');
    return equal(trim(content_type.substr(0, semicolon)), "multipart/form-data");
}

Multipart Multipart::parse(std::string_view content_type, std::string_view body)
{
    std::string type;
    Parameters parameters;
    std::string error;
    if(!parse_parameters(content_type, type, parameters, error))
        return failure("Invalid Content-Type: " + error);
    if(!equal(type, "multipart/form-data"))
        return failure("Content-Type is not multipart/form-data");

    auto boundary_it = parameters.find("boundary");
    if(boundary_it == std::end(parameters) || boundary_it->second.empty())
        return failure("Missing multipart boundary");
    if(boundary_it->second.size() > 70)
        return failure("Multipart boundary is too long");
    if(!valid_boundary(boundary_it->second))
        return failure("Invalid multipart boundary");

    std::string delimiter = "--" + boundary_it->second;
    auto first = find_delimiter(body, delimiter, 0, true);
    if(!first) return failure("Opening multipart boundary not found");

    Multipart result;
    if(first->closing) return result;
    std::size_t cursor = first->next;

    while(cursor <= body.size())
    {
        auto header_end = body.find("\r\n\r\n", cursor);
        if(header_end == std::string_view::npos)
            return failure("Multipart part headers are incomplete");

        Headers headers;
        if(!parse_headers(body.substr(cursor, header_end - cursor), headers, error))
            return failure("Invalid multipart part headers: " + error);

        auto disposition_it = headers.find("content-disposition");
        if(disposition_it == std::end(headers))
            return failure("Multipart part is missing Content-Disposition");

        std::string disposition;
        Parameters disposition_parameters;
        if(!parse_parameters(disposition_it->second, disposition, disposition_parameters, error))
            return failure("Invalid Content-Disposition: " + error);
        if(!equal(disposition, "form-data"))
            return failure("Multipart part disposition is not form-data");

        auto name_it = disposition_parameters.find("name");
        if(name_it == std::end(disposition_parameters))
            return failure("Multipart part is missing a name");

        std::size_t body_start = header_end + 4;
        auto next = find_delimiter(body, delimiter, body_start, false);
        if(!next) return failure("Closing multipart boundary not found");

        Part part;
        part.name = name_it->second;
        if(auto filename_it = disposition_parameters.find("filename"); filename_it != std::end(disposition_parameters))
            part.filename = filename_it->second;
        if(auto part_type = headers.find("content-type"); part_type != std::end(headers))
            part.type = part_type->second;
        part.body = body.substr(body_start, (next->start - 2) - body_start);
        result.parts.push_back(std::move(part));

        if(next->closing) return result;
        cursor = next->next;
    }

    return failure("Closing multipart boundary not found");
}

} // namespace notojs::parser
