/* Configuration options for the Foenix Library
 * Author: Vincent Barrilliot
 */

#ifndef _FOENIX_LIB_CONFIG_H_
#define _FOENIX_LIB_CONFIG_H_

#ifndef LONG_STACK_FRAMES_SUPPORT
# if defined(MC68000)
#  define LONG_STACK_FRAMES_SUPPORT 0
# else
#  define LONG_STACK_FRAMES_SUPPORT 1
# endif
#endif

#endif

