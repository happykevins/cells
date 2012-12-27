SET(SRC_FILES
	CCell.cpp
	CCells.cpp
	CCreationFactory.cpp
	CCreationWorker.cpp
	CDownloader.cpp
	cells.cpp
	CUtils.cpp
	main.cpp
	md5.c
	zpip.c
)


INCLUDE_DIRECTORIES(
${PROJECT_SOURCE_DIR}/inc
${PROJECT_SOURCE_DIR}/thirdparty/include/linux32
)

LINK_DIRECTORIES(
${PROJECT_SOURCE_DIR}/thirdparty/x86
)

ADD_EXECUTABLE(cells ${SRC_FILES})

TARGET_LINK_LIBRARIES(cells tinyxml curl z rt pthread)


