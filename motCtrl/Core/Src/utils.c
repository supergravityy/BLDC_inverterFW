#include "utils.h"

/*----------------------------*/
// INTERPOLATION
/*----------------------------*/

#define MIN_MAP_SIZE	(2u)
#define UINT16_MAX		(65535u)
#define UINT16_MIN		(0u)
#define INT16_MAX		(32767u)
#define INT16_MIN		(-32768)
#define UINT32_MIN		(0u)

/*
 * linear_interpolation
 * 
 *              (y1 - y0)
 *  f(x) = y0 + --------- * (x - x0)
 *              (x1 - x0)
 */
#define UTILS_IPOL_CAL_SLOPE( x0, x1, y0, y1, input)		( y0 + (y1 - y0) / (x1 - x0) * (input - x0) )

uint16_t utils_ipol_u16u16(const uint16_t* mapX, const uint16_t* mapY, uint16_t mapSize, uint16_t input)
{
	uint16_t mapIdx;
	uint16_t x0 = 0, x1 = 0;
	uint16_t y0 = 0, y1 = 0;
	float result;

	/*-----------------------------------------*/
	// check map size validity
	/*-----------------------------------------*/

	if (mapSize < MIN_MAP_SIZE)
	{
		return 0; // map size is too small
	}

	/*-----------------------------------------*/
	// check boundary condition (ascending map only)
	/*-----------------------------------------*/

	if (input < mapX[0])
	{
		return (uint16_t)mapY[0];
	}
	else if (input > mapX[mapSize - 1])
	{
		return (uint16_t)mapY[mapSize - 1];
	}
	else
	{
		// do nothing
	}

	/*-----------------------------------------*/
	// find a proper index for interpolation
	/*-----------------------------------------*/

	for (mapIdx = 0; mapIdx < mapSize-1; mapIdx++)
	{
		if (mapX[mapIdx] <= input && input <= mapX[mapIdx + 1]) // x is ascending
		{
			x0 = mapX[mapIdx];
			x1 = mapX[mapIdx + 1];
			y0 = mapY[mapIdx];
			y1 = mapY[mapIdx + 1];
			break;
		}
		else
		{
			// do nothing
		}
	}

	/*-----------------------------------------*/
	// saturation check
	/*-----------------------------------------*/
	result = UTILS_IPOL_CAL_SLOPE((float)x0, (float)x1, (float)y0, (float)y1, (float)input);

	if (result > (float)UINT16_MAX)			return UINT16_MAX;
	else if (result < (float)UINT16_MIN)	return UINT16_MIN;
	else									return (uint16_t)result;
}

uint32_t utils_ipol_u16u32(const uint16_t* mapX, const uint32_t* mapY, uint16_t mapSize, uint16_t input)
{
	uint16_t mapIdx;
	uint16_t x0 = 0, x1 = 0;
	uint32_t y0 = 0, y1 = 0;
	float result;

	/*-----------------------------------------*/
	// check map size validity
	/*-----------------------------------------*/
	if (mapSize < MIN_MAP_SIZE)
	{
		return 0; // map size is too small
	}

	/*-----------------------------------------*/
	// check boundary condition (ascending map only)
	/*-----------------------------------------*/
	if (input < mapX[0])
	{
		return (uint32_t)mapY[0];
	}
	else if (input > mapX[mapSize - 1])
	{
		return (uint32_t)mapY[mapSize - 1];
	}
	else
	{
		// do nothing
	}

	/*-----------------------------------------*/
	// find a proper index for interpolation
	/*-----------------------------------------*/
	for (mapIdx = 0; mapIdx < mapSize - 1; mapIdx++)
	{
		if (mapX[mapIdx] <= input && input <= mapX[mapIdx + 1]) // x is ascending
		{
			x0 = mapX[mapIdx];
			x1 = mapX[mapIdx + 1];
			y0 = mapY[mapIdx];
			y1 = mapY[mapIdx + 1];
			break;
		}
		else
		{
			// do nothing
		}
	}

	/*-----------------------------------------*/
	// saturation check
	/*-----------------------------------------*/
	result = UTILS_IPOL_CAL_SLOPE((float)x0, (float)x1, (float)y0, (float)y1, (float)input);

	if (result < (float)UINT16_MIN)	return UINT32_MAX;
	else							return (uint32_t)result;
}

int16_t utils_ipol_s16s16(const int16_t* mapX, const int16_t* mapY, uint16_t mapSize, int16_t input)
{
	uint16_t mapIdx;
	int16_t x0 = 0, x1 = 0;
	int16_t y0 = 0, y1 = 0;
	float result;

	/*-----------------------------------------*/
	// check map size validity
	/*-----------------------------------------*/
	if (mapSize < MIN_MAP_SIZE)
	{
		return 0; // map size is too small
	}
	/*-----------------------------------------*/
	// check boundary condition (ascending map only)
	/*-----------------------------------------*/
	if (input < mapX[0])
	{
		return (int16_t)mapY[0];
	}
	else if (input > mapX[mapSize - 1])
	{
		return (int16_t)mapY[mapSize - 1];
	}
	else
	{
		// do nothing
	}
	/*-----------------------------------------*/
	// find a proper index for interpolation
	/*-----------------------------------------*/
	for (mapIdx = 0; mapIdx < mapSize - 1; mapIdx++)
	{
		if (mapX[mapIdx] <= input && input <= mapX[mapIdx + 1]) // x is ascending
		{
			x0 = mapX[mapIdx];
			x1 = mapX[mapIdx + 1];
			y0 = mapY[mapIdx];
			y1 = mapY[mapIdx + 1];
			break;
		}
		else
		{
			// do nothing
		}
	}


	/*-----------------------------------------*/
	// saturation check
	/*-----------------------------------------*/
	result = UTILS_IPOL_CAL_SLOPE((float)x0, (float)x1, (float)y0, (float)y1, (float)input);

	if(result > (float)INT16_MAX)		return INT16_MAX;
	else if(result < (float)INT16_MIN)	return INT16_MIN;
	else								return (int16_t)result;
}

int16_t utils_ipol_s16u16(const int16_t* mapX, const uint16_t* mapY, uint16_t mapSize, int16_t input)
{
	uint16_t mapIdx;
	int16_t x0 = 0, x1 = 0;
	uint16_t y0 = 0, y1 = 0;
	float result;
	/*-----------------------------------------*/
	// check map size validity
	/*-----------------------------------------*/
	if (mapSize < MIN_MAP_SIZE)
	{
		return 0; // map size is too small
	}
	/*-----------------------------------------*/
	// check boundary condition (ascending map only)
	/*-----------------------------------------*/
	if (input < mapX[0])
	{
		return (int16_t)mapY[0];
	}
	else if (input > mapX[mapSize - 1])
	{
		return (int16_t)mapY[mapSize - 1];
	}
	else
	{
		// do nothing
	}
	/*-----------------------------------------*/
	// find a proper index for interpolation
	/*-----------------------------------------*/
	for (mapIdx = 0; mapIdx < mapSize - 1; mapIdx++)
	{
		if (mapX[mapIdx] <= input && input <= mapX[mapIdx + 1]) // x is ascending
		{
			x0 = mapX[mapIdx];
			x1 = mapX[mapIdx + 1];
			y0 = mapY[mapIdx];
			y1 = mapY[mapIdx + 1];
			break;
		}
		else
		{
			// do nothing
		}
	}
	/*-----------------------------------------*/
	// saturation check
	/*-----------------------------------------*/
	result = UTILS_IPOL_CAL_SLOPE((float)x0, (float)x1, (float)y0, (float)y1, (float)input);

	if (result > (float)INT16_MAX)			return INT16_MAX;
	else if (result < (float)INT16_MIN)		return INT16_MIN;
	else									return (uint16_t)result;
}

int16_t utils_ipol_u32s16(const uint32_t* mapX, const int16_t* mapY, uint16_t mapSize, uint32_t input)
{
	uint16_t mapIdx;
	uint32_t x0 = 0, x1 = 0;
	int16_t y0 = 0, y1 = 0;
	float result;
	/*-----------------------------------------*/
	// check map size validity
	/*-----------------------------------------*/
	if (mapSize < MIN_MAP_SIZE)
	{
		return 0; // map size is too small
	}
	/*-----------------------------------------*/
	// check boundary condition (ascending map only)
	/*-----------------------------------------*/
	if (input < mapX[0])
	{
		return (int16_t)mapY[0];
	}
	else if (input > mapX[mapSize - 1])
	{
		return (int16_t)mapY[mapSize - 1];
	}
	else
	{
		// do nothing
	}
	/*-----------------------------------------*/
	// find a proper index for interpolation
	/*-----------------------------------------*/
	for (mapIdx = 0; mapIdx < mapSize - 1; mapIdx++)
	{
		if (mapX[mapIdx] <= input && input <= mapX[mapIdx + 1]) // x is ascending
		{
			x0 = mapX[mapIdx];
			x1 = mapX[mapIdx + 1];
			y0 = mapY[mapIdx];
			y1 = mapY[mapIdx + 1];
			break;
		}
		else
		{
			// do nothing
		}
	}
	/*-----------------------------------------*/
	// saturation check
	/*-----------------------------------------*/
	result = UTILS_IPOL_CAL_SLOPE((float)x0, (float)x1, (float)y0, (float)y1, (float)input);

	if (result > (float)INT16_MAX)			return INT16_MAX;
	else if (result < (float)INT16_MIN)		return INT16_MIN;
	else									return (int16_t)result;
}

/*----------------------------*/
// LPF (FIRST ORDER LOW PASS FILTER)
/*----------------------------*/

typLpf_handle vLPF_phaseCurr_obj, vLPF_RPM_obj;

static float utils_LPF_get_Alpha(float sampling_t, uint32_t cutOff_f)
{
	float result, temp1, temp2;
	temp1 = 2. * PI * cutOff_f * sampling_t;
	temp2 = 1. + temp1;
	result = temp1 / temp2;
	return result;
}

void utils_LPF_RPM_init(void)
{
	vLPF_RPM_obj.cutoff_hz = UTILS_RPM_CUTOFF_HZ;
	vLPF_RPM_obj.prev_out = 0.0f;
	vLPF_RPM_obj.Fx_coeff = utils_LPF_get_Alpha(UTILS_SAMPLING_TIME_SEC, UTILS_RPM_CUTOFF_HZ);
	vLPF_RPM_obj.is_1stRun = true;
	vLPF_RPM_obj.is_initialized = true;
}

void utils_LPF_phaseCurr_init(void)
{
	vLPF_phaseCurr_obj.cutoff_hz = UTILS_PHASE_CURR_CUTOFF_HZ;
	vLPF_phaseCurr_obj.prev_out = 0.0f;
	vLPF_phaseCurr_obj.Fx_coeff = utils_LPF_get_Alpha(UTILS_SAMPLING_TIME_SEC, UTILS_PHASE_CURR_CUTOFF_HZ);
	vLPF_phaseCurr_obj.is_1stRun = true;
	vLPF_phaseCurr_obj.is_initialized = true;
}

float utils_LPF_RPM_filter(float input)
{
	if (vLPF_RPM_obj.is_initialized == false)
	{
		return 0.0f;
	}
	else if (vLPF_RPM_obj.is_1stRun == true)
	{
		vLPF_RPM_obj.prev_out = input;
		vLPF_RPM_obj.is_1stRun = false;
		return input;
	}
	else
	{
		vLPF_RPM_obj.prev_out = (1. - vLPF_RPM_obj.Fx_coeff) * vLPF_RPM_obj.prev_out + (vLPF_RPM_obj.Fx_coeff * input);
		return vLPF_RPM_obj.prev_out;
	}
}

float utils_LPF_phaseCurr_filter(float input)
{
	if (vLPF_phaseCurr_obj.is_initialized == false)
	{
		return 0.0f;
	}
	else if (vLPF_phaseCurr_obj.is_1stRun == true)
	{
		vLPF_phaseCurr_obj.prev_out = input;
		vLPF_phaseCurr_obj.is_1stRun = false;
		return input;
	}
	else
	{
		vLPF_phaseCurr_obj.prev_out = (1. - vLPF_phaseCurr_obj.Fx_coeff) * vLPF_phaseCurr_obj.prev_out + (vLPF_phaseCurr_obj.Fx_coeff * input);
		return vLPF_phaseCurr_obj.prev_out;
	}
}