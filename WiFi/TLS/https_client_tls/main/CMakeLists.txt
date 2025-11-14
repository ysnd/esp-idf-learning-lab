idf_component_register(
    SRCS "https_client_tls.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_wifi esp_netif esp_http_client mbedtls
    COMPONENT_EMBED_FILES "~/embedded-learning/WiFi/TLS/https_client_tls/main/certs/server_cert.pem"
)

