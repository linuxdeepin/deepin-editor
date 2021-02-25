#install iconv
execute_process(COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/iconv_install.sh" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#install enca
execute_process(COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/enca_install.sh" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#install uchardet
execute_process(COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/uchartdet_install.sh" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


