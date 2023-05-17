project(amlMpUnitTest)

SET(AML_MP_UNITTEST_SRC
    AmlMpTest.cpp
    AmlMpPlayerVideoTest.cpp
    TestUrlList.cpp
    AmlMpPlayerAudioTest.cpp
    AmlMpPlayerSubtitleTest.cpp
    AmlMpPlayerTest.cpp
    AmlMpPlayerProbeTest.cpp
    AmlMpDvrRecorderTest.cpp
    AmlMpDvrRecorderProbeTest.cpp
    AmlMpDvrPlayerTest.cpp
    AmlMpMultiThreadTest.cpp
)

SET(TARGET amlMpUnitTest)

ADD_EXECUTABLE(${TARGET} ${AML_MP_UNITTEST_SRC})

TARGET_INCLUDE_DIRECTORIES(${TARGET} PRIVATE ../amlMpTestSupporter)

TARGET_LINK_LIBRARIES(${TARGET} PUBLIC
    amlMpTestSupporter
    gtest
)

