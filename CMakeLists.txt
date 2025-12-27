cmake_minimum_required(VERSION 3.16)
project(DrachmaBlockchain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(DRACHMA_BUILD_TESTS "Build Drachma test executables" ON)
option(DRACHMA_BUILD_GUI "Build the Qt desktop wallet" OFF)
option(DRACHMA_COVERAGE "Enable coverage instrumentation" OFF)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(LevelDB QUIET)

if(DRACHMA_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

# Layer 1: consensus-critical primitives
add_library(drachma_layer1
    layer1-core/crypto/schnorr.cpp
    layer1-core/crypto/tagged_hash.cpp
    layer1-core/script/interpreter.cpp
    layer1-core/merkle/merkle.cpp
    layer1-core/consensus/params.cpp
    layer1-core/consensus/fork_resolution.cpp
    layer1-core/consensus/versioning/versionbits.cpp
    layer1-core/consensus/genesis.cpp
    layer1-core/chainstate/coins.cpp
    layer1-core/pow/difficulty.cpp
    layer1-core/pow/difficulty_adjust.cpp
    layer1-core/pow/sha256d.cpp
    layer1-core/block/block.cpp
    layer1-core/storage/blockstore.cpp
    layer1-core/tx/transaction.cpp
    layer1-core/validation/validation.cpp
    layer1-core/validation/anti_dos.cpp
)

target_include_directories(drachma_layer1
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/layer1-core
)

target_link_libraries(drachma_layer1
    PUBLIC OpenSSL::Crypto
)

if(LevelDB_FOUND)
    target_link_libraries(drachma_layer1 PUBLIC LevelDB::LevelDB)
    target_compile_definitions(drachma_layer1 PUBLIC DRACHMA_HAVE_LEVELDB)
endif()

# Layer 2: services and orchestration
add_library(drachma_layer2
    layer2-services/policy/policy.cpp
    layer2-services/net/p2p.cpp
    layer2-services/wallet/keystore/keystore.cpp
    layer2-services/wallet/wallet.cpp
    layer2-services/index/txindex.cpp
    layer2-services/crosschain/bridge/bridge_manager.cpp
    layer2-services/crosschain/relayer/relayer.cpp
    layer2-services/crosschain/messages/crosschain_msg.cpp
    layer2-services/crosschain/validation/proof_validator.cpp
    layer2-services/mempool/mempool.cpp
    layer2-services/rpc/rpcserver.cpp
)

target_include_directories(drachma_layer2
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/layer2-services
        ${CMAKE_CURRENT_SOURCE_DIR}/layer1-core
)

target_link_libraries(drachma_layer2
    PUBLIC drachma_layer1
    PUBLIC Boost::system
    PUBLIC leveldb
)

if(LevelDB_FOUND)
    target_link_libraries(drachma_layer2 PUBLIC LevelDB::LevelDB)
endif()

# Miners
add_executable(drachma_cpu_miner miners/cpu/main.cpp miners/stratum.cpp)
target_include_directories(drachma_cpu_miner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/layer1-core)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
  target_compile_options(drachma_cpu_miner PRIVATE -msse4.1 -mavx2)
endif()
target_link_libraries(drachma_cpu_miner PRIVATE drachma_layer1 Boost::system)

# Optional Qt GUI (off by default to avoid toolchain requirement in CI)
if(DRACHMA_BUILD_GUI)
    find_package(Qt6 COMPONENTS Widgets Network QUIET)
    find_package(OpenSSL REQUIRED)

    set(DRACHMA_QT_RESOURCES)
    set(DRACHMA_QT_LIBS)
    if(Qt6_FOUND)
        qt_add_resources(DRACHMA_QT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/layer3-app/resources/icons.qrc)
        list(APPEND DRACHMA_QT_LIBS Qt6::Widgets Qt6::Network)
    else()
        find_package(Qt5 COMPONENTS Widgets Network REQUIRED)
        qt5_add_resources(DRACHMA_QT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/layer3-app/resources/icons.qrc)
        list(APPEND DRACHMA_QT_LIBS Qt5::Widgets Qt5::Network)
    endif()

    qt5_add_resources(DRACHMA_QT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/layer3-app/resources/icons.qrc)
    add_executable(drachma_qt layer3-app/qt/main.cpp ${DRACHMA_QT_RESOURCES})
    target_include_directories(drachma_qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/layer1-core ${CMAKE_CURRENT_SOURCE_DIR}/layer2-services)
    target_link_libraries(drachma_qt PRIVATE ${DRACHMA_QT_LIBS} OpenSSL::SSL OpenSSL::Crypto drachma_layer2)
endif()

# Tests
option(DRACHMA_BUILD_FUZZ "Build fuzzing harnesses" OFF)

if(DRACHMA_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)

    enable_testing()
    add_executable(schnorr_test tests/crypto/schnorr_test.cpp)
    target_link_libraries(schnorr_test PRIVATE drachma_layer1)
    add_test(NAME schnorr_test COMMAND schnorr_test)

    add_executable(schnorr_vectors_test tests/crypto/schnorr_vectors_test.cpp)
    target_link_libraries(schnorr_vectors_test PRIVATE drachma_layer1 GTest::gtest_main)
    gtest_discover_tests(schnorr_vectors_test)

    add_executable(merkle_test tests/merkle/merkle_test.cpp)
    target_link_libraries(merkle_test PRIVATE drachma_layer1)
    add_test(NAME merkle_test COMMAND merkle_test)

    add_executable(supply_test tests/consensus/supply_test.cpp)
    target_link_libraries(supply_test PRIVATE drachma_layer1)
    add_test(NAME supply_test COMMAND supply_test)

    add_executable(retarget_test tests/pow/retarget_test.cpp)
    target_link_libraries(retarget_test PRIVATE drachma_layer1)
    add_test(NAME retarget_test COMMAND retarget_test)

    add_executable(difficulty_gtest tests/pow/difficulty_gtest.cpp)
    target_link_libraries(difficulty_gtest PRIVATE drachma_layer1 GTest::gtest_main)
    gtest_discover_tests(difficulty_gtest)

    add_executable(pow_check_gtest tests/pow/pow_check_gtest.cpp)
    target_link_libraries(pow_check_gtest PRIVATE drachma_layer1 GTest::gtest_main)
    gtest_discover_tests(pow_check_gtest)

    add_executable(difficulty_adjust_test tests/pow/difficulty_adjust_test.cpp)
    target_link_libraries(difficulty_adjust_test PRIVATE drachma_layer1)
    add_test(NAME difficulty_adjust_test COMMAND difficulty_adjust_test)

    add_executable(validation_tests tests/validation/validation_tests.cpp)
    target_link_libraries(validation_tests PRIVATE drachma_layer1)
    add_test(NAME validation_tests COMMAND validation_tests)

    add_executable(fork_resolution_test tests/consensus/fork_resolution_test.cpp)
    target_link_libraries(fork_resolution_test PRIVATE drachma_layer1)
    add_test(NAME fork_resolution_test COMMAND fork_resolution_test)

    add_executable(chainstate_tests tests/chainstate/chainstate_tests.cpp)
    target_link_libraries(chainstate_tests PRIVATE drachma_layer1)
    add_test(NAME chainstate_tests COMMAND chainstate_tests)

    add_executable(mempool_tests tests/mempool/mempool_tests.cpp)
    target_link_libraries(mempool_tests PRIVATE drachma_layer2)
    add_test(NAME mempool_tests COMMAND mempool_tests)

    add_executable(mempool_stress_test tests/mempool/mempool_stress_test.cpp)
    target_link_libraries(mempool_stress_test PRIVATE drachma_layer2 GTest::gtest_main)
    gtest_discover_tests(mempool_stress_test)

    add_executable(wallet_sign_gtest tests/wallet/wallet_sign_gtest.cpp)
    target_link_libraries(wallet_sign_gtest PRIVATE drachma_layer2 GTest::gtest_main)
    gtest_discover_tests(wallet_sign_gtest)

    add_executable(block_validation_tests tests/validation/block_validation_tests.cpp)
    target_link_libraries(block_validation_tests PRIVATE drachma_layer1)
    add_test(NAME block_validation_tests COMMAND block_validation_tests)

    add_executable(p2p_integration_test tests/net/p2p_integration_test.cpp)
    target_link_libraries(p2p_integration_test PRIVATE drachma_layer2)
    add_test(NAME p2p_integration_test COMMAND p2p_integration_test)

    add_executable(p2p_dos_sim_test tests/net/p2p_dos_sim_test.cpp)
    target_link_libraries(p2p_dos_sim_test PRIVATE drachma_layer2 GTest::gtest_main)
    gtest_discover_tests(p2p_dos_sim_test)

    add_executable(p2p_multinode_gtest tests/net/p2p_multinode_gtest.cpp)
    target_link_libraries(p2p_multinode_gtest PRIVATE drachma_layer2 GTest::gtest_main)
    gtest_discover_tests(p2p_multinode_gtest)

    add_executable(bloom_filter_gtest tests/net/bloom_filter_gtest.cpp)
    target_link_libraries(bloom_filter_gtest PRIVATE drachma_layer2 GTest::gtest_main)
    gtest_discover_tests(bloom_filter_gtest)

    add_executable(anti_dos_tests tests/validation/anti_dos_tests.cpp)
    target_link_libraries(anti_dos_tests PRIVATE drachma_layer1)
    add_test(NAME anti_dos_tests COMMAND anti_dos_tests)

    add_executable(attacks_sim tests/attacks/attacks_sim.cpp)
    target_link_libraries(attacks_sim PRIVATE drachma_layer1)
    add_test(NAME attacks_sim COMMAND attacks_sim)

    add_executable(rpc_server_test tests/rpc/rpc_server_test.cpp)
    target_link_libraries(rpc_server_test PRIVATE drachma_layer2 GTest::gtest_main)
    gtest_discover_tests(rpc_server_test)

    add_executable(regtest_mode_test tests/integration/regtest_mode_test.cpp)
    target_link_libraries(regtest_mode_test PRIVATE GTest::gtest_main)
    gtest_discover_tests(regtest_mode_test)

    add_executable(regtest_private_chain_test tests/integration/regtest_private_chain_test.cpp)
    target_link_libraries(regtest_private_chain_test PRIVATE drachma_layer1 GTest::gtest_main)
    gtest_discover_tests(regtest_private_chain_test)

    function(drachma_add_fuzz_target target source)
        add_executable(${target} ${source})
        if(ARGC GREATER 2)
            target_link_libraries(${target} PRIVATE ${ARGN})
        else()
            target_link_libraries(${target} PRIVATE drachma_layer1)
        endif()
        if(DRACHMA_BUILD_FUZZ)
            target_compile_options(${target} PRIVATE -fsanitize=fuzzer,address)
            target_link_options(${target} PRIVATE -fsanitize=fuzzer,address)
        endif()
    endfunction()

    if(DRACHMA_BUILD_FUZZ)
        drachma_add_fuzz_target(fuzz_schnorr tests/crypto/fuzz_schnorr.cpp)
        drachma_add_fuzz_target(fuzz_tagged_hash tests/crypto/fuzz_tagged_hash.cpp)
        drachma_add_fuzz_target(fuzz_validation tests/validation/fuzz_validation.cpp)
        drachma_add_fuzz_target(fuzz_fork_resolution tests/consensus/fuzz_fork_resolution.cpp)
        drachma_add_fuzz_target(fuzz_difficulty tests/pow/fuzz_difficulty.cpp)
        drachma_add_fuzz_target(fuzz_merkle tests/merkle/fuzz_merkle.cpp)
        drachma_add_fuzz_target(fuzz_wallet_sign tests/crypto/fuzz_wallet_sign.cpp drachma_layer2)
        drachma_add_fuzz_target(fuzz_block_header tests/validation/fuzz_block_header.cpp)
    endif()
endif()

# Install rules
install(TARGETS drachma_layer1 drachma_layer2 drachma_cpu_miner
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY layer1-core/ DESTINATION include/drachma/layer1 FILES_MATCHING PATTERN "*.h")
install(DIRECTORY layer2-services/ DESTINATION include/drachma/layer2 FILES_MATCHING PATTERN "*.h")
