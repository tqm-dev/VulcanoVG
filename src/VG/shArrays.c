
/*--------------------------------------------
 * Definitions of all the arrays used
 *--------------------------------------------*/

#include "shArrays.h"

#define _ITEM_T  SHint
#define _ARRAY_T SHIntArray
#define _FUNC_T  shIntArray
#define _ARRAY_DEFINE
#include "shArrayBase.h"

#define _ITEM_T  SHuint8
#define _ARRAY_T SHUint8Array
#define _FUNC_T  shUint8Array
#define _ARRAY_DEFINE
#include "shArrayBase.h"

#define _ITEM_T  SHfloat
#define _ARRAY_T SHFloatArray
#define _FUNC_T  shFloatArray
#define _ARRAY_DEFINE
#include "shArrayBase.h"

#define _ITEM_T  SHRectangle
#define _ARRAY_T SHRectArray
#define _FUNC_T  shRectArray
#define _COMPARE_T(x,y) 1
#define _ARRAY_DEFINE
#include "shArrayBase.h"
