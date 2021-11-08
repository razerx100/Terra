#ifndef __VK_THROW_MACROS_HPP__
#define __VK_THROW_MACROS_HPP__
#include <VKExceptions.hpp>

#define VK_THROW_FAILED(err, fun) if((err = fun) < 0) throw VKException(__LINE__, __FILE__, err)
#define VK_GENERIC_THROW(errorMsg) throw GenericException(__LINE__, __FILE__, errorMsg)

#endif
