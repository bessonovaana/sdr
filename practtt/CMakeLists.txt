cmake_minimum_required(VERSION 3.16)
project(practtt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Упрощаем - все выходные файлы сразу в build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Определяем директории проекта
set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

# Создаем include директорию, если её нет
file(MAKE_DIRECTORY ${INCLUDE_DIR})

# Поиск системных библиотек
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SoapySDR REQUIRED)

# Выводим пути для отладки
message(STATUS "SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2_LIBRARIES: ${SDL2_LIBRARIES}")

# Проверяем существование папок с подмодулями
if(EXISTS ${THIRD_PARTY_DIR}/imgui AND EXISTS ${THIRD_PARTY_DIR}/implot)
    message(STATUS "Found ImGui and ImPlot in ${THIRD_PARTY_DIR}")
    
    # Библиотека ImGui
    add_library(imgui STATIC
        ${THIRD_PARTY_DIR}/imgui/imgui.cpp
        ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
        ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
        ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
        ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
        ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
        ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
    )
    
    target_include_directories(imgui PUBLIC 
        ${THIRD_PARTY_DIR}/imgui
        ${THIRD_PARTY_DIR}/imgui/backends
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
    )
    
    target_compile_options(imgui PRIVATE -fPIC)
    
    target_link_libraries(imgui PRIVATE 
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
    )
    
    # Библиотека Implot
    add_library(implot STATIC
        ${THIRD_PARTY_DIR}/implot/implot.cpp
        ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
        ${THIRD_PARTY_DIR}/implot/implot_items.cpp
    )

    target_include_directories(implot PUBLIC 
        ${THIRD_PARTY_DIR}/implot
        ${THIRD_PARTY_DIR}/imgui
        ${SDL2_INCLUDE_DIRS}
    )

    target_link_libraries(implot PRIVATE 
        imgui
        ${SDL2_LIBRARIES}
    )

    target_compile_options(implot PRIVATE -fPIC)
    
    set(HAVE_GUI_LIBS TRUE)
else()
    message(WARNING "ImGui or ImPlot not found in ${THIRD_PARTY_DIR}. GUI features disabled.")
    set(HAVE_GUI_LIBS FALSE)
endif()

# ============ Исполняемый файл ============

# Собираем список исходных файлов
set(MAIN_SOURCE_FILES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/mapper.cpp
    ${SRC_DIR}/gui.cpp
    ${SRC_DIR}/tx.cpp
    ${SRC_DIR}/rx.cpp

)

# Добавляем исполняемый файл
add_executable(main ${MAIN_SOURCE_FILES})

# Базовые библиотеки
target_link_libraries(main PRIVATE
    ${SoapySDR_LIBRARIES}
)

# Подключаем GUI библиотеки, если они есть
if(HAVE_GUI_LIBS)
    target_link_libraries(main PRIVATE
        imgui
        implot
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
    )
endif()

# Подключаем директории для компиляции основного приложения
target_include_directories(main PRIVATE
    ${INCLUDE_DIR}
    ${SRC_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${SoapySDR_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
)

if(HAVE_GUI_LIBS)
    target_include_directories(main PRIVATE
        ${THIRD_PARTY_DIR}/imgui
        ${THIRD_PARTY_DIR}/imgui/backends
        ${THIRD_PARTY_DIR}/implot
    )
endif()

# Добавляем команду для копирования (теперь с проверкой существования)
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND}
)
