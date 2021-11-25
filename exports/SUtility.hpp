#ifndef __S_UTILITY_HPP__
#define __S_UTILITY_HPP__

#ifdef BUILD_TERRA
#define TERRA_DLL __declspec(dllexport)
#else
#define TERRA_DLL __declspec(dllimport)
#endif

struct TERRA_DLL SRect {
	long left;
	long top;
	long right;
	long bottom;
};
#endif
