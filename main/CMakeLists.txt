file(GLOB_RECURSE SOURCES "*.c" "*.cpp")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "."
                       "UI"
                       "wifi"
                       "mqtt"
                       EMBED_TXTFILES "certs/ca_cert.pem"

                       )
