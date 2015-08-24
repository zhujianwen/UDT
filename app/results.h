#ifndef __ZHD_RESULTS_H__
#define __ZHD_RESULTS_H__


//////////////////////////////////////////////////////////////////////////
// data type
typedef long long Int64;
typedef unsigned long long UInt64;
typedef int Int32;
typedef unsigned int UInt32;
typedef short Int16;
typedef unsigned short UInt16;
typedef char Int8;
typedef unsigned char UInt8;


//////////////////////////////////////////////////////////////////////////
// Macros
#define ZHD_SUCCESS                       0
#define ZHD_FAILURE                        (-1)

#define ZHD_FAILED(result)                ((result) != ZHD_SUCCESS)
#define ZHD_SUCCEEDED(result)        ((result) == ZHD_SUCCESS)

#if !defined(ZHD_ERROR_BASE)
#define ZHD_ERROR_BASE -20000
#endif

// error bases
#define ZHD_ERROR_BASE_GENERAL         (ZHD_ERROR_BASE-0)
#define ZHD_ERROR_BASE_LIST                 (ZHD_ERROR_BASE-100)
#define ZHD_ERROR_BASE_FILE                 (ZHD_ERROR_BASE-200)
#define ZHD_ERROR_BASE_IO                    (ZHD_ERROR_BASE-300)
#define ZHD_ERROR_BASE_SOCKET           (ZHD_ERROR_BASE-400)
#define ZHD_ERROR_BASE_INTERFACES    (ZHD_ERROR_BASE-500)
#define ZHD_ERROR_BASE_XML                (ZHD_ERROR_BASE-600)
#define ZHD_ERROR_BASE_UNIX               (ZHD_ERROR_BASE-700)
#define ZHD_ERROR_BASE_HTTP               (ZHD_ERROR_BASE-800)
#define ZHD_ERROR_BASE_THREADS         (ZHD_ERROR_BASE-900)
#define ZHD_ERROR_BASE_SERIAL_PORT   (ZHD_ERROR_BASE-1000)
#define ZHD_ERROR_BASE_TLS                  (ZHD_ERROR_BASE-1100)

// general errors
#define ZHD_ERROR_INVALID_PARAMETERS   (ZHD_ERROR_BASE_GENERAL - 0)
#define ZHD_ERROR_PERMISSION_DENIED     (ZHD_ERROR_BASE_GENERAL - 1)
#define ZHD_ERROR_OUT_OF_MEMORY         (ZHD_ERROR_BASE_GENERAL - 2)
#define ZHD_ERROR_NO_SUCH_NAME           (ZHD_ERROR_BASE_GENERAL - 3)
#define ZHD_ERROR_NO_SUCH_PROPERTY     (ZHD_ERROR_BASE_GENERAL - 4)
#define ZHD_ERROR_NO_SUCH_ITEM             (ZHD_ERROR_BASE_GENERAL - 5)
#define ZHD_ERROR_NO_SUCH_CLASS           (ZHD_ERROR_BASE_GENERAL - 6)
#define ZHD_ERROR_OVERFLOW                    (ZHD_ERROR_BASE_GENERAL - 7)
#define ZHD_ERROR_INTERNAL                      (ZHD_ERROR_BASE_GENERAL - 8)
#define ZHD_ERROR_INVALID_STATE              (ZHD_ERROR_BASE_GENERAL - 9)
#define ZHD_ERROR_INVALID_FORMAT          (ZHD_ERROR_BASE_GENERAL - 10)
#define ZHD_ERROR_INVALID_SYNTAX           (ZHD_ERROR_BASE_GENERAL - 11)
#define ZHD_ERROR_NOT_IMPLEMENTED      (ZHD_ERROR_BASE_GENERAL - 12)
#define ZHD_ERROR_NOT_SUPPORTED          (ZHD_ERROR_BASE_GENERAL - 13)
#define ZHD_ERROR_TIMEOUT                       (ZHD_ERROR_BASE_GENERAL - 14)
#define ZHD_ERROR_WOULD_BLOCK              (ZHD_ERROR_BASE_GENERAL - 15)
#define ZHD_ERROR_TERMINATED                 (ZHD_ERROR_BASE_GENERAL - 16)
#define ZHD_ERROR_OUT_OF_RANGE             (ZHD_ERROR_BASE_GENERAL - 17)
#define ZHD_ERROR_OUT_OF_RESOURCES      (ZHD_ERROR_BASE_GENERAL - 18)
#define ZHD_ERROR_NOT_ENOUGH_SPACE    (ZHD_ERROR_BASE_GENERAL - 19)
#define ZHD_ERROR_INTERRUPTED                (ZHD_ERROR_BASE_GENERAL - 20)
#define ZHD_ERROR_CANCELLED                   (ZHD_ERROR_BASE_GENERAL - 21)

#define ZHD_ERROR_BASE_ERRNO                  (ZHD_ERROR_BASE-2000)
#define ZHD_ERROR_ERRNO(e)                       (ZHD_ERROR_BASE_ERRNO - (e))


#if !defined(ZHD_CONFIG_THREAD_STACK_SIZE)
#define ZHD_CONFIG_THREAD_STACK_SIZE 0
#endif

//////////////////////////////////////////////////////////////////////////
// functions
const char* ResultText(int result);


//////////////////////////////////////////////////////////////////////////
// constants
// error codes
const int ZHD_ERROR_CALLBACK_HANDLER_SHUTDOWN = ZHD_ERROR_BASE_THREADS-0;
const int ZHD_ERROR_CALLBACK_NOTHING_PENDING  = ZHD_ERROR_BASE_THREADS-1;

const int ZHD_TIMEOUT_INFINITE                               = -1;
const int ZHD_THREAD_PRIORITY_MIN                        = -15;
const int ZHD_THREAD_PRIORITY_IDLE                        = -15;
const int ZHD_THREAD_PRIORITY_LOWEST                  =  -2;
const int ZHD_THREAD_PRIORITY_BELOW_NORMAL    =  -1;
const int ZHD_THREAD_PRIORITY_NORMAL                 =   0;
const int ZHD_THREAD_PRIORITY_ABOVE_NORMAL     =   1;
const int ZHD_THREAD_PRIORITY_HIGHEST                  =   2;
const int ZHD_THREAD_PRIORITY_TIME_CRITICAL        =  15;
const int ZHD_THREAD_PRIORITY_MAX                        =  15;


#endif	// __ZHD_RESULTS_H__