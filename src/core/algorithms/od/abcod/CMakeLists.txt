set(NAME od.abcod)
desbordante_add_lib(NAME)
target_sources(
    ${NAME} PRIVATE abcod_verifier.cpp lmb.cpp ordered_sequence.cpp segmentation.cpp
            series_discovery.cpp
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::util spdlog::spdlog_header_only
            Boost::headers
)
