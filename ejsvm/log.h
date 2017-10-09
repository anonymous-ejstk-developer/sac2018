#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG_PRINT

#define LOG(...) fprintf(log_stream, __VA_ARGS__)
#define LOG_FUNC fprintf(log_stream, "%-16s: ", __func__)
#define LOG_ERR(...) do { LOG_FUNC; fprintf(log_stream, __VA_ARGS__); } while (0)
#define LOG_EXIT(...) do { LOG_FUNC; fprintf(log_stream, __VA_ARGS__); exit(1); } while (0)

#else
#define LOG(...)
#define LOG_FUNC
#define LOG_ERR(...)
#define LOG_EXIT(...) exit(1)

#endif // DEBUG

#endif // LOG_H_
