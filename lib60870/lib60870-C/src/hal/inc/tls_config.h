/*
 * tls_config.h
 *
 * TLS Configuration API for protocol stacks using TCP/IP
 *
 * Copyright 2017-2018 MZ Automation GmbH
 *
 * Abstraction layer for configuration of different TLS implementations
 *
 */

#ifndef SRC_TLS_CONFIG_H_
#define SRC_TLS_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * \file tls_config.h
 * \brief TLS API functions
 */

/*! \addtogroup hal Platform (Hardware/OS) abstraction layer
   *
   *  @{
   */

/**
 * @defgroup TLS_CONFIG_API TLS configuration
 *
 * @{
 */

typedef struct sTLSConfiguration* TLSConfiguration;

/**
 * \brief Create a new \ref TLSConfiguration object to represent TLS configuration and certificates
 *
 * \return the new TLS configuration
 */
TLSConfiguration
TLSConfiguration_create(void);

/* will be called by stack automatically when appropriate */
void
TLSConfiguration_setClientMode(TLSConfiguration self);

/**
 * \brief Enables the validation of the certificate trust chain (enabled by default)
 *
 * \param value true to enable chain validation, false to disable
 */
void
TLSConfiguration_setChainValidation(TLSConfiguration self, bool value);

/**
 * \brief Set if only known certificates are accepted.
 *
 * If set to true only known certificates are accepted. Connections with unknown certificates
 * are rejected even if they are signed by a trusted authority.
 *
 * \param value true to enable setting, false otherwise
 */
void
TLSConfiguration_setAllowOnlyKnownCertificates(TLSConfiguration self, bool value);

/**
 * \brief Set own certificate (identity) from a byte buffer
 *
 * \param certificate the certificate buffer
 * \param certLen the lenght of the certificate
 *
 * \return true, when the certificate was set, false otherwise (e.g. unknown certificate format)
 */
bool
TLSConfiguration_setOwnCertificate(TLSConfiguration self, uint8_t* certificate, int certLen);

/**
 * \brief Set own certificate (identity) from a certificate file
 *
 * \param filename of the certificate file
 *
 * \return true, when the certificate was set, false otherwise (e.g. unknown certificate format)
 */
bool
TLSConfiguration_setOwnCertificateFromFile(TLSConfiguration self, const char* filename);

bool
TLSConfiguration_setOwnKey(TLSConfiguration self, uint8_t* key, int keyLen, const char* keyPassword);

bool
TLSConfiguration_setOwnKeyFromFile(TLSConfiguration self, const char* filename, const char* keyPassword);

bool
TLSConfiguration_addAllowedCertificate(TLSConfiguration self, uint8_t* certifcate, int certLen);

bool
TLSConfiguration_addAllowedCertificateFromFile(TLSConfiguration self, const char* filename);

bool
TLSConfiguration_addCACertificate(TLSConfiguration self, uint8_t* certifcate, int certLen);

bool
TLSConfiguration_addCACertificateFromFile(TLSConfiguration self, const char* filename);

/**
 * \brief Set the renegotiation timeout.
 *
 * After the timeout elapsed a TLS session renegotiation has to occur.
 *
 * \param timeInMs session renegotiation timeout in milliseconds
 */
void
TLSConfiguration_setRenegotiationTime(TLSConfiguration self, int timeInMs);

void
TLSConfiguration_destroy(TLSConfiguration self);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SRC_TLS_CONFIG_H_ */
