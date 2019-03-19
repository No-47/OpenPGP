#include <gtest/gtest.h>

#include "Hashes/Hashes.h"

#include "testvectors/sha/sha512shortmsg.h"

TEST(SHA512, short_msg) {

    ASSERT_EQ(SHA512_SHORT_MSG.size(), SHA512_SHORT_MSG_HEXDIGEST.size());

    for ( unsigned int i = 0; i < SHA512_SHORT_MSG.size(); ++i ) {
        auto sha512 = OpenPGP::Hash::use(OpenPGP::Hash::ID::SHA512, unhexlify(SHA512_SHORT_MSG[i]));
        EXPECT_EQ(hexlify(sha512), SHA512_SHORT_MSG_HEXDIGEST[i]);
    }
}


