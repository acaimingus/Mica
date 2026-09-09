/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Class for Elliptic-curve Diffie-Hellman (ECDH) key exchange and PIN/token derivation using OpenSSL.
 */

#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace MicaListener::MicaListenerService::Network::Cryptography
{
/// @brief Class for managing ECDH key exchange using X25519 and deriving authentication data
class EcdhKeyExchange
{
  public:
    /// @brief Constructor, generates a new local X25519 keypair
    EcdhKeyExchange();

    /// @brief Destructor, frees the OpenSSL keypair resource
    ~EcdhKeyExchange();

    /// @brief Retrieves the raw 32-byte public key of the local keypair
    /// @return Vector containing the 32 raw bytes of the public key
    [[nodiscard]] std::vector<uint8_t> GetPublicKey() const;

    /// @brief Computes the shared secret using the local private key and the peer's public key
    /// @param peerPubKey The raw 32-byte public key of the peer
    /// @return Vector containing the computed shared secret, or empty on failure
    [[nodiscard]] std::vector<uint8_t> ComputeSharedSecret(const std::vector<uint8_t> &peerPubKey) const;

    /// @brief Derives a 6-digit numeric verification PIN from the shared secret via SHA-256
    /// @param secret The computed shared secret
    /// @return Formatted 6-digit numeric PIN string with leading zeros if necessary
    static std::string DerivePinFromSecret(const std::vector<uint8_t> &secret);

    /// @brief Generates a 32-byte HMAC-SHA256 authentication token from the shared secret
    /// @param secret The computed shared secret
    /// @return Vector containing the 32-byte authentication token
    static std::vector<uint8_t> GenerateAuthToken(const std::vector<uint8_t> &secret);

  private:
    /// @brief OpenSSL EVP_PKEY keypair structure pointer
    EVP_PKEY *m_pkey = nullptr;
};
} // namespace MicaListener::MicaListenerService::Network::Cryptography
