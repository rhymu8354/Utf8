/**
 * @file Utf8.cpp
 *
 * This module contains the implementation of the Utf8::Utf8 class.
 *
 * © 2018 by Richard Walters
 */

#include <stddef.h>
#include <Utf8/Utf8.hpp>
#include <vector>

namespace {

    /**
     * This is the Unicode replacement character (�) encoded as UTF-8.
     */
    const std::vector< uint8_t > UTF8_ENCODED_REPLACEMENT_CHARACTER = {0xEF, 0xBF, 0xBD};

    /**
     * This is the Unicode replacement character (�) as a code point.
     */
    const Utf8::UnicodeCodePoint REPLACEMENT_CHARACTER = 0xFFFD;

    /**
     * Since RFC 3629 (November 2003), the high and low surrogate halves
     * used by UTF-16 (U+D800 through U+DFFF) and code points not encodable
     * by UTF-16 (those after U+10FFFF) are not legal Unicode values, and
     * their UTF-8 encoding must be treated as an invalid byte sequence.
     */
    const Utf8::UnicodeCodePoint FIRST_SURROGATE = 0xD800;
    const Utf8::UnicodeCodePoint LAST_SURROGATE = 0xDFFF;

    /**
     * This is the very, very, last code point in Unicode that is legal.
     */
    const Utf8::UnicodeCodePoint LAST_LEGAL_UNICODE_CODE_POINT = 0x10FFFF;

    /**
     * This computes the logarithm (base 2) of the given integer.
     *
     * @param[in] integer
     *     This is the integer for which to compute the logarithm.
     *
     * @return
     *     The logarithm (base 2) of the given integer is returned.
     */
    template< typename I > size_t log2n(I integer) {
        size_t answer = 0;
        while (integer > 0) {
            ++answer;
            integer >>= 1;
        }
        return answer;
    }

}

namespace Utf8 {

    std::vector< UnicodeCodePoint > AsciiToUnicode(const std::string& ascii) {
        return std::vector< UnicodeCodePoint >(
            ascii.begin(),
            ascii.end()
        );
    }

    /**
     * This contains the private properties of a Utf8 instance.
     */
    struct Utf8::Impl {
        /**
         * This is where we keep the current character
         * that is being decoded.
         */
        UnicodeCodePoint currentCharacterBeingDecoded = 0;

        /**
         * This is the number of input bytes that we still
         * need to read in before we can fully assemble
         * the current character that is being decoded.
         */
        size_t numBytesRemainingToDecode = 0;
    };

    Utf8::~Utf8() = default;

    Utf8::Utf8()
        : impl_(new Impl)
    {
    }

    std::vector< uint8_t > Utf8::Encode(const std::vector< UnicodeCodePoint >& codePoints) {
        std::vector< uint8_t > encoding;
        for (auto codePoint: codePoints) {
            const auto numBits = log2n(codePoint);
            if (numBits <= 7) {
                encoding.push_back((UnicodeCodePoint)(codePoint & 0x7F));
            } else if (numBits <= 11) {
                encoding.push_back((UnicodeCodePoint)(((codePoint >> 6) & 0x1F) + 0xC0));
                encoding.push_back((UnicodeCodePoint)((codePoint & 0x3F) + 0x80));
            } else if (numBits <= 16) {
                if (
                    (codePoint >= FIRST_SURROGATE)
                    && (codePoint <= LAST_SURROGATE)
                ) {
                    (void)encoding.insert(
                        encoding.end(),
                        UTF8_ENCODED_REPLACEMENT_CHARACTER.begin(),
                        UTF8_ENCODED_REPLACEMENT_CHARACTER.end()
                    );
                } else {
                    encoding.push_back((UnicodeCodePoint)(((codePoint >> 12) & 0x0F) + 0xE0));
                    encoding.push_back((UnicodeCodePoint)(((codePoint >> 6) & 0x3F) + 0x80));
                    encoding.push_back((UnicodeCodePoint)((codePoint & 0x3F) + 0x80));
                }
            } else if (
                (numBits <= 21)
                && (codePoint <= LAST_LEGAL_UNICODE_CODE_POINT)
            ) {
                encoding.push_back((UnicodeCodePoint)(((codePoint >> 18) & 0x07) + 0xF0));
                encoding.push_back((UnicodeCodePoint)(((codePoint >> 12) & 0x3F) + 0x80));
                encoding.push_back((UnicodeCodePoint)(((codePoint >> 6) & 0x3F) + 0x80));
                encoding.push_back((UnicodeCodePoint)((codePoint & 0x3F) + 0x80));
            } else {
                (void)encoding.insert(
                    encoding.end(),
                    UTF8_ENCODED_REPLACEMENT_CHARACTER.begin(),
                    UTF8_ENCODED_REPLACEMENT_CHARACTER.end()
                );
            }
        }
        return encoding;
    }

    std::vector< UnicodeCodePoint > Utf8::Decode(const std::vector< uint8_t >& encoding) {
        std::vector< UnicodeCodePoint > output;
        for (auto octet: encoding) {
            if (impl_->numBytesRemainingToDecode == 0) {
                if ((octet & 0x80) == 0) {
                    output.push_back(octet);
                } else if ((octet & 0xE0) == 0xC0) {
                    impl_->numBytesRemainingToDecode = 1;
                    impl_->currentCharacterBeingDecoded = (octet & 0x1F);
                } else if ((octet & 0xF0) == 0xE0) {
                    impl_->numBytesRemainingToDecode = 2;
                    impl_->currentCharacterBeingDecoded = (octet & 0x0F);
                } else if ((octet & 0xF8) == 0xF0) {
                    impl_->numBytesRemainingToDecode = 3;
                    impl_->currentCharacterBeingDecoded = (octet & 0x07);
                } else {
                    // todo: this is dangerous because the next character
                    // is likely a continuation character, and we'll end up
                    // outputting at least one more replacement character.
                    output.push_back(REPLACEMENT_CHARACTER);
                }
            } else {
                impl_->currentCharacterBeingDecoded <<= 6;
                impl_->currentCharacterBeingDecoded += (octet & 0x3F);
                if (--impl_->numBytesRemainingToDecode == 0) {
                    output.push_back(impl_->currentCharacterBeingDecoded);
                    impl_->currentCharacterBeingDecoded = 0;
                }
            }
        }
        return output;
    }

    std::vector< UnicodeCodePoint > Utf8::Decode(const std::string& encoding) {
        return Decode(
            std::vector< uint8_t >(
                encoding.begin(),
                encoding.end()
            )
        );
    }
}
