/*
 * Copyright (C) 2015-2018 ZhengHaiTao <ming8ren@163.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pump/debug.h"
#include "pump/config.h"
#include "pump/ssl/tls.h"

#if defined(PUMP_HAVE_OPENSSL)
extern "C" {
#include <openssl/ssl.h>
}
#endif

#if defined(PUMP_HAVE_GNUTLS)
extern "C" {
#include <gnutls/gnutls.h>
}
#endif

namespace pump {
namespace ssl {

        void* create_tls_client_certificate() {
#if defined(PUMP_HAVE_GNUTLS)
        gnutls_certificate_credentials_t xcred;
        if (gnutls_certificate_allocate_credentials(&xcred) != 0) {
            PUMP_WARN_LOG(
                "ssl_helper: create tls client certificate failed for gnutls_certificate_allocate_credentials failed");
            return nullptr;
        }
        return xcred;
#elif defined(PUMP_HAVE_OPENSSL)
        SSL_CTX *xcred = SSL_CTX_new(TLS_client_method());
        if (xcred == nullptr) {
            PUMP_WARN_LOG(
                "ssl_helper: create tls_client certificate failed for SSL_CTX_new failed");
            return nullptr;
        }
        SSL_CTX_set_options(xcred, SSL_EXT_TLS1_3_ONLY);
        return xcred;
#else
        return nullptr;
#endif
    }

    void* create_tls_certificate_by_file(
        bool client,
        const std::string &cert,
        const std::string &key) {
#if defined(PUMP_HAVE_GNUTLS)
        gnutls_certificate_credentials_t xcred;
        int32_t ret = gnutls_certificate_allocate_credentials(&xcred);
        if (ret != 0) {
            PUMP_WARN_LOG(
                "ssl_helper: create tls certificate by file failed for gnutls_certificate_allocate_credentials failed");
            return nullptr;
        }

        ret = gnutls_certificate_set_x509_key_file(
            xcred, cert.c_str(), key.c_str(), GNUTLS_X509_FMT_PEM);
        if (ret != 0) {
            PUMP_DEBUG_LOG(
                "ssl_helper: create tls certificate by file failed for gnutls_certificate_set_x509_key_file failed");
            gnutls_certificate_free_credentials(xcred);
            return nullptr;
        }

        return xcred;
#elif defined(PUMP_HAVE_OPENSSL)
        SSL_CTX *xcred = nullptr;
        if (client) {
            xcred = SSL_CTX_new(TLS_client_method());
        } else {
            xcred = SSL_CTX_new(TLS_server_method());
        }
        if (xcred == nullptr) {
            PUMP_WARN_LOG(
                "ssl_helper: create tls certificate by file failed for SSL_CTX_new failed");
            return nullptr;
        }

        /* Set the key and cert */
        if (SSL_CTX_use_certificate_file(xcred, cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
            SSL_CTX_free(xcred);
            PUMP_DEBUG_LOG(
                "ssl_helper: create tls certificate by file failed for SSL_CTX_use_certificate_file failed");
            return nullptr;
        }
        if (SSL_CTX_use_PrivateKey_file(xcred, key.c_str(), SSL_FILETYPE_PEM) <= 0) {
            SSL_CTX_free(xcred);
            PUMP_DEBUG_LOG(
                "ssl_helper: create tls certificate by file failed for SSL_CTX_use_PrivateKey_file failed");
            return nullptr;
        }
        return xcred;
#else
        return nullptr;
#endif
    }

    void* create_tls_certificate_by_buffer(
        bool client,
        const std::string &cert,
        const std::string &key) {
#if defined(PUMP_HAVE_GNUTLS)
        gnutls_certificate_credentials_t xcred;
        if (gnutls_certificate_allocate_credentials(&xcred) != 0) {
            PUMP_WARN_LOG(
                "ssl_helper: create tls certificate by buffer failed for gnutls_certificate_allocate_credentials failed");
            return nullptr;
        }

        gnutls_datum_t gnutls_cert;
        gnutls_cert.data = (uint8_t*)cert.data();
        gnutls_cert.size = (uint32_t)cert.size();

        gnutls_datum_t gnutls_key;
        gnutls_key.data = (uint8_t*)key.data();
        gnutls_key.size = (uint32_t)key.size();

        if (gnutls_certificate_set_x509_key_mem(
                xcred, 
                &gnutls_cert, 
                &gnutls_key, 
                GNUTLS_X509_FMT_PEM) != 0) {
            PUMP_DEBUG_LOG(
                "ssl_helper: create tls certificate by buffer failed for gnutls_certificate_set_x509_key_mem failed");
            gnutls_certificate_free_credentials(xcred);
            return nullptr;
        }

        return xcred;
#elif defined(PUMP_HAVE_OPENSSL)
        SSL_CTX *xcred = nullptr;
        if (client) {
            xcred = SSL_CTX_new(TLS_client_method());
        } else {
            xcred = SSL_CTX_new(TLS_server_method());
        }
        if (xcred == nullptr) {
            PUMP_WARN_LOG(
                "ssl_helper: create tls certificate by buffer failed for SSL_CTX_new failed");
            return nullptr;
        }

        BIO *cert_bio = BIO_new_mem_buf((void*)cert.c_str(), -1);
        if (cert_bio == nullptr) {
            PUMP_WARN_LOG(
                "ssl_helper: create tls certificate by buffer failed for BIO_new_mem_buf failed");
            SSL_CTX_free(xcred);
            return nullptr;
        }
        X509 *x509_cert = PEM_read_bio_X509(cert_bio, nullptr, nullptr, nullptr);
        BIO_free(cert_bio);
        if (!x509_cert) {
            PUMP_DEBUG_LOG(
                "ssl_helper: create tls certificate by buffer failed for PEM_read_bio_X509 failed");
            SSL_CTX_free(xcred);
            return nullptr;
        }

        BIO *key_bio = BIO_new_mem_buf((void*)key.c_str(), -1);
        if (key_bio == nullptr) {
            PUMP_DEBUG_LOG(
                "ssl_helper: create tls certificate by buffer failed for BIO_new_mem_buf failed");
            SSL_CTX_free(xcred);
            X509_free(x509_cert);
            return nullptr;
        }
        EVP_PKEY *evp_key = PEM_read_bio_PrivateKey(key_bio, nullptr, 0, nullptr);
        BIO_free(key_bio);
        if (evp_key == nullptr) {
            PUMP_DEBUG_LOG(
                "ssl_helper: create tls certificate by buffer failed for PEM_read_bio_PrivateKey failed");
            SSL_CTX_free(xcred);
            X509_free(x509_cert);
            return nullptr;
        }

        /* Set the key and cert */
        if (SSL_CTX_use_certificate(xcred, x509_cert) <= 0 ||
            SSL_CTX_use_PrivateKey(xcred, evp_key) <= 0) {
            PUMP_DEBUG_LOG(
                "ssl_helper: create tls certificate by buffer failed for SSL_CTX_use_PrivateKey failed");
            SSL_CTX_free(xcred);
            X509_free(x509_cert);
            EVP_PKEY_free(evp_key);
            return nullptr;
        }

        X509_free(x509_cert);
        EVP_PKEY_free(evp_key);

        return xcred;
#else
        return nullptr;
#endif
    }

    void destory_tls_certificate(void *xcred) {
        if (xcred != nullptr) {
#if defined(PUMP_HAVE_GNUTLS)
            gnutls_certificate_free_credentials((gnutls_certificate_credentials_t)xcred);
#elif defined(PUMP_HAVE_OPENSSL)
            SSL_CTX_free((SSL_CTX*)xcred);
#endif
        }
    }

    tls_session* create_tls_session(
        void *xcred, 
        int32_t fd, 
        bool client) {
#if defined(PUMP_HAVE_GNUTLS)
        tls_session *session = object_create<tls_session>();
        if (session == nullptr) {
            return nullptr;
        }
        gnutls_session_t ssl_ctx = nullptr;
        if (client) {
            gnutls_init(&ssl_ctx, GNUTLS_CLIENT | GNUTLS_NONBLOCK);
        } else {
            gnutls_init(&ssl_ctx, GNUTLS_SERVER | GNUTLS_NONBLOCK);
        }
        gnutls_set_default_priority(ssl_ctx);
        // Set GnuTLS session with credentials
        gnutls_credentials_set(ssl_ctx, GNUTLS_CRD_CERTIFICATE, xcred);
        // Set GnuTLS handshake timeout time.
        gnutls_handshake_set_timeout(ssl_ctx, GNUTLS_INDEFINITE_TIMEOUT);
        // Set GnuTLS transport fd.
        gnutls_transport_set_int(ssl_ctx, fd);

        session->ssl_ctx = ssl_ctx;

        return session;
#elif defined(PUMP_HAVE_OPENSSL)
        tls_session *session = object_create<tls_session>();
        if (session == nullptr) {
            return nullptr;
        }
        SSL *ssl_ctx = SSL_new((SSL_CTX*)xcred);
        if (ssl_ctx == nullptr) {
            object_delete(session);
            return  nullptr;
        }
        SSL_set_fd(ssl_ctx, fd);
        if (client) {
            SSL_set_connect_state(ssl_ctx);
        } else {
            SSL_set_accept_state(ssl_ctx);
        }

        session->ssl_ctx = ssl_ctx;

        return session;
#else
        return nullptr;
#endif
    }

    void destory_tls_session(tls_session *session) {
        if (session == nullptr) {
            return;
        }

        if (session->ssl_ctx != nullptr) {
#if defined(PUMP_HAVE_GNUTLS)
            gnutls_deinit((gnutls_session_t)session->ssl_ctx);
#elif defined(PUMP_HAVE_OPENSSL)
            SSL_free((SSL*)session->ssl_ctx);
#endif
        }
        object_delete(session);
    }

    int32_t tls_handshake(tls_session *session) {
#if defined(PUMP_HAVE_GNUTLS)
        int32_t ret = gnutls_handshake((gnutls_session_t)session->ssl_ctx);
        if (ret == 0) {
            return TLS_HANDSHAKE_OK;
        } else if (gnutls_error_is_fatal(ret) == 0) {
            if (gnutls_record_get_direction((gnutls_session_t)session->ssl_ctx) == 0) {
                return TLS_HANDSHAKE_READ;
            } else {
                return TLS_HANDSHAKE_SEND;
            }
        }
#elif defined(PUMP_HAVE_OPENSSL)
        int32_t ret = SSL_do_handshake((SSL*)session->ssl_ctx);
        int32_t ec = SSL_get_error((SSL*)session->ssl_ctx, ret);
        if (ec != SSL_ERROR_SSL) {
            if (ec == SSL_ERROR_NONE) {
                return TLS_HANDSHAKE_OK;
            }
            if (SSL_want_write((SSL*)session->ssl_ctx)) {
                return TLS_HANDSHAKE_SEND;
            } else if (SSL_want_read((SSL*)session->ssl_ctx)) {
                return TLS_HANDSHAKE_READ;
            }
        }
#endif
        // Handshake error
        return TLS_HANDSHAKE_ERROR;
    }

    bool tls_has_unread_data(tls_session *session) {
#if defined(PUMP_HAVE_GNUTLS)
        if (gnutls_record_check_pending((gnutls_session_t)session->ssl_ctx) > 0) {
            return true;
        }
#elif defined(PUMP_HAVE_OPENSSL)
        if (SSL_pending((SSL*)session->ssl_ctx) == 1) {
            return true;
        }
#endif
        return false;
    }

    int32_t tls_read(
        tls_session *session, 
        block_t *b, 
        int32_t size) {
#if defined(PUMP_HAVE_GNUTLS)
        int32_t ret = (int32_t)gnutls_read((gnutls_session_t)session->ssl_ctx, b, size);
        if (PUMP_LIKELY(ret > 0)) {
            return ret;
        } else if (ret == GNUTLS_E_AGAIN) {
            return -1;
        }
#elif defined(PUMP_HAVE_OPENSSL)
        int32_t ret = SSL_read((SSL*)session->ssl_ctx, b, size);
        if (PUMP_LIKELY(ret > 0)) {
            return ret;
        } else if (SSL_get_error((SSL*)session->ssl_ctx, ret) == SSL_ERROR_WANT_READ) {
            return -1;
        }
#endif
        return 0;
    }

    int32_t tls_send(
        tls_session *session, 
        const block_t *b, 
        int32_t size) {
#if defined(PUMP_HAVE_GNUTLS)
        return (int32_t)gnutls_write((gnutls_session_t)session->ssl_ctx, b, size);
#elif defined(PUMP_HAVE_OPENSSL)
        int32_t ret = SSL_write((SSL*)session->ssl_ctx, b, size);
        if (PUMP_LIKELY(ret > 0)) {
            return ret;
        } else if (SSL_get_error((SSL*)session->ssl_ctx, ret) == SSL_ERROR_WANT_WRITE) {
            return -1;
        }
#endif
        return 0;
    }

}  // namespace ssl
}  // namespace pump