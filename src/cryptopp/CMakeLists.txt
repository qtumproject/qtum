cmake_minimum_required(VERSION 3.10)
project(CryptoppLibrary VERSION 5.6.5)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define source files explicitly
set(CRYPTOPP_SOURCES
    cryptlib.cpp
    cpu.cpp
    integer.cpp
    algebra.cpp
    algparam.cpp
    asn.cpp
    basecode.cpp
    dsa.cpp
    des.cpp
    dessp.cpp
    dll.cpp
    ec2n.cpp
    eccrypto.cpp
    ecp.cpp
    eprecomp.cpp
    filters.cpp
    fips140.cpp
    gf2n.cpp
    gfpcrypt.cpp
    hex.cpp
    hmac.cpp
    hrtimer.cpp
    iterhash.cpp
    keccak.cpp
    misc.cpp
    modes.cpp
    mqueue.cpp
    nbtheory.cpp
    oaep.cpp
    osrng.cpp
    polynomi.cpp
    pubkey.cpp
    queue.cpp
    randpool.cpp
    rdtables.cpp
    rijndael.cpp
    rng.cpp
    sha.cpp
    strciphr.cpp
    winpipes.cpp
    sha3.cpp
)

# Define header files explicitly
set(CRYPTOPP_HEADERS
    cryptlib.h
    cpu.h
    integer.h
    algebra.h
    algparam.h
    asn.h
    basecode.h
    dsa.h
    des.h
    dll.h
    ec2n.h
    eccrypto.h
    ecp.h
    eprecomp.h
    filters.h
    fips140.h
    gf2n.h
    gfpcrypt.h
    hex.h
    hmac.h
    hrtimer.h
    iterhash.h
    keccak.h
    misc.h
    modes.h
    mqueue.h
    nbtheory.h
    oaep.h
    osrng.h
    polynomi.h
    pubkey.h
    queue.h
    randpool.h
    rijndael.h
    rng.h
    sha.h
    strciphr.h
    winpipes.h
    sha3.h
    aes.h
    factory.h
    config.h
    dmac.h
    drbg.h
    esign.h
    fhmqv.h
    hkdf.h
    hmqv.h
    lubyrack.h
    mersenne.h
    modarith.h
    modexppc.h
    nr.h
    pwdbased.h
    secblock.h
    siphash.h
    trunhash.h
    validate.h
    words.h
    seckey.h
    stdcpp.h
    trap.h
    smartptr.h
    simple.h
    argnames.h
    dh.h
    mqv.h
    ecpoint.h
    fltrimpl.h
    oids.h
    ossig.h
    pch.h
    cbcmac.h
    trdlocal.h
    ccm.h
)

# Compile options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CRYPTOPP_COMPILE_FLAGS "-DNDEBUG -fPIC -O2 -g2")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CRYPTOPP_COMPILE_FLAGS "/DNDEBUG /O2 /Zi")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CRYPTOPP_COMPILE_FLAGS}")

# Create the static library
add_library(cryptopp STATIC ${CRYPTOPP_SOURCES} ${CRYPTOPP_HEADERS})

