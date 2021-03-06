#include <a4/debug.h>

namespace a4 { namespace io {

    bool log_error;
    bool log_warning;
    bool log_notice;
    bool log_verbose;
    bool log_debug;

    int the_env_log_level;
    int the_set_log_level;

    const char * program_name;
    static std::string program_name_str;

    void set_program_name(const char * _program_name) {
        program_name_str = _program_name;
        program_name = program_name_str.c_str();
    }

    void set_effective_log_level() {
        int level = the_set_log_level > the_env_log_level ? the_set_log_level : the_env_log_level;
        if (level == -1) level = 3;
        log_error = log_warning = log_notice = log_verbose = log_debug = false;
        if (level > 0) log_error = true;
        if (level > 1) log_warning = true;
        if (level > 2) log_notice = true;
        if (level > 3) log_verbose = true;
        if (level > 4) log_debug = true;
    }

    void set_log_level(int level) {
        the_set_log_level = level;
        set_effective_log_level();
    }

    void set_env_log_level(int level) {
        the_env_log_level = level;
        set_effective_log_level();
    }

}
}

namespace {
    class Initializer {
    public:
        Initializer() {
            const char * level_string = getenv("A4_LOG_LEVEL");
            int log_level = -1;
            if (level_string != NULL) {
                char * endptr = NULL;
                log_level = strtol(level_string, &endptr, 10);
                if (not (*level_string != '\0' and *endptr == '\0')) {
                    std::cerr << "a4: " << "A4_LOG_LEVEL has invalid value '"
                              << level_string << "'. Integer expected."
                              << std::endl;
                    log_level = -1;
                }
            }
            a4::io::set_program_name("a4");
            a4::io::set_log_level(-1);
            a4::io::set_env_log_level(log_level);
        }
    };
    
    static Initializer init;
}
