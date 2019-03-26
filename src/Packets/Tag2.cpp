#include "Packets/Tag2.h"

#include <stdexcept>

namespace OpenPGP {
namespace Packet {

void Tag2::read_subpackets(const std::string & data, Tag2::Subpackets & subpackets) {
    subpackets.clear();
    std::string::size_type pos = 0;

    while (pos < data.size()) {
        // read subpacket data out
        std::string::size_type length;
        Subpacket::Sub::read_subpacket(data, pos, length);  // pos moved past header to [length + data]

        // first octet of data is subpacket type
        // ignore critical bit until later
        Subpacket::Tag2::Sub::Ptr subpacket = nullptr;
        switch (const uint8_t type = data[pos] & 0x7f) {
            case Subpacket::Tag2::SIGNATURE_CREATION_TIME:
                subpacket = std::make_shared <Subpacket::Tag2::Sub2> ();
                break;
            case Subpacket::Tag2::SIGNATURE_EXPIRATION_TIME:
                subpacket = std::make_shared <Subpacket::Tag2::Sub3> ();
                break;
            case Subpacket::Tag2::EXPORTABLE_CERTIFICATION:
                subpacket = std::make_shared <Subpacket::Tag2::Sub4> ();
                break;
            case Subpacket::Tag2::TRUST_SIGNATURE:
                subpacket = std::make_shared <Subpacket::Tag2::Sub5> ();
                break;
            case Subpacket::Tag2::REGULAR_EXPRESSION:
                subpacket = std::make_shared <Subpacket::Tag2::Sub6> ();
                break;
            case Subpacket::Tag2::REVOCABLE:
                subpacket = std::make_shared <Subpacket::Tag2::Sub7> ();
                break;
            case Subpacket::Tag2::KEY_EXPIRATION_TIME:
                subpacket = std::make_shared <Subpacket::Tag2::Sub9> ();
                break;
            case Subpacket::Tag2::PLACEHOLDER_FOR_BACKWARD_COMPATIBILITY:
                subpacket = std::make_shared <Subpacket::Tag2::Sub10> ();
                break;
            case Subpacket::Tag2::PREFERRED_SYMMETRIC_ALGORITHMS:
                subpacket = std::make_shared <Subpacket::Tag2::Sub11> ();
                break;
            case Subpacket::Tag2::REVOCATION_KEY:
                subpacket = std::make_shared <Subpacket::Tag2::Sub12> ();
                break;
            case Subpacket::Tag2::ISSUER:
                subpacket = std::make_shared <Subpacket::Tag2::Sub16> ();
                break;
            case Subpacket::Tag2::NOTATION_DATA:
                subpacket = std::make_shared <Subpacket::Tag2::Sub20> ();
                break;
            case Subpacket::Tag2::PREFERRED_HASH_ALGORITHMS:
                subpacket = std::make_shared <Subpacket::Tag2::Sub21> ();
                break;
            case Subpacket::Tag2::PREFERRED_COMPRESSION_ALGORITHMS:
                subpacket = std::make_shared <Subpacket::Tag2::Sub22> ();
                break;
            case Subpacket::Tag2::KEY_SERVER_PREFERENCES:
                subpacket = std::make_shared <Subpacket::Tag2::Sub23> ();
                break;
            case Subpacket::Tag2::PREFERRED_KEY_SERVER:
                subpacket = std::make_shared <Subpacket::Tag2::Sub24> ();
                break;
            case Subpacket::Tag2::PRIMARY_USER_ID:
                subpacket = std::make_shared <Subpacket::Tag2::Sub25> ();
                break;
            case Subpacket::Tag2::POLICY_URI:
                subpacket = std::make_shared <Subpacket::Tag2::Sub26> ();
                break;
            case Subpacket::Tag2::KEY_FLAGS:
                subpacket = std::make_shared <Subpacket::Tag2::Sub27> ();
                break;
            case Subpacket::Tag2::SIGNERS_USER_ID:
                subpacket = std::make_shared <Subpacket::Tag2::Sub28> ();
                break;
            case Subpacket::Tag2::REASON_FOR_REVOCATION:
                subpacket = std::make_shared <Subpacket::Tag2::Sub29> ();
                break;
            case Subpacket::Tag2::FEATURES:
                subpacket = std::make_shared <Subpacket::Tag2::Sub30> ();
                break;
            case Subpacket::Tag2::SIGNATURE_TARGET:
                subpacket = std::make_shared <Subpacket::Tag2::Sub31> ();
                break;
            case Subpacket::Tag2::EMBEDDED_SIGNATURE:
                subpacket = std::make_shared <Subpacket::Tag2::Sub32> ();
                break;
            #ifdef GPG_COMPATIBLE
            case Subpacket::Tag2::ISSUER_FINGERPRINT:
                subpacket = std::make_shared <Subpacket::Tag2::Sub33> ();
                break;
            #endif
            default:
                throw std::runtime_error("Error: Tag 2 Subpacket tag not defined or reserved: " + std::to_string(type));
        }

        // subpacket guaranteed to be defined
        subpacket -> read(data.substr(pos + 1, length - 1));
        subpacket -> set_critical(data[pos] & 0x80);
        subpackets.push_back(subpacket);

        // go to end of current subpacket
        pos += length;
    }
}

void Tag2::actual_read(const std::string & data) {
    tag = Packet::SIGNATURE;
    set_version(data[0]);
    if (version == 3) {
        if (data[1] != 5) {
            throw std::runtime_error("Error: Length of hashed material must be 5.");
        }
        set_type  (data[2]);
        set_time  (toint(data.substr(3, 4), 256));
        set_keyid (data.substr(7, 8));
        set_pka   (data[15]);
        set_hash  (data[16]);
        set_left16(data.substr(17, 2));

        std::string::size_type pos = 19;
        if (PKA::is_RSA(pka)) {
            mpi.push_back(read_MPI(data, pos)); // RSA m**d mod n
        }
        #ifdef GPG_COMPATIBLE
        else if(pka == PKA::ID::DSA || pka == PKA::ID::ECDSA) {
            mpi.push_back(read_MPI(data, pos)); // r
            mpi.push_back(read_MPI(data, pos)); // s
        }
        #else
        else if (pka == PKA::ID::DSA) {
            mpi.push_back(read_MPI(data, pos)); // DSA r
            mpi.push_back(read_MPI(data, pos)); // DSA s
        }
        #endif
        else{
            throw std::runtime_error("Error: Unknown PKA type: " + std::to_string(pka));
        }
    }
    else if (version == 4) {
        set_type(data[1]);
        set_pka (data[2]);
        set_hash(data[3]);

        // hashed subpackets
        const uint16_t hashed_size = toint(data.substr(4, 2), 256);
        read_subpackets(data.substr(6, hashed_size), hashed_subpackets);

        // unhashed subpacketss
        const uint16_t unhashed_size = toint(data.substr(hashed_size + 6, 2), 256);
        read_subpackets(data.substr(hashed_size + 6 + 2, unhashed_size), unhashed_subpackets);

        // get left 16 bits
        set_left16(data.substr(hashed_size + 6 + 2 + unhashed_size, 2));

        // if (PKA::is_RSA(PKA))
        std::string::size_type pos = hashed_size + 6 + 2 + unhashed_size + 2;
        mpi.push_back(read_MPI(data, pos));        // RSA m**d mod n
        #ifdef GPG_COMPATIBLE
        if(pka == PKA::ID::DSA || pka == PKA::ID::ECDSA || pka == PKA::ID::EdDSA) {
            // mpi.push_back(read_MPI(data, pos)); // r
            mpi.push_back(read_MPI(data, pos));    // s
        }
        #else
        if (pka == PKA::ID::DSA) {
            // mpi.push_back(read_MPI(data, pos)); // DSA r
            mpi.push_back(read_MPI(data, pos));    // DSA s
        }
        #endif
    }
    else{
        throw std::runtime_error("Error: Tag2 Unknown version: " + std::to_string(static_cast <unsigned int> (version)));
    }
}

void Tag2::show_contents(HumanReadable & hr) const {
    hr << "Version: " + std::to_string(version);

    if (version < 4) {
        hr << "Hashed Material:" << HumanReadable::DOWN
           << "Signature Type: " + get_mapped(Signature_Type::NAME, type) + " (type 0x" + makehex(type, 2) + ")"
           << "Creation Time: " + show_time(time)
           << "Signer's Key ID: " + hexlify(keyid)
           << "Public Key Algorithm: " + get_mapped(PKA::NAME, pka) + " (pka " + std::to_string(pka) + ")"
           << "Hash Algorithm: " + get_mapped(Hash::NAME, hash) + " (hash " + std::to_string(hash) + ")"
           << HumanReadable::UP;
    }
    else if (version == 4) {
        hr << "Signature Type: " + get_mapped(Signature_Type::NAME, type) + " (type 0x" + makehex(type, 2) + ")"
           << "Public Key Algorithm: " + get_mapped(PKA::NAME, pka) + " (pka " + std::to_string(pka) + ")"
           << "Hash Algorithm: " + get_mapped(Hash::NAME, hash) + " (hash " + std::to_string(hash) + ")";

        if (hashed_subpackets.size()) {
            uint32_t create_time = 0;

            hr << "Hashed Sub:" << HumanReadable::DOWN;
            for(Subpacket::Tag2::Sub::Ptr const & s : hashed_subpackets) {
                // capture signature creation time to combine with expiration time
                if (s -> get_type() == Subpacket::Tag2::SIGNATURE_CREATION_TIME) {
                    create_time = std::static_pointer_cast <Subpacket::Tag2::Sub2> (s) -> get_time();
                }

                if (s -> get_type() == Subpacket::Tag2::KEY_EXPIRATION_TIME) {
                    std::static_pointer_cast <Subpacket::Tag2::Sub9> (s) -> show(create_time, hr);
                }
                else{
                    s -> show(hr);
                }
            }
            hr << HumanReadable::UP;
        }

        if (unhashed_subpackets.size()) {
            uint32_t create_time = 0;

            hr << "Unhashed Sub:" << HumanReadable::DOWN;
            for(Subpacket::Tag2::Sub::Ptr const & s : unhashed_subpackets) {
                // capture signature creation time to combine with expiration time
                if (s -> get_type() == Subpacket::Tag2::SIGNATURE_CREATION_TIME) {
                    create_time = std::static_pointer_cast <Subpacket::Tag2::Sub2> (s) -> get_time();
                }

                if (s -> get_type() == Subpacket::Tag2::KEY_EXPIRATION_TIME) {
                    std::static_pointer_cast <Subpacket::Tag2::Sub9> (s) -> show(create_time, hr);
                }
                else{
                    s -> show(hr);
                }
            }
            hr << HumanReadable::UP;
        }
    }

    hr << "Hash Left 16 Bits: " + hexlify(left16);

    if (PKA::is_RSA(pka)) {
        hr << "RSA m**d mod n (" + std::to_string(bitsize(mpi[0])) + " bits): " + mpitohex(mpi[0]);
    }
    #ifdef GPG_COMPATIBLE
    else if (pka == PKA::ID::ECDSA) {
        hr << "ECDSA r (" + std::to_string(bitsize(mpi[0])) + " bits): " + mpitohex(mpi[0])
           << "ECDSA s (" + std::to_string(bitsize(mpi[1])) + " bits): " + mpitohex(mpi[1]);
    }
    else if (pka == PKA::ID::EdDSA) {
        hr << "EdDSA r (" + std::to_string(bitsize(mpi[0])) + " bits): " + mpitohex(mpi[0])
           << "EdDSA s (" + std::to_string(bitsize(mpi[1])) + " bits): " + mpitohex(mpi[1]);
    }
    #endif
    else if (pka == PKA::ID::DSA) {
        hr << "DSA r (" + std::to_string(bitsize(mpi[0])) + " bits): " + mpitohex(mpi[0])
           << "DSA s (" + std::to_string(bitsize(mpi[1])) + " bits): " + mpitohex(mpi[1]);
    }
}

std::string Tag2::actual_raw() const {
    std::string out(1, version);
    if (version == 3) {// to recreate older keys
        out += "\x05" + std::string(1, type) + unhexlify(makehex(time, 8)) + keyid + std::string(1, pka) + std::string(1, hash) + left16;
    }
    if (version == 4) {
        std::string hashed_str = "";
        for(Subpacket::Tag2::Sub::Ptr const & s : hashed_subpackets) {
            hashed_str += s -> write();
        }
        std::string unhashed_str = "";
        for(Subpacket::Tag2::Sub::Ptr const & s : unhashed_subpackets) {
            unhashed_str += s -> write();
        }
        out += std::string(1, type) + std::string(1, pka) + std::string(1, hash) + unhexlify(makehex(hashed_str.size(), 4)) + hashed_str + unhexlify(makehex(unhashed_str.size(), 4)) + unhashed_str + left16;
    }
    for(MPI const & i : mpi) {
        out += write_MPI(i);
    }
    return out;
}

Error Tag2::actual_valid(const bool check_mpi) const {
    if ((version != 3) && (version != 4)) {
        return Error::INVALID_VERSION;
    }

    if (!Signature_Type::valid(type)) {
        return Error::INVALID_SIGNATURE_TYPE;
    }

    if (!PKA::valid(pka)) {
        return Error::INVALID_PUBLIC_KEY_ALGORITHM;
    }

    if (!PKA::can_sign(pka)) {
        return Error::PKA_CANNOT_BE_USED;
    }

    if (!Hash::valid(hash)) {
        return Error::INVALID_HASH_ALGORITHM;
    }

    if (version == 4) {
        for(Subpacket::Tag2::Sub::Ptr const & sub : hashed_subpackets) {
            // const Error err = sub -> valid();
            // if (err != Error::SUCCESS) {
            //     return err;
            // }
        }

        for(Subpacket::Tag2::Sub::Ptr const & sub : unhashed_subpackets) {
            // const Error err = sub -> valid();
            // if (err != Error::SUCCESS) {
            //     return err;
            // }
        }
    }

    if (left16.size() != 2) {
        return Error::INVALID_LEFT16_BITS;
    }

    if (check_mpi) {
        bool valid_mpi = false;
        switch (pka) {
            case PKA::ID::RSA_ENCRYPT_OR_SIGN:
            case PKA::ID::RSA_SIGN_ONLY:
                valid_mpi = (mpi.size() == 1);
            break;
            case PKA::ID::DSA:
                valid_mpi = (mpi.size() == 2);
                break;
            #ifdef GPG_COMPATIBLE
            case PKA::ID::ECDSA:
            case PKA::ID::EdDSA:
                valid_mpi = (mpi.size() == 2);
                break;
            #endif
            default:
                break;
        }

        if (!valid_mpi) {
            return Error::INVALID_MPI_COUNT;
        }
    }

    return Error::SUCCESS;
}

Tag2::Tag2()
    : Tag(SIGNATURE),
      type(0),
      pka(0),
      hash(0),
      mpi(),
      left16(),
      time(0),
      keyid(),
      hashed_subpackets(),
      unhashed_subpackets()
{}

Tag2::Tag2(const Tag2 & copy)
    : Tag(copy),
      type(copy.type),
      pka(copy.pka),
      hash(copy.hash),
      mpi(copy.mpi),
      left16(copy.left16),
      time(copy.time),
      keyid(copy.keyid),
      hashed_subpackets(copy.get_hashed_subpackets_clone()),
      unhashed_subpackets(copy.get_unhashed_subpackets_clone())
{}

Tag2::Tag2(const std::string & data)
    : Tag2()
{
    read(data);
}

Tag2::~Tag2() {
    hashed_subpackets.clear();
    unhashed_subpackets.clear();
}

uint8_t Tag2::get_type() const {
    return type;
}

uint8_t Tag2::get_pka() const {
    return pka;
}

uint8_t Tag2::get_hash() const {
    return hash;
}

std::string Tag2::get_left16() const {
    return left16;
}

PKA::Values Tag2::get_mpi() const {
    return mpi;
}

std::array <uint32_t, 3> Tag2::get_times() const {
    std::array <uint32_t, 3> times = {{0, 0, 0}};
    if (version == 3) {
        times[0] = time;
    }
    else if (version == 4) {
        // usually found in hashed subpackets
        for(Subpacket::Tag2::Sub::Ptr const & s : hashed_subpackets) {
            // 5.2.3.4. Signature Creation Time
            //    ...
            //    MUST be present in the hashed area.
            if (s -> get_type() == Subpacket::Tag2::SIGNATURE_CREATION_TIME) {
                times[0] = std::static_pointer_cast <Subpacket::Tag2::Sub2> (s) -> get_time();
            }
            else if (s -> get_type() == Subpacket::Tag2::SIGNATURE_EXPIRATION_TIME) {
                times[1] = std::static_pointer_cast <Subpacket::Tag2::Sub3> (s) -> get_dt();
            }
            else if (s -> get_type() == Subpacket::Tag2::KEY_EXPIRATION_TIME) {
                times[2] = std::static_pointer_cast <Subpacket::Tag2::Sub9> (s) -> get_dt();
            }
        }

        // search unhashed subpackets
        for(Subpacket::Tag2::Sub::Ptr const & s : unhashed_subpackets) {
            if (s -> get_type() == Subpacket::Tag2::SIGNATURE_EXPIRATION_TIME) {
                times[1] = std::static_pointer_cast <Subpacket::Tag2::Sub3> (s) -> get_dt();
            }
            else if (s -> get_type() == Subpacket::Tag2::KEY_EXPIRATION_TIME) {
                times[2] = std::static_pointer_cast <Subpacket::Tag2::Sub9> (s) -> get_dt();
            }
        }

        if (!times[0]) {
            throw std::runtime_error("Error: No signature creation time found.\n");
        }

        if (times[1]) {
            times[1] += times[0];
        }

        if (times[2]) {
            times[2] += times[0];
        }
    }
    else{
        throw std::runtime_error("Error: Signature Packet version " + std::to_string(version) + " not defined.");
    }

    return times;
}

std::string Tag2::get_keyid() const {
    if (version == 3) {
        return keyid;
    }
    else if (version == 4) {
        // usually found in unhashed subpackets
        for(Subpacket::Tag2::Sub::Ptr const & s : unhashed_subpackets) {
            if (s -> get_type() == Subpacket::Tag2::ISSUER) {
                return std::static_pointer_cast <Subpacket::Tag2::Sub16> (s) -> get_keyid();
            }
        }

        // search hashed subpackets if necessary
        for(Subpacket::Tag2::Sub::Ptr const & s : hashed_subpackets) {
            if (s -> get_type() == Subpacket::Tag2::ISSUER) {
                return std::static_pointer_cast <Subpacket::Tag2::Sub16> (s) -> get_keyid();
            }
        }
    }
    else{
        throw std::runtime_error("Error: Signature Packet version " + std::to_string(version) + " not defined.");
    }

    return ""; // should never reach here; mainly just to remove compiler warnings
}

Tag2::Subpackets Tag2::get_hashed_subpackets() const {
    return hashed_subpackets;
}

Tag2::Subpackets Tag2::get_hashed_subpackets_clone() const {
    Subpackets out;
    for(Subpacket::Tag2::Sub::Ptr const & s : hashed_subpackets) {
        out.push_back(s -> clone());
    }
    return out;
}

Tag2::Subpackets Tag2::get_unhashed_subpackets() const {
    return unhashed_subpackets;
}

Tag2::Subpackets Tag2::get_unhashed_subpackets_clone() const {
    Subpackets out;
    for(Subpacket::Tag2::Sub::Ptr const & s : unhashed_subpackets) {
        out.push_back(s -> clone());
    }
    return out;
}

std::string Tag2::get_up_to_hashed() const {
    if (version == 3) {
        return "\x03" + std::string(1, type) + unhexlify(makehex(time, 8));
    }
    else if (version == 4) {
        std::string hashed = "";
        for(Subpacket::Tag2::Sub::Ptr const & s : hashed_subpackets) {
            hashed += s -> write();
        }
        return "\x04" + std::string(1, type) + std::string(1, pka) + std::string(1, hash) + unhexlify(makehex(hashed.size(), 4)) + hashed;
    }
    else{
        throw std::runtime_error("Error: Signature packet version " + std::to_string(version) + " not defined.");
    }
    return ""; // should never reach here; mainly just to remove compiler warnings
}

std::string Tag2::get_without_unhashed() const {
    std::string out(1, version);
    if (version < 4) {// to recreate older keys
        out += "\x05" + std::string(1, type) + unhexlify(makehex(time, 8)) + keyid + std::string(1, pka) + std::string(1, hash) + left16;
    }
    if (version == 4) {
        std::string hashed_str = "";
        for(Subpacket::Tag2::Sub::Ptr const & s : hashed_subpackets) {
            hashed_str += s -> write();
        }
        out += std::string(1, type) + std::string(1, pka) + std::string(1, hash) + unhexlify(makehex(hashed_str.size(), 4)) + hashed_str + zero + zero + left16;
    }
    for(MPI const & i : mpi) {
        out += write_MPI(i);
    }
    return out;
}

void Tag2::set_type(const uint8_t t) {
    type = t;
}

void Tag2::set_pka(const uint8_t p) {
    pka = p;
}

void Tag2::set_hash(const uint8_t h) {
    hash = h;
}

void Tag2::set_left16(const std::string & l) {
    left16 = l;
}

void Tag2::set_mpi(const PKA::Values & m) {
    mpi = m;
}

void Tag2::set_time(const uint32_t t) {
    if (version == 3) {
        time = t;
    }
    else if (version == 4) {
        unsigned int i;
        for(i = 0; i < hashed_subpackets.size(); i++) {
            if (hashed_subpackets[i] -> get_type() == 2) {
                break;
            }
        }
        Subpacket::Tag2::Sub2::Ptr sub2 = std::make_shared <Subpacket::Tag2::Sub2> ();
        sub2 -> set_time(t);
        if (i == hashed_subpackets.size()) { // not found
            hashed_subpackets.push_back(sub2);
        }
        else{                                // found
            hashed_subpackets[i] = sub2;
        }
    }
}

void Tag2::set_keyid(const std::string & k) {
    if (k.size() != 8) {
        throw std::runtime_error("Error: Key ID must be 8 octets.");
    }

    if (version == 3) {
        keyid = k;
    }
    else if (version == 4) {
        unsigned int i;
        for(i = 0; i < unhashed_subpackets.size(); i++) {
            if (unhashed_subpackets[i] -> get_type() == 16) {
                break;
            }
        }
        Subpacket::Tag2::Sub16::Ptr sub16 = std::make_shared <Subpacket::Tag2::Sub16> ();
        sub16 -> set_keyid(k);
        if (i == unhashed_subpackets.size()) {   // not found
            unhashed_subpackets.push_back(sub16);
        }
        else{                                    // found
            unhashed_subpackets[i] = sub16;
        }
    }
}

void Tag2::set_hashed_subpackets(const Tag2::Subpackets & h) {
    hashed_subpackets.clear();
    for(Subpacket::Tag2::Sub::Ptr const & s : h) {
        hashed_subpackets.push_back(s -> clone());
    }
}

void Tag2::set_unhashed_subpackets(const Tag2::Subpackets & u) {
    unhashed_subpackets.clear();
    for(Subpacket::Tag2::Sub::Ptr const & s : u) {
        unhashed_subpackets.push_back(s -> clone());
    }
}

std::string Tag2::find_subpacket(const uint8_t sub) const {
    // 5.2.4.1. Subpacket Hints
    //
    //   It is certainly possible for a signature to contain conflicting
    //   information in subpackets. For example, a signature may contain
    //   multiple copies of a preference or multiple expiration times. In
    //   most cases, an implementation SHOULD use the last subpacket in the
    //   signature, but MAY use any conflict resolution scheme that makes
    //   more sense.

    std::string out;
    for(Subpacket::Tag2::Sub::Ptr const & s : hashed_subpackets) {
        if (s -> get_type() == sub) {
            out = s -> raw();
            break;
        }
    }
    for(Subpacket::Tag2::Sub::Ptr const & s : unhashed_subpackets) {
        if (s -> get_type() == sub) {
            out = s -> raw();
            break;
        }
    }
    return out;
}


Tag::Ptr Tag2::clone() const {
    Ptr out = std::make_shared <Tag2> (*this);
    out -> hashed_subpackets = get_hashed_subpackets_clone();
    out -> unhashed_subpackets = get_unhashed_subpackets_clone();
    return out;
}

Tag2 & Tag2::operator=(const Tag2 & tag2) {
    Tag::operator=(tag2);
    type = tag2.type;
    pka = tag2.pka;
    hash = tag2.hash;
    mpi = tag2.mpi;
    left16 = tag2.left16;
    time = tag2.time;
    keyid = tag2.keyid;
    hashed_subpackets = tag2.get_hashed_subpackets_clone();
    unhashed_subpackets = tag2.get_unhashed_subpackets_clone();
    return *this;
}

}
}
