#pragma once

// Construct argument list from NUL-delimitted
// strings within a character array. Arguments
// are taken until an empty string is found (i.e.
// two NULs in succession.)

struct mockargs {
    explicit mockargs(const char* argstr) {
        set(argstr);
    }

    void set(const char* argstr) {
        char* p = const_cast<char*>(argstr);
        args.push_back(p);
        for (; *p; ) {
            while (*p++) ;
            args.push_back(p);
        }
        args.back() = 0;
        argv = args.data();
        argc = args.size()-1;
    }

    int argc;
    char** argv;
    std::vector<char*> args;
};


