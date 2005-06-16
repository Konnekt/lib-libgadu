// Zwraca komunikat b³êdu

#ifdef LIBGADU_EXPORTS
#define LIBGADU_DLL __declspec(dllexport)
#else
#define LIBGADU_DLL __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBGADU_DLL
const char * WSAstrerror(int err);
LIBGADU_DLL
int WSAerrno(void);

int ioctl(int fd, int request, ...);

#ifdef __cplusplus
};
#endif


