#include "Misc/s2k.h"

#include <memory>

namespace OpenPGP {
namespace S2K {

std::string S2K::show_title() const {
    return NAME.at(type) + " (s2k " + std::to_string(type) + "):";
}

S2K::S2K(uint8_t t)
    : type(t),
      hash()
{}

S2K::~S2K() {}

void S2K::read(const std::string & data) {
    std::string::size_type pos = 0;
    read(data, pos);
}

void S2K::read(const std::string & data, std::string::size_type & pos) {
    set_type(data[pos]);
    set_hash(data[pos + 1]);
    pos += 2;
}

void S2K::show(HumanReadable & hr) const {
    hr << show_title() << HumanReadable::DOWN;
    show_contents(hr);
    hr << HumanReadable::UP;
}

std::string S2K::write() const {
    return raw();
}

uint8_t S2K::get_type() const {
    return type;
}

uint8_t S2K::get_hash() const {
    return hash;
}

void S2K::set_type(const uint8_t t) {
    type = t;
}

void S2K::set_hash(const uint8_t h) {
    hash = h;
}

void S2K0::show_contents(HumanReadable & hr) const {
    hr << "Hash: " + Hash::NAME.at(hash) + " (hash " + std::to_string(hash) + ")";
}

S2K0::S2K0(uint8_t t)
    : S2K(t)
{}

S2K0::S2K0()
    : S2K0(ID::SIMPLE_S2K)
{}

S2K0::S2K0(const std::string & data)
    : S2K0()
{
    S2K::read(data);
}

S2K0::~S2K0() {}

void S2K0::read(const std::string & data, std::string::size_type & pos) {
    S2K::read(data, pos);
}

std::string S2K0::raw() const {
    return std::string(1, type) + std::string(1, hash);
}

std::string S2K0::run(const std::string & pass, const std::size_t sym_key_len) const {
    std::string out = "";
    std::size_t counter = 0;
    while (out.size() < sym_key_len) {
        out += Hash::use(hash, std::string(counter++, 0) + pass);
    }
    return out.substr(0, sym_key_len);
}

S2K::Ptr S2K0::clone() const {
    return std::make_shared <S2K0> (*this);
}

void S2K1::show_contents(HumanReadable & hr) const {
    hr << "Hash: " + Hash::NAME.at(hash) + " (hash " + std::to_string(hash) + ")"
       << "Salt: " + hexlify(salt);
}

S2K1::S2K1(uint8_t t)
    : S2K0(t),
      salt()
{}

S2K1::S2K1()
    : S2K1(ID::SALTED_S2K)
{}

S2K1::S2K1(const std::string & data)
    : S2K1()
{
    S2K::read(data);
}

S2K1::~S2K1() {}

void S2K1::read(const std::string & data, std::string::size_type & pos) {
    S2K0::read(data, pos);
    salt = data.substr(pos, 8);
    pos += 8;
}

std::string S2K1::raw() const {
    return std::string(1, type) + std::string(1, hash) + salt;
}

std::string S2K1::run(const std::string & pass, const std::size_t sym_key_len) const {
    std::string out = "";
    std::size_t counter = 0;
    while (out.size() < sym_key_len) {
        out += Hash::use(hash, std::string(counter++, 0) + salt + pass);
    }
    return out.substr(0, sym_key_len);
}

std::string S2K1::get_salt() const {
    return salt;
}

void S2K1::set_salt(const std::string & s) {
    if (s.size() != 8) {
        throw std::runtime_error("Error: Salt length must be 8 octets.");
    }

    salt = s;
}

S2K::Ptr S2K1::clone() const {
    return std::make_shared <S2K1> (*this);
}

uint32_t S2K3::coded_count(const uint8_t c) {
    return (16 + (c & 15)) << ((c >> 4) + S2K3::EXPBIAS);
}

void S2K3::show_contents(HumanReadable & hr) const {
    hr << "Hash: " + Hash::NAME.at(hash) + " (hash " + std::to_string(hash) + ")"
       << "Salt: " + hexlify(salt)
       << "Coded Count: " + std::to_string(S2K3::coded_count(count)) + " (count " + std::to_string(count) + ")";
}

S2K3::S2K3()
    : S2K1(ID::ITERATED_AND_SALTED_S2K),
      count()
{}

S2K3::S2K3(const std::string & data)
    : S2K3()
{
    S2K::read(data);
}

S2K3::~S2K3() {}

void S2K3::read(const std::string & data, std::string::size_type & pos) {
    S2K1::read(data, pos);
    count = data[pos];
    pos += 1;
}

std::string S2K3::raw() const {
    return std::string(1, type) + std::string(1, hash) + salt + unhexlify(makehex(count, 2));
}

std::string S2K3::run(const std::string & pass, const std::size_t sym_key_len) const {
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

uint8_t S2K3::get_count() const {
    return count;
}

void S2K3::set_count(const uint8_t c) {
    count = c;
}

S2K::Ptr S2K3::clone() const {
    return std::make_shared <S2K3> (*this);
}

}
}
