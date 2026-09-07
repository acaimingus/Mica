/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Implementation of ECDH key exchange and PIN/token derivation using OpenSSL.
 */

#include "ecdhkeyexchange.hpp"

namespace MicaListener::MicaListenerService::Network::Cryptography
{
    EcdhKeyExchange::EcdhKeyExchange()
    {
        EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
        EVP_PKEY_keygen_init(pctx);
        EVP_PKEY_keygen(pctx, &m_pkey);
        EVP_PKEY_CTX_free(pctx);
    }

    EcdhKeyExchange::~EcdhKeyExchange()
    {
        if (m_pkey)
        {
            EVP_PKEY_free(m_pkey);
        }
    }

    std::vector<uint8_t> EcdhKeyExchange::GetPublicKey() const
    {
        std::vector<uint8_t> pubKey(32);
        size_t len = pubKey.size();
        EVP_PKEY_get_raw_public_key(m_pkey, pubKey.data(), &len);
        return pubKey;
    }

    std::vector<uint8_t> EcdhKeyExchange::ComputeSharedSecret(const std::vector<uint8_t> &peerPubKey) const
    {
        EVP_PKEY *peerKey = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr, peerPubKey.data(), peerPubKey.size());
        if (!peerKey) return {};

        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(m_pkey, nullptr);
        if (EVP_PKEY_derive_init(ctx) <= 0)
        {
            EVP_PKEY_free(peerKey);
            EVP_PKEY_CTX_free(ctx);
            return {};
        }

        if (EVP_PKEY_derive_set_peer(ctx, peerKey) <= 0)
        {
            EVP_PKEY_free(peerKey);
            EVP_PKEY_CTX_free(ctx);
            return {};
        }

        size_t secretLen = 0;
        EVP_PKEY_derive(ctx, nullptr, &secretLen);
        std::vector<uint8_t> secret(secretLen);
        EVP_PKEY_derive(ctx, secret.data(), &secretLen);

        EVP_PKEY_free(peerKey);
        EVP_PKEY_CTX_free(ctx);

        return secret;
    }

    std::string EcdhKeyExchange::DerivePinFromSecret(const std::vector<uint8_t> &secret)
    {
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256(secret.data(), secret.size(), hash);

        // Take first 4 bytes of hash to form an integer, then mod 1000000 for a 6 digit PIN
        uint32_t val = (hash[0] << 24) | (hash[1] << 16) | (hash[2] << 8) | hash[3];
        uint32_t pin = val % 1000000;

        std::string pinStr = std::to_string(pin);
        while (pinStr.length() < 6)
        {
            pinStr = "0" + pinStr;
        }
        return pinStr;
    }

    std::vector<uint8_t> EcdhKeyExchange::GenerateAuthToken(const std::vector<uint8_t> &secret)
    {
        std::vector<uint8_t> token(32);
        unsigned int len = 32;
        const char* msg = "MICA_STREAM";
        HMAC(EVP_sha256(), secret.data(), secret.size(),
             reinterpret_cast<const unsigned char*>(msg), 11,
             token.data(), &len);
        return token;
    }
}