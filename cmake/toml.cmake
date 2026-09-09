#[[

Reading TOML from CMake

Python lives in scripts/

    include(toml)
    toml_read("${dir}/tests.toml" doc)
    toml_get("${doc}" name    name)
    toml_get("${doc}" deps    depends)        # arrays work
    toml_get("${doc}" limit   extra tier_limit) # nested tables work

Requires CMake >= 3.19 for string(JSON) and python3 >= 3.11 for tomllib

]]

if (COMMAND toml_read)
    return()
endif ()

find_package(
    Python3 3.11
    COMPONENTS Interpreter
    REQUIRED)

set(_TOML_TO_JSON "${CMAKE_SOURCE_DIR}/scripts/toml_to_json.py")
if (NOT EXISTS "${_TOML_TO_JSON}")
    message(FATAL_ERROR "cmake/toml.cmake needs ${_TOML_TO_JSON}, which is missing")
endif ()

# toml_read(<path> <out_var>)
# Parses <path> and sets <out_var> to the document as a JSON string
function (toml_read toml_path out_var)
    if (NOT EXISTS "${toml_path}")
        set(${out_var}
            "{}"
            PARENT_SCOPE)
        return()
    endif ()

    # editing a .toml triggers a reconfigure
    if (NOT CMAKE_SCRIPT_MODE_FILE)
        set_property(
            DIRECTORY "${CMAKE_SOURCE_DIR}"
            APPEND
            PROPERTY CMAKE_CONFIGURE_DEPENDS "${toml_path}")
    endif ()

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${_TOML_TO_JSON}" "${toml_path}"
        OUTPUT_VARIABLE _json
        ERROR_VARIABLE _err
        RESULT_VARIABLE _rc)

    if (NOT _rc EQUAL 0)
        string(STRIP "${_err}" _err)
        message(FATAL_ERROR "${toml_path}: not valid TOML\n  ${_err}")
    endif ()

    set(${out_var}
        "${_json}"
        PARENT_SCOPE)
endfunction ()

# toml_get(<json> <out_var> <key>...)
#
# Fetches a value by key path
function (toml_get json out_var)
    string(
        JSON
        _value
        ERROR_VARIABLE
        _err
        GET
        "${json}"
        ${ARGN})
    if (_err)
        set(${out_var}
            ""
            PARENT_SCOPE)
    else ()
        set(${out_var}
            "${_value}"
            PARENT_SCOPE)
    endif ()
endfunction ()

# toml_length(<json> <out_var> <key>...)
function (toml_length json out_var)
    string(JSON _len ERROR_VARIABLE _err LENGTH "${json}" ${ARGN})
    if (_err)
        set(_len 0)
    endif ()
    set(${out_var}
        "${_len}"
        PARENT_SCOPE)
endfunction ()

# toml_get_list(<json> <out_var> <key>...)
function (toml_get_list json out_var)
    toml_length("${json}" _count ${ARGN})
    set(_items "")
    if (_count GREATER 0)
        math(EXPR _last "${_count} - 1")
        foreach (_i RANGE ${_last})
            string(JSON _item GET "${json}" ${ARGN} ${_i})
            list(APPEND _items "${_item}")
        endforeach ()
    endif ()
    set(${out_var}
        "${_items}"
        PARENT_SCOPE)
endfunction ()
