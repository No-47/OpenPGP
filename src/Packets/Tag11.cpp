#include "Packets/Tag11.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "Misc/pgptime.h"

namespace OpenPGP {
namespace Packet {

bool Literal::valid(const uint8_t format) {
    return (NAME.find(format) != NAME.end());
}

void Tag11::actual_read(const std::string & data) {
    set_data_format(data[0]);
    const uint8_t len = data[1];
    set_filename(data.substr(2, len));
    set_time(toint(data.substr(2 + len, 4), 256));
    set_literal(data.substr(len + 6, data.size() - len - 6));
}

std::string Tag11::show_title() const {
    return Tag::show_title() + Partial::show_title();
}

void Tag11::show_contents(HumanReadable & hr) const {
    hr << "Data_Format: " + get_mapped(Literal::NAME, data_format)
       << "Data (" + std::to_string(1 + filename.size() + 4 + literal.size()) + " octets):"
       << HumanReadable::DOWN
       << "Filename: " + filename
       << "Creation Date: " + show_time(time)
       << "Data: " + literal
       << HumanReadable::UP;
}

std::string Tag11::actual_raw() const {
    return std::string(1, data_format) + std::string(1, filename.size()) + filename + unhexlify(makehex(time, 8)) + literal;
}

std::string Tag11::actual_write() const {
    return Partial::write(header_format, tag, raw());
}

Error Tag11::actual_valid(const bool) const {
    if (!Literal::valid(data_format)) {
        return Error::INVALID_LITERAL_DATA_FORMAT;
    }

    return Error::SUCCESS;
}

Tag11::Tag11(const PartialBodyLength & part)
    : Tag(LITERAL_DATA),
      Partial(part),
      data_format(),
      filename(),
      time(),
      literal()
{}

Tag11::Tag11(const std::string & data)
    : Tag11()
{
    read(data);
}

std::string Tag11::write() const {
    return Partial::write(header_format, LITERAL_DATA, raw());
}

uint8_t Tag11::get_data_format() const {
    return data_format;
}

std::string Tag11::get_filename() const {
    return filename;
}

uint32_t Tag11::get_time() const {
    return time;
}

std::string Tag11::get_literal() const {
    return literal;
}

std::string Tag11::out(const bool writefile) {
    if ((filename != "") && writefile) {
        std::ofstream f;
        switch (data_format) {
            case Literal::BINARY:
                f.open(filename.c_str(), std::ios::binary);
                break;
            case Literal::TEXT:
            case Literal::UTF8_TEXT:
                f.open(filename.c_str());
                break;
            default:
                throw std::runtime_error("Error: Unknown Literal Data format type: " + std::to_string(data_format));
                break;
        }
        if (!f) {
            throw std::runtime_error("Error: Failed to open file to write literal data.");
        }
        f << literal;
    }
    else{
        return literal;
    }
    return "Data written to file '" + filename + "'.";
}

void Tag11::set_data_format(const uint8_t f) {
    data_format = f;
}

void Tag11::set_filename(const std::string & f) {
    filename = f;
}

void Tag11::set_time(const uint32_t t) {
    time = t;
}

void Tag11::set_literal(const std::string & l) {
    literal = l;
}

bool Tag11::console() const {
    return (filename == "_CONSOLE");
}

Tag::Ptr Tag11::clone() const {
    return std::make_shared <Packet::Tag11> (*this);
}

}
}
