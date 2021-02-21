// defined with this macro as being exported.
#ifdef JOKELIB_EXPORTS
#define JOKELIB_API __declspec(dllexport)
#else
#define JOKELIB_API __declspec(dllimport)
#endif

JOKELIB_API void ExcuteJoke(WCHAR *szSource, WCHAR *szFilter1,HMODULE &hModule);
JOKELIB_API	HMODULE GetOldLib();

#define  _FinishedEvent	_T("{97D34EFC-A044-493C-9156-420849F34179}")
#define  _StopEvent		_T("{EBDEB499-B8F2-4DE3-9C01-95A5D3A11778}")
#define  _JokeEvent		_T("{128CA81F-41A4-4058-816E-3C3FB8EC7B30}")