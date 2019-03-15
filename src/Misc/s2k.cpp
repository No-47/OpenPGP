#include "Misc/s2k.h"

#include <memory>

namespace OpenPGP {
namespace S2K {

std::string S2K::show_title() const{
    return NAME.at(type) + " (s2k " + std::to_string(type) + "):";
}

S2K::S2K(uint8_t t)
    : type(t),
      hash()
{}

S2K::~S2K(){}

std::string S2K::write() const{
    return raw();
}

uint8_t S2K::get_type() const{
    return type;
}

uint8_t S2K::get_hash() const{
    return hash;
}

void S2K::set_type(const uint8_t t){
    type = t;
}

void S2K::set_hash(const uint8_t h){
    hash = h;
}

S2K0::S2K0(uint8_t t)
    : S2K(t)
{}

S2K0::S2K0()
    : S2K0(ID::SIMPLE_S2K)
{}

S2K0::~S2K0(){}

void S2K0::read(const std::string & data, std::string::size_type & pos){
    type = data[pos];
    hash = data[pos + 1];
    pos += 2;
}

std::string S2K0::show(const std::size_t indents, const std::size_t indent_size) const{
    const std::string indent(indents * indent_size, ' ');
    const std::string tab(indent_size, ' ');
    return indent + tab + show_title() + "\n" +
           indent + tab + tab + "Hash: " + Hash::NAME.at(hash) + " (hash " + std::to_string(hash) + ")";
}

std::string S2K0::raw() const{
    return "\x00" + std::string(1, hash);
}

std::string S2K0::run(const std::string & pass, const std::size_t sym_key_len) const{
    std::string out = "";
    std::size_t counter = 0;
    while (out.size() < sym_key_len){
        out += Hash::use(hash, std::string(counter++, 0) + pass);
    }
    return out.substr(0, sym_key_len);
}

S2K::Ptr S2K0::clone() const{
    return std::make_shared <S2K0> (*this);
}

S2K1::S2K1(uint8_t t)
    : S2K0(t),
      salt()
{}

S2K1::S2K1()
    : S2K1(ID::SALTED_S2K)
{}

S2K1::~S2K1(){}

void S2K1::read(const std::string & data, std::string::size_type & pos){
    type = data[pos];
    hash = data[pos + 1];
    salt = data.substr(pos + 2, 8);
    pos += 10;
}

std::string S2K1::show(const std::size_t indents, const std::size_t indent_size) const{
    const std::string indent(indents * indent_size, ' ');
    const std::string tab(indent_size, ' ');
    return indent + tab + show_title() + "\n" +
           indent + tab + tab + "Hash: " + Hash::NAME.at(hash) + " (hash " + std::to_string(hash) + ")" +
           indent + tab + tab + "Salt: " + hexlify(salt);
}

std::string S2K1::raw() const{
    return "\x01" + std::string(1, hash) + salt;
}

std::string S2K1::run(const std::string & pass, const std::size_t sym_key_len) const{
    std::string out = "";
    std::size_t counter = 0;
    while (out.size() < sym_key_len){
        out += Hash::use(hash, std::string(counter++, 0) + salt + pass);
    }
    return out.substr(0, sym_key_len);
}

std::string S2K1::get_salt() const{
    return salt;
}

void S2K1::set_salt(const std::string & s){
    if (s.size() != 8){
        throw std::runtime_error("Error: Salt length must be 8 octets.");
    }

    salt = s;
}

S2K::Ptr S2K1::clone() const{
    return std::make_shared <S2K1> (*this);
}

uint32_t S2K3::coded_count(const uint8_t c){
    return (16 + (c & 15)) << ((c >> 4) + S2K3::EXPBIAS);
}

S2K3::S2K3()
    : S2K1(ID::ITERATED_AND_SALTED_S2K),
      count()
{}

S2K3::~S2K3(){}

void S2K3::read(const std::string & data, std::string::size_type & pos){
    type  = data[pos];
    hash  = data[pos + 1];
    salt  = data.substr(pos + 2, 8);
    count = data[pos + 10];
    pos += 11;
}

std::string S2K3::show(const std::size_t indents, const std::size_t indent_size) const{
    const std::string indent(indents * indent_size, ' ');
    const std::string tab(indent_size, ' ');
    return indent + tab + show_title() + "\n" +
           indent + tab + tab + "Hash: " + Hash::NAME.at(hash) + " (hash " + std::to_string(hash) + ")\n" +
           indent + tab + tab + "Salt: " + hexlify(salt) + "\n" +
           indent + tab + tab + "Coded Count: " + std::to_string(S2K3::coded_count(count)) + " (count " + std::to_string(count) + ")";
}

std::string S2K3::raw() const{
    return "\x03" + std::string(1, hash) + salt + unhexlify(makehex(count, 2));
}

std::string S2K3::run(const std::string & pass, const std::size_t sym_key_len) const{
    const std::size_t coded = coded_count(count);
    const std::string combined = salt + pass;

    const std::size_t digest_octets = Hash::LENGTH.at(hash) >> 3;
    const std::size_t contexts = (digest_octets / sym_key_len) + (bool) (digest_octets % sym_key_len);

    std::string out = "";
    for(std::size_t context = 0; context < contexts; context++) {
        Hash::Instance h = Hash::get_instance(hash, std::string(context++, '\x00'));

        std::size_t hashed = 0;
        do {
            h -> update(combined);
            hashed += combined.size();
        } while ((hashed + combined.size()) < coded);

        if (hashed < coded) {
            h -> update(combined.substr(0, coded - hashed));
        }
        out += h -> digest();
    }

    return out.substr(0, sym_key_len);
}

uint8_t S2K3::get_count() const{
    return count;
}

void S2K3::set_count(const uint8_t c){
    count = c;
}

S2K::Ptr S2K3::clone() const{
    return std::make_shared <S2K3> (*this);
}

}
}