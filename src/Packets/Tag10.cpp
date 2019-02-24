#include "Packets/Tag10.h"

namespace OpenPGP {
namespace Packet {

const std::string Tag10::body = "PGP";

Tag10::Tag10()
    : Tag(MARKER_PACKET),
      pgp(body)
{}

Tag10::Tag10(const Tag10 & copy)
    : Tag(copy),
      pgp(copy.pgp)
{}

Tag10::Tag10(const std::string & data)
    : Tag10()
{
    read(data);
}

void Tag10::read(const std::string & data){
    size = data.size();
    if (data != body){
        throw std::runtime_error("Error: Tag 10 packet did not contain data \"" + body + "\".");
    }
}

std::string Tag10::show(const std::size_t indents, const std::size_t indent_size) const{
    const std::string indent(indents * indent_size, ' ');
    const std::string tab(indent_size, ' ');
    return indent + show_title() + "\n" +
           indent + tab + pgp;
}

std::string Tag10::raw() const{
    return pgp;
}

std::string Tag10::get_pgp() const{
    return pgp;
}

void Tag10::set_pgp(const std::string & s){
    if (s != body){
        throw std::runtime_error("Error: Tag 10 input data not string \"" + body + "\".");
    }
    pgp = s;
    size = 3;
}

Tag::Ptr Tag10::clone() const{
    return std::make_shared <Packet::Tag10> (*this);
}

}
}
