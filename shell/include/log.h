#ifndef LOG_H
#define LOG_H

void log_init();                              // for initialization
void log_store(const char *cmd);              // for storing raw command
void log_show();                              // for displaying log (unused)
void log_command(int argc, char **argv, const char *raw_input);      // handle builtin log

#endif
