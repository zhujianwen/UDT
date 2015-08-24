#include "results.h"


const char* ResultText(int result)
{
	switch (result)
	{
	case ZHD_SUCCESS: return "SUCCESS";
	case ZHD_FAILURE: return "FAILURE";

	default: return "UNKNOWN";
	}
}